// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do Id da DPS de 45 posições (NL-1.3). Regra pura, sem I/O.
// Fontes: tiposSimples_v1.01.xsd:44-54 (TSIdDPS, regra de formação em 48 e pattern em 54);
// docs/pesquisa/onda1_leiaute.md, seção 3, item (d): CPF ocupa as posições finais com "000" à ESQUERDA, tipo 1 para CPF e 2 para CNPJ.

#include <nl/back/dps/id_dps.h>

#include <QRegularExpression>
#include <QtTest>

#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/cpf.h>
#include <nl/back/identificacao/inscricao.h>

using nl::back::Cnpj;
using nl::back::CodigoMunicipio;
using nl::back::Cpf;
using nl::back::IdDps;
using nl::back::Inscricao;
using nl::back::NumeroDps;
using nl::back::Serie;

namespace {

// Exemplo do leiaute de onda1_leiaute.md, seção 3: município 4305108, CPF 12345678909, série 1, número 1.
constexpr auto kMunicipioExemplo = u"4305108";
constexpr auto kCpfExemplo = u"12345678909";
constexpr auto kCnpjAlfanumerico = u"12ABC34501DE35";

// Padrão do XSD por inteiro, ancorado. Pattern de tiposSimples_v1.01.xsd:54.
const QRegularExpression& padraoXsd()
{
    static const QRegularExpression padrao{QRegularExpression::anchoredPattern(
        QStringLiteral("DPS[0-9]{7}(1[0-9]{14}|2[0-9A-Z]{14})[0-9]{20}"))};
    return padrao;
}

Inscricao cpfExemplo()
{
    return Inscricao{*Cpf::de(kCpfExemplo)};
}

} // namespace

class TstIdDps : public QObject
{
    Q_OBJECT

private slots:
    void idDpsTem45Posicoes();
    void idDpsComCpfZerosAEsquerda();
    void idDpsComCpfNuncaZerosADireita();
    void idDpsComCnpjAlfanumericoUsaTipo2();
    void idDpsCasaComPatternDoXsd();
    void idDpsComCnpjNumericoCasaComPatternDoXsd();
};

void TstIdDps::idDpsTem45Posicoes()
{
    const auto id = IdDps::montar(*CodigoMunicipio::de(kMunicipioExemplo), cpfExemplo(), *Serie::de(1), *NumeroDps::de(1));
    QVERIFY(id.has_value());
    QCOMPARE(id->texto().size(), qsizetype{45});
}

void TstIdDps::idDpsComCpfZerosAEsquerda()
{
    // Saída exata do exemplo de onda1_leiaute.md, seção 3: "DPS4305108" + tipo "1" + "00012345678909" + série "00001" + número "000000000000001".
    const auto id = IdDps::montar(*CodigoMunicipio::de(kMunicipioExemplo), cpfExemplo(), *Serie::de(1), *NumeroDps::de(1));
    QVERIFY(id.has_value());
    QCOMPARE(id->texto(), QStringLiteral("DPS4305108") + QStringLiteral("1") + QStringLiteral("00012345678909")
                              + QStringLiteral("00001") + QStringLiteral("000000000000001"));
    QCOMPARE(id->texto(), QStringLiteral("DPS4305108100012345678909") + QStringLiteral("00001") + QStringLiteral("000000000000001"));
}

void TstIdDps::idDpsComCpfNuncaZerosADireita()
{
    // O pattern do XSD aceita "000" à direita e à esquerda (onda1_leiaute.md, seção 3, linha 133). A regra é posicional:
    // as posições 11 a 24 (índices de 0) têm de ser "000" + CPF, e nunca CPF + "000".
    const auto id = IdDps::montar(*CodigoMunicipio::de(kMunicipioExemplo), cpfExemplo(), *Serie::de(1), *NumeroDps::de(1));
    QVERIFY(id.has_value());
    QCOMPARE(id->texto().mid(11, 14), QStringLiteral("00012345678909"));
    QVERIFY(id->texto().mid(11, 14) != QStringLiteral("12345678909000"));
}

void TstIdDps::idDpsComCnpjAlfanumericoUsaTipo2()
{
    // Vetor oficial 12ABC34501DE35 (Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.14, citado em tst_cnpj.cpp).
    // O tipo 2 marca CNPJ; a inscrição ocupa as 14 posições sem preenchimento.
    const auto cnpj = Cnpj::de(kCnpjAlfanumerico);
    QVERIFY(cnpj.has_value());
    const auto id = IdDps::montar(*CodigoMunicipio::de(kMunicipioExemplo), Inscricao{*cnpj}, *Serie::de(1), *NumeroDps::de(1));
    QVERIFY(id.has_value());
    QCOMPARE(id->texto(), QStringLiteral("DPS4305108") + QStringLiteral("2") + QStringLiteral("12ABC34501DE35")
                              + QStringLiteral("00001") + QStringLiteral("000000000000001"));
}

void TstIdDps::idDpsCasaComPatternDoXsd()
{
    // Pattern de tiposSimples_v1.01.xsd:54 aplicado por inteiro (ramo CPF, tipo 1), com o Id de exemplo.
    const auto id = IdDps::montar(*CodigoMunicipio::de(kMunicipioExemplo), cpfExemplo(), *Serie::de(1), *NumeroDps::de(1));
    QVERIFY(id.has_value());
    QVERIFY(padraoXsd().match(id->texto()).hasMatch());
}

void TstIdDps::idDpsComCnpjNumericoCasaComPatternDoXsd()
{
    // Ramo CNPJ do pattern de tiposSimples_v1.01.xsd:54 (tipo 2, 14 de [0-9A-Z]) com CNPJ alfanumérico real (vetor 12ABC34501DE35).
    const auto cnpj = Cnpj::de(kCnpjAlfanumerico);
    QVERIFY(cnpj.has_value());
    const auto id = IdDps::montar(*CodigoMunicipio::de(kMunicipioExemplo), Inscricao{*cnpj}, *Serie::de(49999), *NumeroDps::de(999999999999999));
    QVERIFY(id.has_value());
    QVERIFY(padraoXsd().match(id->texto()).hasMatch());
    QCOMPARE(id->texto().size(), qsizetype{45});
}

QTEST_GUILESS_MAIN(TstIdDps)
#include "tst_id_dps.moc"
