// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// E2E de `notalivre emitir` contra o servidor falso (NL-2.9, DA-2.44 e DA-2.45). A CLI roda como o usuário a roda, num
// pseudo-terminal que este teste cria: a senha é digitada no prompt e nunca vai por argv nem por ambiente. Só fixtures
// sintéticas (NL_CERTIFICADOS_DIR, certificados de teste da NL-2.1). Nenhum .pfx do líder.
#include <nl/teste/tabela_sha1_distro.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

#include <poll.h>
#include <pty.h>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <vector>

namespace {

const QString kCli = QStringLiteral(NL_CLI_EMITIR);
const QString kLancador = QStringLiteral(NL_LANCADOR_EMITIR);
const QString kPython = QStringLiteral(NL_PYTHON3_EMITIR);
const QString kCertificados = QStringLiteral(NL_CERTIFICADOS_EMITIR);
const QString kEntradas = QStringLiteral(NL_ENTRADAS_EMITIR);
const QString kXsd = QStringLiteral(NL_XSD_EMITIR);
const QString kManifesto = QStringLiteral(NL_MANIFESTO_EMITIR);
const QString kTabelaSha1 = QStringLiteral(NL_TABELA_SHA1_EMITIR);

QByteArray lerArquivo(const QString& caminho);

// Expectativa do assinador para a distro desta máquina (DA-2.38), lida da tabela versionada pelo auxiliar único (DA-2.61):
// "sha1" ou "algoritmo_bloqueado". Vazio quando a distro não está na tabela, e o teste reprova (DA-2.40, item 1).
QString esperadoDaDistro()
{
    return nl::teste::expectativaSha1DaDistroAtual(kTabelaSha1);
}

QByteArray lerArquivo(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

// Servidor falso em processo próprio, com oráculo. Fecha o stdin ao sair.
class ServidorFalso
{
public:
    /// falhaPost, se vier, é o roteiro do POST /nfse (ex.: rejeicao_gerada:E0014, DA-2.58). prefixo, se vier, é o caminho da API
    /// (DA-2.5, NL-2.10a): o servidor responde só nele.
    explicit ServidorFalso(const QString& falhaPost = QString(), const QString& prefixo = QStringLiteral("/SefinNacional"))
    {
        QStringList argumentos{QStringLiteral("-I"), kLancador, kCertificados, QStringLiteral("--com-oraculo")};
        if (!falhaPost.isEmpty()) {
            argumentos << QStringLiteral("--falha") << QStringLiteral("POST") << QStringLiteral("/nfse") << falhaPost;
        }
        if (!prefixo.isEmpty()) {
            argumentos << QStringLiteral("--prefixo") << prefixo;
        }
        processo_.start(kPython, argumentos);
        if (processo_.waitForReadyRead(20000)) {
            const QString linha = QString::fromUtf8(processo_.readLine()).trimmed();
            if (linha.startsWith(QStringLiteral("PORTA "))) {
                porta_ = linha.mid(6).toInt();
            }
        }
        if (porta_ == 0) {
            diagnostico_ = QStringLiteral("servidor falso não subiu; stderr: ") +
                           QString::fromUtf8(processo_.readAllStandardError());
        }
    }
    ~ServidorFalso()
    {
        processo_.closeWriteChannel();
        processo_.waitForFinished(5000);
    }
    /// Fecha o stdin, espera o servidor sair e devolve quantas requisições ele recebeu (DA-2.51). -1 se o servidor não disse.
    int requisicoesAoFechar()
    {
        processo_.closeWriteChannel();
        processo_.waitForFinished(5000);
        const QString resto = QString::fromUtf8(processo_.readAllStandardOutput());
        for (const QString& linha : resto.split(QLatin1Char('\n'))) {
            if (linha.startsWith(QStringLiteral("REQUISICOES "))) {
                return linha.mid(12).trimmed().toInt();
            }
        }
        return -1;
    }
    /// Fecha o stdin e devolve os caminhos dos POST /nfse que chegaram (NL-2.10a, passo 3): o lancador os imprime ao sair.
    QStringList caminhosAoFechar()
    {
        processo_.closeWriteChannel();
        processo_.waitForFinished(5000);
        const QString resto = QString::fromUtf8(processo_.readAllStandardOutput());
        for (const QString& linha : resto.split(QLatin1Char('\n'))) {
            if (linha.startsWith(QStringLiteral("CAMINHOS "))) {
                return linha.mid(9).trimmed().split(QLatin1Char(','), Qt::SkipEmptyParts);
            }
        }
        return {};
    }
    /// URI do Algorithm do SignatureMethod de cada POST /nfse que chegou (NL-2.10a, rodada 2). Stub: ainda não lê o servidor.
    QStringList algoritmosAoFechar()
    {
        processo_.closeWriteChannel();
        processo_.waitForFinished(5000);
        const QString resto = QString::fromUtf8(processo_.readAllStandardOutput());
        for (const QString& linha : resto.split(QLatin1Char('\n'))) {
            if (linha.startsWith(QStringLiteral("ALGORITMOS "))) {
                return linha.mid(11).trimmed().split(QLatin1Char(','), Qt::SkipEmptyParts);
            }
        }
        return {};
    }
    int porta() const { return porta_; }
    QString diagnostico() const { return diagnostico_; }

private:
    QProcess processo_;
    int porta_ = 0;
    QString diagnostico_;
};

struct Execucao
{
    QByteArray saida;   ///< tudo o que o pseudo-terminal mostrou: prompt, eco (se houvesse) e a saída da CLI
    int codigo = -1;    ///< código de saída da CLI; -1 se não saiu normalmente; -2 se estourou a espera; -3 se a escrita falhou
};

// Grava a linha inteira no pseudo-terminal ou devolve falso. write pode gravar menos que o pedido e pode ser interrompido
// (EINTR). O retorno nunca é descartado: o SDK do Flatpak compila com _FORTIFY_SOURCE=3 e -Werror=unused-result, e um
// static_cast<void> não cala esse aviso (CI run 41257357615).
bool escreverTudo(int fd, const QByteArray& dados)
{
    const char* ponteiro = dados.constData();
    qsizetype restante = dados.size();
    while (restante > 0) {
        const ssize_t gravados = write(fd, ponteiro, static_cast<size_t>(restante));
        if (gravados < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        ponteiro += gravados;
        restante -= gravados;
    }
    return true;
}

// Roda a CLI num pseudo-terminal próprio. Quando o prompt da senha aparece, digita a senha e a linha. Limite de espera.
Execucao rodarCliNoTerminal(const QStringList& argumentos, const QString& configDir, const QByteArray& senha,
                           const QByteArray& confirmacao = QByteArray())
{
    std::vector<QByteArray> dados;
    dados.push_back(kCli.toLocal8Bit());
    for (const QString& argumento : argumentos) {
        dados.push_back(argumento.toLocal8Bit());
    }
    std::vector<char*> argv;
    for (QByteArray& argumento : dados) {
        argv.push_back(argumento.data());
    }
    argv.push_back(nullptr);

    int mestre = -1;
    const pid_t pid = forkpty(&mestre, nullptr, nullptr, nullptr);
    if (pid == 0) {
        setenv("XDG_CONFIG_HOME", configDir.toLocal8Bit().constData(), 1);
        const QByteArray estado = (configDir + QStringLiteral("/state")).toLocal8Bit();  // respostas da emissão (DA-2.59 item 4)
        setenv("XDG_STATE_HOME", estado.constData(), 1);
        execv(argv[0], argv.data());
        _exit(127);
    }
    Execucao execucao;
    if (pid < 0) {
        return execucao;
    }
    bool senhaEnviada = false;
    bool confirmacaoEnviada = false;
    bool expirou = false;
    bool escritaFalhou = false;
    char buffer[1024];
    for (;;) {
        pollfd espera{mestre, POLLIN, 0};
        if (poll(&espera, 1, 60000) <= 0) {
            expirou = true;
            break;  // limite de espera: a CLI não respondeu
        }
        const ssize_t lidos = read(mestre, buffer, sizeof buffer);
        if (lidos <= 0) {
            break;  // fim do terminal: a CLI saiu
        }
        execucao.saida.append(buffer, static_cast<int>(lidos));
        if (!confirmacaoEnviada && execucao.saida.contains("confirme o CPF ou CNPJ")) {
            // a confirmação digitada; vazia para o caso de recusa
            if (!escreverTudo(mestre, confirmacao + "\n")) {
                escritaFalhou = true;
                break;
            }
            confirmacaoEnviada = true;
        }
        if (!senhaEnviada && execucao.saida.contains("senha do certificado")) {
            if (!escreverTudo(mestre, senha + "\n")) {
                escritaFalhou = true;
                break;
            }
            senhaEnviada = true;
        }
    }
    if (expirou || escritaFalhou) {
        kill(pid, SIGKILL);  // a CLI não responde mais: mata antes do waitpid, que bloquearia
    }
    int status = 0;
    waitpid(pid, &status, 0);
    close(mestre);
    if (escritaFalhou) {
        execucao.codigo = -3;  // a linha não entrou no terminal: o caso falha, nunca passa como saída normal
    } else if (expirou) {
        execucao.codigo = -2;  // limite de espera estourado
    } else if (WIFEXITED(status)) {
        execucao.codigo = WEXITSTATUS(status);
    }
    return execucao;
}

} // namespace

// Configuração XDG para os casos de recusa: a CLI recusa antes do envio, então nenhum servidor é necessário.
void escreverConfigRecusa(const QTemporaryDir& pasta, bool producaoLiberada, const QString& url = QStringLiteral("https://127.0.0.1:9443"))
{
    QDir().mkpath(pasta.filePath(QStringLiteral("notalivre")));
    QFile arquivo(pasta.filePath(QStringLiteral("notalivre/config.ini")));
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    // NL-2.10b (condição do CISO, 31/10/2026; BAIXA-4): âncoras e [xsd] só existem em Homologação (loopback). Fora dela, definir
    // uma delas é erro de configuração (código 10), então a configuração de Produção não as traz.
    const bool homologacao = url.startsWith(QStringLiteral("https://127.0.0.1"));
    const QString ancoras = homologacao ? QStringLiteral("ancoras_tls = %1/ac.pem\n[certificado]\nancoras = %1/ac.pem\n")
                                        : QStringLiteral("[certificado]\n");
    const QString xsd = homologacao ? QStringLiteral("[xsd]\ndiretorio = %1\nmanifesto = %2\n").arg(kXsd, kManifesto) : QString();
    const QString texto = QStringLiteral("[sefin]\nurl = %1\nprefixo = /SefinNacional\n%2caminho = %3/cnpj_ok.pfx\n%4"
                                         "[emissao]\nproducao_liberada = %5\n")
                              .arg(url, ancoras.arg(kCertificados), kCertificados, xsd,
                                   producaoLiberada ? QStringLiteral("true") : QStringLiteral("false"));
    arquivo.write(texto.toUtf8());
}

// Emissão contra o servidor falso com uma falha no POST /nfse (DA-2.59, E0014). A distro com SHA-1 bloqueado nunca chega ao POST.
Execucao emitirContraServidorComFalhaNoPost(const QString& falha, int* requisicoes = nullptr)
{
    QTemporaryDir configDir;
    ServidorFalso servidor(falha);
    if (!configDir.isValid() || servidor.porta() <= 0) {
        Execucao semServidor;
        semServidor.saida = QByteArray("servidor falso não subiu: ") + servidor.diagnostico().toUtf8();
        return semServidor;
    }
    escreverConfigRecusa(configDir, false, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    if (requisicoes != nullptr) {
        *requisicoes = servidor.requisicoesAoFechar();
    }
    return execucao;
}

namespace {

// DA-2.38 e DA-2.61: nesta distro o SHA-1 é recusado, então a CLI sai com 1 e o código da tabela, e nenhuma requisição chega
// ao servidor. Vale para os slots que, na distro que aceita o SHA-1, chegariam ao POST; o comportamento completo continua
// testado nas outras três imagens.
void exigeRecusaSha1SemEnvio(const Execucao& execucao, int requisicoesRecebidas)
{
    QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("assinatura.algoritmo_bloqueado"), "nesta distro, a assinatura SHA-1 tem de ser recusada pelo código da tabela");
    QCOMPARE(requisicoesRecebidas, 0);
}

} // namespace

class TstCliEmitir : public QObject
{
    Q_OBJECT

private slots:
    // A DPS de entrada sai pela CLI, é assinada, aceita pelo servidor falso e a chave volta na saída. A senha é digitada no
    // prompt e não aparece na saída do terminal.
    void cliEmiteDpsAceitaPeloServidorFalso();
    // Produção sem liberação por configuração: a CLI recusa antes de pedir a senha.
    void cliRecusaProducaoBloqueada();
    // Produção liberada por configuração, sem o CNPJ digitado: a CLI recusa antes de pedir a senha.
    void cliRecusaProducaoSemCnpjDigitado();
    // Senha por argumento: a CLI recusa sem pedir a senha no terminal, e o valor não aparece na saída nem no log (DA-2.52).
    void senhaNaoApareceEmArgvNemLog();
    // DPS de entrada que não confere no leiaute: sai com 4, e o servidor falso recebe zero requisições (DA-2.51, mutante H).
    void cliRecusaDpsInvalidaSemPost();
    // E0014 na primeira tentativa com o mesmo infDPS: a CLI recupera a chave e sai com 0 (DA-2.58).
    void e0014DaMesmaDpsRecuperaAChave();
    // E0014 com infDPS diferente para o mesmo Id: DPS duplicada, código 5, sem renumerar (DA-2.58).
    void e0014ComInfDpsDiferenteSaiComCinco();
    // E0014 sem conseguir comparar o infDPS: código 7 (DA-2.58).
    void e0014SemComparacaoSaiComSete();
    // DA-2.59 item 4: a resposta crua da Sefin fica num arquivo local 0600, com o XML e a chave mascarados, sem senha.
    void respostaCruaGravadaParaAnalise();
    void cliGravaARespostaDaRecusa();
    void cliAvisaQuandoAGravacaoFalha();
    // NL-2.10a, passo 3 (DA-2.5): [sefin] prefixo vai ao caminho do POST; o servidor com o mesmo prefixo recebe em /SefinNacional/nfse.
    void prefixoDaConfiguracaoVaiNaUrl();
    // NL-2.10a, passo 3: sem [sefin] prefixo, a CLI recusa com 10 e nomeia a chave, antes de pedir a senha.
    void prefixoAusenteSaiComDez();
    // NL-2.10a, passo 5: o caminho do .pfx vem de [certificado] caminho, sem o argumento --certificado.
    void caminhoDoCertificadoVemDaConfiguracao();
    // NL-2.10a, passo 5: arquivo ilegível ou inexistente sai com 10, código certificado.caminho_ilegivel, e a mensagem diz como dar
    // acesso ao arquivo no Flatpak (ordem do passo 4).
    void certificadoIlegivelSaiComDezEInstrucao();
    // NL-2.10a, passo 5: o caminho do certificado não aparece na saída da CLI (pode conter nome de pessoa).
    void caminhoDoCertificadoNuncaVaiAoLog();
    // NL-2.10a, rodada 2 (QA, item 1): o XML que chega ao servidor leva o URI rsa-sha256 quando a configuração pede rsa-sha256.
    void algoritmoRsaSha256NoXmlEnviado();
    // NL-2.10a, rodada 2 (QA, item 1): sem [assinatura], o XML leva rsa-sha1 (padrão); no fedora44 a política recusa antes do envio.
    void algoritmoRsaSha1NoXmlPadrao();
    // NL-2.10a, rodada 2 (QA, item 2): as âncoras do servidor TLS são as da chave ancoras_tls, não as do A1 (M3 do QA).
    void ancorasTlsSozinhasValidamOServidor();
};

void TstCliEmitir::cliEmiteDpsAceitaPeloServidorFalso()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    QDir().mkpath(configDir.filePath(QStringLiteral("notalivre")));

    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));

    const QString config = QStringLiteral(
        "[sefin]\n"
        "url = https://127.0.0.1:%1\n"
        "prefixo = /SefinNacional\n"
        "ancoras_tls = %2/ac.pem\n"
        "[certificado]\n"
        "ancoras = %2/ac.pem\n"
        "caminho = %2/cnpj_ok.pfx\n"
        "[xsd]\n"
        "diretorio = %3\n"
        "manifesto = %4\n"
        "[emissao]\n"
        "producao_liberada = false\n")
                             .arg(servidor.porta())
                             .arg(kCertificados)
                             .arg(kXsd)
                             .arg(kManifesto);
    QFile arquivoConfig(configDir.filePath(QStringLiteral("notalivre/config.ini")));
    QVERIFY(arquivoConfig.open(QIODevice::WriteOnly | QIODevice::Text));
    arquivoConfig.write(config.toUtf8());
    arquivoConfig.close();

    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    QVERIFY2(!senha.isEmpty(), "a senha da fixture sintética tem de existir");
    // NL-2.9, item 10: os segredos desta execução vão para o arquivo de referência que o portão de PII lê junto da lista fixa.
    // O portão reprova se a lista vier vazia, e a senha aqui é o que o log não pode mostrar.
    QFile referencia(qEnvironmentVariable("NL_SEGREDOS_PII_REF"));
    QVERIFY2(referencia.open(QIODevice::WriteOnly | QIODevice::Text), "o arquivo de segredos da execução tem de abrir");
    referencia.write("# segredos desta execução\nsenha|" + senha + "\n");
    referencia.close();
    // DA-2.49 e DA-2.38: o SHA-1 da assinatura é recusado pelo OpenSSL do fedora 44 (tabela por distro). Ali o caso espera
    // assinatura.algoritmo_bloqueado e nenhum envio; nas demais distros, a emissão tem de ser aceita.
    const QString esperado = esperadoDaDistro();
    QVERIFY2(!esperado.isEmpty(), "a distro desta máquina tem de estar em tabela_sha1_por_distro.txt");

    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"), QStringLiteral("--serie"), QStringLiteral("1"),
                                 QStringLiteral("--numero"), QStringLiteral("1"), QStringLiteral("--ambiente"),
                                 QStringLiteral("homologacao")};
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);

    // ADR-0005 e: a saída da CLI no e2e vai para a pasta que o portão de PII varre (cli.portaoPiiVarreLogsDaCli).
    const QString pastaPii = qEnvironmentVariable("NL_LOGS_PII_CLI_DIR");
    QVERIFY2(!pastaPii.isEmpty(), "NL_LOGS_PII_CLI_DIR tem de estar definido pelo CTest");
    QVERIFY2(QDir().mkpath(pastaPii), "a pasta de logs da CLI tem de existir");
    QFile logPii(pastaPii + QStringLiteral("/emitir_e2e.log"));
    QVERIFY2(logPii.open(QIODevice::WriteOnly), "o log do e2e tem de abrir na pasta de logs da CLI");
    logPii.write(execucao.saida);

    if (esperado == QStringLiteral("algoritmo_bloqueado")) {
        QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
        QVERIFY2(execucao.saida.contains("assinatura.algoritmo_bloqueado"), "nesta distro, a assinatura SHA-1 tem de ser recusada pelo código da tabela");
        return;
    }
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída do terminal: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("emitida: chave "), "a CLI tem de mostrar a chave da NFS-e emitida");
    QVERIFY2(!execucao.saida.contains(senha), "a senha não pode aparecer no terminal");
}

