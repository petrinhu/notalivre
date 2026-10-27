// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do CNPJ (NL-1.1, DA-1.1). Regra pura, sem I/O. A fonte de cada vetor oficial fica no comentário do teste.

#include <nl/back/identificacao/cnpj.h>

#include <array>
#include <cstddef>
#include <string_view>

#include <QtTest>

namespace {

QStringView comoVista(std::u16string_view texto)
{
    return QStringView{texto.data(), static_cast<qsizetype>(texto.size())};
}

} // namespace

using nl::back::Cnpj;

class TstCnpj : public QObject
{
    Q_OBJECT

private slots:
    void cnpjNumericoValidoAceito();
    void cnpjAlfanumericoValidoAceito();
    void cnpjAlfanumericoRestoUmDaDvZero();
    void cnpjLetraNaOrdemAceito();
    void cnpjComMascaraNormalizado();
    void cnpjMinusculoNormalizadoParaMaiusculo();
    void normalizarRemoveSoSeparadoresPermitidos();
    void cnpjCaractereForaDoAlfabetoRejeitado();
    void cnpjCurtoNaoCompletaZero();
    void cnpjMaisDeQuatorzeRejeitado();
    void cnpjLetraNoDvRejeitada();
    void cnpjDvErradoRejeitado();
    void cnpjExibidoComMascaraOficial();
    void normalizacaoEhIdempotente();
    void normalizacaoExaustivaEhIdempotente();
    void cnpjTodoZeroRejeitado();
    void cnpjRaizZeroOutraOrdemAceita();
};

void TstCnpj::cnpjNumericoValidoAceito()
{
    // Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.17:
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    const auto cnpj = Cnpj::de(u"12345678000195");
    QVERIFY(cnpj.has_value());
    QCOMPARE(cnpj->texto(), QStringLiteral("12345678000195"));
}

void TstCnpj::cnpjAlfanumericoValidoAceito()
{
    // Serpro, Cálculo dos dígitos verificadores de CNPJ alfanumérico, 1.1 e 1.2:
    // https://serpro.gov.br/menu/noticias/videos/calculodvcnpjalfanaumerico.pdf
    // Mesmo exemplo na Receita Federal, Perguntas e Respostas, Q.14:
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    const auto cnpj = Cnpj::de(u"12ABC34501DE35");
    QVERIFY(cnpj.has_value());
    QCOMPARE(cnpj->texto(), QStringLiteral("12ABC34501DE35"));
}

void TstCnpj::cnpjAlfanumericoRestoUmDaDvZero()
{
    // Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.23 Ex3 (sem máscara; a máscara oficial é a da Q.21):
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    // Único vetor oficial em que o resto da primeira soma é 1: o primeiro DV tem de sair 0.
    const auto cnpj = Cnpj::de(u"12345678000A08");
    QVERIFY(cnpj.has_value());
    QCOMPARE(cnpj->texto(), QStringLiteral("12345678000A08"));
}

void TstCnpj::cnpjLetraNaOrdemAceito()
{
    // Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.23 Ex2:
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    const auto cnpj = Cnpj::de(u"AA345678000A29");
    QVERIFY(cnpj.has_value());
    QCOMPARE(cnpj->texto(), QStringLiteral("AA345678000A29"));
}

void TstCnpj::cnpjComMascaraNormalizado()
{
    // Máscara oficial (Q.21) entra na entrada e sai sem separadores (DA-1.1).
    // Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.21:
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    const auto cnpj = Cnpj::de(u"12.ABC.345/01DE-35");
    QVERIFY(cnpj.has_value());
    QCOMPARE(cnpj->texto(), QStringLiteral("12ABC34501DE35"));
}

void TstCnpj::cnpjMinusculoNormalizadoParaMaiusculo()
{
    // DA-1.1: a-z vira A-Z antes do DV. Q.2 define as letras como maiúsculas; a minúscula é a mesma letra.
    // Caso do risco medido em onda1_cnpj_alfanumerico.md, a3: sem normalizar, o DV sai errado em silêncio.
    // Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.2 (letras maiúsculas de A a Z):
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    const auto cnpj = Cnpj::de(u"12abc34501de35");
    QVERIFY(cnpj.has_value());
    QCOMPARE(cnpj->texto(), QStringLiteral("12ABC34501DE35"));
}

