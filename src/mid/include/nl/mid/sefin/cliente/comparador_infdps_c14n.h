// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparador real do infDPS (NL-2.8, adendo da DA-2.41). Usa só a API pública do motor xmldsig (NL-2.3): o digest
// canônico da referência. Não tem C14N própria: a canonicalização é a do motor, e os parâmetros do leiaute vêm da pasta
// prodrest_20261020 (L-33: o Mid depende da Foundation, nunca o contrário).
#ifndef NL_MID_SEFIN_CLIENTE_COMPARADOR_INFDPS_C14N_H
#define NL_MID_SEFIN_CLIENTE_COMPARADOR_INFDPS_C14N_H

#include <nl/mid/sefin/cliente/comparador_infdps.h>

namespace nl::mid::sefin::cliente {

class ComparadorInfDpsC14n final : public ComparadorInfDps {
public:
    /// Igual quando o digest canônico do infDPS (Id = id) é o mesmo nas duas pontas; Diferente quando não é; Indisponivel
    /// quando alguma ponta não tem o infDPS na forma que o motor aceita.
    [[nodiscard]] ResultadoComparacaoInfDps comparar(const nl::back::IdDps& id, const QByteArray& dpsXml,
                                                     const QByteArray& nfseXml) const override;
};

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_COMPARADOR_INFDPS_C14N_H