void TstCliEmitir::cliRecusaProducaoBloqueada()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    escreverConfigRecusa(configDir, false, QStringLiteral("https://sefin.nfse.gov.br/SefinNacional"));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("producao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("bloqueada"), "a recusa tem de dizer que Produção está bloqueada");
    QVERIFY2(!execucao.saida.contains("senha do certificado"), "a senha não pode ser pedida antes da recusa");
}

void TstCliEmitir::cliRecusaProducaoSemCnpjDigitado()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    escreverConfigRecusa(configDir, true, QStringLiteral("https://sefin.nfse.gov.br/SefinNacional"));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("producao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("CNPJ"), "a recusa tem de exigir o CNPJ digitado");
    QVERIFY2(!execucao.saida.contains("senha do certificado"), "a senha não pode ser pedida antes da recusa");
}

void TstCliEmitir::senhaNaoApareceEmArgvNemLog()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    escreverConfigRecusa(configDir, false);
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 
                                 QStringLiteral("--senha"), QString::fromUtf8(senha)};
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    QVERIFY2(execucao.codigo == 2, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("linha de comando"), "a recusa tem de dizer que a senha não vai pela linha de comando");
    QVERIFY2(!execucao.saida.contains("senha do certificado"), "a CLI não pode pedir a senha depois de recusar --senha");
    QVERIFY2(!execucao.saida.contains(senha), "a senha por argumento não pode aparecer na saída");
}