void TstCnpj::normalizarRemoveSoSeparadoresPermitidos()
{
    // DA-1.1: remove só ponto, barra, hífen e espaço (U+0020). Tabulação e sublinhado ficam e são erro de formato depois.
    QCOMPARE(Cnpj::normalizar(u"12.ABC/345-01 DE35"), QStringLiteral("12ABC34501DE35"));
    QCOMPARE(Cnpj::normalizar(u"a\tb"), QStringLiteral("A\tB"));
    QCOMPARE(Cnpj::normalizar(u"x_y"), QStringLiteral("X_Y"));
}

void TstCnpj::cnpjCaractereForaDoAlfabetoRejeitado()
{
    // DA-1.1: qualquer outro caractere é erro de formato. Os casos têm 14 caracteres, para o erro ser de alfabeto e não de tamanho.
    // Letra acentuada (U+00C9), dígito de largura total (U+FF11), tabulação, sublinhado e i sem ponto (U+0131).
    // O i sem ponto é o caso que QString::toUpper converteria em I ASCII: a normalização tem de ser só ASCII.
    const std::array<std::u16string_view, 5> casos{
        u"12ABC34501DÉ35",
        u"12ABC34501D１35",
        u"12ABC34501D\t35",
        u"12ABC34501D_35",
        u"12ABC34501ıE35",
    };
    for (const auto caso : casos) {
        const auto cnpj = Cnpj::de(comoVista(caso));
        QVERIFY(!cnpj.has_value());
        QCOMPARE(QString::fromStdString(cnpj.error().codigo()), QStringLiteral("cnpj.formato"));
    }
}

void TstCnpj::cnpjCurtoNaoCompletaZero()
{
    // DA-1.1: menos de 14 caracteres é erro de tamanho. O tipo não completa zero à esquerda: nada é emitido com dado adivinhado.
    const std::array<std::u16string_view, 3> casos{u"1234567800019", u"12ABC34501DE", u""};
    for (const auto caso : casos) {
        const auto cnpj = Cnpj::de(comoVista(caso));
        QVERIFY(!cnpj.has_value());
        QCOMPARE(QString::fromStdString(cnpj.error().codigo()), QStringLiteral("cnpj.tamanho"));
    }
}

void TstCnpj::cnpjMaisDeQuatorzeRejeitado()
{
    // DA-1.1: 15 caracteres válidos no alfabeto dão erro de tamanho.
    const auto cnpj = Cnpj::de(u"12ABC34501DE350");
    QVERIFY(!cnpj.has_value());
    QCOMPARE(QString::fromStdString(cnpj.error().codigo()), QStringLiteral("cnpj.tamanho"));
}

void TstCnpj::cnpjLetraNoDvRejeitada()
{
    // DA-1.1: posições 13 e 14 são numéricas. Letra nelas é erro de formato, não de DV.
    const std::array<std::u16string_view, 2> casos{u"12ABC34501DEA5", u"12ABC34501DE3A"};
    for (const auto caso : casos) {
        const auto cnpj = Cnpj::de(comoVista(caso));
        QVERIFY(!cnpj.has_value());
        QCOMPARE(QString::fromStdString(cnpj.error().codigo()), QStringLiteral("cnpj.formato"));
    }
}

void TstCnpj::cnpjDvErradoRejeitado()
{
    // DV errado com formato válido: "cnpj.dv". O alfanumérico (DV 36 em vez de 35) e o numérico (196 em vez de 195).
    const std::array<std::u16string_view, 2> casos{u"12ABC34501DE36", u"12345678000196"};
    for (const auto caso : casos) {
        const auto cnpj = Cnpj::de(comoVista(caso));
        QVERIFY(!cnpj.has_value());
        QCOMPARE(QString::fromStdString(cnpj.error().codigo()), QStringLiteral("cnpj.dv"));
    }
}

void TstCnpj::cnpjExibidoComMascaraOficial()
{
    // Q.21: máscara XX.XXX.XXX/XXXX-DD, com as letras em maiúsculas.
    // Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.21 e Q.2:
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    const auto alfanumerico = Cnpj::de(u"12ABC34501DE35");
    QVERIFY(alfanumerico.has_value());
    QCOMPARE(alfanumerico->comMascara(), QStringLiteral("12.ABC.345/01DE-35"));
    QCOMPARE(alfanumerico->raiz(), QStringLiteral("12ABC345"));

    // Q.23 Ex3, máscara da Q.21.
    const auto comResto = Cnpj::de(u"12345678000A08");
    QVERIFY(comResto.has_value());
    QCOMPARE(comResto->comMascara(), QStringLiteral("12.345.678/000A-08"));
}

