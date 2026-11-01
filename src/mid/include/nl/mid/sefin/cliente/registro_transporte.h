// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Evento sefin.transporte_falhou (NL-2.10e, E4, DA-2.75): a falha do transporte no envio sai no log com campos de conjunto fechado.
// Nada do texto do Qt, nenhum host, caminho, subject ou URL. Valor fora do conjunto vira "outro".
#ifndef NL_MID_SEFIN_CLIENTE_REGISTRO_TRANSPORTE_H
#define NL_MID_SEFIN_CLIENTE_REGISTRO_TRANSPORTE_H

#include <nl/foundation/log/log.h>
#include <nl/foundation/transporte/transporte_https.h>

#include <QtGlobal>

#include <string_view>
#include <vector>

namespace nl::mid::sefin::cliente {

/// Registra a falha do transporte no envio. etapa: "POST" ou "GET"; qualquer outro valor vira "outro".
/// codigosSsl: inteiros do QSslError::SslError, no máximo 16 (vazio: "nenhum"). versaoTls: "tls1.2", "tls1.3" ou "nenhuma";
/// protocoloNegociado: "http/1.1", "h2" ou "nenhum". Valor fora do conjunto vira "outro".
void registrarTransporteFalhou(nl::foundation::log::Log& registro, nl::foundation::transporte::CausaErro causa, int codigoQt,
                               bool handshake, qint64 bytesEscritos, std::string_view etapa,
                               const std::vector<int>& codigosSsl = {}, std::string_view versaoTls = {},
                               std::string_view protocoloNegociado = {});

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_REGISTRO_TRANSPORTE_H
