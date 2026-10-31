// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// E2E de `notalivre emitir` contra o servidor falso (NL-2.9, DA-2.44 e DA-2.45). A CLI roda como o usuário a roda, num
// pseudo-terminal que este teste cria: a senha é digitada no prompt e nunca vai por argv nem por ambiente. Só fixtures
// sintéticas (NL_CERTIFICADOS_DIR, certificados de teste da NL-2.1). Nenhum .pfx do líder.
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

// Expectativa do assinador para a distro desta máquina (DA-2.38, tabela versionada): "sha1" ou "algoritmo_bloqueado". Vazio
// quando a distro não está na tabela, e o teste reprova (DA-2.40, item 1).
QByteArray lerArquivo(const QString& caminho);

QString esperadoDaDistro()
{
    QString id;
    QString versao = QStringLiteral("rolling");
    for (const QString& linha : QString::fromUtf8(lerArquivo(QStringLiteral("/etc/os-release"))).split(QLatin1Char('\n'))) {
        if (linha.startsWith(QStringLiteral("ID="))) {
            id = linha.mid(3).remove(QLatin1Char('"'));
        } else if (linha.startsWith(QStringLiteral("VERSION_ID="))) {
            versao = linha.mid(11).remove(QLatin1Char('"'));
        }
    }
    for (const QString& linha : QString::fromUtf8(lerArquivo(kTabelaSha1)).split(QLatin1Char('\n'))) {
        const QStringList campos = linha.simplified().split(QLatin1Char(' '));
        if (linha.startsWith(QLatin1Char('#')) || campos.size() != 3) {
            continue;
        }
        if (campos.at(0) == id && campos.at(1) == versao) {
            return campos.at(2);
        }
    }
    return QString();
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
    /// falhaPost, se vier, é o roteiro do POST /nfse (ex.: rejeicao_gerada:E0014, DA-2.58).
    explicit ServidorFalso(const QString& falhaPost = QString())
    {
        QStringList argumentos{QStringLiteral("-I"), kLancador, kCertificados, QStringLiteral("--com-oraculo")};
        if (!falhaPost.isEmpty()) {
            argumentos << QStringLiteral("--falha") << QStringLiteral("POST") << QStringLiteral("/nfse") << falhaPost;
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
    int codigo = -1;    ///< código de saída da CLI; -1 se não saiu normalmente
};

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
            QByteArray linha = confirmacao + "\n";  // a confirmação digitada; vazia para o caso de recusa
            static_cast<void>(write(mestre, linha.constData(), static_cast<size_t>(linha.size())));
            confirmacaoEnviada = true;
        }
        if (!senhaEnviada && execucao.saida.contains("senha do certificado")) {
            QByteArray linha = senha + "\n";
            static_cast<void>(write(mestre, linha.constData(), static_cast<size_t>(linha.size())));
            senhaEnviada = true;
        }
    }
    if (expirou) {
        kill(pid, SIGKILL);  // a CLI não respondeu: mata antes do waitpid, que bloquearia
    }
    int status = 0;
    waitpid(pid, &status, 0);
    close(mestre);
    if (expirou) {
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
    const QString texto = QStringLiteral(
        "[sefin]\nurl = %5\nancoras = %1/ac.pem\n"
        "[xsd]\ndiretorio = %2\nmanifesto = %3\n"
        "[emissao]\nproducao_liberada = %4\n")
                              .arg(kCertificados, kXsd, kManifesto, producaoLiberada ? QStringLiteral("true") : QStringLiteral("false"), url);
    arquivo.write(texto.toUtf8());
}

