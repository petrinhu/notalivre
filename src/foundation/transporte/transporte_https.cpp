// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Transporte HTTPS com mTLS (NL-2.5). Ver transporte_https.h.
// Fase vermelha: o stub recusa tudo com um erro que nenhum teste espera (Definitivo, Configuracao).
#include <nl/foundation/transporte/transporte_https.h>

namespace nl::foundation::transporte {

TransporteHttps::TransporteHttps(UrlBase urlBase, ConfiguracaoTls tls)
    : urlBase_(std::move(urlBase)), tls_(std::move(tls)) {}

std::expected<RespostaHttp, ErroTransporte> TransporteHttps::enviar(const RequisicaoHttp&) {
    return std::unexpected(ErroTransporte{TipoErro::Definitivo, CausaErro::Configuracao,
                                          QStringLiteral("transporte ainda não implementado (fase vermelha)")});
}

} // namespace nl::foundation::transporte
