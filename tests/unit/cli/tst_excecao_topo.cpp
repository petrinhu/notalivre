// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da exceção do topo da CLI (NL-2.9, ADR-0005 d): o Log recebe tipo e posição, nunca a mensagem da exceção.
// O CPF da mensagem é o da fixture de referência do portão de PII (tests/fixtures/pii/referencia.txt). O registro também
// vai para um arquivo na pasta que o portão varre (NL_LOGS_PII_CLI_DIR).
#include "excecao_topo.h"

#include <nl/foundation/log/log.h>

#include <QDir>
#include <QString>
#include <QtTest>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

// CPF da fixture de referência (tests/fixtures/pii/referencia.txt, categoria cpf).
constexpr const char* kCpfFixture = "52998224725";

} // namespace

class TstExcecaoTopo : public QObject
{
    Q_OBJECT

private slots:
    // A mensagem com CPF não vai ao registro; o tipo e a posição vão.
    void excecaoNoTopoLogaSoTipoEPosicao();
};

void TstExcecaoTopo::excecaoNoTopoLogaSoTipoEPosicao()
{
    const std::runtime_error excecao(std::string("falha ao ler o CPF ") + kCpfFixture);

    std::ostringstream destino;
    nl::foundation::log::Log registro{destino, "notalivre-cli",
                                      {nl::foundation::log::DeclaracaoCampo{"tipo", nl::foundation::log::Regra::Cru},
                                       nl::foundation::log::DeclaracaoCampo{"posicao", nl::foundation::log::Regra::Cru}},
                                      {"cli.excecao"}};
    nl::cli::registrarExcecaoNoTopo(registro, excecao, "apps/cli/main.cpp:despachar");

    // O registro vai ao arquivo que o portão de PII varre (ADR-0005 e). Grava ANTES das asserções: se o código vazar o CPF,
    // o arquivo já existe com o vazamento e o portão reprova por PII, não por diretório ausente.
    const QString pasta = qEnvironmentVariable("NL_LOGS_PII_CLI_DIR");
    QVERIFY2(!pasta.isEmpty(), "NL_LOGS_PII_CLI_DIR tem de estar definido pelo CTest");
    QVERIFY2(QDir().mkpath(pasta), "a pasta de logs da CLI tem de existir");
    {
        std::ofstream arquivo(pasta.toStdString() + "/excecao_topo.log");
        QVERIFY2(arquivo.is_open(), "o log da exceção tem de abrir na pasta de logs da CLI");
        nl::foundation::log::Log registroArquivo{arquivo, "notalivre-cli",
                                                 {nl::foundation::log::DeclaracaoCampo{"tipo", nl::foundation::log::Regra::Cru},
                                                  nl::foundation::log::DeclaracaoCampo{"posicao", nl::foundation::log::Regra::Cru}},
                                                 {"cli.excecao"}};
        nl::cli::registrarExcecaoNoTopo(registroArquivo, excecao, "apps/cli/main.cpp:despachar");
    }

    const std::string saida = destino.str();
    QVERIFY2(saida.find(kCpfFixture) == std::string::npos, "a mensagem da exceção (com CPF) não pode ir ao Log");
    QVERIFY2(saida.find("apps/cli/main.cpp:despachar") != std::string::npos, "a posição tem de ir ao Log");
    QVERIFY2(saida.find("runtime_error") != std::string::npos, "o tipo tem de ir ao Log");
}

QTEST_APPLESS_MAIN(TstExcecaoTopo)
#include "tst_excecao_topo.moc"
