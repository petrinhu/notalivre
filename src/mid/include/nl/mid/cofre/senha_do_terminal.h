// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura da senha do certificado no terminal sem eco (NL-2.9, item 3, DA-2.57b). A senha vai direto para SenhaSegura. Só a
// Mid usa este header: a CLI chama a fachada de emissão, que lê a senha por aqui.
// NL-2.27 (DA-2.68, correção 2): o motivo da falha sai pelo enum, para a CLI dar a mensagem certa. Terminal em segundo plano
// (o processo não está no grupo de primeiro plano do terminal de controle) é falha rápida, antes de qualquer tcsetattr que
// pararia o processo com SIGTTOU.
#ifndef NL_MID_COFRE_SENHA_DO_TERMINAL_H
#define NL_MID_COFRE_SENHA_DO_TERMINAL_H

#include <nl/foundation/pkcs12/senha_segura.h>

#include <QString>

#include <expected>

namespace nl::mid::cofre {

/// Motivo de a senha não ter sido lida no terminal. Cada um vira um código estável na fachada.
enum class ErroDoTerminal {
    SemTerminal,      ///< Não há terminal de controle (/dev/tty ausente): a CLI não tem como pedir a senha.
    EmSegundoPlano,   ///< O terminal existe, mas o processo está em segundo plano: a CLI não está em primeiro plano nele.
    FalhaDeLeitura,   ///< O terminal existe e está em primeiro plano, mas a leitura falhou ou a linha passou do teto.
};

/// Pede a senha no terminal sem eco. Devolve a senha, ou o motivo da falha. Nunca para o processo por SIGTTOU.
std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoTerminal> lerSenhaDoTerminal(const QString& prompt);

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_SENHA_DO_TERMINAL_H
