// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Grupos de pessoa do TCDPS: prest, toma, endereço, identificação e regTrib (NL-1.6b).
#include "codigos.h"
#include "grupos.h"

#include <nl/back/dps/endereco.h>
#include <nl/back/dps/endereco_exterior.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/motivo_nao_nif.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>
#include <nl/back/identificacao/inscricao.h>

#include <variant>

namespace nl::mid::sefin::prodrest_20261020::grupos {

namespace nb = nl::back;

namespace {

/// Identificação de quem tem CNPJ ou CPF (prest e toma compartilham a escolha do XSD).
void escreveInscricao(QXmlStreamWriter& w, const nb::Inscricao& inscricao)
{
    std::visit(Visitante{
                   [&w](const nb::Cnpj& cnpj) { w.writeTextElement(QStringLiteral("CNPJ"), cnpj.texto()); },
                   [&w](const nb::Cpf& cpf) { w.writeTextElement(QStringLiteral("CPF"), cpf.texto()); },
               },
               inscricao);
}

/// Identificação do tomador, que além de CNPJ e CPF aceita NIF ou cNaoNIF (TSNIF e TSCodNaoNIF).
void escreveIdentificacaoPessoa(QXmlStreamWriter& w, const nb::IdentificacaoPessoa& identificacao)
{
    std::visit(Visitante{
                   [&w](const nb::Cnpj& cnpj) { w.writeTextElement(QStringLiteral("CNPJ"), cnpj.texto()); },
                   [&w](const nb::Cpf& cpf) { w.writeTextElement(QStringLiteral("CPF"), cpf.texto()); },
                   [&w](const nb::Nif& nif) { w.writeTextElement(QStringLiteral("NIF"), nif.texto()); },
                   [&w](const nb::MotivoNaoNif& motivo) {
                       w.writeTextElement(QStringLiteral("cNaoNIF"), codigos::motivoNaoNif(motivo.valor()));
                   },
               },
               identificacao);
}

/// Endereço (TCEndereco): endNac com CEP de 8 dígitos ou endExt, depois logradouro, número, complemento e bairro.
void escreveEndereco(QXmlStreamWriter& w, const nb::Endereco& endereco)
{
    w.writeStartElement(QStringLiteral("end"));
    std::visit(Visitante{
                   [&w](const nb::EnderecoNacional& nacional) {
                       w.writeStartElement(QStringLiteral("endNac"));
                       w.writeTextElement(QStringLiteral("cMun"), nacional.cMun().texto());
                       w.writeTextElement(QStringLiteral("CEP"), nacional.cep().texto());
                       w.writeEndElement();
                   },
                   [&w](const nb::EnderecoExterior& exterior) {
                       w.writeStartElement(QStringLiteral("endExt"));
                       w.writeTextElement(QStringLiteral("cPais"), exterior.cPais().texto());
                       w.writeTextElement(QStringLiteral("cEndPost"), exterior.cEndPost().texto());
                       w.writeTextElement(QStringLiteral("xCidade"), exterior.xCidade().texto());
                       w.writeTextElement(QStringLiteral("xEstProvReg"), exterior.xEstProvReg().texto());
                       w.writeEndElement();
                   },
               },
               endereco.local());
    w.writeTextElement(QStringLiteral("xLgr"), endereco.xLgr().texto());
    w.writeTextElement(QStringLiteral("nro"), endereco.nro().texto());
    if (endereco.xCpl()) {
        w.writeTextElement(QStringLiteral("xCpl"), endereco.xCpl()->texto());
    }
    w.writeTextElement(QStringLiteral("xBairro"), endereco.xBairro().texto());
    w.writeEndElement();
}

/// regTrib (TCRegTrib): opSimpNac, regApTribSN (só com opSimpNac 3) e regEspTrib.
void escreveRegime(QXmlStreamWriter& w, const nb::RegimeTributacao& regime)
{
    w.writeStartElement(QStringLiteral("regTrib"));
    w.writeTextElement(QStringLiteral("opSimpNac"), codigos::opSimpNac(regime.opSimpNac().valor()));
    if (regime.regApTribSN()) {
        w.writeTextElement(QStringLiteral("regApTribSN"), codigos::regApTribSN(regime.regApTribSN()->valor()));
    }
    w.writeTextElement(QStringLiteral("regEspTrib"), codigos::regEspTrib(regime.regEspTrib().valor()));
    w.writeEndElement();
}

} // namespace

void escrevePrestador(QXmlStreamWriter& w, const nb::Prestador& prestador)
{
    w.writeStartElement(QStringLiteral("prest"));
    escreveInscricao(w, prestador.inscricao());
    if (prestador.im()) {
        w.writeTextElement(QStringLiteral("IM"), prestador.im()->texto());
    }
    if (prestador.xNome()) {
        w.writeTextElement(QStringLiteral("xNome"), prestador.xNome()->texto());
    }
    if (prestador.endereco()) {
        escreveEndereco(w, *prestador.endereco());
    }
    if (prestador.fone()) {
        w.writeTextElement(QStringLiteral("fone"), prestador.fone()->texto());
    }
    if (prestador.email()) {
        w.writeTextElement(QStringLiteral("email"), prestador.email()->texto());
    }
    escreveRegime(w, prestador.regime());
    w.writeEndElement();
}

void escreveTomador(QXmlStreamWriter& w, const nb::Tomador& tomador)
{
    w.writeStartElement(QStringLiteral("toma"));
    escreveIdentificacaoPessoa(w, tomador.identificacao());
    if (tomador.im()) {
        w.writeTextElement(QStringLiteral("IM"), tomador.im()->texto());
    }
    if (tomador.xNome()) {
        w.writeTextElement(QStringLiteral("xNome"), tomador.xNome()->texto());
    }
    if (tomador.endereco()) {
        escreveEndereco(w, *tomador.endereco());
    }
    if (tomador.fone()) {
        w.writeTextElement(QStringLiteral("fone"), tomador.fone()->texto());
    }
    if (tomador.email()) {
        w.writeTextElement(QStringLiteral("email"), tomador.email()->texto());
    }
    w.writeEndElement();
}

} // namespace nl::mid::sefin::prodrest_20261020::grupos
