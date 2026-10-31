// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada de validação da DPS (NL-1.7): compõe leitura do JSON, montagem pelo Back, codec do leiaute e validação no XSD.
#include <nl/mid/composicao/dps_validar.h>

#include "campos_json.h"
#include "leitores_dps.h"

#include <nl/back/dps/dps.h>
#include <nl/foundation/xml/erros_xsd.h>
#include <nl/foundation/xml/esquema.h>
#include <nl/foundation/xml/validador_xsd.h>
#include <nl/mid/sefin/leiaute/prodrest_20261020/codec_dps.h>

#include <utility>

namespace nl::mid::composicao {

namespace nb = nl::back;
namespace xsd = nl::foundation::xml;
namespace prod = nl::mid::sefin::prodrest_20261020;

namespace {

/// Raiz do esquema da DPS no leiaute de Produção Restrita de 20/10/2026. É fixa do leiaute, por isso mora aqui.
const QString kRaizEsquema = QStringLiteral("DPS_v1.01.xsd");

std::vector<nl::base::Erro> umaFalha(nl::base::Erro erro)
{
    std::vector<nl::base::Erro> falhas;
    falhas.push_back(std::move(erro));
    return falhas;
}

FalhaDpsValidar reprovada(std::vector<nl::base::Erro> erros)
{
    return FalhaDpsValidar{NaturezaFalha::reprovada, std::move(erros)};
}

/// Falha do XSD é do documento só quando a libxml2 aponta o XML; falha do esquema ou da própria libxml2 é do ambiente.
FalhaDpsValidar falhaDoXsd(const xsd::ErrosXsd& erros)
{
    const bool culpaDoXml = erros.motivo() == xsd::ErrosXsd::Motivo::invalido || erros.motivo() == xsd::ErrosXsd::Motivo::xmlMalformado;
    return FalhaDpsValidar{culpaDoXml ? NaturezaFalha::reprovada : NaturezaFalha::ambiente, erros.erros()};
}

} // namespace

std::expected<QByteArray, FalhaDpsValidar> DpsValidar::executar(QByteArrayView entradaJson, const LocalizacaoEsquemaDps& localizacao)
{
    const auto raiz = campos::lerRaiz(entradaJson);
    if (!raiz) {
        return std::unexpected(reprovada(umaFalha(raiz.error())));
    }

    auto dados = leitores::lerDadosDps(*raiz);
    if (!dados) {
        return std::unexpected(reprovada(umaFalha(dados.error())));
    }

    const auto dps = nb::Dps::montar(std::move(*dados));
    if (!dps) {
        return std::unexpected(reprovada(umaFalha(dps.error())));
    }

    const QByteArray xml = prod::CodecDps::serializar(*dps);

    const auto esquema = xsd::Esquema::abrir(localizacao.diretorio, localizacao.manifesto, kRaizEsquema);
    if (!esquema) {
        return std::unexpected(falhaDoXsd(esquema.error()));
    }

    const auto validacao = xsd::ValidadorXsd::validar(xml, *esquema);
    if (!validacao) {
        return std::unexpected(falhaDoXsd(validacao.error()));
    }
    return xml;
}

std::expected<void, FalhaDpsValidar> DpsValidar::validarXml(QByteArrayView xml, const LocalizacaoEsquemaDps& localizacao)
{
    static_cast<void>(xml);
    static_cast<void>(localizacao);
    return {};
}

} // namespace nl::mid::composicao
