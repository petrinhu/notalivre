// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparador real do infDPS (NL-2.8). Ver comparador_infdps_c14n.h. Usa só a API pública do motor xmldsig: o digest
// canônico da referência, nas duas pontas, com os parâmetros da pasta do leiaute. Não há C14N aqui.
#include <nl/mid/sefin/cliente/comparador_infdps_c14n.h>

#include <nl/foundation/xmldsig/assinador_xml.h>
#include <nl/mid/sefin/leiaute/prodrest_20261020/parametros_infdps.h>

namespace nl::mid::sefin::cliente {

ResultadoComparacaoInfDps ComparadorInfDpsC14n::comparar(const nl::back::IdDps& id, const QByteArray& dpsXml,
                                                         const QByteArray& nfseXml) const {
    namespace xd = nl::foundation::xmldsig;
    const QString idTexto = id.texto();
    const auto parametros = nl::mid::sefin::prodrest_20261020::parametrosDoDigestDeComparacao();
    const auto digestDps = xd::digestCanonicoDaReferencia(dpsXml, idTexto, parametros);
    const auto digestNfse = xd::digestCanonicoDaReferencia(nfseXml, idTexto, parametros);
    // Sem o infDPS numa das pontas, na forma que o motor aceita, não há comparação: Indisponivel (estado seguro).
    if (!digestDps || !digestNfse) {
        return ResultadoComparacaoInfDps::Indisponivel;
    }
    return *digestDps == *digestNfse ? ResultadoComparacaoInfDps::Igual : ResultadoComparacaoInfDps::Diferente;
}

} // namespace nl::mid::sefin::cliente
