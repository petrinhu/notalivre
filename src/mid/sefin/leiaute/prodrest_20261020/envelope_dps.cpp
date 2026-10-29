// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// STUB VERMELHO (NL-2.4, commit de teste): ainda não monta nem lê o envelope. Os testes de tst_envelope_dps têm de falhar aqui.

#include <nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h>

#include <nl/base/erro.h>

#include <expected>
#include <optional>

namespace nl::mid::sefin::prodrest_20261020 {

std::expected<EnvelopeDps, nl::base::Erro> montarEnvelopeDps(const QByteArray&)
{
    return std::unexpected(nl::base::Erro{"envelope.nao_implementado", std::nullopt, "envelope ainda não montado"});
}

std::expected<QByteArray, nl::base::Erro> lerEnvelopeDps(const QByteArray&)
{
    return std::unexpected(nl::base::Erro{"envelope.nao_implementado", std::nullopt, "envelope ainda não lido"});
}

} // namespace nl::mid::sefin::prodrest_20261020
