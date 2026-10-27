// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do código de município (NL-1.1). Regra pura, sem I/O. Só o formato de 7 dígitos é validado aqui.

#include <nl/back/identificacao/codigo_municipio.h>

#include <array>
#include <string_view>

#include <QtTest>

namespace {

QStringView comoVista(std::u16string_view texto)
{
    return QStringView{texto.data(), static_cast<qsizetype>(texto.size())};
}

} // namespace

using nl::back::CodigoMunicipio;

class TstCodigoMunicipio : public QObject
{
    Q_OBJECT

private slots:
    void codigoMunicipioValidoAceito();
    void codigoMunicipioPreservaZeroAEsquerda();
    void codigoMunicipioNaoCompletaZero();
    void codigoMunicipioCaractereNaoDigitoRejeitado();
};

void TstCodigoMunicipio::codigoMunicipioValidoAceito()
{
    // 3550308: código de 7 dígitos do IBGE usado como exemplo de formato.
    const auto codigo = CodigoMunicipio::de(u"3550308");
    QVERIFY(codigo.has_value());
    QCOMPARE(codigo->texto(), QStringLiteral("3550308"));
}

void TstCodigoMunicipio::codigoMunicipioPreservaZeroAEsquerda()
{
    // Exemplo sintético de 7 dígitos com zero à esquerda: o tipo guarda o texto, não o número.
    const auto codigo = CodigoMunicipio::de(u"0123456");
    QVERIFY(codigo.has_value());
    QCOMPARE(codigo->texto(), QStringLiteral("0123456"));
}

void TstCodigoMunicipio::codigoMunicipioNaoCompletaZero()
{
    // Menos de 7 dígitos é erro de tamanho: o tipo não completa zero à esquerda (DA-1.1 pelo mesmo princípio).
    const std::array<std::u16string_view, 2> casos{u"123456", u"12345678"};
    for (const auto caso : casos) {
        const auto codigo = CodigoMunicipio::de(comoVista(caso));
        QVERIFY(!codigo.has_value());
        QCOMPARE(QString::fromStdString(codigo.error().codigo()), QStringLiteral("municipio.tamanho"));
    }
}

void TstCodigoMunicipio::codigoMunicipioCaractereNaoDigitoRejeitado()
{
    const auto codigo = CodigoMunicipio::de(u"355030A");
    QVERIFY(!codigo.has_value());
    QCOMPARE(QString::fromStdString(codigo.error().codigo()), QStringLiteral("municipio.formato"));
}

QTEST_GUILESS_MAIN(TstCodigoMunicipio)
#include "tst_codigo_municipio.moc"
