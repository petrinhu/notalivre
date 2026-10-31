// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura da senha do certificado no terminal sem eco (NL-2.9, item 3, DA-2.57b). A senha vai direto para SenhaSegura. Só a
// Mid usa este header: a CLI chama a fachada de emissão, que lê a senha por aqui.
#ifndef NL_MID_COFRE_SENHA_DO_TERMINAL_H
#define NL_MID_COFRE_SENHA_DO_TERMINAL_H

#include <nl/foundation/pkcs12/senha_segura.h>

#include <QString>

#include <optional>

namespace nl::mid::cofre {

/// Pede a senha no terminal sem eco. nullopt se não há terminal de controle, se a leitura falha ou se a linha passa do teto.
std::optional<nl::foundation::pkcs12::SenhaSegura> lerSenhaDoTerminal(const QString& prompt);

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_SENHA_DO_TERMINAL_H
