// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de RegEspTrib (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1049-1062 (TSRegEspTrib).

#include <nl/back/dps/reg_esp_trib.h>

#include <optional>

namespace nl::back {

std::expected<RegEspTrib, nl::base::Erro> RegEspTrib::de(QStringView entrada)
{
    if (entrada == u"0") {
        return RegEspTrib{RegEspTrib::Valor::nenhum};
    }
    if (entrada == u"1") {
        return RegEspTrib{RegEspTrib::Valor::atoCooperado};
    }
    if (entrada == u"2") {
        return RegEspTrib{RegEspTrib::Valor::estimativa};
    }
    if (entrada == u"3") {
        return RegEspTrib{RegEspTrib::Valor::microempresaMunicipal};
    }
    if (entrada == u"4") {
        return RegEspTrib{RegEspTrib::Valor::notarioRegistrador};
    }
    if (entrada == u"5") {
        return RegEspTrib{RegEspTrib::Valor::profissionalAutonomo};
    }
    if (entrada == u"6") {
        return RegEspTrib{RegEspTrib::Valor::sociedadeProfissionais};
    }
    if (entrada == u"9") {
        return RegEspTrib{RegEspTrib::Valor::outros};
    }
    return std::unexpected(nl::base::Erro{"regime.regesptrib", std::nullopt, "RegEspTrib: valor fora do conjunto do XSD"});
}

} // namespace nl::back
