// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Codec XML da DPS, leiaute de Produção Restrita de 20/10/2026 (NL-1.6b). Sem estado; só esta pasta conhece tag e namespace.
#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_CODEC_DPS_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_CODEC_DPS_H

#include <QByteArray>

namespace nl::back {
class Dps;
}

namespace nl::mid::sefin::prodrest_20261020 {

/// Serializa uma Dps já válida (invariantes no Back) no TCDPS do XSD, com namespace http://www.sped.fazenda.gov.br/nfse.
/// Ordem dos elementos igual à xs:sequence do XSD. Não valida: quem valida é ValidadorXsd (Foundation).
class CodecDps
{
public:
    CodecDps() = delete;

    /// Devolve o XML em UTF-8, com declaração XML. Não tem caminho de erro e não valida: o dhEmi já vem validado pelo tipo
    /// DhEmi (ano de 2000 a 2099, offset de hora cheia de -11:00 a +12:00, DA-1.27); o resto do XML é conferido pelo ValidadorXsd.
    static QByteArray serializar(const nl::back::Dps& dps);
};

} // namespace nl::mid::sefin::prodrest_20261020

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_CODEC_DPS_H
