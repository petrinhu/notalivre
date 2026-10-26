// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do regime tributário do prestador (NL-1.7): opSimpNac, regApTribSN (opcional) e regEspTrib.
#include "campos_json.h"
#include "leitores_comuns.h"

#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/reg_ap_trib_sn.h>
#include <nl/back/dps/reg_esp_trib.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::RegimeTributacao, nl::base::Erro> lerRegime(const QJsonObject& pai, std::string_view caminhoPai)
{
    const std::string caminho = campos::caminhoDe(caminhoPai, "regime");
    const auto regimeObjeto = campos::objetoObrigatorio(pai, caminhoPai, "regime");
    if (!regimeObjeto) {
        return std::unexpected(regimeObjeto.error());
    }

    const auto opSimpNacTexto = campos::textoObrigatorio(*regimeObjeto, caminho, "opSimpNac");
    if (!opSimpNacTexto) {
        return std::unexpected(opSimpNacTexto.error());
    }
    const auto opSimpNac = nb::OpSimpNac::de(*opSimpNacTexto);
    if (!opSimpNac) {
        return std::unexpected(opSimpNac.error());
    }

    const auto regApTexto = campos::textoOpcional(*regimeObjeto, caminho, "regApTribSN");
    if (!regApTexto) {
        return std::unexpected(regApTexto.error());
    }
    std::optional<nb::RegApTribSN> regApTribSN;
    if (regApTexto->has_value()) {
        const auto valor = nb::RegApTribSN::de(**regApTexto);
        if (!valor) {
            return std::unexpected(valor.error());
        }
        regApTribSN = *valor;
    }

    const auto regEspTribTexto = campos::textoObrigatorio(*regimeObjeto, caminho, "regEspTrib");
    if (!regEspTribTexto) {
        return std::unexpected(regEspTribTexto.error());
    }
    const auto regEspTrib = nb::RegEspTrib::de(*regEspTribTexto);
    if (!regEspTrib) {
        return std::unexpected(regEspTrib.error());
    }

    return nb::RegimeTributacao::montar(*opSimpNac, regApTribSN, *regEspTrib);
}

} // namespace nl::mid::composicao::leitores
