// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Enumeração do Back para o código do XML, só nesta pasta do leiaute (NL-1.6b). Cada função cobre todos os valores do enum.
#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_CODIGOS_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_CODIGOS_H

#include <nl/back/dps/codigo_justificativa_substituicao.h>
#include <nl/back/dps/indicador_destinatario.h>
#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/motivo_nao_nif.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/reg_ap_trib_sn.h>
#include <nl/back/dps/reg_esp_trib.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/tipo_ambiente.h>
#include <nl/back/dps/tipo_operacao.h>
#include <nl/back/dps/trib_issqn.h>

#include <QString>

namespace nl::mid::sefin::prodrest_20261020::codigos {

QString ambiente(nl::back::TipoAmbiente::Valor valor);
QString opSimpNac(nl::back::OpSimpNac::Valor valor);
QString regApTribSN(nl::back::RegApTribSN::Valor valor);
QString regEspTrib(nl::back::RegEspTrib::Valor valor);
QString tribIssqn(nl::back::TribIssqn::Valor valor);
QString retencaoIssqn(nl::back::RetencaoIssqn::Valor valor);
QString indTotTrib(nl::back::IndicadorTotalTributos::Valor valor);
QString motivoNaoNif(nl::back::MotivoNaoNif::Valor valor);
QString tipoOperacao(nl::back::TipoOperacao::Valor valor);
QString indDestinatario(nl::back::IndicadorDestinatario::Valor valor);
QString justificativaSubstituicao(nl::back::CodigoJustificativaSubstituicao::Valor valor);

} // namespace nl::mid::sefin::prodrest_20261020::codigos

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_CODIGOS_H
