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
    ServidorFalso()
    {
        processo_.start(kPython, QStringList{QStringLiteral("-I"), kLancador, kCertificados, QStringLiteral("--com-oraculo")});
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
Execucao rodarCliNoTerminal(const QStringList& argumentos, const QString& configDir, const QByteArray& senha)
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
        execv(argv[0], argv.data());
        _exit(127);
    }
    Execucao execucao;
    if (pid < 0) {
        return execucao;
    }
    bool senhaEnviada = false;
    char buffer[1024];
    for (;;) {
        pollfd espera{mestre, POLLIN, 0};
        if (poll(&espera, 1, 60000) <= 0) {
            break;  // limite de espera: a CLI não respondeu
        }
        const ssize_t lidos = read(mestre, buffer, sizeof buffer);
        if (lidos <= 0) {
            break;  // fim do terminal: a CLI saiu
        }
        execucao.saida.append(buffer, static_cast<int>(lidos));
        if (!senhaEnviada && execucao.saida.contains("senha do certificado")) {
            QByteArray linha = senha + "\n";
            static_cast<void>(write(mestre, linha.constData(), static_cast<size_t>(linha.size())));
            senhaEnviada = true;
        }
    }
    int status = 0;
    waitpid(pid, &status, 0);
    close(mestre);
    if (WIFEXITED(status)) {
        execucao.codigo = WEXITSTATUS(status);
    }
    return execucao;
}

} // namespace

class TstCliEmitir : public QObject
{
    Q_OBJECT

private slots:
    // A DPS de entrada sai pela CLI, é assinada, aceita pelo servidor falso e a chave volta na saída. A senha é digitada no
    // prompt e não aparece na saída do terminal.
    void cliEmiteDpsAceitaPeloServidorFalso();
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

    const QStringList argumentos{kEntradas + QStringLiteral("/mei_minima.json"), QStringLiteral("--serie"), QStringLiteral("1"),
                                 QStringLiteral("--numero"), QStringLiteral("1"), QStringLiteral("--ambiente"),
                                 QStringLiteral("homologacao"), QStringLiteral("--certificado"),
                                 kCertificados + QStringLiteral("/cnpj_ok.pfx")};
    const Execucao execucao = rodarCliNoTerminal(argumentos, configDir.path(), senha);

    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1; saída do terminal: %2").arg(execucao.codigo).arg(QString::fromUtf8(execucao.saida))));
    QVERIFY2(execucao.saida.contains("emitida: chave "), "a CLI tem de mostrar a chave da NFS-e emitida");
    QVERIFY2(!execucao.saida.contains(senha), "a senha não pode aparecer no terminal");
}

QTEST_APPLESS_MAIN(TstCliEmitir)
#include "tst_cli_emitir.moc"
