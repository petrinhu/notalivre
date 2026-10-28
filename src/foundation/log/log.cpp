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

// Separadores de linha Unicode que leitores tratam como quebra (splitlines do Python e similares). U+2028 e U+2029
// têm sequência própria; os C1 (U+0080 a U+009F, em UTF-8 C2 80 a C2 9F) são tratados numa faixa só.
constexpr std::string_view kSeparadorLinha = "\xE2\x80\xA8";     // U+2028
constexpr std::string_view kSeparadorParagrafo = "\xE2\x80\xA9"; // U+2029
constexpr unsigned char kPrimeiroC1Segundo = 0x80;               // segundo byte de U+0080 em UTF-8
constexpr unsigned char kUltimoC1Segundo = 0x9F;                 // segundo byte de U+009F em UTF-8

constexpr std::string_view kEventoInvalido = "evento.invalido";
constexpr std::size_t kTamanhoMaximoEvento = 64;

struct SequenciaEscapada
{
    std::string escape;
    std::size_t bytes;
};

/// Se o byte em posicao começa um separador Unicode ou um controle C1, devolve a sequência de escape e quantos bytes
/// ela ocupa. Os C1 incluem U+0085 (NEL) e U+009B (CSI de 8 bits), que terminais também interpretam.
std::optional<SequenciaEscapada> separadorUnicodeEm(std::string_view texto, std::size_t posicao)
{
    const std::string_view resto = texto.substr(posicao);
    if (resto.starts_with(kSeparadorLinha)) {
        return SequenciaEscapada{"\\u2028", kSeparadorLinha.size()};
    }
    if (resto.starts_with(kSeparadorParagrafo)) {
        return SequenciaEscapada{"\\u2029", kSeparadorParagrafo.size()};
    }
    if (resto.size() >= 2 && resto[0] == '\xC2') {
        const auto segundo = static_cast<unsigned char>(resto[1]);
        if (segundo >= kPrimeiroC1Segundo && segundo <= kUltimoC1Segundo) {
            return SequenciaEscapada{std::format("\\u{:04x}", static_cast<unsigned int>(segundo)), 2};
        }
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
/// ou CNPJ não cabe nele. É a segunda barreira do catálogo fechado (DA-2.24).
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
    const bool eventoOk = eventoAceito(evento);
    if (!eventoOk) {
        ++descartadosNesta;  // evento fora do catálogo conta como descarte, como um campo
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

bool Log::eventoAceito(std::string_view evento) const
{
    if (!eventoEhCodigo(evento)) {
        return false;
    }
    return std::ranges::any_of(catalogoEventos_, [evento](const std::string& declarado) { return declarado == evento; });
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
    const auto declarado = std::ranges::find_if(allowlist_, [&campo](const DeclaracaoCampo& declaracao) {
        return declaracao.nome == campo.nome;
    });
    if (declarado == allowlist_.end()) {
        return std::nullopt;  // campo sem declaração: descartado, valor nunca gravado (ADR-S4, seção 3)
    }
    if (ehCampoSoMascara(campo.nome)) {
        if (declarado->regra != Regra::InscricaoMascarada) {
            return std::nullopt;  // DA-2.23: idDps, idNfse e chaveAcesso só aceitam a máscara
        }
    } else if (ehCampoProibido(campo.nome)) {
        return std::nullopt;  // classe Proibido: sai em nenhuma hipótese, declarada ou não
    }
    return aplicarRegra(declarado->regra, campo.valor);
}

} // namespace nl::foundation::log
