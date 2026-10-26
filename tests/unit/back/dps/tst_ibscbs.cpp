// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do grupo IBS/CBS (NL-1.4, DA-1.5): gRefNFSe de 1 a 99 chaves e a regra tpOper 2 ou 3 (E0905 e E0906 da aba RN de 20/10).

#include <nl/back/dps/codigo_classificacao_tributaria.h>
#include <nl/back/dps/codigo_situacao_tributaria.h>
#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/indicador_destinatario.h>
#include <nl/back/dps/ref_nfse.h>
#include <nl/back/dps/tipo_operacao.h>
#include <nl/back/dps/tributacao_ibscbs.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::ChaveNfse;
using nl::back::CodigoClassificacaoTributaria;
using nl::back::CodigoSituacaoTributaria;
using nl::back::IndicadorDestinatario;
using nl::back::Ibscbs;
using nl::back::RefNfse;
using nl::back::TipoOperacao;
using nl::back::TributacaoIbscbs;

namespace {

std::optional<ChaveNfse> chaveValida()
{
    return teste::valorSeOk(ChaveNfse::de(u"123456" u"12ABC34501DE35" u"123456789012345678901234567890"));
}

std::optional<TributacaoIbscbs> tributacaoValida()
{
    const auto cst = teste::valorSeOk(CodigoSituacaoTributaria::de(u"000"));
    const auto cClassTrib = teste::valorSeOk(CodigoClassificacaoTributaria::de(u"000001"));
    if (!cst || !cClassTrib) {
        return std::nullopt;
    }
    return TributacaoIbscbs::montar(*cst, *cClassTrib);
}

std::optional<RefNfse> umaReferencia()
{
    const auto chave = chaveValida();
    if (!chave) {
        return std::nullopt;
    }
    return teste::valorSeOk(RefNfse::de(std::vector<ChaveNfse>{*chave}));
}

} // namespace

class TstIbscbs : public QObject
{
    Q_OBJECT

private slots:
    void gRefNFSeAte99Chaves();
    void gRefNFSeSemChavesRejeitado();
    void gRefNFSeComCemChavesRejeitado();
    void tpOper2ExigeGRefNFSe();
    void tpOper3ComGRefNFSeAceito();
    void tpOper1ComGRefNFSeRejeitado();
    void tpOper4ComGRefNFSeRejeitado();
    void tpOper5ComGRefNFSeRejeitado();
    void ibscbsValoresObrigatorioQuandoGrupoExiste();
    void semTpOperComGRefNFSeRejeitado();
    void semTpOperSemGRefNFSeAceito();
    void tipoOperacaoDeUmACinco();
    void indicadorDestinatarioZeroOuUm();
};

void TstIbscbs::gRefNFSeAte99Chaves()
{
    const auto chave = chaveValida();
    QVERIFY(chave.has_value());
    QVERIFY(RefNfse::de(std::vector<ChaveNfse>(1, *chave)).has_value());
    const auto noLimite = RefNfse::de(std::vector<ChaveNfse>(99, *chave));
    QVERIFY(noLimite.has_value());
    QCOMPARE(noLimite->chaves().size(), std::size_t{99});
}

void TstIbscbs::gRefNFSeSemChavesRejeitado()
{
    QCOMPARE(teste::codigoDoErro(RefNfse::de({})), QStringLiteral("ref_nfse.quantidade"));
}

void TstIbscbs::gRefNFSeComCemChavesRejeitado()
{
    // refNFSe maxOccurs=99 (tiposComplexos_v1.01.xsd:2399). Cem é um a mais que o limite.
    const auto chave = chaveValida();
    QVERIFY(chave.has_value());
    QCOMPARE(teste::codigoDoErro(RefNfse::de(std::vector<ChaveNfse>(100, *chave))), QStringLiteral("ref_nfse.quantidade"));
}

void TstIbscbs::tpOper2ExigeGRefNFSe()
{
    // E0905 (aba RN de 20/10, linha Excel 576): gRefNFSe obrigatório com tpOper 2 ou 3.
    const auto tributacao = tributacaoValida();
    const auto tpOper = teste::valorSeOk(TipoOperacao::de(u"2"));
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(tpOper.has_value());
    QVERIFY(indDest.has_value());
    QCOMPARE(teste::codigoDoErro(Ibscbs::montar(std::nullopt, *tpOper, std::nullopt, *indDest, *tributacao)),
             QStringLiteral("E0905"));
}

void TstIbscbs::tpOper3ComGRefNFSeAceito()
{
    const auto tributacao = tributacaoValida();
    const auto tpOper = teste::valorSeOk(TipoOperacao::de(u"3"));
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(tpOper.has_value());
    QVERIFY(indDest.has_value());
    QVERIFY(umaReferencia().has_value());
    QVERIFY(Ibscbs::montar(std::nullopt, *tpOper, umaReferencia(), *indDest, *tributacao).has_value());
}

