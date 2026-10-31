// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre emitir` (NL-2.9). WIP: aplica as travas (argumentos, ambiente, Produção, CNPJ digitado, senha do
// terminal) e para antes do envio, que ainda não está ligado.
#ifndef NL_CLI_COMANDO_EMITIR_H
#define NL_CLI_COMANDO_EMITIR_H

#include <QStringList>

namespace nl::mid::composicao {
class RegistroCli;
}

namespace nl::cli {

/// Código de saída: 0 emissão concluída (ainda não alcançável nesta fatia WIP); 1 recusa de regra (Produção bloqueada, CNPJ
/// não digitado); 2 uso, ambiente ou senha ausente; 3 envio ainda não ligado (estado WIP).
int executarEmitir(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro);

} // namespace nl::cli

#endif // NL_CLI_COMANDO_EMITIR_H
