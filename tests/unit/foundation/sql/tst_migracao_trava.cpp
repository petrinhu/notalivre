// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de unidade da resposta do GET_LOCK do migrador (NL-3.1b, parte b2; parecer do CISO, I-a): sem servidor, um caso
// por forma de resposta. No R, o esboço devolve sql.nao_implementado e cada caso falha.

#include <nl/base/erro.h>
#include <nl/foundation/sql/parametro.h>
#include <nl/foundation/sql/trava_migracao.h>

#include <QByteArray>
#include <QtTest>

#include <expected>

namespace sql = nl::foundation::sql;

namespace {

QByteArray codigoDe(const std::expected<void, nl::base::Erro>& resultado)
{
    return resultado ? QByteArray("ok") : QByteArray::fromStdString(resultado.error().codigo());
}

} // namespace

class TesteTravaDaMigracao : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void umDaATrava();
    void zeroEhTravaOcupada();
    void nuloEhTravaFalhou();
    void semLinhaEhTravaFalhou();
    void duasColunasEhTravaFalhou();
    void textoEhTravaFalhou();
};

void TesteTravaDaMigracao::umDaATrava()
{
    const auto resultado = sql::detalhe::respostaDaTrava(sql::Linhas{sql::Linha{sql::Valor{qint64{1}}}});
    QVERIFY2(resultado.has_value(), codigoDe(resultado).constData());
}

void TesteTravaDaMigracao::zeroEhTravaOcupada()
{
    const auto resultado = sql::detalhe::respostaDaTrava(sql::Linhas{sql::Linha{sql::Valor{qint64{0}}}});
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado), QByteArrayLiteral("migracao.trava_ocupada"));
}

// O servidor devolve NULL quando a trava dá erro: nunca é trava dada.
void TesteTravaDaMigracao::nuloEhTravaFalhou()
{
    const auto resultado = sql::detalhe::respostaDaTrava(sql::Linhas{sql::Linha{sql::Valor{sql::Nulo{}}}});
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado), QByteArrayLiteral("migracao.trava_falhou"));
}

void TesteTravaDaMigracao::semLinhaEhTravaFalhou()
{
    const auto resultado = sql::detalhe::respostaDaTrava(sql::Linhas{});
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado), QByteArrayLiteral("migracao.trava_falhou"));
}

void TesteTravaDaMigracao::duasColunasEhTravaFalhou()
{
    const auto resultado =
        sql::detalhe::respostaDaTrava(sql::Linhas{sql::Linha{sql::Valor{qint64{1}}, sql::Valor{qint64{1}}}});
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado), QByteArrayLiteral("migracao.trava_falhou"));
}

void TesteTravaDaMigracao::textoEhTravaFalhou()
{
    const auto resultado = sql::detalhe::respostaDaTrava(sql::Linhas{sql::Linha{sql::Valor{QByteArray("1")}}});
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado), QByteArrayLiteral("migracao.trava_falhou"));
}

QTEST_GUILESS_MAIN(TesteTravaDaMigracao)
#include "tst_migracao_trava.moc"
