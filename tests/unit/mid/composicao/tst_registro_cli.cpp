// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do registro da CLI (NL-2.9, itens 5 e DA-2.54): exceção do topo sem mensagem, e Id da DPS mascarado no evento de
// emissão pela allowlist do cliente Sefin.
#include <nl/foundation/log/log.h>
#include <nl/mid/composicao/guarda_manipulador_qt.h>
#include <nl/mid/composicao/registro_cli.h>

#include <QString>
#include <QtTest>

#include <sstream>
#include <stdexcept>
#include <string>

#include <unistd.h>

namespace {

// Captura o que o processo escreve na saída de erro durante o trecho (descritor 2 redirecionado para um canal).
std::string capturarStderr(void (*trecho)())
{
    int canal[2];
    if (pipe(canal) != 0) {
        return "pipe-falhou";
    }
    const int salvo = dup(STDERR_FILENO);
    dup2(canal[1], STDERR_FILENO);
    trecho();
    dup2(salvo, STDERR_FILENO);
    close(salvo);
    close(canal[1]);
    std::string saida;
    char buffer[256];
    ssize_t lidos = 0;
    while ((lidos = read(canal[0], buffer, sizeof buffer)) > 0) {
        saida.append(buffer, static_cast<std::size_t>(lidos));
    }
    close(canal[0]);
    return saida;
}

nl::mid::composicao::RegistroCli* registroDoTrecho = nullptr;

// Aviso do Qt com CPF, dentro da guarda ligada ao registro, e depois dela: nada pode sair pela saída de erro (DA-2.52).
void trechoDoAvisoComCpf()
{
    {
        nl::mid::composicao::GuardaManipuladorQt guarda(registroDoTrecho->logDoCliente());
        qWarning("CPF 52998224725 no aviso do Qt, dentro da guarda");
    }
    qWarning("CPF 52998224725 no aviso do Qt, depois da guarda");
}


constexpr const char* kCpfFixture = "52998224725";
constexpr const char* kIdDps = "DPS430510821122233300018100001000000000000001";
constexpr const char* kInscricaoDoId = "11222233300018";

} // namespace

class TstRegistroCli : public QObject
{
    Q_OBJECT

private slots:
    // A mensagem com CPF não vai ao registro; o tipo e a posição vão, pelo registro que a fábrica monta.
    void excecaoNoTopoLogaSoTipoEPosicao();
    // O Id da DPS sai com a inscrição mascarada, pela allowlist do cliente Sefin (DA-2.54).
    void eventoDeEmissaoMascaraIdDaDps();
    // Mutante E (revisor): a allowlist da CLI não pode aceitar o campo what(); o texto da exceção nunca sai pelo registro.
    void allowlistDaCliNaoAceitaWhat();
    // Mutante J (revisor): a mensagem do Qt, mesmo ligada ao registro da CLI, não pode sair como campo tipo nem texto.
    void qWarningComCpfPeloRegistroNaoVaiAoLog();
    // DA-2.52: o aviso do Qt com CPF não sai pela saída de erro do processo, nem dentro nem depois da guarda, e não vai ao registro.
    void qWarningComCpfNaoVaiAoStderrNemAoLog();
};

void TstRegistroCli::excecaoNoTopoLogaSoTipoEPosicao()
{
    const std::runtime_error excecao(std::string("falha ao ler o CPF ") + kCpfFixture);
    std::ostringstream destino;
    auto registro = nl::mid::composicao::criarLogDaCli(destino);
    registro->registrarExcecaoNoTopo(excecao, "apps/cli/main.cpp:despachar");
    const std::string saida = destino.str();
    QVERIFY2(saida.find("cli.excecao") != std::string::npos, "o evento da exceção tem de sair no registro");
    QVERIFY2(saida.find("apps/cli/main.cpp:despachar") != std::string::npos, "a posição tem de ir ao registro");
    QVERIFY2(saida.find(kCpfFixture) == std::string::npos, "a mensagem da exceção (com CPF) não pode ir ao registro");
}

void TstRegistroCli::eventoDeEmissaoMascaraIdDaDps()
{
    std::ostringstream destino;
    auto registro = nl::mid::composicao::criarLogDaCli(destino);
    registro->registrarEmissaoAceita(kIdDps, "");
    const std::string saida = destino.str();
    QVERIFY2(saida.find(kInscricaoDoId) == std::string::npos, "a inscrição do Id da DPS não pode sair sem máscara");
    QVERIFY2(saida.find("emissao.aceita") != std::string::npos, "o evento de emissão aceita tem de sair no registro");
}

void TstRegistroCli::allowlistDaCliNaoAceitaWhat()
{
    std::ostringstream destino;
    auto registro = nl::mid::composicao::criarLogDaCli(destino);
    registro->logDoCliente()->registrar(nl::foundation::log::Nivel::Erro, "cli.excecao",
                                        {nl::foundation::log::Campo{"what", std::string("falha com o CPF ") + kCpfFixture},
                                         nl::foundation::log::Campo{"tipo", "std::runtime_error"}});
    const std::string saida = destino.str();
    QVERIFY2(saida.find(kCpfFixture) == std::string::npos, "o texto de what() não pode sair pelo registro da CLI");
    QVERIFY2(saida.find("what") == std::string::npos, "o campo what não está na allowlist da CLI");
}

void TstRegistroCli::qWarningComCpfPeloRegistroNaoVaiAoLog()
{
    std::ostringstream destino;
    auto registro = nl::mid::composicao::criarLogDaCli(destino);
    {
        nl::mid::composicao::GuardaManipuladorQt guarda(registro->logDoCliente());
        qWarning("CPF 52998224725 no aviso do Qt");
    }
    const std::string saida = destino.str();
    QVERIFY2(saida.find(kCpfFixture) == std::string::npos, "a mensagem do Qt com CPF não pode sair pelo registro da CLI");
    QVERIFY2(saida.find("qt.mensagem") != std::string::npos, "o evento qt.mensagem tem de existir, mas sem o texto");
}

void TstRegistroCli::qWarningComCpfNaoVaiAoStderrNemAoLog()
{
    std::ostringstream destino;
    auto registro = nl::mid::composicao::criarLogDaCli(destino);
    registroDoTrecho = registro.get();
    const std::string stderrDoProcesso = capturarStderr(trechoDoAvisoComCpf);
    registroDoTrecho = nullptr;
    QVERIFY2(stderrDoProcesso.find(kCpfFixture) == std::string::npos, "o CPF do aviso do Qt não pode sair pela saída de erro");
    QVERIFY2(stderrDoProcesso.empty(), "com a guarda ativa ou depois dela, a saída de erro do processo tem de ficar vazia");
    QVERIFY2(destino.str().find(kCpfFixture) == std::string::npos, "o CPF do aviso do Qt não pode ir ao registro");
}

QTEST_APPLESS_MAIN(TstRegistroCli)
#include "tst_registro_cli.moc"
