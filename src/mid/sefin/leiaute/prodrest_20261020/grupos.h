// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Escritores dos grupos do TCDPS, um por assunto (NL-1.6b). Cada um escreve na ordem da xs:sequence do XSD.
#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_GRUPOS_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_GRUPOS_H

#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/prestador.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/valores_servico.h>

#include <QXmlStreamWriter>

namespace nl::mid::sefin::prodrest_20261020::grupos {

/// Visitante de std::visit com lambdas: uma linha por alternativa, sem cadeia de get_if.
template <typename... Ts>
struct Visitante : Ts...
{
    using Ts::operator()...;
};
template <typename... Ts>
Visitante(Ts...) -> Visitante<Ts...>;

/// prest (TCInfoPrestador): identificação, IM, xNome, endereço, contato e regTrib.
void escrevePrestador(QXmlStreamWriter& w, const nl::back::Prestador& prestador);

/// toma (TCInfoPessoa): identificação (CNPJ, CPF, NIF ou cNaoNIF), IM, xNome, endereço, contato.
void escreveTomador(QXmlStreamWriter& w, const nl::back::Tomador& tomador);

/// serv (TCServ): locPrest e cServ (cTribNac, cNBS, xDescServ).
void escreveServico(QXmlStreamWriter& w, const nl::back::Servico& servico);

/// valores (TCInfoValores): vServPrest e trib (tribMun e totTrib).
void escreveValores(QXmlStreamWriter& w, const nl::back::ValoresServico& valores);

/// IBSCBS (TCRTCInfoIBSCBS): cIndOp, tpOper, gRefNFSe, indDest e valores/trib (CST e cClassTrib).
void escreveIbscbs(QXmlStreamWriter& w, const nl::back::Ibscbs& ibscbs);

} // namespace nl::mid::sefin::prodrest_20261020::grupos

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_GRUPOS_H
