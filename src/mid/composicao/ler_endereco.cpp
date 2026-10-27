// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do endereço nacional (NL-1.7): cMun, cep, xLgr, nro, xBairro. O endereço no exterior não entra na entrada da v1.
#include "campos_json.h"
#include "leitores_comuns.h"

#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/identificacao/cep.h>
#include <nl/back/identificacao/codigo_municipio.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<std::optional<nb::Endereco>, nl::base::Erro> lerEnderecoOpcional(const QJsonObject& pai, std::string_view caminhoPai)
{
    const auto enderecoObjeto = campos::objetoOpcional(pai, caminhoPai, "endereco");
    if (!enderecoObjeto) {
        return std::unexpected(enderecoObjeto.error());
    }
    if (!enderecoObjeto->has_value()) {
        return std::optional<nb::Endereco>{std::nullopt};
    }
    const std::string caminho = campos::caminhoDe(caminhoPai, "endereco");
    const QJsonObject& endereco = **enderecoObjeto;

    const auto cMunTexto = campos::textoObrigatorio(endereco, caminho, "cMun");
    if (!cMunTexto) {
        return std::unexpected(cMunTexto.error());
    }
    const auto cMun = nb::CodigoMunicipio::de(*cMunTexto);
    if (!cMun) {
        return std::unexpected(cMun.error());
    }

    const auto cepTexto = campos::textoObrigatorio(endereco, caminho, "cep");
    if (!cepTexto) {
        return std::unexpected(cepTexto.error());
    }
    const auto cep = nb::Cep::de(*cepTexto);
    if (!cep) {
        return std::unexpected(cep.error());
    }

    const auto xLgrTexto = campos::textoObrigatorio(endereco, caminho, "xLgr");
    if (!xLgrTexto) {
        return std::unexpected(xLgrTexto.error());
    }
    const auto xLgr = nb::Logradouro::de(*xLgrTexto, "xLgr");
    if (!xLgr) {
        return std::unexpected(xLgr.error());
    }

    const auto nroTexto = campos::textoObrigatorio(endereco, caminho, "nro");
    if (!nroTexto) {
        return std::unexpected(nroTexto.error());
    }
    const auto nro = nb::NumeroEndereco::de(*nroTexto, "nro");
    if (!nro) {
        return std::unexpected(nro.error());
    }

    const auto xBairroTexto = campos::textoObrigatorio(endereco, caminho, "xBairro");
    if (!xBairroTexto) {
        return std::unexpected(xBairroTexto.error());
    }
    const auto xBairro = nb::Bairro::de(*xBairroTexto, "xBairro");
    if (!xBairro) {
        return std::unexpected(xBairro.error());
    }

    return std::optional<nb::Endereco>{nb::Endereco::montar(nb::LocalEndereco{nb::EnderecoNacional::montar(*cMun, *cep)}, *xLgr, *nro,
                                                            std::nullopt, *xBairro)};
}

} // namespace nl::mid::composicao::leitores
