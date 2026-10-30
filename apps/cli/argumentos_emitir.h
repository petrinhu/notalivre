// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura da linha de comando de `notalivre emitir` (NL-2.9). Só interpreta argumentos. Senha nunca entra por argumento.
#ifndef NL_CLI_ARGUMENTOS_EMITIR_H
#define NL_CLI_ARGUMENTOS_EMITIR_H

#include <QString>
#include <QStringList>

#include <expected>

namespace nl::cli {

/// Argumentos já conferidos. A série e o número vêm da linha de comando enquanto o protocolo durável (NL-3.7) não existe.
struct ArgumentosEmitir
{
    QString entrada;
    QString serie;
    QString numero;
    QString ambiente;
    QString cnpjConfirmado; ///< opcional; exigido para Produção (confirmação digitada, spec 4.12)
};

/// Espera a lista inteira da linha de comando, começando por "emitir".
/// Uso: notalivre emitir <entrada.json> --serie <S> --numero <N> --ambiente <A> [--cnpj-confirmado <14 dígitos>].
/// Qualquer opção com "senha" no nome é recusada: a senha é lida do terminal, sem eco.
std::expected<ArgumentosEmitir, QString> lerArgumentosEmitir(const QStringList& argumentos);

} // namespace nl::cli

#endif // NL_CLI_ARGUMENTOS_EMITIR_H