void TstCnpj::normalizacaoEhIdempotente()
{
    // Propriedade (plano, NL-1.1): normalizar(normalizar(x)) == normalizar(x).
    // Entradas fixas e enumeradas (sem sorteio): máscara, caixa e separadores em combinações. Todas são CNPJs válidos.
    struct Caso
    {
        std::u16string_view entrada;
        std::u16string_view esperado;
    };
    const std::array<Caso, 8> casos{
        Caso{u"12ABC34501DE35", u"12ABC34501DE35"},
        Caso{u"12.ABC.345/01DE-35", u"12ABC34501DE35"},
        Caso{u"12abc34501de35", u"12ABC34501DE35"},
        Caso{u"12.abc.345/01de-35", u"12ABC34501DE35"},
        Caso{u"12345678000195", u"12345678000195"},
        Caso{u"12.345.678/0001-95", u"12345678000195"},
        Caso{u"AA345678000A29", u"AA345678000A29"},
        Caso{u"aa.345.678/000a-29", u"AA345678000A29"},
    };
    for (const auto caso : casos) {
        const QString primeiro = Cnpj::normalizar(comoVista(caso.entrada));
        QCOMPARE(primeiro, QString::fromUtf16(caso.esperado.data(), static_cast<qsizetype>(caso.esperado.size())));

        const QString segundo = Cnpj::normalizar(primeiro);
        QCOMPARE(segundo, primeiro);

        const auto cnpj = Cnpj::de(comoVista(caso.entrada));
        QVERIFY(cnpj.has_value());
        QCOMPARE(cnpj->texto(), primeiro);
    }
}

void TstCnpj::normalizacaoExaustivaEhIdempotente()
{
    // Enumeração exaustiva, sem sorteio: todas as strings de 0 a 3 caracteres sobre um alfabeto de 14 símbolos.
    // Símbolos: dígitos 0 e 9, letras A, Z, a, z, ponto, barra, hífen, espaço, tabulação, É (U+00C9),
    // é (U+00E9) e dígito de largura total (U+FF11). Total: 1 + 14 + 14^2 + 14^3 = 2955 entradas.
    const std::array<char16_t, 14> alfabeto{u'0', u'9', u'A', u'Z', u'a', u'z', u'.', u'/',
                                            u'-', u' ', u'\t', u'É', u'é', u'１'};
    constexpr qsizetype base = 14;
    constexpr int tamanhoMaximo = 3;

    qsizetype total = 0;
    for (int tamanho = 0; tamanho <= tamanhoMaximo; ++tamanho) {
        qsizetype combinacoes = 1;
        for (int posicao = 0; posicao < tamanho; ++posicao) {
            combinacoes *= base;
        }
        for (qsizetype indice = 0; indice < combinacoes; ++indice) {
            QString entrada;
            qsizetype resto = indice;
            for (int posicao = 0; posicao < tamanho; ++posicao) {
                entrada.append(QChar{alfabeto[static_cast<std::size_t>(resto % base)]});
                resto /= base;
            }
            const QString primeiro = Cnpj::normalizar(entrada);
            const QString segundo = Cnpj::normalizar(primeiro);
            QCOMPARE(segundo, primeiro);
            ++total;
        }
    }
    QCOMPARE(total, qsizetype{2955});
}

void TstCnpj::cnpjTodoZeroRejeitado()
{
    // DA-1.9 (decisão do CTO): 00000000000000 é rejeitado com o código "cnpj.zerado". Só esse valor.
    // O DV dele confere pelo módulo 11 (soma zero), então a rejeição precisa ser explícita.
    const auto cnpj = Cnpj::de(u"00000000000000");
    QVERIFY(!cnpj.has_value());
    QCOMPARE(QString::fromStdString(cnpj.error().codigo()), QStringLiteral("cnpj.zerado"));
}

void TstCnpj::cnpjRaizZeroOutraOrdemAceita()
{
    // DA-1.9: a raiz 00000000 com outra ordem continua válida. 00.000.000/0001-91 existe (informado pelo CTO).
    // DV conferido pela regra do módulo 11 (Receita Federal, Perguntas e Respostas do CNPJ alfanumérico, Q.14):
    // https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf
    // Não é vetor publicado na Q&A: a conferência foi feita pela regra, não por fonte citada.
    const auto cnpj = Cnpj::de(u"00000000000191");
    QVERIFY(cnpj.has_value());
    QCOMPARE(cnpj->texto(), QStringLiteral("00000000000191"));
}

QTEST_GUILESS_MAIN(TstCnpj)
#include "tst_cnpj.moc"
