// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparador do infDPS (NL-2.8, adendo da DA-2.41, 29/10/2026): compara o infDPS canonicalizado (C14N 1.0) da DPS
// enviada com o infDPS da NFS-e devolvida pela Sefin. Esta é a interface do Mid. A implementação real usa a C14N da
// NL-2.3 (Foundation) e entra depois, pelo Mid. Enquanto ela não está ligada, o padrão é ComparadorIndisponivel, que
// faz a reconciliação terminar em Indeterminado: o estado seguro.
#ifndef NL_MID_SEFIN_CLIENTE_COMPARADOR_INFDPS_H
#define NL_MID_SEFIN_CLIENTE_COMPARADOR_INFDPS_H

#include <QByteArray>

namespace nl::mid::sefin::cliente {

enum class ResultadoComparacaoInfDps {
    Igual,        ///< Mesmo infDPS canonicalizado: a nota recuperada é a da DPS enviada.
    Diferente,    ///< Outro infDPS com a mesma série e o mesmo número: colisão de numeração, vai ao usuário (DpsDuplicada).
    Indisponivel, ///< Não foi possível comparar: o estado é Indeterminado, nunca sucesso silencioso.
};

class ComparadorInfDps {
public:
    virtual ~ComparadorInfDps() = default;

    /// Compara o infDPS da DPS enviada (dpsXml) com o infDPS da NFS-e devolvida (nfseXml).
    [[nodiscard]] virtual ResultadoComparacaoInfDps comparar(const QByteArray& dpsXml,
                                                             const QByteArray& nfseXml) const = 0;
};

/// Sem comparador real: nunca diz igual nem diferente. Não tem estado, então uma única instância basta.
class ComparadorIndisponivel final : public ComparadorInfDps {
public:
    [[nodiscard]] ResultadoComparacaoInfDps comparar(const QByteArray&, const QByteArray&) const override
    {
        return ResultadoComparacaoInfDps::Indisponivel;
    }
};

/// Comparador padrão do cliente: a instância é `static const` e sem estado, e vale para toda chamada.
inline const ComparadorInfDps& comparadorIndisponivel()
{
    static const ComparadorIndisponivel instancia;
    return instancia;
}

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_COMPARADOR_INFDPS_H
