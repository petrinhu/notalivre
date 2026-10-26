// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do valor monetário da camada nl_back (NL-1.2). Escritos antes do código: vermelho por asserção, depois verde.
// Formato do leiaute: TSDec15V2, pattern em /var/tmp/notalivre-pesquisa-onda1/xsd-dl/x/tiposSimples_v1.01.xsd:1452.

#include <nl/back/dinheiro/valor.h>
#include <nl/base/erro.h>

#include <QRegularExpression>
#include <QtTest>

#include <cstdint>
#include <expected>
#include <limits>
#include <string>
#include <vector>

namespace {

using nl::back::Valor;
using Resultado = std::expected<Valor, nl::base::Erro>;

// Código do erro para a asserção. Quando o valor foi aceito, devolve "<sucesso>" para a mensagem de falha mostrar isso.
QString codigoDe(const Resultado& resultado)
{
    if (resultado.has_value()) {
        return QStringLiteral("<sucesso>");
    }
    return QString::fromStdString(resultado.error().codigo());
}

// Pattern do TSDec15V2 (tiposSimples_v1.01.xsd:1452), ancorado: o texto inteiro tem de casar.
bool aceitoPeloLeiaute(const QString& texto)
{
    static const QRegularExpression padrao{QStringLiteral(R"(\A(?:0|0\.[0-9]{2}|[1-9][0-9]{0,14}(?:\.[0-9]{2})?)\z)")};
    return padrao.match(texto).hasMatch();
}

} // namespace

class TstValor : public QObject
{
    Q_OBJECT

private slots:
    void virgulaDecimalAceita();
    void tresCasasRejeitada();
    void somaDeLoteSemPerda();
    void negativoRejeitadoOndeLeiauteProibe();
    void separadorDeMilharPtBr();
    void separadorTrocadoRejeitado();
    void semCasasCompletaDoisZeros();
    void umaCasaCompletaZero();
    void textoVazioRejeitado();
    void lixoRejeitado();
    void digitoLarguraTotalRejeitado();
    void espacosESimboloRejeitados();
    void zeroAEsquerdaRejeitada();
    void tetoAceitoEProximoRejeitado();
    void deCentavosForaDoIntervaloRejeitado();
    void somaQueUltrapassaTetoRejeitada();
    void somaDeLoteVazioEhZero();
    void paraLeiauteComPontoEDuasCasas();
    void paraLeiauteAceitoPeloLeiauteDoXsd();
};

void TstValor::virgulaDecimalAceita()
{
    const auto dez = Valor::deTextoPtBr(u"10,50");
    QVERIFY2(dez.has_value(), qPrintable(codigoDe(dez)));
    QCOMPARE(dez->centavos(), std::int64_t{1050});

    const auto centavo = Valor::deTextoPtBr(u"0,01");
    QVERIFY2(centavo.has_value(), qPrintable(codigoDe(centavo)));
    QCOMPARE(centavo->centavos(), std::int64_t{1});
}

void TstValor::tresCasasRejeitada()
{
    const auto tresCasas = Valor::deTextoPtBr(u"10,123");
    QVERIFY2(!tresCasas.has_value(), "10,123 foi aceito: o leiaute aceita no máximo duas casas");
    QCOMPARE(codigoDe(tresCasas), QStringLiteral("valor.casas"));

    const auto zeroTresCasas = Valor::deTextoPtBr(u"0,000");
    QCOMPARE(codigoDe(zeroTresCasas), QStringLiteral("valor.casas"));
}

void TstValor::somaDeLoteSemPerda()
{
    // 1000 x 0,01 tem de dar 10,00 exato; em ponto flutuante binário acumula erro.
    const auto umCentavo = Valor::deTextoPtBr(u"0,01");
    QVERIFY(umCentavo.has_value());
    const std::vector<Valor> milCentavos(1000, *umCentavo);
    const auto somaMil = Valor::somaDeLote(milCentavos);
    QVERIFY2(somaMil.has_value(), qPrintable(codigoDe(somaMil)));
    QCOMPARE(somaMil->centavos(), std::int64_t{1000});

    // 0,10 + 0,20 tem de dar 0,30 exato (em ponto flutuante binário não dá).
    const auto dez = Valor::deTextoPtBr(u"0,10");
    const auto vinte = Valor::deTextoPtBr(u"0,20");
    QVERIFY(dez.has_value());
    QVERIFY(vinte.has_value());
    const std::vector<Valor> dezMaisVinte{*dez, *vinte};
    const auto trinta = Valor::somaDeLote(dezMaisVinte);
    QVERIFY2(trinta.has_value(), qPrintable(codigoDe(trinta)));
    QCOMPARE(trinta->centavos(), std::int64_t{30});
    QCOMPARE(trinta->paraLeiaute(), QStringLiteral("0.30"));
}

