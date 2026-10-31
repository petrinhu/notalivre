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

/// Código de saída: 0 emissão aceita; 1 recusa de regra (Produção bloqueada, confirmação não digitada ou não confere, envio
/// recusado); 2 uso, ambiente, endpoint ou senha ausente; 4 documento reprovado no leiaute (DPS de entrada ou XML assinado).
int executarEmitir(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro);

} // namespace nl::cli

#endif // NL_CLI_COMANDO_EMITIR_H
