// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0910 (NL-3.L): indDest = 1 exige o grupo dest do destinatário, que o app ainda não tem; a regra recusa 1
// localmente, como a E0390 faz com atvEvento. indDest = 0 passa. Linha Excel 300 da aba RN DPS_NFS-e 1.01 do Anexo I de
// 24/11/2026 (rejeição ativa: ponto 5 ratificado pelo líder em 27/11/2026). TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0910.h>

#include "fabrica_dps.h"

using nl::back::RegraE0910;

class TstRegraE0910 : public QObject
{
    Q_OBJECT

private slots:
    void indDestUmSemDestRejeitadoPelaE0910();
    void indDestZeroPassaNaE0910();
    void mensagemDizQueOAppAindaNaoTemOGrupoDest();
};

void TstRegraE0910::indDestUmSemDestRejeitadoPelaE0910()
{
    teste::Opcoes opcoes;
    opcoes.indDest = "1";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const auto erro = RegraE0910{}.verificar(*dps);
    QCOMPARE(teste::codigoDe(erro), QStringLiteral("E0910"));
    QVERIFY(erro.has_value());
    QCOMPARE(QString::fromStdString(erro->campo().value_or(std::string{})), QStringLiteral("Dps.infDPS.dest"));
}

void TstRegraE0910::indDestZeroPassaNaE0910()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0910{}.verificar(*dps).has_value());
}

void TstRegraE0910::mensagemDizQueOAppAindaNaoTemOGrupoDest()
{
    teste::Opcoes opcoes;
    opcoes.indDest = "1";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const auto erro = RegraE0910{}.verificar(*dps);
    QVERIFY(erro.has_value());
    QVERIFY(QString::fromStdString(erro->mensagem()).contains(QStringLiteral("o app ainda não tem o grupo dest")));
}

QTEST_GUILESS_MAIN(TstRegraE0910)
#include "tst_regra_e0910.moc"
