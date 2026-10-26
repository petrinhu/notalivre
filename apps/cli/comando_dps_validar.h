// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre dps validar` (NL-1.7): lê a entrada, chama a fachada da Mid e grava o XML só se a DPS passar.
#ifndef NL_CLI_COMANDO_DPS_VALIDAR_H
#define NL_CLI_COMANDO_DPS_VALIDAR_H

#include <QStringList>

namespace nl::cli {

/// Código de saída: 0 DPS gravada; 1 entrada ou DPS reprovada (erros no stderr); 2 uso, erro de arquivo ou falha de ambiente
/// (XSD ausente, manifesto ilegível, libxml2 com falha interna).
int executarDpsValidar(const QStringList& argumentos);

} // namespace nl::cli

#endif // NL_CLI_COMANDO_DPS_VALIDAR_H
