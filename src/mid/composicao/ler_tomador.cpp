// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do tomador (NL-1.7), opcional: CNPJ ou CPF, xNome opcional e endereço nacional opcional.
#include "campos_json.h"
#include "leitores_comuns.h"
#include "leitores_dps.h"

#include <nl/back/dps/identificacao_pessoa.h>

#include <variant>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<std::optional<nb::Tomador>, nl::base::Erro> lerTomador(const QJsonObject& raiz)
{
    constexpr std::string_view caminho = "tomador";
    const auto tomador = campos::objetoOpcional(raiz, "", caminho);
    if (!tomador) {
        return std::unexpected(tomador.error());
    }
    if (!tomador->has_value()) {
        return std::optional<nb::Tomador>{std::nullopt};
    }
    const QJsonObject& objeto = **tomador;

    const auto inscricao = lerInscricao(objeto, caminho);
    if (!inscricao) {
        return std::unexpected(inscricao.error());
    }

    const auto xNomeTexto = campos::textoOpcional(objeto, caminho, "xNome");
    if (!xNomeTexto) {
        return std::unexpected(xNomeTexto.error());
    }
    const auto xNome = converterOpcional<nb::NomeRazaoSocial>(*xNomeTexto, [](QStringView texto) {
        return nb::NomeRazaoSocial::de(texto, "xNome");
    });
    if (!xNome) {
        return std::unexpected(xNome.error());
    }

    const auto endereco = lerEnderecoOpcional(objeto, caminho);
    if (!endereco) {
        return std::unexpected(endereco.error());
    }

    // Inscrição de CNPJ ou CPF vira IdentificacaoPessoa pela alternativa que já existe no variant: a fábrica do Back não muda.
    const nb::IdentificacaoPessoa identificacao = std::visit([](const auto& inscrita) {
        return nb::IdentificacaoPessoa{inscrita};
    }, *inscricao);

    return std::optional<nb::Tomador>{
        nb::Tomador::montar(identificacao, std::nullopt, *xNome, *endereco, std::nullopt, std::nullopt)};
}

} // namespace nl::mid::composicao::leitores