void TstIbscbs::tpOper1ComGRefNFSeRejeitado()
{
    // E0906 (aba RN de 20/10, linha Excel 577): gRefNFSe não pode existir com tpOper 1, 4 ou 5.
    const auto tributacao = tributacaoValida();
    const auto tpOper = teste::valorSeOk(TipoOperacao::de(u"1"));
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(tpOper.has_value());
    QVERIFY(indDest.has_value());
    QVERIFY(umaReferencia().has_value());
    QCOMPARE(teste::codigoDoErro(Ibscbs::montar(std::nullopt, *tpOper, umaReferencia(), *indDest, *tributacao)),
             QStringLiteral("E0906"));
}

void TstIbscbs::tpOper4ComGRefNFSeRejeitado()
{
    // E0906 (aba RN de 20/10, linha Excel 577): gRefNFSe não pode existir com tpOper 1, 4 ou 5.
    const auto tributacao = tributacaoValida();
    const auto tpOper = teste::valorSeOk(TipoOperacao::de(u"4"));
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(tpOper.has_value());
    QVERIFY(indDest.has_value());
    QVERIFY(umaReferencia().has_value());
    QCOMPARE(teste::codigoDoErro(Ibscbs::montar(std::nullopt, *tpOper, umaReferencia(), *indDest, *tributacao)),
             QStringLiteral("E0906"));
}

void TstIbscbs::tpOper5ComGRefNFSeRejeitado()
{
    // E0906 (aba RN de 20/10, linha Excel 577): tpOper 5 (fornecimento e recebimento concomitantes) também proíbe gRefNFSe.
    const auto tributacao = tributacaoValida();
    const auto tpOper = teste::valorSeOk(TipoOperacao::de(u"5"));
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(tpOper.has_value());
    QVERIFY(indDest.has_value());
    QVERIFY(umaReferencia().has_value());
    QCOMPARE(teste::codigoDoErro(Ibscbs::montar(std::nullopt, *tpOper, umaReferencia(), *indDest, *tributacao)),
             QStringLiteral("E0906"));
}

void TstIbscbs::ibscbsValoresObrigatorioQuandoGrupoExiste()
{
    // IBSCBS/valores é 1-1 (Anexo de 20/10, linha Excel 390; tiposComplexos_v1.01.xsd:2387). Quando o grupo existe, trib vem com CST e cClassTrib.
    // O tipo já impõe isso (sem valor padrão); o teste documenta a regra e pega qualquer afrouxamento futuro.
    const auto tributacao = tributacaoValida();
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(indDest.has_value());
    const auto ibscbs = Ibscbs::montar(std::nullopt, std::nullopt, std::nullopt, *indDest, *tributacao);
    QVERIFY(ibscbs.has_value());
    QCOMPARE(ibscbs->trib().cst().texto(), QStringLiteral("000"));
    QCOMPARE(ibscbs->trib().cClassTrib().texto(), QStringLiteral("000001"));
}

void TstIbscbs::semTpOperComGRefNFSeRejeitado()
{
    const auto tributacao = tributacaoValida();
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(indDest.has_value());
    QVERIFY(umaReferencia().has_value());
    QCOMPARE(teste::codigoDoErro(Ibscbs::montar(std::nullopt, std::nullopt, umaReferencia(), *indDest, *tributacao)),
             QStringLiteral("E0906"));
}

void TstIbscbs::semTpOperSemGRefNFSeAceito()
{
    const auto tributacao = tributacaoValida();
    const auto indDest = teste::valorSeOk(IndicadorDestinatario::de(u"0"));
    QVERIFY(tributacao.has_value());
    QVERIFY(indDest.has_value());
    QVERIFY(Ibscbs::montar(std::nullopt, std::nullopt, std::nullopt, *indDest, *tributacao).has_value());
}

void TstIbscbs::tipoOperacaoDeUmACinco()
{
    // TSRTCTpOper (tiposSimples_v1.01.xsd:1719-1736): 1 a 5.
    for (const char16_t* valor : {u"1", u"2", u"3", u"4", u"5"}) {
        QVERIFY(TipoOperacao::de(valor).has_value());
    }
    QCOMPARE(teste::codigoDoErro(TipoOperacao::de(u"6")), QStringLiteral("tipooperacao.valor"));
}

void TstIbscbs::indicadorDestinatarioZeroOuUm()
{
    // TSRTCIndDest (tiposSimples_v1.01.xsd:1758-1768): 0 e 1.
    QVERIFY(IndicadorDestinatario::de(u"0").has_value());
    QVERIFY(IndicadorDestinatario::de(u"1").has_value());
    QCOMPARE(teste::codigoDoErro(IndicadorDestinatario::de(u"2")), QStringLiteral("indicadordestinatario.valor"));
}

QTEST_GUILESS_MAIN(TstIbscbs)
#include "tst_ibscbs.moc"
