// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Reprodução da NL-2.27 (passo 1): a leitura da senha num processo que está em segundo plano do terminal de controle. Com openpty,
// um filho vira líder de sessão com o escravo como terminal de controle (fica em primeiro plano) e cria um neto em grupo próprio,
// que é de segundo plano. O neto chama lerSenhaDoTerminal direto, como a CLI faria. Antes da correção, o tcsetattr do neto manda
// SIGTTOU e para o processo; depois, ele sai sem parar, com o motivo "segundo plano".
#include <nl/mid/cofre/senha_do_terminal.h>

#include <QString>
#include <QtTest>

#include <csignal>
#include <fcntl.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {

// Códigos de saída (NL-2.27): o neto sai com 30 + motivo do enum (SemTerminal 0, EmSegundoPlano 1, FalhaDeLeitura 2); 33 se a
// leitura devolveu senha, o que é inesperado sem digitação. O filho devolve 100 + o código do neto; 40 se o neto parou por SIGTTOU.
constexpr int kBaseDoNeto = 30;
constexpr int kSaidaComSenhaInesperada = 33;
constexpr int kSaidaDoFilhoParadoPorSigttou = 40;
constexpr int kSaidaDoFilhoComNeto = 100;

// Neto: o processo que tenta ler a senha direto do terminal. Restaura SIGTTOU ao padrão (o filho pode ter herdado SIG_IGN) e
// tem alarme de 10 s, para morrer em vez de ficar órfão se a leitura bloquear.
[[noreturn]] void corpoDoNeto()
{
    std::signal(SIGTTOU, SIG_DFL);
    alarm(10);
    const auto senha = nl::mid::cofre::lerSenhaDoTerminal(QStringLiteral("senha de teste: "));
    _exit(senha ? kSaidaComSenhaInesperada : kBaseDoNeto + static_cast<int>(senha.error()));
}

// Filho: líder de uma sessão nova. Com comTerminal, o escravo vira o terminal de controle e o filho fica em primeiro plano. Cria o
// neto em grupo próprio, espera-o parar ou sair, e devolve o resultado no código de saída.
[[noreturn]] void corpoDoFilho(int escravo, bool comTerminal)
{
    if (setsid() < 0) {
        _exit(90);
    }
    if (comTerminal && ioctl(escravo, TIOCSCTTY, 0) != 0) {
        _exit(91);
    }
    const pid_t neto = fork();
    if (neto < 0) {
        _exit(92);
    }
    if (neto == 0) {
        setpgid(0, 0);  // grupo próprio: fora do primeiro plano do terminal
        corpoDoNeto();
    }
    int status = 0;
    if (waitpid(neto, &status, WUNTRACED) < 0) {
        _exit(93);
    }
    if (WIFSTOPPED(status)) {
        kill(neto, SIGKILL);
        waitpid(neto, nullptr, 0);
        _exit(WSTOPSIG(status) == SIGTTOU ? kSaidaDoFilhoParadoPorSigttou : 41);
    }
    if (WIFEXITED(status)) {
        _exit(kSaidaDoFilhoComNeto + WEXITSTATUS(status));
    }
    _exit(200);  // o neto morreu por sinal (o alarme)
}

// Roda o cenário e devolve o código de saída do filho, ou um negativo se o cenário nem começou. O escravo continua aberto para o
// teste conferir o eco; o mestre fica com o teste.
int executarNoSegundoPlano(int& mestre, int& escravo, bool comTerminal)
{
    if (openpty(&mestre, &escravo, nullptr, nullptr, nullptr) != 0) {
        return -2;
    }
    const pid_t filho = fork();
    if (filho < 0) {
        return -3;
    }
    if (filho == 0) {
        close(mestre);
        corpoDoFilho(escravo, comTerminal);
    }
    int status = 0;
    if (waitpid(filho, &status, 0) < 0 || !WIFEXITED(status)) {
        return -4;
    }
    return WEXITSTATUS(status);
}

} // namespace

class TstTerminalSegundoPlano : public QObject
{
    Q_OBJECT

private slots:
    // Em segundo plano, a leitura da senha falha rápido com EmSegundoPlano e o processo sai sem parar.
    void terminalEmSegundoPlanoSaiSemParar();
    // Em segundo plano, nenhuma mudança de eco e nenhum byte do prompt chegam ao terminal.
    void terminalEmSegundoPlanoNaoMexeNoEco();
    // Sem terminal de controle, o motivo continua SemTerminal (o comportamento da 1ª execução do líder).
    void semTerminalContinuaSenhaAusente();
};

void TstTerminalSegundoPlano::terminalEmSegundoPlanoSaiSemParar()
{
    int mestre = -1;
    int escravo = -1;
    const int codigo = executarNoSegundoPlano(mestre, escravo, true);
    QVERIFY2(mestre >= 0, "openpty tem de funcionar no ambiente do teste");
    QVERIFY2(codigo != kSaidaDoFilhoParadoPorSigttou,
             "o neto em segundo plano parou por SIGTTOU: a leitura tocou o terminal antes de conferir o grupo");
    QCOMPARE(codigo, kSaidaDoFilhoComNeto + kBaseDoNeto + 1);
    close(escravo);
    close(mestre);
}

void TstTerminalSegundoPlano::terminalEmSegundoPlanoNaoMexeNoEco()
{
    int mestre = -1;
    int escravo = -1;
    const int codigo = executarNoSegundoPlano(mestre, escravo, true);
    QVERIFY2(mestre >= 0, "openpty tem de funcionar no ambiente do teste");
    QCOMPARE(codigo, kSaidaDoFilhoComNeto + kBaseDoNeto + 1);
    termios estado{};
    QCOMPARE(tcgetattr(escravo, &estado), 0);
    QVERIFY2((estado.c_lflag & ECHO) != 0, "quem está em segundo plano não pode desligar o eco do terminal");
    fcntl(mestre, F_SETFL, fcntl(mestre, F_GETFL) | O_NONBLOCK);
    char sobra[64];
    QVERIFY2(read(mestre, sobra, sizeof sobra) < 0, "nenhum byte do prompt pode aparecer no terminal");
    close(escravo);
    close(mestre);
}

void TstTerminalSegundoPlano::semTerminalContinuaSenhaAusente()
{
    int mestre = -1;
    int escravo = -1;
    const int codigo = executarNoSegundoPlano(mestre, escravo, false);
    QVERIFY2(mestre >= 0, "openpty tem de funcionar no ambiente do teste");
    QCOMPARE(codigo, kSaidaDoFilhoComNeto + kBaseDoNeto + 0);
    close(escravo);
    close(mestre);
}

QTEST_APPLESS_MAIN(TstTerminalSegundoPlano)
#include "tst_terminal_segundo_plano.moc"
