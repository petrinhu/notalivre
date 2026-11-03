// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Allowlist do Log do cliente Sefin (NL-2.9, DA-2.54): definida uma vez no Mid sefin e usada pelo registro da CLI. Campos de
// Id e de chave saem mascarados; código E sai só depois de validado. Nenhuma segunda cópia (L-34).
#ifndef NL_MID_SEFIN_CLIENTE_ALLOWLIST_LOG_SEFIN_H
#define NL_MID_SEFIN_CLIENTE_ALLOWLIST_LOG_SEFIN_H

#include <nl/foundation/log/log.h>

#include <string>
#include <vector>

namespace nl::mid::sefin::cliente {

/// Campos que o cliente Sefin e a emissão podem registrar: metodo, status, codigoE (Cru); idDps e chaveAcesso (mascarados).
std::vector<nl::foundation::log::DeclaracaoCampo> allowlistLogSefin();

/// Eventos do cliente Sefin e da emissão que o catálogo fechado aceita.
std::vector<std::string> eventosLogSefin();

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_ALLOWLIST_LOG_SEFIN_H
