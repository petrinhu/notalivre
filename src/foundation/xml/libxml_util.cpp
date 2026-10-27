// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Auxiliares privados da libxml2 (NL-1.6a). Ver libxml_util.h.
#include "libxml_util.h"

#include <cctype>
#include <cstddef>
#include <string>
#include <utility>

namespace nl::foundation::xml::detail {
namespace {

std::string aparado(const char* texto)
{
    if (texto == nullptr) {
        return {};
    }
    std::string resultado{texto};
    while (!resultado.empty() && std::isspace(static_cast<unsigned char>(resultado.back())) != 0) {
        resultado.pop_back();
    }
    return resultado;
}

// Nome dentro de "{ns}nome", a partir de inicio (que aponta para o '{' ou para o primeiro caractere do nome).
std::optional<std::string> nomeLocal(std::string_view mensagem, std::size_t inicio, char fimDoNome)
{
    if (inicio >= mensagem.size()) {
        return std::nullopt;
    }
    std::size_t nomeInicio = inicio;
    if (mensagem[inicio] == '{') {
        const std::size_t fechaNs = mensagem.find('}', inicio);
        if (fechaNs == std::string_view::npos) {
            return std::nullopt;
        }
        nomeInicio = fechaNs + 1;
    }
    const std::size_t nomeFim = mensagem.find(fimDoNome, nomeInicio);
    if (nomeFim == std::string_view::npos || nomeFim <= nomeInicio) {
        return std::nullopt;
    }
    return std::string{mensagem.substr(nomeInicio, nomeFim - nomeInicio)};
}

} // namespace

void coletarErro(void* userData, ErroLibxml erro)
{
    if (userData == nullptr || erro == nullptr) {
        return;
    }
    auto* coletor = static_cast<ColetorErros*>(userData);
    const std::string mensagem = aparado(erro->message);
    std::string texto = "linha " + std::to_string(erro->line) + ": " + mensagem;
    coletor->erros.emplace_back(coletor->codigo, campoDaMensagem(mensagem), std::move(texto));
}

nl::base::Erro erroDaUltimaLeitura(std::string codigo)
{
    const xmlError* ultimo = xmlGetLastError();
    if (ultimo == nullptr || ultimo->message == nullptr) {
        return nl::base::Erro{std::move(codigo), std::nullopt, "XML malformado (a libxml2 não deu detalhe)"};
    }
    std::string texto = "linha " + std::to_string(ultimo->line) + ": " + aparado(ultimo->message);
    return nl::base::Erro{std::move(codigo), std::nullopt, std::move(texto)};
}

std::optional<std::string> campoDaMensagem(std::string_view mensagem)
{
    constexpr std::string_view faltante{"Expected is ( {"};
    const std::size_t posFaltante = mensagem.find(faltante);
    if (posFaltante != std::string_view::npos) {
        // O elemento que falta é o esperado, não o que apareceu no lugar dele.
        const std::size_t inicio = posFaltante + faltante.size() - 1;
        const std::size_t fechaNs = mensagem.find('}', inicio);
        if (fechaNs != std::string_view::npos) {
            const std::size_t fim = mensagem.find(" )", fechaNs);
            if (fim != std::string_view::npos && fim > fechaNs + 1) {
                return std::string{mensagem.substr(fechaNs + 1, fim - fechaNs - 1)};
            }
        }
    }
    constexpr std::string_view elemento{"Element '"};
    const std::size_t posElemento = mensagem.find(elemento);
    if (posElemento != std::string_view::npos) {
        return nomeLocal(mensagem, posElemento + elemento.size(), '\'');
    }
    return std::nullopt;
}

} // namespace nl::foundation::xml::detail