void TstValor::negativoRejeitadoOndeLeiauteProibe()
{
    // TSDec15V2 não tem sinal: o pattern não aceita "-". Texto negativo e centavos negativos são erro.
    for (const char16_t* texto : {u"-1,00", u"-0,01", u"-"}) {
        const auto resultado = Valor::deTextoPtBr(QStringView{texto});
        QCOMPARE(codigoDe(resultado), QStringLiteral("valor.negativo"));
    }
    QCOMPARE(codigoDe(Valor::de(-1)), QStringLiteral("valor.negativo"));
    QCOMPARE(codigoDe(Valor::de(std::numeric_limits<std::int64_t>::min())), QStringLiteral("valor.negativo"));
}

void TstValor::separadorDeMilharPtBr()
{
    const auto milhar = Valor::deTextoPtBr(u"1.234,56");
    QVERIFY2(milhar.has_value(), qPrintable(codigoDe(milhar)));
    QCOMPARE(milhar->centavos(), std::int64_t{123456});

    const auto milhoes = Valor::deTextoPtBr(u"1.234.567,89");
    QVERIFY2(milhoes.has_value(), qPrintable(codigoDe(milhoes)));
    QCOMPARE(milhoes->centavos(), std::int64_t{123456789});

    // Milhar sem centavos: 1.234 é mil duzentos e trinta e quatro reais em pt-br.
    const auto soMilhar = Valor::deTextoPtBr(u"1.234");
    QVERIFY2(soMilhar.has_value(), qPrintable(codigoDe(soMilhar)));
    QCOMPARE(soMilhar->centavos(), std::int64_t{123400});
}

void TstValor::separadorTrocadoRejeitado()
{
    // Ponto usado como decimal (en-US) não agrupa três dígitos: é o separador trocado, e vira erro, não valor silencioso.
    for (const char16_t* texto : {u"10.5", u"1.23", u"1.5000,00", u"12.34,56"}) {
        const auto resultado = Valor::deTextoPtBr(QStringView{texto});
        QCOMPARE(codigoDe(resultado), QStringLiteral("valor.separador"));
    }
}

void TstValor::semCasasCompletaDoisZeros()
{
    const auto dez = Valor::deTextoPtBr(u"10");
    QVERIFY2(dez.has_value(), qPrintable(codigoDe(dez)));
    QCOMPARE(dez->centavos(), std::int64_t{1000});

    const auto zero = Valor::deTextoPtBr(u"0");
    QVERIFY2(zero.has_value(), qPrintable(codigoDe(zero)));
    QCOMPARE(zero->centavos(), std::int64_t{0});
}

void TstValor::umaCasaCompletaZero()
{
    const auto dezCinco = Valor::deTextoPtBr(u"10,5");
    QVERIFY2(dezCinco.has_value(), qPrintable(codigoDe(dezCinco)));
    QCOMPARE(dezCinco->centavos(), std::int64_t{1050});

    const auto meio = Valor::deTextoPtBr(u"0,5");
    QVERIFY2(meio.has_value(), qPrintable(codigoDe(meio)));
    QCOMPARE(meio->centavos(), std::int64_t{50});
}

void TstValor::textoVazioRejeitado()
{
    QCOMPARE(codigoDe(Valor::deTextoPtBr(u"")), QStringLiteral("valor.vazio"));
}

void TstValor::lixoRejeitado()
{
    for (const char16_t* texto : {u"abc", u"1,2,3", u"10,", u",50", u"10,5x", u"1a0,00", u"1,"}) {
        const auto resultado = Valor::deTextoPtBr(QStringView{texto});
        QCOMPARE(codigoDe(resultado), QStringLiteral("valor.formato"));
    }
}

void TstValor::digitoLarguraTotalRejeitado()
{
    // Dígitos ASCII apenas: QChar::isDigit() aceitaria estes; a regra de valor não aceita.
    // Largura total (U+FF11, U+FF10), espaço não separador (U+00A0) e dígito árabe-indico (U+0661).
    for (const char16_t* texto : {u"１０,00", u"1 234,56", u"١,00"}) {
        const auto resultado = Valor::deTextoPtBr(QStringView{texto});
        QCOMPARE(codigoDe(resultado), QStringLiteral("valor.formato"));
    }
}

void TstValor::espacosESimboloRejeitados()
{
    // Sem trim: a Back valida gramática estrita; a Front é quem tira espaço da tela.
    for (const char16_t* texto : {u" 10,00", u"10,00 ", u"R$ 10,00", u"+10,00"}) {
        const auto resultado = Valor::deTextoPtBr(QStringView{texto});
        QCOMPARE(codigoDe(resultado), QStringLiteral("valor.formato"));
    }
}

void TstValor::zeroAEsquerdaRejeitada()
{
    // O zero à esquerda não existe no leiaute (TSDec15V2 começa em [1-9] ou é "0" sozinho).
    const auto zeroComCentavos = Valor::deTextoPtBr(u"0,50");
    QVERIFY2(zeroComCentavos.has_value(), qPrintable(codigoDe(zeroComCentavos)));
    QCOMPARE(zeroComCentavos->centavos(), std::int64_t{50});

    for (const char16_t* texto : {u"01,00", u"00", u"007"}) {
        const auto resultado = Valor::deTextoPtBr(QStringView{texto});
        QCOMPARE(codigoDe(resultado), QStringLiteral("valor.formato"));
    }
}

