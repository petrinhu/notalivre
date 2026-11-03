// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Manipulador de mensagens do Qt ligado ao Log (NL-2.9, ADR-0005 item a). Mora na raiz de composição da Mid. O texto da
// mensagem do Qt não tem declaração na allowlist, então nunca sai: o Log descarta. O Log precisa viver enquanto o manipulador
// estiver instalado. O qInstallMessageHandler não recebe contexto do usuário, por isso o ponteiro do registro é estático.
#ifndef NL_MID_COMPOSICAO_MANIPULADOR_QT_H
#define NL_MID_COMPOSICAO_MANIPULADOR_QT_H

namespace nl::foundation::log {
class Log;
}

namespace nl::mid::composicao {

/// Instala o manipulador apontado para o registro. nullptr desliga: sem Log, a mensagem do Qt é descartada (falha fechada).
void instalarManipuladorQt(nl::foundation::log::Log* registro);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_MANIPULADOR_QT_H