void TstCliEmitir::cliRecusaDpsInvalidaSemPost()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigRecusa(configDir, false, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/xdescserv_2001.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    QVERIFY2(execucao.codigo == 4, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    // Fixture xdescserv_2001.json: xDescServ com 2001 caracteres. O domínio (Back) não limita o texto; o XSD limita em 2000
    // (third_party, cache do XSD: tiposSimples_v1.01.xsd, linha 1442, TSDesc2000, maxLength 2000). Sem dado real.
    // A entrada passa no domínio (Back) e reprova no XSD ANTES da assinatura: o código tem de ser xsd.invalido. Se a validação de
    // entrada for pulada (mutante H), a reprovação vem só depois de assinar, como xsd.pos_assinatura, e o teste reprova.
    QVERIFY2(execucao.saida.contains("xsd.invalido"), "a DPS tem de sair reprovada pela validação de entrada, antes da assinatura");
    QVERIFY2(!execucao.saida.contains("xsd.pos_assinatura"), "a reprovação não pode vir da revalidação depois de assinar");
    QVERIFY2(!execucao.saida.contains(senha), "a senha não pode aparecer no terminal");
    QCOMPARE(servidor.requisicoesAoFechar(), 0);
}

void TstCliEmitir::e0014DaMesmaDpsRecuperaAChave()
{
    int requisicoes = -1;
    const Execucao execucao = emitirContraServidorComFalhaNoPost(QStringLiteral("rejeicao_gerada:E0014"), &requisicoes);
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1SemEnvio(execucao, requisicoes);
        return;
    }
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("emitida: chave "), "o mesmo infDPS tem de recuperar a chave da nota");
}

