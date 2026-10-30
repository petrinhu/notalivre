// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de ambiente da emissão (NL-2.9, DA-2.5). Ver politica_emissao.h. WIP: sem verificação.
#include <nl/mid/composicao/politica_emissao.h>

#include <optional>

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

std::expected<void, nl::base::Erro> liberarAmbiente(const ConfigEmissao& config)
{
    if (config.ambiente == Ambiente::Producao && !config.producaoLiberada) {
        return std::unexpected(nl::base::Erro("emissao.producao_bloqueada", std::nullopt,
                                              "Produção está bloqueada por configuração até o go/no-go da Onda 12"));
    }
    return {};
}

} // namespace nl::mid::composicao
