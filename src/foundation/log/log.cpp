// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Log da Foundation (NL-2.7, ADR-S4): uma linha JSON por registro, com allowlist e redação no ponto de escrita.

#include <nl/foundation/log/log.h>
#include <nl/foundation/log/redacao.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nl::foundation::log {
namespace {

/// Escape de JSON. Aspas e barra viram sequência; controle (0x00 a 0x1F) nunca sai cru, para que quebra de linha
/// injetada num valor não crie linha falsa no log (ADR-S4, seção 3, sanitização).
std::string escaparJson(std::string_view texto)
{
    std::string saida;
    saida.reserve(texto.size());
    for (const char caractere : texto) {
        switch (caractere) {
        case '"':
            saida += "\\\"";
            break;
        case '\\':
            saida += "\\\\";
            break;
        case '\n':
            saida += "\\n";
            break;
        case '\r':
            saida += "\\r";
            break;
        case '\t':
            saida += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(caractere) < 0x20) {
                saida += std::format("\\u{:04x}", static_cast<unsigned char>(caractere));
            } else {
                saida += caractere;
            }
            break;
        }
    }
    return saida;
}

std::string_view nomeDoNivel(Nivel nivel)
{
    switch (nivel) {
    case Nivel::Info:
        return "info";
    case Nivel::Aviso:
        return "aviso";
    case Nivel::Erro:
        return "erro";
    }
    return "erro";  // falha fechada: nível desconhecido vira erro, nunca some
}

std::string carimboUtc()
{
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}

} // namespace

Log::Log(std::ostream& destino, std::string servico, std::vector<DeclaracaoCampo> allowlist)
    : destino_{destino}, servico_{std::move(servico)}, allowlist_{std::move(allowlist)}
{
}

std::expected<void, nl::base::Erro> Log::registrar(Nivel nivel, std::string_view evento, const std::vector<Campo>& campos)
{
    std::string linha = "{\"level\":\"";
    linha += nomeDoNivel(nivel);
    linha += "\",\"ts\":\"";
    linha += carimboUtc();
    linha += "\",\"service\":\"";
    linha += escaparJson(servico_);
    linha += "\",\"evento\":\"";
    linha += escaparJson(evento);
    linha += "\",\"campos\":{";

    std::size_t descartadosNesta = 0;
    bool primeiro = true;
    for (const Campo& campo : campos) {
        const std::optional<std::string> valor = valorGravavel(campo);
        if (!valor) {
            ++descartadosNesta;
            continue;
        }
        if (!primeiro) {
            linha += ',';
        }
        primeiro = false;
        linha += '"';
        linha += escaparJson(campo.nome);
        linha += "\":\"";
        linha += escaparJson(*valor);
        linha += '"';
    }
    linha += "},\"descartados\":";
    linha += std::to_string(descartadosNesta);
    linha += "}\n";
    descartados_ += descartadosNesta;

    destino_ << linha;
    if (!destino_) {
        return std::unexpected(nl::base::Erro{"log.escrita", std::nullopt, "não consegui gravar a linha do log"});
    }
    return {};
}

std::size_t Log::descartados() const noexcept
{
    return descartados_;
}

std::optional<std::string> Log::valorGravavel(const Campo& campo) const
{
    const auto declarado = std::ranges::find_if(allowlist_, [&campo](const DeclaracaoCampo& declaracao) {
        return declaracao.nome == campo.nome;
    });
    if (declarado == allowlist_.end()) {
        return std::nullopt;  // campo sem declaração: descartado, valor nunca gravado (ADR-S4, seção 3)
    }
    return aplicarRegra(declarado->regra, campo.valor);
}

} // namespace nl::foundation::log