void TstCliEmitir::e0014ComInfDpsDiferenteSaiComCinco()
{
    int requisicoes = -1;
    const Execucao execucao = emitirContraServidorComFalhaNoPost(QStringLiteral("rejeicao_gerada_diferente:E0014"), &requisicoes);
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1SemEnvio(execucao, requisicoes);
        return;
    }
    QVERIFY2(execucao.codigo == 5, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("sefin.dps_duplicada"), "o infDPS diferente tem de sair como DPS duplicada");
}

void TstCliEmitir::e0014SemComparacaoSaiComSete()
{
    int requisicoes = -1;
    const Execucao execucao = emitirContraServidorComFalhaNoPost(QStringLiteral("rejeicao_gerada_sem_comparacao:E0014"), &requisicoes);
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1SemEnvio(execucao, requisicoes);
        return;
    }
    QVERIFY2(execucao.codigo == 7, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("sefin.comparacao_indisponivel"), "a comparação impossível tem de sair com o código de comparação indisponível");
}

namespace {

// Dono lê e escreve; grupo e outros sem nenhum bit (0600). O Qt devolve o bit de usuário junto do de dono, então a
// igualdade exata com ReadOwner|WriteOwner nunca bate; por isso a conferência é por máscara.
bool permissaoSoDoDono(const QString& caminho)
{
    const QFileDevice::Permissions p = QFile::permissions(caminho);
    const QFileDevice::Permissions dono = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    const QFileDevice::Permissions outros = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                            QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
    return (p & dono) == dono && (p & outros) == 0;
}

} // namespace

