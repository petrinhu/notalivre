// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da entidade Dps (NL-1.4): constantes por construção (tpEmit 1, finNFSe 0), grupo IBS/CBS opcional em 2026,
// tomador como retrato da época, datas válidas e Id derivado da montagem (NL-1.3).

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/dps.h>
#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/reg_esp_trib.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/tipo_ambiente.h>
#include <nl/back/dps/trib_issqn.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>
#include <nl/back/dps/valores_servico.h>
#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/descricao_servico.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/inscricao.h>
#include <nl/back/dinheiro/valor.h>

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QTimeZone>
#include <QtTest>

#include "ajudantes.h"

using nl::back::Cnpj;
using nl::back::DadosDps;
using nl::back::Dps;
using nl::back::Prestador;
using nl::back::Servico;
using nl::back::Tomador;
using nl::back::TipoAmbiente;
using nl::back::Inscricao;
using nl::back::RegimeTributacao;
using nl::back::ValoresServico;
using nl::back::Tributacao;
using nl::back::TributacaoMunicipal;
using nl::back::TotalTributos;

namespace {

/// Monta um conjunto de dados válidos. Devolve std::nullopt se algum tipo de entrada falhar (o teste acusa com QVERIFY).
std::optional<DadosDps> dadosValidos()
{
    using namespace nl::back;
    const auto tpAmb = teste::valorSeOk(TipoAmbiente::de(u"2"));
    const auto verAplic = teste::valorSeOk(VerAplic::de(u"NotaLivre 0.1", "verAplic"));
    const auto serie = teste::valorSeOk(Serie::de(1));
    const auto nDps = teste::valorSeOk(NumeroDps::de(1));
    const auto cLocEmi = teste::valorSeOk(CodigoMunicipio::de(u"4305108"));
    const auto cnpj = teste::valorSeOk(Cnpj::de(u"11222333000181"));
    const auto opSimpNac = teste::valorSeOk(OpSimpNac::de(u"2"));
    const auto regEspTrib = teste::valorSeOk(RegEspTrib::de(u"0"));
    const auto cLocPrestacao = teste::valorSeOk(CodigoMunicipio::de(u"4305108"));
    const auto cTribNac = teste::valorSeOk(CodigoTribNacional::de(u"170201"));
    const auto xDescServ = teste::valorSeOk(DescricaoServico::de(u"Consultoria em tecnologia"));
    const auto vServ = teste::valorSeOk(Valor::de(10000));
    const auto issqn = teste::valorSeOk(TribIssqn::de(u"1"));
    const auto retencao = teste::valorSeOk(RetencaoIssqn::de(u"1"));
    const auto indTot = teste::valorSeOk(IndicadorTotalTributos::de(u"0"));
    if (!tpAmb || !verAplic || !serie || !nDps || !cLocEmi || !cnpj || !opSimpNac || !regEspTrib || !cLocPrestacao || !cTribNac
        || !xDescServ || !vServ || !issqn || !retencao || !indTot) {
        return std::nullopt;
    }
    const auto regime = teste::valorSeOk(RegimeTributacao::montar(*opSimpNac, std::nullopt, *regEspTrib));
    const auto municipal = teste::valorSeOk(TributacaoMunicipal::montar(*issqn, *retencao, std::nullopt));
    if (!regime || !municipal) {
        return std::nullopt;
    }

    DadosDps dados{
        .tpAmb = *tpAmb,
        .dhEmi = QDateTime{QDate{2026, 10, 25}, QTime{10, 0}, QTimeZone::utc()},
        .verAplic = *verAplic,
        .serie = *serie,
        .nDps = *nDps,
        .dCompet = QDate{2026, 10, 1},
        .cLocEmi = *cLocEmi,
        .subst = std::nullopt,
        .prest = Prestador::montar(Inscricao{*cnpj}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, *regime),
        .toma = std::nullopt,
        .serv = Servico::montar(*cLocPrestacao, *cTribNac, *xDescServ, std::nullopt),
        .valores = ValoresServico::montar(*vServ, Tributacao::montar(*municipal, TotalTributos{*indTot})),
        .ibscbs = std::nullopt,
    };
    return dados;
}

} // namespace