void TstValor::tetoAceitoEProximoRejeitado()
{
    // Leiaute: [1-9][0-9]{0,14} com duas casas, ou seja 15 dígitos inteiros. Maior: 999999999999999,99.
    const auto maior = Valor::deTextoPtBr(u"999.999.999.999.999,99");
    QVERIFY2(maior.has_value(), qPrintable(codigoDe(maior)));
    QCOMPARE(maior->centavos(), Valor::teto);
    QCOMPARE(maior->paraLeiaute(), QStringLiteral("999999999999999.99"));
    QVERIFY(aceitoPeloLeiaute(maior->paraLeiaute()));

    // O seguinte, 1000000000000000,00, tem 16 dígitos inteiros e não cabe.
    QCOMPARE(codigoDe(Valor::deTextoPtBr(u"1.000.000.000.000.000,00")), QStringLiteral("valor.limite"));
    QCOMPARE(codigoDe(Valor::deTextoPtBr(u"1000000000000000")), QStringLiteral("valor.limite"));
    // Confere com o próprio pattern do XSD: o texto do seguinte não casa.
    QVERIFY(!aceitoPeloLeiaute(QStringLiteral("1000000000000000.00")));

    // Muitos dígitos: a conta não pode estourar int64 antes de rejeitar.
    QCOMPARE(codigoDe(Valor::deTextoPtBr(u"9999999999999999999999999,99")), QStringLiteral("valor.limite"));
}

void TstValor::deCentavosForaDoIntervaloRejeitado()
{
    QCOMPARE(codigoDe(Valor::de(std::numeric_limits<std::int64_t>::max())), QStringLiteral("valor.limite"));
    QCOMPARE(codigoDe(Valor::de(Valor::teto + 1)), QStringLiteral("valor.limite"));

    const auto zero = Valor::de(0);
    QVERIFY2(zero.has_value(), qPrintable(codigoDe(zero)));
    QCOMPARE(zero->centavos(), std::int64_t{0});

    const auto teto = Valor::de(Valor::teto);
    QVERIFY2(teto.has_value(), qPrintable(codigoDe(teto)));
    QCOMPARE(teto->centavos(), Valor::teto);
}

void TstValor::somaQueUltrapassaTetoRejeitada()
{
    // 200 x teto passa de int64 se não houver checagem: tem de virar erro, nunca valor enrolado.
    const auto maior = Valor::deTextoPtBr(u"999.999.999.999.999,99");
    const auto umCentavo = Valor::deTextoPtBr(u"0,01");
    QVERIFY(maior.has_value());
    QVERIFY(umCentavo.has_value());

    const std::vector<Valor> estouraSeNaoChecar(200, *maior);
    QCOMPARE(codigoDe(Valor::somaDeLote(estouraSeNaoChecar)), QStringLiteral("valor.limite"));

    const std::vector<Valor> passaDoTeto{*maior, *umCentavo};
    QCOMPARE(codigoDe(Valor::somaDeLote(passaDoTeto)), QStringLiteral("valor.limite"));
}

void TstValor::somaDeLoteVazioEhZero()
{
    const std::vector<Valor> vazio;
    const auto zero = Valor::somaDeLote(vazio);
    QVERIFY2(zero.has_value(), qPrintable(codigoDe(zero)));
    QCOMPARE(zero->centavos(), std::int64_t{0});
}

void TstValor::paraLeiauteComPontoEDuasCasas()
{
    struct Caso
    {
        const char16_t* entrada;
        const char* esperado;
    };
    const Caso casos[] = {
        {u"1.234,56", "1234.56"},
        {u"0", "0.00"},
        {u"10", "10.00"},
        {u"0,05", "0.05"},
        {u"10,5", "10.50"},
    };
    for (const Caso& caso : casos) {
        const auto valor = Valor::deTextoPtBr(QStringView{caso.entrada});
        QVERIFY2(valor.has_value(), qPrintable(codigoDe(valor)));
        QCOMPARE(valor->paraLeiaute(), QString::fromUtf8(caso.esperado));
    }
}

void TstValor::paraLeiauteAceitoPeloLeiauteDoXsd()
{
    for (const char16_t* entrada : {u"0", u"0,05", u"1.234,56", u"10,5", u"999.999.999.999.999,99"}) {
        const auto valor = Valor::deTextoPtBr(QStringView{entrada});
        QVERIFY2(valor.has_value(), qPrintable(codigoDe(valor)));
        QVERIFY2(aceitoPeloLeiaute(valor->paraLeiaute()), qPrintable(valor->paraLeiaute()));
    }
}

QTEST_GUILESS_MAIN(TstValor)
#include "tst_valor.moc"
