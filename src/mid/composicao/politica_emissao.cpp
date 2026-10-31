// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de ambiente da emissão (NL-2.9, DA-2.5). Ver politica_emissao.h. WIP: sem verificação.
#include <nl/mid/composicao/politica_emissao.h>

#include <optional>

#include <QUrl>

namespace nl::mid::composicao {

std::expected<Ambiente, nl::base::Erro> ambienteDeTexto(std::string_view texto)
{
    if (texto == "homologacao") {
        return Ambiente::Homologacao;
    }
    if (texto == "producao-restrita") {
        return Ambiente::ProducaoRestrita;
    }
    if (texto == "producao") {
        return Ambiente::Producao;
    }
    return std::unexpected(nl::base::Erro("emissao.ambiente_invalido", std::nullopt, "ambiente desconhecido"));
}

// Ambiente que o endpoint diz ser (NL-2.9, item 1): o host decide. Ver politica_emissao.h.
std::expected<Ambiente, nl::base::Erro> ambienteDoEndpoint(QStringView url)
{
    const QString host = QUrl(url.toString()).host().toLower();
    if (host == QStringLiteral("sefin.nfse.gov.br")) {
        return Ambiente::Producao;
    }
    if (host == QStringLiteral("sefin.producaorestrita.nfse.gov.br")) {
        return Ambiente::ProducaoRestrita;
    }
    if (host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost")) {
        return Ambiente::Homologacao;
    }
    return std::unexpected(nl::base::Erro("emissao.endpoint_desconhecido", std::nullopt,
                                          "o endpoint não é de Produção, de Produção Restrita nem do servidor local de teste"));
}

std::expected<void, nl::base::Erro> conferirFlagDoAmbiente(Ambiente flag, Ambiente derivado)
{
    if (flag != derivado) {
        return std::unexpected(nl::base::Erro("emissao.ambiente_divergente", std::nullopt,
                                              "a flag --ambiente diverge do endpoint configurado; o endpoint decide"));
    }
    return {};
}

std::expected<void, nl::base::Erro> liberarAmbiente(const ConfigEmissao& config)
{
    if (config.ambiente == Ambiente::Producao && !config.producaoLiberada) {
        return std::unexpected(nl::base::Erro("emissao.producao_bloqueada", std::nullopt,
                                              "Produção está bloqueada por configuração até o go/no-go da Onda 12"));
    }
    return {};
}

} // namespace nl::mid::composicao