// Emissão contra o servidor falso com uma falha no POST /nfse (DA-2.59, E0014). A distro com SHA-1 bloqueado nunca chega ao POST.
Execucao emitirContraServidorComFalhaNoPost(const QString& falha)
{
    QTemporaryDir configDir;
    ServidorFalso servidor(falha);
    if (!configDir.isValid() || servidor.porta() <= 0) {
        return Execucao{};
    }
    escreverConfigRecusa(configDir, false, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 QStringLiteral("--certificado"), kCertificados + QStringLiteral("/cnpj_ok.pfx")};
    return rodarCliNoTerminal(argumentos, configDir.path(), senha);
}

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
        "ancoras = %2/ac.pem\n"
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
                                 QStringLiteral("homologacao"), QStringLiteral("--certificado"),
                                 kCertificados + QStringLiteral("/cnpj_ok.pfx")};
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
                                 QStringLiteral("--certificado"), kCertificados + QStringLiteral("/cnpj_ok.pfx")};
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
                                 QStringLiteral("--certificado"), kCertificados + QStringLiteral("/cnpj_ok.pfx")};
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
                                 QStringLiteral("--certificado"), kCertificados + QStringLiteral("/cnpj_ok.pfx"),
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
                                 QStringLiteral("--certificado"), kCertificados + QStringLiteral("/cnpj_ok.pfx")};
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
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        QSKIP("distro com SHA-1 bloqueado: a emissão não chega ao POST (DA-2.38)");
    }
    const Execucao execucao = emitirContraServidorComFalhaNoPost(QStringLiteral("rejeicao_gerada:E0014"));
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("emitida: chave "), "o mesmo infDPS tem de recuperar a chave da nota");
}

void TstCliEmitir::e0014ComInfDpsDiferenteSaiComCinco()
{
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        QSKIP("distro com SHA-1 bloqueado: a emissão não chega ao POST (DA-2.38)");
    }
    const Execucao execucao = emitirContraServidorComFalhaNoPost(QStringLiteral("rejeicao_gerada_diferente:E0014"));
    QVERIFY2(execucao.codigo == 5, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("sefin.dps_duplicada"), "o infDPS diferente tem de sair como DPS duplicada");
}

void TstCliEmitir::e0014SemComparacaoSaiComSete()
{
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        QSKIP("distro com SHA-1 bloqueado: a emissão não chega ao POST (DA-2.38)");
    }
    const Execucao execucao = emitirContraServidorComFalhaNoPost(QStringLiteral("rejeicao_gerada_sem_comparacao:E0014"));
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
    if (esperadoDaDistro() == QStringLiteral("algoritmo_bloqueado")) {
        QSKIP("distro com SHA-1 bloqueado: a emissão não chega ao POST (DA-2.38)");
    }
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    ServidorFalso servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    escreverConfigRecusa(configDir, false, QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta()));
    const QByteArray senha = lerArquivo(kCertificados + QStringLiteral("/senha_teste.txt")).trimmed();
    const QStringList argumentos{QStringLiteral("emitir"), kEntradas + QStringLiteral("/mei_minima.json"),
                                 QStringLiteral("--serie"), QStringLiteral("1"), QStringLiteral("--numero"), QStringLiteral("1"),
                                 QStringLiteral("--ambiente"), QStringLiteral("homologacao"),
                                 QStringLiteral("--certificado"), kCertificados + QStringLiteral("/cnpj_ok.pfx")};
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    const QDir pasta(configDir.path() + QStringLiteral("/state/notalivre/respostas"));
    const QStringList arquivos = pasta.entryList({QStringLiteral("*.json")}, QDir::Files);
    QCOMPARE(arquivos.size(), 1);
    const QString caminho = pasta.filePath(arquivos.first());
    QVERIFY2(permissaoSoDoDono(caminho), "o arquivo da resposta tem de ser 0600: dono lê e escreve, grupo e outros sem bit");
    const QByteArray conteudo = lerArquivo(caminho);
    QVERIFY2(!conteudo.contains("nfseXmlGZipB64\":\"H4s"), "o XML da NFS-e (GZip em Base64) não pode ir ao arquivo");
    QVERIFY2(!conteudo.contains(senha), "a senha não pode ir ao arquivo");
    QVERIFY2(!conteudo.contains("11222333000181"), "o CNPJ do certificado não pode ir ao arquivo sem máscara");
}

QTEST_APPLESS_MAIN(TstCliEmitir)
#include "tst_cli_emitir.moc"