void TstCliEmitir::respostaCruaGravadaParaAnalise()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigRecusa(configDir, false, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1SemEnvio(execucao, servidor.requisicoesAoFechar());
        QVERIFY2(QDir(configDir.path() + QStringLiteral("/state/notalivre/respostas")).entryList({QStringLiteral("*.jsonl")}, QDir::Files).isEmpty(),
                 "nesta distro nada chega ao POST, e não pode haver resposta crua gravada");
        return;
    }
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    const QDir pasta(configDir.path() + QStringLiteral("/state/notalivre/respostas"));
    const QStringList arquivos = pasta.entryList({QStringLiteral("*.jsonl")}, QDir::Files);
    QCOMPARE(arquivos.size(), 1);
    const QString caminho = pasta.filePath(arquivos.first());
    QVERIFY2(permissaoSoDoDono(caminho), "o arquivo da resposta tem de ser 0600: dono lê e escreve, grupo e outros sem bit");
    const QByteArray conteudo = lerArquivo(caminho);
    QVERIFY2(!conteudo.contains("nfseXmlGZipB64\":\"H4s"), "o XML da NFS-e (GZip em Base64) não pode ir ao arquivo");
    QVERIFY2(!conteudo.contains(senha), "a senha não pode ir ao arquivo");
    QVERIFY2(!conteudo.contains("11222333000181"), "o CNPJ do certificado não pode ir ao arquivo sem máscara");
}

