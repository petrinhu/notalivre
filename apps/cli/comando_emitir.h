// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre emitir` (NL-2.9). WIP: aplica as travas (argumentos, ambiente, Produção, CNPJ digitado, senha do
// terminal) e para antes do envio, que ainda não está ligado.
#ifndef NL_CLI_COMANDO_EMITIR_H
#define NL_CLI_COMANDO_EMITIR_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <expected>

namespace nl::mid::composicao {
class RegistroCli;
}

namespace nl::cli {

struct ArgumentosEmitir;

/// NL-2.10b, emenda 3: --serie e --numero entram na DPS. nDps da DPS diferente do --numero recusa com erro de uso (código 2),
/// sem escolher um dos dois. Devolve o JSON da DPS com serie e nDps aplicados.
/// Falha de série ou número com a saída da CLI: 4 para campo inválido no JSON (entrada.numero_invalido), 2 para argumento.
struct ErroNumero
{
    int saida;
    QString mensagem;
};

std::expected<QByteArray, ErroNumero> aplicarSerieENumero(const QByteArray& dpsJson, const ArgumentosEmitir& argumentos);

/// Código de saída: 0 emissão aceita; 1 recusa de regra (Produção bloqueada, confirmação não digitada ou não confere, envio
/// recusado); 2 uso, ambiente, endpoint ou senha ausente; 4 documento reprovado no leiaute (DPS de entrada ou XML assinado).
int executarEmitir(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro);

} // namespace nl::cli

#endif // NL_CLI_COMANDO_EMITIR_H
