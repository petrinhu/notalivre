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

#include <QCryptographicHash>
#include <QFile>
#include <QIODevice>

#include <optional>
#include <string_view>
#include <utility>

#ifndef NL_XSD_MANIFESTO_SHA256
#error "NL_XSD_MANIFESTO_SHA256 vem do CMake (third_party/nfse-xsd): sem ele o manifesto do XSD não é fixado"
#endif

namespace nl::mid::composicao {

namespace nb = nl::back;
namespace xsd = nl::foundation::xml;
namespace prod = nl::mid::sefin::prodrest_20261020;

namespace {

/// Raiz do esquema da DPS no leiaute de Produção Restrita de 20/10/2026. É fixa do leiaute, por isso mora aqui.
const QString kRaizEsquema = QStringLiteral("DPS_v1.01.xsd");

/// SHA-256 do SHA256SUMS do XSD, embutido pelo CMake (NL-2.10b, BAIXA-4). Nenhuma configuração troca este valor.
constexpr std::string_view kShaManifestoXsdFixado = NL_XSD_MANIFESTO_SHA256;

/// Pin do manifesto: o SHA-256 do arquivo tem de ser o embutido. Divergente é erro de configuração, código 10, recusado antes de
/// abrir o esquema. Manifesto ausente não é aqui: a abertura do esquema devolve xsd.esquema, como antes.
std::optional<FalhaDpsValidar> manifestoForaDoPin(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const QByteArray obtido = QCryptographicHash::hash(arquivo.readAll(), QCryptographicHash::Sha256).toHex();
    if (std::string_view(obtido.constData(), static_cast<std::size_t>(obtido.size())) == kShaManifestoXsdFixado) {
        return std::nullopt;
    }
    return FalhaDpsValidar{NaturezaFalha::ambiente,
                           {nl::base::Erro{"configuracao.xsd_manifesto_nao_fixado", std::nullopt,
                                           "o SHA256SUMS do XSD não é o fixado no build; o manifesto não foi usado"}}};
}

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
    if (auto revalidado = DpsValidar::validarXml(xml, localizacao); !revalidado) {
        return std::unexpected(revalidado.error());
    }
    return xml;
}

std::expected<void, FalhaDpsValidar> DpsValidar::validarXml(QByteArrayView xml, const LocalizacaoEsquemaDps& localizacao)
{
    // Pin (NL-2.10b, BAIXA-4): o manifesto tem o hash embutido no build, antes de qualquer arquivo do esquema ser aberto.
    if (auto fora = manifestoForaDoPin(localizacao.manifesto)) {
        return std::unexpected(*fora);
    }
    const auto esquema = xsd::Esquema::abrir(localizacao.diretorio, localizacao.manifesto, kRaizEsquema);
    if (!esquema) {
        return std::unexpected(falhaDoXsd(esquema.error()));
    }
    const auto validacao = xsd::ValidadorXsd::validar(xml, *esquema);
    if (!validacao) {
        return std::unexpected(falhaDoXsd(validacao.error()));
    }
    return {};
}

} // namespace nl::mid::composicao
