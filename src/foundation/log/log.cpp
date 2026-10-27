// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// STUB VERMELHO (NL-2.7, commit de teste): grava os campos crus, sem allowlist nem escape. Os testes de tst_log têm de falhar aqui.

#include <nl/foundation/log/log.h>

#include <expected>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nl::foundation::log {

Log::Log(std::ostream& destino, std::string servico, std::vector<DeclaracaoCampo> allowlist)
    : destino_{destino}, servico_{std::move(servico)}, allowlist_{std::move(allowlist)}
{
}

std::expected<void, nl::base::Erro> Log::registrar(Nivel nivel, std::string_view evento, const std::vector<Campo>& campos)
{
    destino_ << "{\"level\":" << static_cast<int>(nivel) << ",\"evento\":\"" << evento << "\",\"campos\":{";
    bool primeiro = true;
    for (const Campo& campo : campos) {
        if (!primeiro) {
            destino_ << ',';
        }
        primeiro = false;
        destino_ << '"' << campo.nome << "\":\"" << campo.valor << '"';
    }
    destino_ << "}}\n";
    return {};
}

std::size_t Log::descartados() const noexcept
{
    return descartados_;
}

std::optional<std::string> Log::valorGravavel(const Campo& campo) const
{
    return std::string{campo.valor};
}

} // namespace nl::foundation::log
