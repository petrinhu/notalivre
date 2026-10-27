// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do total de tributos (NL-1.7), chave totTrib de valores: indicador OU valores monetários, nunca os dois.
#include "campos_json.h"
#include "leitores_valores.h"

#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/tributos_monetarios.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::TotalTributos, nl::base::Erro> lerTotTrib(const QJsonObject& valores, std::string_view caminhoValores)
{
    constexpr std::string_view chave = "totTrib";
    const auto totTrib = campos::objetoObrigatorio(valores, caminhoValores, chave);
    if (!totTrib) {
        return std::unexpected(totTrib.error());
    }
    const std::string caminho = campos::caminhoDe(caminhoValores, chave);

    const auto indTexto = campos::textoOpcional(*totTrib, caminho, "indTotTrib");
    if (!indTexto) {
        return std::unexpected(indTexto.error());
    }
    const auto vFedTexto = campos::textoOpcional(*totTrib, caminho, "vTotTribFed");
    if (!vFedTexto) {
        return std::unexpected(vFedTexto.error());
    }
    const auto vEstTexto = campos::textoOpcional(*totTrib, caminho, "vTotTribEst");
    if (!vEstTexto) {
        return std::unexpected(vEstTexto.error());
    }
    const auto vMunTexto = campos::textoOpcional(*totTrib, caminho, "vTotTribMun");
    if (!vMunTexto) {
        return std::unexpected(vMunTexto.error());
    }
    const bool temMonetario = vFedTexto->has_value() || vEstTexto->has_value() || vMunTexto->has_value();
    if (indTexto->has_value() && temMonetario) {
        return std::unexpected(nl::base::Erro{"entrada.campos_conflitantes", caminho,
                                              "informe indTotTrib ou os valores monetários de totTrib, não os dois"});
    }

    if (indTexto->has_value()) {
        const auto indicador = nb::IndicadorTotalTributos::de(**indTexto);
        if (!indicador) {
            return std::unexpected(indicador.error());
        }
        return nb::TotalTributos{*indicador};
    }

    const auto vFed = lerValorObrigatorio(*totTrib, caminho, "vTotTribFed");
    if (!vFed) {
        return std::unexpected(vFed.error());
    }
    const auto vEst = lerValorObrigatorio(*totTrib, caminho, "vTotTribEst");
    if (!vEst) {
        return std::unexpected(vEst.error());
    }
    const auto vMun = lerValorObrigatorio(*totTrib, caminho, "vTotTribMun");
    if (!vMun) {
        return std::unexpected(vMun.error());
    }
    return nb::TotalTributos{nb::TributosMonetarios::montar(*vFed, *vEst, *vMun)};
}

} // namespace nl::mid::composicao::leitores
