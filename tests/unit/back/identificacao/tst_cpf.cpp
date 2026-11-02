// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do CPF (NL-1.1, DA-1.1). Regra pura, sem I/O.
// Vetor de conferência do DV: sequência 123456789, DV 09. Conferido à mão nesta fatia pela regra módulo 11
// (pesos 10 a 2 para o primeiro DV, 11 a 2 para o segundo), não por fonte oficial citada.

#include <nl/back/identificacao/cpf.h>

#include <array>
#include <string_view>

#include <QtTest>

namespace {

QStringView comoVista(std::u16string_view texto)
{
    return QStringView{texto.data(), static_cast<qsizetype>(texto.size())};
}

} // namespace

using nl::back::Cpf;

class TstCpf : public QObject
{
    Q_OBJECT

private slots:
    void cpfComMascaraNormalizado();
    void cpfSemMascaraAceito();
    void cpfExibidoComMascara();
    void cpfDigitosRepetidosRejeitado();
    void cpfDvErradoRejeitado();
    void cpfTamanhoErradoRejeitado();
    void cpfFormatoRejeitado();
    void cpfComZeroInicialPreservaZero();
    void cpfComZeroInicialDvErradoRejeitado();
};

void TstCpf::cpfComMascaraNormalizado()
{
    // DA-1.1: remove ponto, hífen e espaço.
    const auto cpf = Cpf::de(u"123.456.789-09");
    QVERIFY(cpf.has_value());
    QCOMPARE(cpf->texto(), QStringLiteral("12345678909"));
}

void TstCpf::cpfSemMascaraAceito()
{
    const auto cpf = Cpf::de(u"12345678909");
    QVERIFY(cpf.has_value());
    QCOMPARE(cpf->texto(), QStringLiteral("12345678909"));
}

void TstCpf::cpfExibidoComMascara()
{
    const auto cpf = Cpf::de(u"12345678909");
    QVERIFY(cpf.has_value());
    QCOMPARE(cpf->comMascara(), QStringLiteral("123.456.789-09"));
}

void TstCpf::cpfDigitosRepetidosRejeitado()
{
    // Dígitos repetidos passam no módulo 11 (o DV de 00000000000 é 00), então a rejeição é explícita.
    const std::array<std::u16string_view, 2> casos{u"111.111.111-11", u"00000000000"};
    for (const auto caso : casos) {
        const auto cpf = Cpf::de(comoVista(caso));
        QVERIFY(!cpf.has_value());
        QCOMPARE(QString::fromStdString(cpf.error().codigo()), QStringLiteral("cpf.repetido"));
    }
}

void TstCpf::cpfDvErradoRejeitado()
{
    const std::array<std::u16string_view, 2> casos{u"123.456.789-10", u"12345678910"};
    for (const auto caso : casos) {
        const auto cpf = Cpf::de(comoVista(caso));
        QVERIFY(!cpf.has_value());
        QCOMPARE(QString::fromStdString(cpf.error().codigo()), QStringLiteral("cpf.dv"));
    }
}

void TstCpf::cpfTamanhoErradoRejeitado()
{
    // 10 dígitos e 12 dígitos: erro de tamanho, sem completar zero.
    const std::array<std::u16string_view, 2> casos{u"1234567890", u"123456789090"};
    for (const auto caso : casos) {
        const auto cpf = Cpf::de(comoVista(caso));
        QVERIFY(!cpf.has_value());
        QCOMPARE(QString::fromStdString(cpf.error().codigo()), QStringLiteral("cpf.tamanho"));
    }
}

void TstCpf::cpfComZeroInicialPreservaZero()
{
    // Fixture fictícia (base 012345678, DVs 90 pelo módulo 11, calculados no contêiner): o zero da frente é dígito e tem de sair no texto.
    // Com a máscara, o zero aparece na primeira posição: 012.345.678-90.
    const auto cpf = Cpf::de(u"012.345.678-90");
    QVERIFY(cpf.has_value());
    QCOMPARE(cpf->texto(), QStringLiteral("01234567890"));
    QCOMPARE(cpf->texto().size(), qsizetype{11});
    QCOMPARE(cpf->comMascara(), QStringLiteral("012.345.678-90"));
}

void TstCpf::cpfComZeroInicialDvErradoRejeitado()
{
    // Mesma fixture com o último dígito trocado: a rejeição vem do DV, não do zero à frente.
    const auto cpf = Cpf::de(u"01234567891");
    QVERIFY(!cpf.has_value());
    QCOMPARE(QString::fromStdString(cpf.error().codigo()), QStringLiteral("cpf.dv"));
}

void TstCpf::cpfFormatoRejeitado()
{
    // Letra no CPF é formato. A barra não é separador de CPF (só ponto, hífen e espaço), então "123/456789-09" é formato.
    const std::array<std::u16string_view, 2> casos{u"1234567890A", u"123/456789-09"};
    for (const auto caso : casos) {
        const auto cpf = Cpf::de(comoVista(caso));
        QVERIFY(!cpf.has_value());
        QCOMPARE(QString::fromStdString(cpf.error().codigo()), QStringLiteral("cpf.formato"));
    }
}

QTEST_GUILESS_MAIN(TstCpf)
#include "tst_cpf.moc"
