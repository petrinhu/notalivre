// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do prestador (NL-1.7): inscrição (CNPJ ou CPF), IM e xNome opcionais, endereço opcional e regime tributário.
#include "campos_json.h"
#include "leitores_comuns.h"
#include "leitores_dps.h"

#include <nl/back/dps/identificacao_pessoa.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::Prestador, nl::base::Erro> lerPrestador(const QJsonObject& raiz)
{
    constexpr std::string_view caminho = "prestador";
    const auto prestador = campos::objetoObrigatorio(raiz, "", caminho);
    if (!prestador) {
        return std::unexpected(prestador.error());
    }

    const auto inscricao = lerInscricao(*prestador, caminho);
    if (!inscricao) {
        return std::unexpected(inscricao.error());
    }

    const auto imTexto = campos::textoOpcional(*prestador, caminho, "im");
    if (!imTexto) {
        return std::unexpected(imTexto.error());
    }
    const auto im = converterOpcional<nb::InscricaoMunicipal>(*imTexto, [](QStringView texto) {
        return nb::InscricaoMunicipal::de(texto, "IM");
    });
    if (!im) {
        return std::unexpected(im.error());
    }

    const auto xNomeTexto = campos::textoOpcional(*prestador, caminho, "xNome");
    if (!xNomeTexto) {
        return std::unexpected(xNomeTexto.error());
    }
    const auto xNome = converterOpcional<nb::NomeRazaoSocial>(*xNomeTexto, [](QStringView texto) {
        return nb::NomeRazaoSocial::de(texto, "xNome");
    });
    if (!xNome) {
        return std::unexpected(xNome.error());
    }

    const auto endereco = lerEnderecoOpcional(*prestador, caminho);
    if (!endereco) {
        return std::unexpected(endereco.error());
    }

    const auto regime = lerRegime(*prestador, caminho);
    if (!regime) {
        return std::unexpected(regime.error());
    }

    return nb::Prestador::montar(*inscricao, *im, *xNome, *endereco, std::nullopt, std::nullopt, *regime);
}

} // namespace nl::mid::composicao::leitores
