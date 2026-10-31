// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de ambiente da emissão (NL-2.9, DA-2.5, spec 4.12). Função pura. Produção nasce bloqueada por configuração até o
// go/no-go do líder (Onda 12): sem liberação explícita, a emissão para Produção é recusada antes de qualquer envio.
#ifndef NL_MID_COMPOSICAO_POLITICA_EMISSAO_H
#define NL_MID_COMPOSICAO_POLITICA_EMISSAO_H

#include <nl/base/erro.h>

#include <QStringView>

#include <expected>
#include <string_view>

namespace nl::mid::composicao {

enum class Ambiente { Homologacao, ProducaoRestrita, Producao };

/// Configuração de ambiente. producaoLiberada vem do arquivo de configuração e nasce falso.
struct ConfigEmissao
{
    Ambiente ambiente;
    bool producaoLiberada;
};

/// Texto do ambiente na linha de comando: "homologacao", "producao-restrita" ou "producao". Outro texto é vazio.
std::expected<Ambiente, nl::base::Erro> ambienteDeTexto(std::string_view texto);

/// Ambiente que o endpoint configurado diz ser (NL-2.9, item 1 da triagem do CISO): o host decide, nunca a flag. Host de
/// Produção é Producao; host de Produção Restrita é ProducaoRestrita; loopback é Homologacao (só o servidor falso). Outro host
/// é "emissao.endpoint_desconhecido".
std::expected<Ambiente, nl::base::Erro> ambienteDoEndpoint(QStringView url);

/// Flag --ambiente que diverge do ambiente derivado do endpoint é recusada ("emissao.ambiente_divergente") antes de assinar.
std::expected<void, nl::base::Erro> conferirFlagDoAmbiente(Ambiente flag, Ambiente derivado);

/// Recusa Producao sem liberação ("emissao.producao_bloqueada"). Homologação e Produção Restrita passam.
std::expected<void, nl::base::Erro> liberarAmbiente(const ConfigEmissao& config);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_POLITICA_EMISSAO_H
