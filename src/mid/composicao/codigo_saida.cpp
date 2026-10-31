// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de saída da emissão (NL-2.9, DA-2.57c). Ver codigo_saida.h. Listas exatas primeiro; prefixos depois; o resto é 70.
#include <nl/mid/composicao/codigo_saida.h>

#include <array>
#include <string>

namespace nl::mid::composicao {
namespace {

constexpr int kSaidaDocumento = 4;
constexpr int kSaidaAmbienteXsd = 2;
constexpr int kSaidaDuplicada = 5;
constexpr int kSaidaComparacaoIndisponivel = 7;
constexpr int kSaidaRecusa = 1;
constexpr int kSaidaInterna = 70;
// Configuração ou banco (NL-3.8, código 10): chave ausente, arquivo ou caminho ilegível, algoritmo fora da lista.
constexpr int kSaidaConfiguracao = 10;

bool comecaCom(std::string_view texto, std::string_view prefixo)
{
    return texto.substr(0, prefixo.size()) == prefixo;
}

} // namespace

int codigoDeSaidaDaEmissao(std::string_view codigo)
{
    constexpr std::array<std::string_view, 2> documentoExato{"xsd.invalido", "xsd.pos_assinatura"};
    constexpr std::array<std::string_view, 4> documentoPrefixo{"cpf.", "cnpj.", "dps.", "entrada."};
    constexpr std::array<std::string_view, 2> ambienteXsd{"xsd.esquema", "xsd.interno"};
    constexpr std::array<std::string_view, 5> recusaPrefixo{"credencial.", "assinatura.", "confirmacao.", "cofre.", "sefin."};
    // Configuração (NL-2.10a, emenda 4): saída 10 por entrada explícita, antes de qualquer prefixo, porque "assinatura." e
    // "certificado." não podem cair na recusa comum.
    // Âncora definida fora de Homologação (NL-2.10b, condição do CISO de 31/10/2026, emenda 2): também sai com 10.
    constexpr std::array<std::string_view, 6> configuracaoExato{"configuracao.chave_ausente", "configuracao.arquivo_ilegivel",
                                                                "certificado.caminho_ilegivel", "assinatura.algoritmo_invalido",
                                                                "configuracao.ancoras_a1_fora_de_homologacao",
                                                                "configuracao.ancoras_tls_fora_de_homologacao"};
    for (auto exato : configuracaoExato) {
        if (codigo == exato) {
            return kSaidaConfiguracao;
        }
    }
    // Falhas internas com código próprio (DA-2.59, item 3): não são recusa de regra nem documento, e saem como 70 por entrada explícita.
    constexpr std::array<std::string_view, 2> internoExato{"emissao.id_dps_invalido", "emissao.id_vazio"};
    for (auto exato : internoExato) {
        if (codigo == exato) {
            return kSaidaInterna;
        }
    }
    for (auto exato : documentoExato) {
        if (codigo == exato) {
            return kSaidaDocumento;
        }
    }
    for (auto prefixo : documentoPrefixo) {
        if (comecaCom(codigo, prefixo)) {
            return kSaidaDocumento;
        }
    }
    for (auto exato : ambienteXsd) {
        if (codigo == exato) {
            return kSaidaAmbienteXsd;
        }
    }
    if (codigo == "sefin.dps_duplicada") {
        return kSaidaDuplicada;
    }
    if (codigo == "sefin.comparacao_indisponivel") {
        return kSaidaComparacaoIndisponivel;
    }
    for (auto prefixo : recusaPrefixo) {
        if (comecaCom(codigo, prefixo)) {
            return kSaidaRecusa;
        }
    }
    return kSaidaInterna;
}

} // namespace nl::mid::composicao
