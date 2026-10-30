// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da exceção do topo da CLI (NL-2.9, ADR-0005 d): o Log recebe tipo e posição, nunca a mensagem da exceção.
#include "excecao_topo.h"

#include <nl/foundation/log/log.h>

#include <QtTest>

#include <sstream>
#include <stdexcept>

class TstExcecaoTopo : public QObject
{
    Q_OBJECT

private slots:
    // A mensagem com CPF não vai ao registro; o tipo e a posição vão.
    void excecaoNoTopoLogaSoTipoEPosicao();
};

void TstExcecaoTopo::excecaoNoTopoLogaSoTipoEPosicao()
{
    std::ostringstream destino;
    nl::foundation::log::Log registro{destino, "notalivre-cli",
                                      {nl::foundation::log::DeclaracaoCampo{"tipo", nl::foundation::log::Regra::Cru},
                                       nl::foundation::log::DeclaracaoCampo{"posicao", nl::foundation::log::Regra::Cru}},
                                      {"cli.excecao"}};
    const std::runtime_error excecao("falha ao ler o CPF 12345678901");
    nl::cli::registrarExcecaoNoTopo(registro, excecao, "apps/cli/main.cpp:despachar");
    const std::string saida = destino.str();
    QVERIFY2(saida.find("12345678901") == std::string::npos, "a mensagem da exceção (com CPF) não pode ir ao Log");
    QVERIFY2(saida.find("apps/cli/main.cpp:despachar") != std::string::npos, "a posição tem de ir ao Log");
    QVERIFY2(saida.find("runtime_error") != std::string::npos, "o tipo tem de ir ao Log");
}

QTEST_APPLESS_MAIN(TstExcecaoTopo)
#include "tst_excecao_topo.moc"