void TstCliEmitir::cliGravaARespostaDaRecusa()
{
    // NL-2.10c (DA-2.66, passo 4): a CLI recebe uma recusa com código E (rejeicao:E1207: 400 com um CPF no texto livre). A
    // resposta da recusa vai ao .jsonl da DPS, com o status 400, em 0600, sem o texto livre e sem nenhum dígito do CPF.
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor(QStringLiteral("rejeicao:E1207"));
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigRecusa(configDir, false, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1SemEnvio(execucao, servidor.requisicoesAoFechar());
        return;
    }
    QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("sefin.E1207"), "a recusa tem de sair com o código E da Sefin");
    const QDir pasta(configDir.path() + QStringLiteral("/state/notalivre/respostas"));
    const QStringList arquivos = pasta.entryList({QStringLiteral("*.jsonl")}, QDir::Files);
    QCOMPARE(arquivos.size(), 1);
    const QString caminho = pasta.filePath(arquivos.first());
    QVERIFY2(permissaoSoDoDono(caminho), "o arquivo da recusa tem de ser 0600: dono lê e escreve, grupo e outros sem bit");
    const QByteArray conteudo = lerArquivo(caminho);
    QVERIFY2(conteudo.contains("\"status\":400"), "a linha do POST recusado leva o status 400");
    QVERIFY2(conteudo.contains("\"metodo\":\"POST\""), "a linha do POST recusado leva o método");
    QVERIFY2(!conteudo.contains("texto livre devolvido pela API"), "o texto livre da recusa não pode ir ao arquivo");
    QVERIFY2(conteudo.count("12345678909") == 0, "nenhum dígito do CPF da fixture pode ir ao arquivo");
}

void TstCliEmitir::cliAvisaQuandoAGravacaoFalha()
{
    // NL-2.10c, Emenda 3, BAIXA-7 e M11: a falha de gravação da resposta de análise chega ao log da CLI como aviso, e a emissão
    // segue (a Sefin já aceitou). Um arquivo comum no lugar da pasta de estado impede a gravação.
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigRecusa(configDir, false, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()));
    QFile bloqueio(configDir.path() + QStringLiteral("/state"));
    QVERIFY(bloqueio.open(QIODevice::WriteOnly));
    bloqueio.write("x");
    bloqueio.close();
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1SemEnvio(execucao, servidor.requisicoesAoFechar());
        return;
    }
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("resposta_crua.nao_gravada"), "a falha de gravação tem de sair como aviso no log da CLI");
    QVERIFY2(!execucao.saida.contains(configDir.path().toUtf8()), "o caminho da pasta de estado não pode ir ao log");
}

