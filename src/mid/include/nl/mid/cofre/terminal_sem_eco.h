// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do terminal sem eco (NL-2.9, itens 2 e 3, DA-2.57b): /dev/tty, com ECHO desligado só durante a leitura. Mora em
// src/mid/cofre, junto do material que ela entrega (sem diretório novo). A senha vai direto para SenhaSegura, sem QByteArray nem QString
// no caminho; a confirmação é texto curto (CPF ou CNPJ) e volta como QString. Nunca vem por argumento.
#ifndef NL_MID_COFRE_TERMINAL_SEM_ECO_H
#define NL_MID_COFRE_TERMINAL_SEM_ECO_H

#include <nl/foundation/pkcs12/senha_segura.h>

#include <QString>

#include <optional>

namespace nl::mid::cofre {

/// Pede a senha no terminal sem eco. nullopt se não há terminal de controle, se a leitura falha ou se a linha passa do teto.
std::optional<nl::foundation::pkcs12::SenhaSegura> lerSenhaDoTerminal(const QString& prompt);

/// Pede a confirmação (CPF ou CNPJ digitado) no terminal sem eco. nullopt sem terminal, em falha de leitura ou acima do teto.
std::optional<QString> lerConfirmacaoDoTerminal(const QString& prompt);

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_TERMINAL_SEM_ECO_H
