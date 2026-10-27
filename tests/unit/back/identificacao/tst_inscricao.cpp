// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da inscrição (NL-1.1): a variante guarda CNPJ ou CPF, sem perder o tipo.

#include <nl/back/identificacao/inscricao.h>

#include <variant>

#include <QtTest>

using nl::back::Cnpj;
using nl::back::Cpf;
using nl::back::Inscricao;

class TstInscricao : public QObject
{
    Q_OBJECT

private slots:
    void inscricaoGuardaCnpjOuCpf();
};

void TstInscricao::inscricaoGuardaCnpjOuCpf()
{
    const auto cnpj = Cnpj::de(u"12345678000195");
    QVERIFY(cnpj.has_value());
    const Inscricao porCnpj = *cnpj;
    QVERIFY(std::holds_alternative<Cnpj>(porCnpj));
    QCOMPARE(std::get<Cnpj>(porCnpj).texto(), QStringLiteral("12345678000195"));

    const auto cpf = Cpf::de(u"123.456.789-09");
    QVERIFY(cpf.has_value());
    const Inscricao porCpf = *cpf;
    QVERIFY(std::holds_alternative<Cpf>(porCpf));
    QCOMPARE(std::get<Cpf>(porCpf).texto(), QStringLiteral("12345678909"));
}

QTEST_GUILESS_MAIN(TstInscricao)
#include "tst_inscricao.moc"