// Configuração XDG com [sefin] url, prefixo e algoritmo (NL-2.10a, passo 3). Sem prefixo ou algoritmo, a linha não é escrita.
void escreverConfigPrefixo(const QTemporaryDir& pasta, const QString& url, const QString& prefixo, const QString& algoritmo,
                           const QString& caminhoCertificado = QString())
{
    QDir().mkpath(pasta.filePath(QStringLiteral("notalivre")));
    QFile arquivo(pasta.filePath(QStringLiteral("notalivre/config.ini")));
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    // [sefin] com prefixo e âncoras do servidor; [certificado] com as do A1, na ordem em que o INI as lê por seção.
    QString texto = QStringLiteral("[sefin]\nurl = %1\n").arg(url);
    if (!prefixo.isEmpty()) {
        texto += QStringLiteral("prefixo = %1\n").arg(prefixo);
    }
    texto += QStringLiteral("ancoras_tls = %1/ac.pem\n[certificado]\nancoras = %1/ac.pem\n").arg(kCertificados);
    const QString caminho = caminhoCertificado.isEmpty() ? kCertificados + QStringLiteral("/cnpj_ok.pfx") : caminhoCertificado;
    texto += QStringLiteral("caminho = %1\n").arg(caminho);
    texto += QStringLiteral("[xsd]\ndiretorio = %1\nmanifesto = %2\n[emissao]\nproducao_liberada = false\n").arg(kXsd, kManifesto);
    if (!algoritmo.isEmpty()) {
        texto += QStringLiteral("[assinatura]\nalgoritmo = %1\n").arg(algoritmo);
    }
    arquivo.write(texto.toUtf8());
}

void TstCliEmitir::prefixoDaConfiguracaoVaiNaUrl()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor(QString(), QStringLiteral("/SefinNacional"));
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigPrefixo(configDir, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()), QStringLiteral("/SefinNacional"),
                          QStringLiteral("rsa-sha256"));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("emitida: chave "), "a CLI tem de mostrar a chave da NFS-e emitida");
    const QStringList caminhos = servidor.caminhosAoFechar();
    QVERIFY2(caminhos.contains(QStringLiteral("/SefinNacional/nfse")),
             qPrintable(QStringLiteral("o POST tinha de chegar em /SefinNacional/nfse; chegou em: [%1]").arg(caminhos.join(QLatin1Char(',')))));
}

void TstCliEmitir::prefixoAusenteSaiComDez()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    escreverConfigPrefixo(configDir, QStringLiteral("https://127.0.0.1:9443"), QString(), QString());
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 };
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    QVERIFY2(execucao.codigo == 10, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("sefin.prefixo"), "a mensagem tem de nomear a chave que falta");
    QVERIFY2(!execucao.saida.contains("senha do certificado"), "a senha não pode ser pedida antes da recusa");
}

// Argumentos sem --certificado (NL-2.10a, passo 5): o .pfx só entra pela configuração.
QStringList argumentosSemCertificado()
{
    return {QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"), QStringLiteral("--serie"), QStringLiteral("1"),
            QStringLiteral("--numero"), QStringLiteral("1"), QStringLiteral("--ambiente"), QStringLiteral("homologacao")};
}

void TstCliEmitir::caminhoDoCertificadoVemDaConfiguracao()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigPrefixo(configDir, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()), QStringLiteral("/SefinNacional"),
                          QStringLiteral("rsa-sha256"), kCertificados + QStringLiteral("/cnpj_ok.pfx"));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const Execucao execucao = rodarCliNoTerminal(argumentosSemCertificado(), configDir.path(), senha);
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("emitida: chave "), "a CLI tem de mostrar a chave da NFS-e emitida");
    QVERIFY2(servidor.caminhosAoFechar().contains(QStringLiteral("/SefinNacional/nfse")), "o POST tem de sair com o .pfx da configuração");
}

void TstCliEmitir::certificadoIlegivelSaiComDezEInstrucao()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    escreverConfigPrefixo(configDir, QStringLiteral("https://127.0.0.1:9443"), QStringLiteral("/SefinNacional"), QString(),
                          kCertificados + QStringLiteral("/inexistente.pfx"));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const Execucao execucao = rodarCliNoTerminal(argumentosSemCertificado(), configDir.path(), senha);
    QVERIFY2(execucao.codigo == 10, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("certificado.caminho_ilegivel"), "a saída tem de trazer o código estável");
    QVERIFY2(execucao.saida.contains("Flatpak"), "a mensagem tem de dizer que, no Flatpak, o arquivo precisa estar acessível ao app");
    QVERIFY2(!execucao.saida.contains("senha do certificado"), "a senha não pode ser pedida antes da recusa");
}

