// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura da linha de comando de `notalivre dps validar` (NL-1.7). Só interpreta argumentos; não abre arquivo.
#ifndef NL_CLI_ARGUMENTOS_DPS_VALIDAR_H
#define NL_CLI_ARGUMENTOS_DPS_VALIDAR_H

#include <QString>
#include <QStringList>

#include <expected>

namespace nl::cli {

/// Argumentos já conferidos: caminhos de entrada, de saída e a localização do XSD de Produção Restrita.
struct ArgumentosDpsValidar
{
    QString entrada;
    QString saida;
    QString diretorioXsd;
    QString manifesto;
};

/// Espera a lista inteira da linha de comando, começando por "dps" e "validar".
/// Uso: notalivre dps validar <entrada.json> <saida.xml> --xsd <diretorio> --manifesto <arquivo>.
/// Falha: texto de uso em pt-br, com o motivo.
std::expected<ArgumentosDpsValidar, QString> lerArgumentosDpsValidar(const QStringList& argumentos);

} // namespace nl::cli

#endif // NL_CLI_ARGUMENTOS_DPS_VALIDAR_H
