// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do manipulador de mensagens do Qt ligado ao Log (NL-2.9, ADR-0005 a): mensagem com CPF não sai pelo Log.
#include <nl/foundation/log/log.h>
#include <nl/mid/composicao/manipulador_qt.h>

#include <QtTest>

#include <sstream>

class TstManipuladorQt : public QObject
{
    Q_OBJECT

private slots:
    // qWarning com CPF não aparece no registro; o descarte fica contado.
    void qWarningComCpfNaoVaiAoLog();
};

void TstManipuladorQt::qWarningComCpfNaoVaiAoLog()
{
    std::ostringstream destino;
    nl::foundation::log::Log registro{destino, "notalivre-cli", {}, {"qt.mensagem"}};
    nl::mid::composicao::instalarManipuladorQt(&registro);
    qWarning("CPF 12345678901 no aviso do Qt");
    nl::mid::composicao::instalarManipuladorQt(nullptr);
    QVERIFY2(destino.str().find("12345678901") == std::string::npos, "o CPF da mensagem do Qt não pode ir ao Log");
    QCOMPARE(registro.descartados(), std::size_t{1});
}

QTEST_APPLESS_MAIN(TstManipuladorQt)
#include "tst_manipulador_qt.moc"
