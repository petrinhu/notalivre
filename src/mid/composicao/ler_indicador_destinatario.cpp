// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do indDest da raiz da entrada (NL-3.L). No leiaute de Produção Restrita de 24/11/2026 o indDest saiu do grupo IBSCBS e
// foi para a raiz de infDPS, obrigatório em toda DPS. Na entrada da CLI fica na raiz e é opcional, com padrão "0" (o destinatário
// é o próprio tomador). Decisão do líder, 27/11/2026, por AskUserQuestion: "Raiz, padrão 0 (Recomendado)".
#include "campos_json.h"
#include "leitores_dps.h"

#include <nl/back/dps/indicador_destinatario.h>

#include <string>
#include <string_view>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

namespace {

/// indDest quando a raiz da entrada não traz a chave: 0, o destinatário é o próprio tomador (TSRTCIndDest).
constexpr char16_t kIndDestPadrao[] = u"0";

/// Nome da chave na raiz da entrada: o campo que o erro aponta para o usuário corrigir.
constexpr std::string_view kCampoIndDest = "indDest";

} // namespace

std::expected<nb::IndicadorDestinatario, nl::base::Erro> lerIndicadorDestinatario(const QJsonObject& raiz)
{
    const auto texto = campos::textoOpcional(raiz, "", "indDest");
    if (!texto) {
        return std::unexpected(texto.error());
    }
    if (!texto->has_value()) {
        return nb::IndicadorDestinatario::de(kIndDestPadrao);
    }
    const auto indDest = nb::IndicadorDestinatario::de(**texto);
    if (!indDest) {
        // O Back não sabe em que chave o texto veio: o erro leva o campo e diz onde corrigir (NF-18). Código do domínio mantido.
        return std::unexpected(nl::base::Erro{indDest.error().codigo(), std::string{kCampoIndDest},
                                              "campo indDest na raiz da entrada: valor fora do conjunto do XSD (aceito 0 ou 1)"});
    }
    return *indDest;
}

} // namespace nl::mid::composicao::leitores
