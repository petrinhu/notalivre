// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Envelope do POST /nfse da Sefin, leiaute de Produção Restrita de 20/10/2026 (NL-2.4, DA-2.4).
// INFERÊNCIA até a NL-2.10: corpo {"dpsXmlGZipB64": "<Base64 do GZip RFC 1952 do XML assinado>"} com Content-Type
// application/json. Fontes secundárias (docs/pesquisa/onda2_api_sefin.md, seção 1.2); nenhum manual oficial lido traz o envelope.
// Só esta pasta conhece a chave e o Content-Type: trocar o formato muda este arquivo, o .cpp dele e os testes.

#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_ENVELOPE_DPS_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_ENVELOPE_DPS_H

#include <nl/base/erro.h>

#include <QByteArray>

#include <expected>
#include <string_view>

namespace nl::mid::sefin::prodrest_20261020 {

/// Nome da chave do JSON do envelope. INFERÊNCIA até a NL-2.10 (três fontes secundárias concordam).
inline constexpr std::string_view kChaveEnvelopeDps = "dpsXmlGZipB64";

/// Content-Type do POST. INFERÊNCIA até a NL-2.10: nenhuma fonte oficial lida informa o cabeçalho.
inline constexpr std::string_view kContentTypeEnvelopeDps = "application/json";

/// Corpo pronto para enviar: JSON compacto e o Content-Type que o acompanha.
struct EnvelopeDps
{
    QByteArray corpo;
    QByteArray contentType;
};

/// Monta o envelope a partir do GZip (RFC 1952) do XML assinado, já compactado pelo chamador. Só codifica: não compacta
/// e não assina. Gzip vazio é recusado (envelope.vazio), porque um envelope sem XML nunca é uma DPS.
[[nodiscard]] std::expected<EnvelopeDps, nl::base::Erro> montarEnvelopeDps(const QByteArray& gzipDoXmlAssinado);

/// Desfaz o envelope e devolve o GZip que estava em dpsXmlGZipB64. Usado pelos testes de ida e volta.
[[nodiscard]] std::expected<QByteArray, nl::base::Erro> lerEnvelopeDps(const QByteArray& corpo);

} // namespace nl::mid::sefin::prodrest_20261020

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_ENVELOPE_DPS_H
