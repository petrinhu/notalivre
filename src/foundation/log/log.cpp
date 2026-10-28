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

// Separadores Unicode que leitores de linha tratam como quebra (splitlines do Python e similares).
constexpr std::string_view kNel = "\xC2\x85";        // U+0085
constexpr std::string_view kSeparadorLinha = "\xE2\x80\xA8";     // U+2028
constexpr std::string_view kSeparadorParagrafo = "\xE2\x80\xA9"; // U+2029

constexpr std::string_view kEventoInvalido = "evento.invalido";
constexpr std::size_t kTamanhoMaximoEvento = 64;

struct SequenciaEscapada
{
    std::string_view escape;
    std::size_t bytes;
};

/// Se o byte em posicao começa um separador Unicode, devolve a sequência de escape e quantos bytes ela ocupa.
std::optional<SequenciaEscapada> separadorUnicodeEm(std::string_view texto, std::size_t posicao)
{
    const std::string_view resto = texto.substr(posicao);
    if (resto.starts_with(kNel)) {
        return SequenciaEscapada{"\\u0085", kNel.size()};
    }
    if (resto.starts_with(kSeparadorLinha)) {
        return SequenciaEscapada{"\\u2028", kSeparadorLinha.size()};
    }
    if (resto.starts_with(kSeparadorParagrafo)) {
        return SequenciaEscapada{"\\u2029", kSeparadorParagrafo.size()};
    }
    return std::nullopt;
}

/// Acrescenta um byte ao JSON. Aspas e barra viram sequência; controle (0x00 a 0x1F) nunca sai cru, para que quebra
/// ou ESC injetado num valor não crie linha falsa nem sequência de terminal no log (ADR-S4, seção 3).
void anexarCaractere(std::string& saida, char caractere)
{
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

std::string escaparJson(std::string_view texto)
{
    std::string saida;
    saida.reserve(texto.size());
    for (std::size_t posicao = 0; posicao < texto.size(); ++posicao) {
        if (const std::optional<SequenciaEscapada> separador = separadorUnicodeEm(texto, posicao)) {
            saida += separador->escape;
            posicao += separador->bytes - 1;
            continue;
        }
        anexarCaractere(saida, texto[posicao]);
    }
    return saida;
}

/// Nome de evento é código, não texto: letras minúsculas, ponto e sublinhado, até 64 caracteres. Sem dígito, um CPF
/// ou CNPJ não cabe nele. Fora disso o evento vira "evento.invalido" e conta como descartado (ADR-S4, seção 3).
bool eventoEhCodigo(std::string_view evento)
{
    if (evento.empty() || evento.size() > kTamanhoMaximoEvento) {
        return false;
    }
    return std::ranges::all_of(evento, [](char caractere) {
        return (caractere >= 'a' && caractere <= 'z') || caractere == '.' || caractere == '_';
    });
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

Log::Log(std::ostream& destino, std::string servico, std::vector<DeclaracaoCampo> allowlist,
         std::vector<std::string> catalogoEventos)
    : destino_{destino}, servico_{std::move(servico)}, allowlist_{std::move(allowlist)},
      catalogoEventos_{std::move(catalogoEventos)}
{
}

std::expected<void, nl::base::Erro> Log::registrar(Nivel nivel, std::string_view evento, const std::vector<Campo>& campos)
{
    std::size_t descartadosNesta = 0;
    const bool eventoOk = eventoEhCodigo(evento);
    if (!eventoOk) {
        ++descartadosNesta;
    }

    std::string linha = "{\"level\":\"";
    linha += nomeDoNivel(nivel);
    linha += "\",\"ts\":\"";
    linha += carimboUtc();
    linha += "\",\"service\":\"";
    linha += escaparJson(servico_);
    linha += "\",\"evento\":\"";
    linha += escaparJson(eventoOk ? evento : kEventoInvalido);
    linha += "\",\"campos\":{";
    linha += serializarCampos(campos, descartadosNesta);
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

std::string Log::serializarCampos(const std::vector<Campo>& campos, std::size_t& descartados) const
{
    std::string pares;
    bool primeiro = true;
    for (const Campo& campo : campos) {
        const std::optional<std::string> valor = valorGravavel(campo);
        if (!valor) {
            ++descartados;
            continue;
        }
        if (!primeiro) {
            pares += ',';
        }
        primeiro = false;
        pares += '"';
        pares += escaparJson(campo.nome);
        pares += "\":\"";
        pares += escaparJson(*valor);
        pares += '"';
    }
    return pares;
}

std::size_t Log::descartados() const noexcept
{
    return descartados_;
}

std::optional<std::string> Log::valorGravavel(const Campo& campo) const
{
    if (ehCampoProibido(campo.nome)) {
        return std::nullopt;  // classe Proibido da ADR-0005 §1: sai em nenhuma hipótese, declarada ou não
    }
    const auto declarado = std::ranges::find_if(allowlist_, [&campo](const DeclaracaoCampo& declaracao) {
        return declaracao.nome == campo.nome;
    });
    if (declarado == allowlist_.end()) {
        return std::nullopt;  // campo sem declaração: descartado, valor nunca gravado (ADR-S4, seção 3)
    }
    return aplicarRegra(declarado->regra, campo.valor);
}

} // namespace nl::foundation::log