void TstCliEmitir::caminhoDoCertificadoNuncaVaiAoLog()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const QString caminho = kCertificados + QStringLiteral("/cnpj_ok.pfx");
    escreverConfigPrefixo(configDir, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()), QStringLiteral("/SefinNacional"),
                          QStringLiteral("rsa-sha256"), caminho);
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const Execucao execucao = rodarCliNoTerminal(argumentosSemCertificado(), configDir.path(), senha);
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(!execucao.saida.contains(caminho.toUtf8()), "o caminho do .pfx não pode aparecer na saída da CLI");
    QVERIFY2(!execucao.saida.contains("cnpj_ok.pfx"), "nem o nome do arquivo");
}

void TstCliEmitir::algoritmoRsaSha256NoXmlEnviado()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigPrefixo(configDir, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()), QStringLiteral("/SefinNacional"),
                          QStringLiteral("rsa-sha256"), kCertificados + QStringLiteral("/cnpj_ok.pfx"));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const Execucao execucao = rodarCliNoTerminal(argumentosSemCertificado(), configDir.path(), senha);
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("emitida: chave "), "a CLI tem de mostrar a chave da NFS-e emitida");
    // INFERÊNCIA: o URI vem do RFC 4051, §2.3.2 (xmldsig-more#rsa-sha256), e é o do SignatureMethod exigido pelo W3C XMLDSig §4.4.2.
    QCOMPARE(servidor.algoritmosAoFechar(), QStringList{QStringLiteral("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256")});
}

void TstCliEmitir::algoritmoRsaSha1NoXmlPadrao()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigPrefixo(configDir, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()), QStringLiteral("/SefinNacional"),
                          QString(), kCertificados + QStringLiteral("/cnpj_ok.pfx"));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const Execucao execucao = rodarCliNoTerminal(argumentosSemCertificado(), configDir.path(), senha);
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1SemEnvio(execucao, servidor.requisicoesAoFechar());
        return;
    }
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QCOMPARE(servidor.algoritmosAoFechar(), QStringList{QStringLiteral("http://www.w3.org/2000/09/xmldsig#rsa-sha1")});
}

void TstCliEmitir::ancorasTlsSozinhasValidamOServidor()
{
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    // Âncoras do servidor TLS vazias: a conexão tem de ser recusada, mesmo com o A1 válido. Se o código trocar as âncoras do A1
    // pelas do TLS, a conexão passa e o teste reprova (M3 do QA).
    const QString vazio = configDir.filePath(QStringLiteral("tls_vazio.pem"));
    QFile semAncora(vazio);
    QVERIFY(semAncora.open(QIODevice::WriteOnly));
    semAncora.close();
    QVERIFY(QDir().mkpath(configDir.filePath(QStringLiteral("notalivre"))));
    QFile config(configDir.filePath(QStringLiteral("notalivre/config.ini")));
    QVERIFY(config.open(QIODevice::WriteOnly | QIODevice::Text));
    config.write(QStringLiteral("[sefin]\nurl = https://127.0.0.1:%1\nprefixo = /SefinNacional\nancoras_tls = %2\n"
                                "[certificado]\nancoras = %3/ac.pem\ncaminho = %3/cnpj_ok.pfx\n"
                                "[xsd]\ndiretorio = %4\nmanifesto = %5\n[emissao]\nproducao_liberada = false\n"
                                "[assinatura]\nalgoritmo = rsa-sha256\n")
                     .arg(servidor.porta())
                     .arg(vazio)
                     .arg(kCertificados)
                     .arg(kXsd)
                     .arg(kManifesto)
                     .toUtf8());
    config.close();
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const Execucao execucao = rodarCliNoTerminal(argumentosSemCertificado(), configDir.path(), senha);
    QVERIFY2(execucao.codigo != -3, "a escrita da senha no terminal falhou: o caso não pode passar em silêncio");
    QVERIFY2(execucao.codigo != 0, qPrintable(QStringLiteral("a conexão tinha de ser recusada; rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(!execucao.saida.contains("emitida: chave "), "sem âncora TLS, nenhuma NFS-e pode sair como emitida");
    QCOMPARE(servidor.requisicoesAoFechar(), 0);
}

QTEST_APPLESS_MAIN(TstCliEmitir)
#include "tst_cli_emitir.moc"
