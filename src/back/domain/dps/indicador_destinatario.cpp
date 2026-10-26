// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de IndicadorDestinatario (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1758-1768 (TSRTCIndDest).

#include <nl/back/dps/indicador_destinatario.h>

#include <optional>

namespace nl::back {

std::expected<IndicadorDestinatario, nl::base::Erro> IndicadorDestinatario::de(QStringView entrada)
{
    if (entrada == u"0") {
        return IndicadorDestinatario{IndicadorDestinatario::Valor::tomadorEDestinatario};
    }
    if (entrada == u"1") {
        return IndicadorDestinatario{IndicadorDestinatario::Valor::outroDestinatario};
    }
    return std::unexpected(nl::base::Erro{"indicadordestinatario.valor", std::nullopt, "IndicadorDestinatario: valor fora do conjunto do XSD"});
}

} // namespace nl::back
