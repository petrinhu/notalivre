// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0238 (NL-3.L): o cMun do endereço nacional do tomador existe na tabela de municípios do IBGE, consultada no
// port (o mesmo da E0302). Tomador sem endereço, no exterior ou ausente não dispara. Linha Excel 283 da aba RN DPS_NFS-e 1.01
// do Anexo I de 24/11/2026. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0238.h>

#include "catalogo_falso.h"
#include "fabrica_dps.h"

using nl::back::RegraE0238;

class TstRegraE0238 : public QObject
{
    Q_OBJECT

private slots:
    void municipioDoTomadorInexistenteRejeitadoPelaE0238();
    void municipioDoTomadorExistentePassa();
    void tomadorNoExteriorOuSemEnderecoNaoDisparaE0238();
};

void TstRegraE0238::municipioDoTomadorInexistenteRejeitadoPelaE0238()
{
    teste::Opcoes opcoes;
    opcoes.cMunTomador = "9999999";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    const auto erro = RegraE0238{catalogo}.verificar(*dps);
    QCOMPARE(teste::codigoDe(erro), QStringLiteral("E0238"));
    QVERIFY(erro.has_value());
    QCOMPARE(QString::fromStdString(erro->campo().value_or(std::string{})), QStringLiteral("Dps.infDPS.toma.end.endNac.cMun"));
}

void TstRegraE0238::municipioDoTomadorExistentePassa()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QVERIFY(!RegraE0238{catalogo}.verificar(*dps).has_value());
}

void TstRegraE0238::tomadorNoExteriorOuSemEnderecoNaoDisparaE0238()
{
    // Com cMunTomador inexistente: só o endereço nacional do tomador é conferido, então nenhum destes casos dispara.
    const teste::CatalogoFalso catalogo;
    for (const auto tipo : {teste::TipoTomador::exteriorComPais, teste::TipoTomador::cnpjSemEndereco, teste::TipoTomador::semTomador}) {
        teste::Opcoes opcoes;
        opcoes.cMunTomador = "9999999";
        opcoes.tomador = tipo;
        const auto dps = teste::dpsCom(opcoes);
        QVERIFY(dps.has_value());
        QVERIFY(!RegraE0238{catalogo}.verificar(*dps).has_value());
    }
}

QTEST_GUILESS_MAIN(TstRegraE0238)
#include "tst_regra_e0238.moc"
