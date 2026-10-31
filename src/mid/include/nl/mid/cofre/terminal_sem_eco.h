// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura da confirmação (CPF ou CNPJ digitado) no terminal sem eco (NL-2.9, item 2, DA-2.57b). Mora no cofre. Este header
// não inclui nada de foundation: a CLI (Front) o usa, e a senha tem o seu próprio header, só da Mid (senha_do_terminal.h).
#ifndef NL_MID_COFRE_TERMINAL_SEM_ECO_H
#define NL_MID_COFRE_TERMINAL_SEM_ECO_H

#include <QString>

#include <optional>

namespace nl::mid::cofre {

/// Pede a confirmação no terminal sem eco. nullopt sem terminal, em falha de leitura ou acima do teto.
std::optional<QString> lerConfirmacaoDoTerminal(const QString& prompt);

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_TERMINAL_SEM_ECO_H
