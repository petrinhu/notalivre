// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura de senha do terminal sem eco (NL-2.9): /dev/tty, com ECHO desligado só durante a leitura. Nunca vem por argumento.
#ifndef NL_CLI_SENHA_TERMINAL_H
#define NL_CLI_SENHA_TERMINAL_H

#include <QByteArray>
#include <QString>

#include <optional>

namespace nl::cli {

/// Pede a senha no terminal sem eco. Vazio quando não há terminal de controle (a CLI não lê senha de entrada redirecionada).
std::optional<QByteArray> lerSenhaSemEco(const QString& prompt);

} // namespace nl::cli

#endif // NL_CLI_SENHA_TERMINAL_H
