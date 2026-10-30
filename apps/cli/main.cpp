// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ponto de entrada da CLI notalivre (NL-1.7, NL-2.9): liga o manipulador do Qt ao Log antes de tudo (ADR-0005 a), despacha
// `dps validar` e `emitir`, e registra no Log só o tipo e a posição de uma exceção que chegue ao topo (ADR-0005 d).
#include "comando_dps_validar.h"
#include "comando_emitir.h"
#include "excecao_topo.h"

#include <nl/foundation/log/log.h>
#include <nl/mid/composicao/manipulador_qt.h>

#include <QString>
#include <QStringList>
#include <QTextStream>

#include <cstdio>
#include <exception>
#include <iostream>

namespace {

int despachar(const QStringList& argumentos)
{
    if (argumentos.size() >= 2 && argumentos.at(0) == QLatin1String("dps") && argumentos.at(1) == QLatin1String("validar")) {
        return nl::cli::executarDpsValidar(argumentos);
    }
    if (!argumentos.isEmpty() && argumentos.at(0) == QLatin1String("emitir")) {
        return nl::cli::executarEmitir(argumentos);
    }
    QTextStream erro{stderr};
    erro << QStringLiteral("uso: notalivre dps validar <entrada.json> <saida.xml> --xsd <diretorio> --manifesto <arquivo>") << Qt::endl;
    erro << QStringLiteral("     notalivre emitir <entrada.json> --serie <S> --numero <N> --ambiente <A> [--cnpj-confirmado <CNPJ>]") << Qt::endl;
    return 2;
}

} // namespace

int main(int argc, char* argv[])
{
    // Registro da CLI: só campos declarados entram. A exceção do topo usa tipo e posicao (ADR-0005 d).
    nl::foundation::log::Log registro{std::cerr, "notalivre-cli",
                                      {nl::foundation::log::DeclaracaoCampo{"tipo", nl::foundation::log::Regra::Cru},
                                       nl::foundation::log::DeclaracaoCampo{"posicao", nl::foundation::log::Regra::Cru}},
                                      {"cli.excecao", "qt.mensagem"}};
    nl::mid::composicao::instalarManipuladorQt(&registro);  // ADR-0005 a: antes de qualquer outra coisa

    QStringList argumentos;
    for (int i = 1; i < argc; ++i) {
        argumentos.append(QString::fromLocal8Bit(argv[i]));
    }
    try {
        return despachar(argumentos);
    } catch (const std::exception& excecao) {
        nl::cli::registrarExcecaoNoTopo(registro, excecao, "apps/cli/main.cpp:despachar");
        nl::mid::composicao::instalarManipuladorQt(nullptr);
        return 2;
    }
}
