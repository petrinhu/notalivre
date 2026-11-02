// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoTribMunicipal (NL-2.10h, H1). Padrão: tiposSimples_v1.01.xsd:568; 000 pela regra E0315 (Anexo I, linha Excel 342).

#include <nl/back/dps/codigo_trib_municipal.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<CodigoTribMunicipal, nl::base::Erro> CodigoTribMunicipal::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{3})PAD")) {
        return std::unexpected(nl::base::Erro{"cod_trib_municipal.formato", std::nullopt, "CodigoTribMunicipal: texto fora do padrão do XSD (3 dígitos)"});
    }
    if (entrada == QStringView{u"000"}) {
        return std::unexpected(nl::base::Erro{"cod_trib_municipal.zero", std::nullopt,
                                              "CodigoTribMunicipal: 000 não é permitido (regra E0315, linha Excel 342 do Anexo I)"});
    }
    return CodigoTribMunicipal{entrada.toString()};
}

QString CodigoTribMunicipal::texto() const
{
    return texto_;
}

CodigoTribMunicipal::CodigoTribMunicipal(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
