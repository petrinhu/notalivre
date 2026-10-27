// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do grupo valores (NL-1.7): compõe vServ, tribMun e totTrib, cada um lido pelo seu próprio arquivo.
#include "campos_json.h"
#include "leitores_dps.h"
#include "leitores_valores.h"

#include <nl/back/dps/tributacao.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::ValoresServico, nl::base::Erro> lerValores(const QJsonObject& raiz)
{
    constexpr std::string_view caminho = "valores";
    const auto valores = campos::objetoObrigatorio(raiz, "", caminho);
    if (!valores) {
        return std::unexpected(valores.error());
    }

    const auto vServ = lerValorObrigatorio(*valores, caminho, "vServ");
    if (!vServ) {
        return std::unexpected(vServ.error());
    }

    const auto tribMun = lerTributacaoMunicipal(*valores, caminho);
    if (!tribMun) {
        return std::unexpected(tribMun.error());
    }

    const auto totTrib = lerTotTrib(*valores, caminho);
    if (!totTrib) {
        return std::unexpected(totTrib.error());
    }

    return nb::ValoresServico::montar(*vServ, nb::Tributacao::montar(*tribMun, *totTrib));
}

} // namespace nl::mid::composicao::leitores
