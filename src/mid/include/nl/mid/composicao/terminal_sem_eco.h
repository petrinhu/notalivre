// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do terminal sem eco (NL-2.9, item 3 e item 2): /dev/tty, com ECHO desligado só durante a leitura. Mora na Mid
// porque a Front só enxerga a composição (matriz do canon, L-33). A senha vai direto para SenhaSegura, sem QByteArray nem QString
// no caminho; a confirmação é texto curto (CPF ou CNPJ) e volta como QString. Nunca vem por argumento.
#ifndef NL_MID_COMPOSICAO_TERMINAL_SEM_ECO_H
#define NL_MID_COMPOSICAO_TERMINAL_SEM_ECO_H

#include <nl/foundation/pkcs12/senha_segura.h>

#include <QString>

#include <optional>

namespace nl::mid::composicao {

/// Pede a senha no terminal sem eco. nullopt se não há terminal de controle, se a leitura falha ou se a linha passa do teto.
std::optional<nl::foundation::pkcs12::SenhaSegura> lerSenhaDoTerminal(const QString& prompt);

/// Pede a confirmação (CPF ou CNPJ digitado) no terminal sem eco. nullopt sem terminal, em falha de leitura ou acima do teto.
std::optional<QString> lerConfirmacaoDoTerminal(const QString& prompt);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_TERMINAL_SEM_ECO_H
