// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código do XML de cada enum do Back (NL-1.6b). Os números são os do XSD de 20/10/2026 (tiposSimples_v1.01.xsd).
#include "codigos.h"

#include <utility>

namespace nl::mid::sefin::prodrest_20261020::codigos {

namespace nb = nl::back;

QString ambiente(nb::TipoAmbiente::Valor valor)
{
    switch (valor) {
    case nb::TipoAmbiente::Valor::producao:
        return QStringLiteral("1");
    case nb::TipoAmbiente::Valor::homologacao:
        return QStringLiteral("2");
    }
    std::unreachable();
}

QString opSimpNac(nb::OpSimpNac::Valor valor)
{
    switch (valor) {
    case nb::OpSimpNac::Valor::naoOptante:
        return QStringLiteral("1");
    case nb::OpSimpNac::Valor::mei:
        return QStringLiteral("2");
    case nb::OpSimpNac::Valor::meEpp:
        return QStringLiteral("3");
    }
    std::unreachable();
}

QString regApTribSN(nb::RegApTribSN::Valor valor)
{
    switch (valor) {
    case nb::RegApTribSN::Valor::federalEMunicipalPeloSN:
        return QStringLiteral("1");
    case nb::RegApTribSN::Valor::federalPeloSnIssForaSN:
        return QStringLiteral("2");
    case nb::RegApTribSN::Valor::federalEMunicipalForaSN:
        return QStringLiteral("3");
    }
    std::unreachable();
}

QString regEspTrib(nb::RegEspTrib::Valor valor)
{
    switch (valor) {
    case nb::RegEspTrib::Valor::nenhum:
        return QStringLiteral("0");
    case nb::RegEspTrib::Valor::atoCooperado:
        return QStringLiteral("1");
    case nb::RegEspTrib::Valor::estimativa:
        return QStringLiteral("2");
    case nb::RegEspTrib::Valor::microempresaMunicipal:
        return QStringLiteral("3");
    case nb::RegEspTrib::Valor::notarioRegistrador:
        return QStringLiteral("4");
    case nb::RegEspTrib::Valor::profissionalAutonomo:
        return QStringLiteral("5");
    case nb::RegEspTrib::Valor::sociedadeProfissionais:
        return QStringLiteral("6");
    case nb::RegEspTrib::Valor::outros:
        return QStringLiteral("9");
    }
    std::unreachable();
}

QString tribIssqn(nb::TribIssqn::Valor valor)
{
    switch (valor) {
    case nb::TribIssqn::Valor::operacaoTributavel:
        return QStringLiteral("1");
    case nb::TribIssqn::Valor::imunidade:
        return QStringLiteral("2");
    case nb::TribIssqn::Valor::exportacaoServico:
        return QStringLiteral("3");
    case nb::TribIssqn::Valor::naoIncidencia:
        return QStringLiteral("4");
    }
    std::unreachable();
}

QString retencaoIssqn(nb::RetencaoIssqn::Valor valor)
{
    switch (valor) {
    case nb::RetencaoIssqn::Valor::naoRetido:
        return QStringLiteral("1");
    case nb::RetencaoIssqn::Valor::retidoPeloTomador:
        return QStringLiteral("2");
    case nb::RetencaoIssqn::Valor::retidoPeloIntermediario:
        return QStringLiteral("3");
    }
    std::unreachable();
}

QString indTotTrib(nb::IndicadorTotalTributos::Valor valor)
{
    switch (valor) {
    case nb::IndicadorTotalTributos::Valor::naoInformar:
        return QStringLiteral("0");
    }
    std::unreachable();
}

QString motivoNaoNif(nb::MotivoNaoNif::Valor valor)
{
    switch (valor) {
    case nb::MotivoNaoNif::Valor::naoInformadoNaOrigem:
        return QStringLiteral("0");
    case nb::MotivoNaoNif::Valor::dispensadoDoNif:
        return QStringLiteral("1");
    case nb::MotivoNaoNif::Valor::naoExigenciaDoNif:
        return QStringLiteral("2");
    }
    std::unreachable();
}

QString tipoOperacao(nb::TipoOperacao::Valor valor)
{
    switch (valor) {
    case nb::TipoOperacao::Valor::fornecimentoComPagamentoPosterior:
        return QStringLiteral("1");
    case nb::TipoOperacao::Valor::recebimentoComFornecimentoRealizado:
        return QStringLiteral("2");
    case nb::TipoOperacao::Valor::fornecimentoComPagamentoRealizado:
        return QStringLiteral("3");
    case nb::TipoOperacao::Valor::recebimentoComFornecimentoPosterior:
        return QStringLiteral("4");
    case nb::TipoOperacao::Valor::fornecimentoERecebimentoConcomitantes:
        return QStringLiteral("5");
    }
    std::unreachable();
}

QString indDestinatario(nb::IndicadorDestinatario::Valor valor)
{
    switch (valor) {
    case nb::IndicadorDestinatario::Valor::tomadorEDestinatario:
        return QStringLiteral("0");
    case nb::IndicadorDestinatario::Valor::outroDestinatario:
        return QStringLiteral("1");
    }
    std::unreachable();
}

QString justificativaSubstituicao(nb::CodigoJustificativaSubstituicao::Valor valor)
{
    switch (valor) {
    case nb::CodigoJustificativaSubstituicao::Valor::desenquadramentoSimplesNacional:
        return QStringLiteral("01");
    case nb::CodigoJustificativaSubstituicao::Valor::enquadramentoSimplesNacional:
        return QStringLiteral("02");
    case nb::CodigoJustificativaSubstituicao::Valor::inclusaoRetroativaImunidade:
        return QStringLiteral("03");
    case nb::CodigoJustificativaSubstituicao::Valor::exclusaoRetroativaImunidade:
        return QStringLiteral("04");
    case nb::CodigoJustificativaSubstituicao::Valor::rejeicaoPeloTomador:
        return QStringLiteral("05");
    case nb::CodigoJustificativaSubstituicao::Valor::outros:
        return QStringLiteral("99");
    }
    std::unreachable();
}

} // namespace nl::mid::sefin::prodrest_20261020::codigos
