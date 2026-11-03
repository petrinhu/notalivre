// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do leitor do totTrib (NL-2.10j, J1-r2; B-1 da revisão QA). Uma forma por grupo, duas formas recusadas
// (entrada.campos_conflitantes), percentual mal formado e campo ausente. Sem I/O: o JSON é literal.

#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/percentual_tributo.h>
#include <nl/back/dps/percentual_tributo_sn.h>
#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/tributos_monetarios.h>
#include <nl/back/dps/tributos_percentuais.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include <expected>
#include <variant>

#include "leitores_valores.h"

namespace nb = nl::back;

class TstLerTotTrib : public QObject
{
    Q_OBJECT

private slots:
    void indicadorUnicoAceita();
    void monetariosUnicosAceitamEValoresCentavos();
    void percentuaisUnicosAceitamEPreservamTexto();
    void snUnicoAceita();
    void indicadorEMonetariosRecusamComCamposConflitantes();
    void percentuaisEMonetariosRecusamComCamposConflitantes();
    void snEPercentuaisRecusamComCamposConflitantes();
    void percentualParcialRecusaComCampoAusente();
    void percentualMalFormadoRecusa();
    void snMalFormadoRecusa();
    void semFormaRecusaComCampoAusente();
};

namespace {

/// Lê o grupo totTrib de um JSON literal, com o caminho "valores" que o leitor usa na DPS.
std::expected<nb::TotalTributos, nl::base::Erro> lerDe(const char* totTrib)
{
    const QByteArray texto = QByteArray{"{\"totTrib\": "} + totTrib + "}";
    const QJsonObject valores = QJsonDocument::fromJson(texto).object();
    return nl::mid::composicao::leitores::lerTotTrib(valores, "valores");
}

QString codigoDe(const std::expected<nb::TotalTributos, nl::base::Erro>& resultado)
{
    return resultado ? QString{} : QString::fromStdString(resultado.error().codigo());
}

} // namespace

void TstLerTotTrib::indicadorUnicoAceita()
{
    const auto resultado = lerDe(R"({"indTotTrib": "0"})");
    QVERIFY2(resultado.has_value(), qPrintable(codigoDe(resultado)));
    QVERIFY(std::holds_alternative<nb::IndicadorTotalTributos>(*resultado));
}

void TstLerTotTrib::monetariosUnicosAceitamEValoresCentavos()
{
    const auto resultado = lerDe(R"({"vTotTribFed": "1,00", "vTotTribEst": "2,50", "vTotTribMun": "0,00"})");
    QVERIFY2(resultado.has_value(), qPrintable(codigoDe(resultado)));
    const auto* monetarios = std::get_if<nb::TributosMonetarios>(&*resultado);
    QVERIFY(monetarios != nullptr);
    QCOMPARE(monetarios->vTotTribFed().centavos(), std::int64_t{100});
    QCOMPARE(monetarios->vTotTribEst().centavos(), std::int64_t{250});
    QCOMPARE(monetarios->vTotTribMun().centavos(), std::int64_t{0});
}

void TstLerTotTrib::percentuaisUnicosAceitamEPreservamTexto()
{
    const auto resultado = lerDe(R"({"pTotTribFed": "1.00", "pTotTribEst": "0.00", "pTotTribMun": "100.00"})");
    QVERIFY2(resultado.has_value(), qPrintable(codigoDe(resultado)));
    const auto* percentuais = std::get_if<nb::TributosPercentuais>(&*resultado);
    QVERIFY(percentuais != nullptr);
    QCOMPARE(percentuais->pTotTribFed().texto(), QStringLiteral("1.00"));
    QCOMPARE(percentuais->pTotTribMun().texto(), QStringLiteral("100.00"));
}

void TstLerTotTrib::snUnicoAceita()
{
    const auto resultado = lerDe(R"({"pTotTribSN": "4.50"})");
    QVERIFY2(resultado.has_value(), qPrintable(codigoDe(resultado)));
    const auto* sn = std::get_if<nb::PercentualTributoSn>(&*resultado);
    QVERIFY(sn != nullptr);
    QCOMPARE(sn->texto(), QStringLiteral("4.50"));
}

void TstLerTotTrib::indicadorEMonetariosRecusamComCamposConflitantes()
{
    const auto resultado = lerDe(R"({"indTotTrib": "0", "vTotTribFed": "1,00", "vTotTribEst": "0,00", "vTotTribMun": "0,00"})");
    QCOMPARE(codigoDe(resultado), QStringLiteral("entrada.campos_conflitantes"));
}

void TstLerTotTrib::percentuaisEMonetariosRecusamComCamposConflitantes()
{
    const auto resultado = lerDe(R"({"vTotTribFed": "1,00", "vTotTribEst": "0,00", "vTotTribMun": "0,00", "pTotTribFed": "1.00", "pTotTribEst": "0.00", "pTotTribMun": "0.00"})");
    QCOMPARE(codigoDe(resultado), QStringLiteral("entrada.campos_conflitantes"));
}

void TstLerTotTrib::snEPercentuaisRecusamComCamposConflitantes()
{
    const auto resultado = lerDe(R"({"pTotTribSN": "4.50", "pTotTribFed": "1.00", "pTotTribEst": "0.00", "pTotTribMun": "0.00"})");
    QCOMPARE(codigoDe(resultado), QStringLiteral("entrada.campos_conflitantes"));
}

void TstLerTotTrib::percentualParcialRecusaComCampoAusente()
{
    const auto resultado = lerDe(R"({"pTotTribFed": "1.00"})");
    QCOMPARE(codigoDe(resultado), QStringLiteral("entrada.campo_ausente"));
}

void TstLerTotTrib::percentualMalFormadoRecusa()
{
    const auto resultado = lerDe(R"({"pTotTribFed": "12,5", "pTotTribEst": "0.00", "pTotTribMun": "0.00"})");
    QCOMPARE(codigoDe(resultado), QStringLiteral("percentual.formato"));
}

void TstLerTotTrib::snMalFormadoRecusa()
{
    const auto resultado = lerDe(R"({"pTotTribSN": "12,5"})");
    QCOMPARE(codigoDe(resultado), QStringLiteral("percentual_sn.formato"));
}

void TstLerTotTrib::semFormaRecusaComCampoAusente()
{
    const auto resultado = lerDe(R"({})");
    QCOMPARE(codigoDe(resultado), QStringLiteral("entrada.campo_ausente"));
}

QTEST_GUILESS_MAIN(TstLerTotTrib)
#include "tst_ler_tottrib.moc"
