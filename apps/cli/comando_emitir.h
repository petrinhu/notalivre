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
#include <optional>

#include <nl/mid/composicao/emissao_cli.h>

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
    QString codigo;  ///< código estável do erro (entrada.numero_invalido, emissao.numero_conflitante, argumento.numero_invalido)
    QString mensagem;
};

std::expected<QByteArray, ErroNumero> aplicarSerieENumero(const QByteArray& dpsJson, const ArgumentosEmitir& argumentos);

/// Código de saída: 0 emissão aceita; 1 recusa de regra (Produção bloqueada, confirmação não digitada ou não confere, envio
/// recusado); 2 uso, ambiente, endpoint ou senha ausente; 4 documento reprovado no leiaute (DPS de entrada ou XML assinado).
int executarEmitir(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro,
                    const nl::mid::composicao::TomadaDaSenha* tomada = nullptr);

/// NL-2.27b (DA-2.72): toma a posse do descritor da senha na linha de `emitir`, antes da QCoreApplication e de qualquer abertura de
/// arquivo. Troca o número pedido na linha pelo número do descritor tomado. Sem a opção, devolve vazio e não faz nada.
/// Recusa: o código de entrada da senha (cofre.*), com a saída que o chamador aplica.
std::expected<std::optional<nl::mid::composicao::TomadaDaSenha>, nl::mid::composicao::FalhaEmissao> tomarEntradaDaSenhaNaLinha(
    QStringList& argumentos);

} // namespace nl::cli

#endif // NL_CLI_COMANDO_EMITIR_H