class TstDps : public QObject
{
    Q_OBJECT

private slots:
    void tpEmitSempre1();
    void grupoIbsCbsAusentePermitidoEm2026();
    void tomadorNaDpsGuardaRetratoDaEpoca();
    void dhEmiInvalidoRejeitado();
    void dCompetInvalidoRejeitado();
    void idDerivadoDaMontagem();
    void tipoAmbienteDeUmADois();
};

void TstDps::tpEmitSempre1()
{
    // tpEmit = 1 por construção (DA-1.3). static_assert exige constexpr: uma versão configurável não compila aqui.
    static_assert(Dps::tpEmit() == 1, "tpEmit tem de ser 1 por construção");
    static_assert(Dps::finNFSe() == 0, "finNFSe tem de ser 0 por construção");
    QCOMPARE(Dps::tpEmit(), 1);
    QCOMPARE(Dps::finNFSe(), 0);
}

void TstDps::grupoIbsCbsAusentePermitidoEm2026()
{
    // IBSCBS é 0-1 em infDPS (tiposComplexos_v1.01.xsd:851 e DPS linha 390 do Anexo de 20/10 para o trib). Opcional em 2026 (spec 4.11).
    const auto dados = dadosValidos();
    QVERIFY(dados.has_value());
    QVERIFY(!dados->ibscbs.has_value());
    QVERIFY(Dps::montar(*dados).has_value());
}

void TstDps::tomadorNaDpsGuardaRetratoDaEpoca()
{
    // A DPS copia o tomador na montagem. Trocar o dado de origem depois não altera a nota já montada.
    const auto dados = dadosValidos();
    QVERIFY(dados.has_value());
    DadosDps origem = *dados;
    const auto cpf = teste::valorSeOk(nl::back::Cpf::de(u"52998224725"));
    const auto nomeAntes = teste::valorSeOk(nl::back::NomeRazaoSocial::de(u"Cliente Antes", "toma/xNome"));
    QVERIFY(cpf.has_value());
    QVERIFY(nomeAntes.has_value());
    origem.toma = Tomador::montar(nl::back::IdentificacaoPessoa{*cpf}, std::nullopt, *nomeAntes, std::nullopt, std::nullopt, std::nullopt);
    const auto dps = Dps::montar(origem);
    QVERIFY(dps.has_value());

    const auto nomeDepois = teste::valorSeOk(nl::back::NomeRazaoSocial::de(u"Cliente Depois", "toma/xNome"));
    QVERIFY(nomeDepois.has_value());
    origem.toma = Tomador::montar(nl::back::IdentificacaoPessoa{*cpf}, std::nullopt, *nomeDepois, std::nullopt, std::nullopt, std::nullopt);

    QVERIFY(dps->dados().toma.has_value());
    QCOMPARE(dps->dados().toma->xNome()->texto(), QStringLiteral("Cliente Antes"));
}

void TstDps::dhEmiInvalidoRejeitado()
{
    auto dados = dadosValidos();
    QVERIFY(dados.has_value());
    dados->dhEmi = QDateTime{};
    QCOMPARE(teste::codigoDoErro(Dps::montar(*dados)), QStringLiteral("dps.dhemi"));
}

void TstDps::dCompetInvalidoRejeitado()
{
    auto dados = dadosValidos();
    QVERIFY(dados.has_value());
    dados->dCompet = QDate{};
    QCOMPARE(teste::codigoDoErro(Dps::montar(*dados)), QStringLiteral("dps.dcompet"));
}

void TstDps::idDerivadoDaMontagem()
{
    // Id = "DPS" + cLocEmi(7) + tipo 2 (CNPJ) + inscrição(14) + série(5) + nDPS(15): 45 posições (NL-1.3).
    const auto dados = dadosValidos();
    QVERIFY(dados.has_value());
    const auto dps = Dps::montar(*dados);
    QVERIFY(dps.has_value());
    QCOMPARE(dps->id().texto(), QStringLiteral("DPS4305108" "2" "11222333000181" "00001" "000000000000001"));
}

void TstDps::tipoAmbienteDeUmADois()
{
    QVERIFY(TipoAmbiente::de(u"1").has_value());
    QVERIFY(TipoAmbiente::de(u"2").has_value());
    QCOMPARE(teste::codigoDoErro(TipoAmbiente::de(u"3")), QStringLiteral("ambiente.valor"));
}

QTEST_GUILESS_MAIN(TstDps)
#include "tst_dps.moc"
