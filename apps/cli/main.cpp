// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ponto de entrada da CLI notalivre (NL-2.9, itens 4 e 5, DA-2.53): o registro e a guarda do manipulador do Qt nascem antes da
// QCoreApplication. A CLI só chama a raiz de composição da Mid; não inclui nada de nl/foundation/log.
// NL-2.27b (DA-2.72): a posse do descritor da senha é tomada antes de tudo: antes do registro, da guarda, da QCoreApplication e de
// qualquer abertura de objeto do Qt. Sem isso, um eventfd da QCoreApplication (anon_inode, medido em 0d7da83) ocupava o número pedido.
#include "comando_config.h"
#include "comando_dps_validar.h"
#include "comando_emitir.h"

#include <nl/mid/composicao/codigo_saida.h>
#include <nl/mid/composicao/guarda_manipulador_qt.h>
#include <nl/mid/composicao/registro_cli.h>

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <exception>
#include <iostream>
#include <optional>

namespace {

int despachar(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro,
              const nl::mid::composicao::TomadaDaSenha* tomada)
{
    if (argumentos.size() >= 2 && argumentos.at(0) == QLatin1String("dps") && argumentos.at(1) == QLatin1String("validar")) {
        return nl::cli::executarDpsValidar(argumentos);
    }
    if (!argumentos.isEmpty() && argumentos.at(0) == QLatin1String("emitir")) {
        return nl::cli::executarEmitir(argumentos, registro, tomada);
    }
    if (!argumentos.isEmpty() && argumentos.at(0) == QLatin1String("config")) {
        return nl::cli::executarConfig(argumentos);
    }
    QTextStream erro{stderr};
    erro << QStringLiteral("uso: notalivre dps validar <entrada.json> <saida.xml> --xsd <diretorio> --manifesto <arquivo>") << Qt::endl;
    erro << QStringLiteral("     notalivre emitir <entrada.json> --serie <S> --numero <N> --ambiente <A>") << Qt::endl;
    return 2;
}

} // namespace

int main(int argc, char* argv[])
{
    // NL-2.27b (DA-2.72): primeiro de tudo. Recusa sai com o código de entrada da senha, sem nenhum evento de configuração.
    QStringList argumentos;
    for (int i = 1; i < argc; ++i) {
        argumentos.append(QString::fromLocal8Bit(argv[i]));
    }
    const auto posse = nl::cli::tomarEntradaDaSenhaNaLinha(argumentos);
    if (!posse) {
        QTextStream erro{stderr};
        erro << QString::fromStdString(posse.error().codigo) << ": " << QString::fromStdString(posse.error().mensagem) << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao(posse.error().codigo);
    }

    // Ordem fixa (NL-2.9, item 4): o registro nasce antes da QCoreApplication; a guarda liga o manipulador do Qt ao registro
    // e, ao sair, instala o descarte. A guarda é destruída antes do registro, por ordem inversa de declaração.
    auto registro = nl::mid::composicao::criarLogDaCli(std::cerr);
    nl::mid::composicao::GuardaManipuladorQt guarda(registro->logDoCliente());

    QCoreApplication aplicacao(argc, argv);  // o envio HTTPS usa o laço de eventos do Qt; sem a aplicação, nada termina

    const nl::mid::composicao::TomadaDaSenha* tomada = posse->has_value() ? &**posse : nullptr;
    try {
        return despachar(argumentos, *registro, tomada);
    } catch (const std::exception& excecao) {
        registro->registrarExcecaoNoTopo(excecao, "apps/cli/main.cpp:despachar");
        return 2;
    }
}
