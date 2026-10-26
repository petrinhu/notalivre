// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de TipoAmbiente (NL-1.4). Fonte: tiposSimples_v1.01.xsd:57-66 (TSTipoAmbiente).

#include <nl/back/dps/tipo_ambiente.h>

#include <optional>

namespace nl::back {

std::expected<TipoAmbiente, nl::base::Erro> TipoAmbiente::de(QStringView entrada)
{
    if (entrada == u"1") {
        return TipoAmbiente{TipoAmbiente::Valor::producao};
    }
    if (entrada == u"2") {
        return TipoAmbiente{TipoAmbiente::Valor::homologacao};
    }
    return std::unexpected(nl::base::Erro{"ambiente.valor", std::nullopt, "TipoAmbiente: valor fora do conjunto do XSD"});
}

} // namespace nl::back
