// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de unidade da montagem dos vínculos da Foundation sql (NL-3.1a, bloco U-R): sem servidor. No R, o stub devolve
// sql.nao_implementado e cada slot falha citando esse código; o U-G põe os valores reais.

#include <nl/base/erro.h>
#include <nl/foundation/sql/parametro.h>
#include <nl/foundation/sql/vinculos.h>

#include <QByteArray>
#include <QDate>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <expected>
#include <vector>

namespace sql = nl::foundation::sql;
namespace detalhe = nl::foundation::sql::detalhe;

class TstSqlVinculos : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void umVinculoPorParametro();
    void tamanhoDoTextoEhOReal();
    void textoComNulNoMeioMantemTamanho();
    void nuloMarcaIsNull();
    void dataSemFuso();
    void blobComTipoBinario();
};

void TstSqlVinculos::umVinculoPorParametro()
{
    const std::vector<sql::Parametro> parametros{
        sql::Parametro::inteiro(1),
        sql::Parametro::texto(QByteArray("a")),
        sql::Parametro::bytes(QByteArray("\x01", 1)),
        sql::Parametro::data(QDate(2026, 11, 21)),
        sql::Parametro::nulo(),
    };
    const auto resultado = detalhe::montarVinculos(parametros);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    QCOMPARE(resultado->vinculos.size(), std::size_t(5));
}

void TstSqlVinculos::tamanhoDoTextoEhOReal()
{
    const std::vector<sql::Parametro> parametros{sql::Parametro::texto(QByteArray("abcde"))};
    const auto resultado = detalhe::montarVinculos(parametros);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    const MYSQL_BIND& vinculo = resultado->vinculos.front();
    QCOMPARE(vinculo.buffer_length, 5UL);
    QVERIFY(vinculo.length != nullptr);
    QCOMPARE(*vinculo.length, 5UL);
}

void TstSqlVinculos::textoComNulNoMeioMantemTamanho()
{
    const std::vector<sql::Parametro> parametros{sql::Parametro::texto(QByteArray("a\0b", 3))};
    const auto resultado = detalhe::montarVinculos(parametros);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    const MYSQL_BIND& vinculo = resultado->vinculos.front();
    QVERIFY(vinculo.length != nullptr);
    QCOMPARE(*vinculo.length, 3UL);
}

void TstSqlVinculos::nuloMarcaIsNull()
{
    const std::vector<sql::Parametro> parametros{sql::Parametro::nulo()};
    const auto resultado = detalhe::montarVinculos(parametros);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    const MYSQL_BIND& vinculo = resultado->vinculos.front();
    QVERIFY(vinculo.is_null != nullptr);
    QCOMPARE(static_cast<int>(*vinculo.is_null), 1);
}

void TstSqlVinculos::dataSemFuso()
{
    const std::vector<sql::Parametro> parametros{sql::Parametro::data(QDate(2026, 11, 21))};
    const auto resultado = detalhe::montarVinculos(parametros);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    const MYSQL_BIND& vinculo = resultado->vinculos.front();
    QCOMPARE(static_cast<int>(vinculo.buffer_type), static_cast<int>(MYSQL_TYPE_DATE));
    QVERIFY(vinculo.buffer != nullptr);
    const auto* tempo = static_cast<const MYSQL_TIME*>(vinculo.buffer);
    QCOMPARE(tempo->year, 2026U);
    QCOMPARE(tempo->month, 11U);
    QCOMPARE(tempo->day, 21U);
    QCOMPARE(tempo->hour, 0U);
    QCOMPARE(static_cast<int>(tempo->time_type), static_cast<int>(MYSQL_TIMESTAMP_DATE));
}

void TstSqlVinculos::blobComTipoBinario()
{
    const std::vector<sql::Parametro> binarios{sql::Parametro::bytes(QByteArray("\x00\x01", 2))};
    const auto resultadoBinario = detalhe::montarVinculos(binarios);
    QVERIFY2(resultadoBinario.has_value(),
             resultadoBinario.has_value() ? "" : resultadoBinario.error().codigo().c_str());
    QCOMPARE(static_cast<int>(resultadoBinario->vinculos.front().buffer_type), static_cast<int>(MYSQL_TYPE_BLOB));

    const std::vector<sql::Parametro> textos{sql::Parametro::texto(QByteArray("a"))};
    const auto resultadoTexto = detalhe::montarVinculos(textos);
    QVERIFY2(resultadoTexto.has_value(),
             resultadoTexto.has_value() ? "" : resultadoTexto.error().codigo().c_str());
    QCOMPARE(static_cast<int>(resultadoTexto->vinculos.front().buffer_type), static_cast<int>(MYSQL_TYPE_STRING));
}

QTEST_GUILESS_MAIN(TstSqlVinculos)
#include "tst_sql_vinculos.moc"
