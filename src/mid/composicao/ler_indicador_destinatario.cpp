// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do indDest da raiz da entrada (NL-3.L). No leiaute de Produção Restrita de 24/11/2026 o indDest saiu do grupo IBSCBS e
// foi para a raiz de infDPS, obrigatório em toda DPS. Na entrada da CLI fica na raiz e é opcional, com padrão "0" (o destinatário
// é o próprio tomador). Decisão do líder, 27/11/2026, por AskUserQuestion: "Raiz, padrão 0 (Recomendado)".
#include "campos_json.h"
#include "leitores_dps.h"

#include <nl/back/dps/indicador_destinatario.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

namespace {

/// indDest quando a raiz da entrada não traz a chave: 0, o destinatário é o próprio tomador (TSRTCIndDest).
constexpr char16_t kIndDestPadrao[] = u"0";

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
    return nb::IndicadorDestinatario::de(**texto);
}

} // namespace nl::mid::composicao::leitores
