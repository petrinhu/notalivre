// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ponto de entrada da CLI notalivre (NL-2.9, itens 4 e 5, DA-2.53): o registro e a guarda do manipulador do Qt nascem antes da
// QCoreApplication. A CLI só chama a raiz de composição da Mid; não inclui nada de nl/foundation/log.
#include "comando_dps_validar.h"
#include "comando_emitir.h"

#include <nl/mid/composicao/guarda_manipulador_qt.h>
#include <nl/mid/composicao/registro_cli.h>

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <exception>
#include <iostream>

namespace {

int despachar(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro)
{
    if (argumentos.size() >= 2 && argumentos.at(0) == QLatin1String("dps") && argumentos.at(1) == QLatin1String("validar")) {
        return nl::cli::executarDpsValidar(argumentos);
    }
    if (!argumentos.isEmpty() && argumentos.at(0) == QLatin1String("emitir")) {
        return nl::cli::executarEmitir(argumentos, registro);
    }
    QTextStream erro{stderr};
    erro << QStringLiteral("uso: notalivre dps validar <entrada.json> <saida.xml> --xsd <diretorio> --manifesto <arquivo>") << Qt::endl;
    erro << QStringLiteral("     notalivre emitir <entrada.json> --serie <S> --numero <N> --ambiente <A>") << Qt::endl;
    return 2;
}

} // namespace

int main(int argc, char* argv[])
{
    // Ordem fixa (NL-2.9, item 4): o registro nasce antes da QCoreApplication; a guarda liga o manipulador do Qt ao registro
    // e, ao sair, instala o descarte. A guarda é destruída antes do registro, por ordem inversa de declaração.
    auto registro = nl::mid::composicao::criarLogDaCli(std::cerr);
    nl::mid::composicao::GuardaManipuladorQt guarda(registro->logDoCliente());

    QCoreApplication aplicacao(argc, argv);  // o envio HTTPS usa o laço de eventos do Qt; sem a aplicação, nada termina

    QStringList argumentos;
    for (int i = 1; i < argc; ++i) {
        argumentos.append(QString::fromLocal8Bit(argv[i]));
    }
    try {
        return despachar(argumentos, *registro);
    } catch (const std::exception& excecao) {
        registro->registrarExcecaoNoTopo(excecao, "apps/cli/main.cpp:despachar");
        return 2;
    }
}
