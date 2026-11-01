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
// Entrada da senha (NL-2.27, DA-2.68 e DA-2.16): sem terminal, terminal em segundo plano, descritor recusado, senha vazia ou acima
// do teto. São erros de uso da entrada, não recusa de regra: saem com 2 por entrada explícita, antes do prefixo cofre.
constexpr int kSaidaEntradaDaSenha = 2;
// Produção sem autorização headless (NL-2.27, DA-2.68): a senha por descritor não libera Produção antes da NL-3.9.
constexpr int kSaidaHeadlessSemAutorizacao = 11;

bool comecaCom(std::string_view texto, std::string_view prefixo)
{
    return texto.substr(0, prefixo.size()) == prefixo;
}

} // namespace

int codigoDeSaidaDaEmissao(std::string_view codigo)
{
    // xml.com_dtd (NL-2.21, Emenda 2 do CTO): DOCTYPE recusado pelo validador é culpa do documento, sai com 4 por entrada
    // explícita. Sem ela, cairia no padrão 70 (falha interna).
    constexpr std::array<std::string_view, 3> documentoExato{"xsd.invalido", "xsd.pos_assinatura", "xml.com_dtd"};
    constexpr std::array<std::string_view, 4> documentoPrefixo{"cpf.", "cnpj.", "dps.", "entrada."};
    constexpr std::array<std::string_view, 2> ambienteXsd{"xsd.esquema", "xsd.interno"};
    constexpr std::array<std::string_view, 5> recusaPrefixo{"credencial.", "assinatura.", "confirmacao.", "cofre.", "sefin."};
    // Configuração (NL-2.10a, emenda 4): saída 10 por entrada explícita, antes de qualquer prefixo, porque "assinatura." e
    // "certificado." não podem cair na recusa comum.
    // Âncora definida fora de Homologação (NL-2.10b, condição do CISO de 31/10/2026, emenda 2): também sai com 10.
    constexpr std::array<std::string_view, 11> configuracaoExato{"configuracao.chave_ausente", "configuracao.arquivo_ilegivel",
                                                                "certificado.caminho_ilegivel", "assinatura.algoritmo_invalido",
                                                                "configuracao.ancoras_a1_fora_de_homologacao",
                                                                "configuracao.ancoras_tls_fora_de_homologacao", "configuracao.raizes_diretorio_vazio",
                                                                "configuracao.xsd_manifesto_nao_fixado", "configuracao.xsd_fora_de_homologacao",
                                                                "configuracao.chave_duplicada", "configuracao.chave_vazia"};
    for (auto exato : configuracaoExato) {
        if (codigo == exato) {
            return kSaidaConfiguracao;
        }
    }
    constexpr std::array<std::string_view, 8> entradaDaSenhaExato{
        "cofre.senha_ausente", "cofre.terminal_em_segundo_plano", "cofre.descritor_invalido", "cofre.descritor_terminal",
        "cofre.descritor_legivel_por_outros", "cofre.senha_vazia", "cofre.senha_acima_do_teto", "cofre.descritor_falha_de_leitura"};
    for (auto exato : entradaDaSenhaExato) {
        if (codigo == exato) {
            return kSaidaEntradaDaSenha;
        }
    }
    if (codigo == "headless.sem_autorizacao") {
        return kSaidaHeadlessSemAutorizacao;
    }
    // Falhas internas com código próprio (DA-2.59, item 3): não são recusa de regra nem documento, e saem como 70 por entrada explícita.
    constexpr std::array<std::string_view, 3> internoExato{"emissao.id_dps_invalido", "emissao.id_vazio", "emissao.ambiente_divergente"};
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
