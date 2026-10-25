// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste de sanidade do NL-0.2. Vermelho antes da camada nl_foundation_base expor nomeCamada(); verde depois.

#include <nl/base/camada.h>

#include <QtTest>

class TstSanidade : public QObject
{
    Q_OBJECT

private slots:
    void nomeCamadaDaBaseConfere();
};

void TstSanidade::nomeCamadaDaBaseConfere()
{
    QVERIFY2(nl::base::nomeCamada() == std::string_view{"base"},
             "nl::base::nomeCamada() deve devolver \"base\"");
}

QTEST_GUILESS_MAIN(TstSanidade)
#include "tst_sanidade.moc"
