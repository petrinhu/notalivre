// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regime de apuração tributária pelo Simples Nacional (regApTribSN), opcional; a regra cruzada com o MEI é NL-1.5 (E0162). (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1019-1032 (TSRegimeApuracaoSimpNac); campo regApTribSN em tiposComplexos_v1.01.xsd:978. Regra pura: sem I/O.

#ifndef NL_BACK_DPS_REG_AP_TRIB_SN_H
#define NL_BACK_DPS_REG_AP_TRIB_SN_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Regime de apuração tributária pelo Simples Nacional (regApTribSN), opcional; a regra cruzada com o MEI é NL-1.5 (E0162).
/// Só se constrói por RegApTribSN::de, que aceita somente os valores do XSD. O construtor é privado.
class RegApTribSN
{
public:
    enum class Valor
    {
        federalEMunicipalPeloSN, // XML "1": 1 - Federal e municipal pelo SN
        federalPeloSnIssForaSN, // XML "2": 2 - Federal pelo SN e ISSQN fora do SN
        federalEMunicipalForaSN, // XML "3": 3 - Federal e municipal fora do SN
    };

    /// Erro: "regime.regapttribsn" (valor fora do conjunto do XSD).
    static std::expected<RegApTribSN, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const RegApTribSN& outro) const noexcept = default;

private:
    explicit RegApTribSN(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_REG_AP_TRIB_SN_H
