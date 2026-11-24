// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do alfabeto do leiaute nos textos que derivam de TSString (NL-3.B1, parte 1 da NL-I.351): pattern
// [!-ÿ]{1}[ -ÿ]{0,}[!-ÿ]{1}|[!-ÿ]{1} (tiposSimples_v1.01.xsd:13-20). Os caracteres entram por número de ponto de código,
// para que o teste não dependa da codificação do arquivo-fonte.

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/endereco.h>
#include <nl/back/dps/endereco_exterior.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/substituicao.h>
#include <nl/back/dps/texto_limitado.h>

#include <QtTest>

#include "ajudantes.h"

namespace nb = nl::back;

namespace {

/// Campo de teste com o alfabeto do TSString e a faixa de 1 a 10.
using CampoLeiaute = nb::TextoLimitado<1, 10, nb::Alfabeto::tsString>;

/// Texto de um ponto de código só, dado pelo número (fora do BMP vira par substituto no QString).
QString ponto(char32_t codigo)
{
    return QString::fromUcs4(&codigo, 1);
}

/// "a", o ponto de código do meio e "b": confere o caractere no meio, longe das pontas.
QString noMeio(char32_t codigo)
{
    return QStringLiteral("a") + ponto(codigo) + QStringLiteral("b");
}

} // namespace

class TstTextoLeiaute : public QObject
{
    Q_OBJECT

private slots:
    void limitesDoAlfabetoAceitos();
    void pontasDeU0021AU00FFAceitas();
    void caractereAbaixoDeU0020Recusado();
    void caractereAcimaDeU00FFRecusado();
    void parSubstitutoRecusadoEContadoComoUm();
    void espacoNoComecoOuNoFimRecusado();
    void soEspacoRecusado();
    void vazioENuloRecusadosPeloTamanho();
    void erroNomeiaOCampo();
    void textoAceitoFicaComoVeio();
    void seteTiposDoTSStringConferemOAlfabeto();
    void tiposSemPatternAceitamQualquerCaractere();
};

void TstTextoLeiaute::limitesDoAlfabetoAceitos()
{
    // No meio vale de U+0020 a U+00FF: espaço, "!", "~", DEL, NBSP, "Á" e "ÿ".
    for (const char32_t codigo : {char32_t{0x20}, char32_t{0x21}, char32_t{0x7E}, char32_t{0x7F}, char32_t{0xA0},
                                  char32_t{0xC1}, char32_t{0xFF}}) {
        QVERIFY2(CampoLeiaute::de(noMeio(codigo), "xLgr").has_value(), qPrintable(QString::number(codigo, 16)));
    }
}

void TstTextoLeiaute::pontasDeU0021AU00FFAceitas()
{
    // Um caractere só, e as duas pontas: "!" (U+0021), NBSP (U+00A0, não é o espaço U+0020) e "ÿ" (U+00FF).
    QVERIFY(CampoLeiaute::de(ponto(0x21), "xLgr").has_value());
    QVERIFY(CampoLeiaute::de(ponto(0xFF), "xLgr").has_value());
    QVERIFY(CampoLeiaute::de(ponto(0xA0) + QStringLiteral("a") + ponto(0xA0), "xLgr").has_value());
    QVERIFY(CampoLeiaute::de(QStringLiteral("a b"), "xLgr").has_value());
}

void TstTextoLeiaute::caractereAbaixoDeU0020Recusado()
{
    // U+001F, tabulação, quebra de linha e NUL, no meio e numa ponta.
    for (const char32_t codigo : {char32_t{0x1F}, char32_t{0x09}, char32_t{0x0A}, char32_t{0x00}}) {
        QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(noMeio(codigo), "xLgr")), QStringLiteral("texto.caractere"));
        QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral("a") + ponto(codigo), "xLgr")),
                 QStringLiteral("texto.caractere"));
    }
}

void TstTextoLeiaute::caractereAcimaDeU00FFRecusado()
{
    // U+0100 (o primeiro fora), apóstrofo curvo U+2019 do teclado do celular, travessão U+2014 e reticências U+2026.
    for (const char32_t codigo : {char32_t{0x100}, char32_t{0x2019}, char32_t{0x2014}, char32_t{0x2026}}) {
        QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(noMeio(codigo), "xLgr")), QStringLiteral("texto.caractere"));
    }
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral("D") + ponto(0x2019) + QStringLiteral("Avila"), "xLgr")),
             QStringLiteral("texto.caractere"));
}

void TstTextoLeiaute::parSubstitutoRecusadoEContadoComoUm()
{
    // Emoji U+1F600: dois QChar no QString, um ponto de código no tamanho, fora do alfabeto.
    const QString emoji = ponto(0x1F600);
    QCOMPARE(emoji.size(), qsizetype{2});
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(emoji, "xLgr")), QStringLiteral("texto.caractere"));
    // Nove "a" e o emoji: 10 pontos de código cabem na faixa, e o erro é o do caractere, não o do tamanho.
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QString(9, QChar{'a'}) + emoji, "xLgr")),
             QStringLiteral("texto.caractere"));
}

void TstTextoLeiaute::espacoNoComecoOuNoFimRecusado()
{
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral(" Centro"), "xLgr")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral("Centro "), "xLgr")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral(" a "), "xLgr")), QStringLiteral("texto.espaco_na_ponta"));
}

void TstTextoLeiaute::soEspacoRecusado()
{
    // A alternativa de um caractere só do pattern é [!-ÿ]: o espaço sozinho não passa.
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral(" "), "xLgr")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral("   "), "xLgr")), QStringLiteral("texto.espaco_na_ponta"));
}

void TstTextoLeiaute::vazioENuloRecusadosPeloTamanho()
{
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QString{}, "xLgr")), QStringLiteral("texto.tamanho"));
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QStringLiteral(""), "xLgr")), QStringLiteral("texto.tamanho"));
    QCOMPARE(teste::codigoDoErro(CampoLeiaute::de(QString(11, QChar{'a'}), "xLgr")), QStringLiteral("texto.tamanho"));
    QVERIFY(CampoLeiaute::de(QString(10, QChar{'a'}), "xLgr").has_value());
}

void TstTextoLeiaute::erroNomeiaOCampo()
{
    const auto caractere = CampoLeiaute::de(noMeio(0x2019), "toma/end/xLgr");
    QVERIFY(!caractere.has_value());
    QCOMPARE(caractere.error().campo().value_or(std::string{}), std::string{"toma/end/xLgr"});
    const auto ponta = CampoLeiaute::de(QStringLiteral("Centro "), "toma/end/xBairro");
    QVERIFY(!ponta.has_value());
    QCOMPARE(ponta.error().campo().value_or(std::string{}), std::string{"toma/end/xBairro"});
}

void TstTextoLeiaute::textoAceitoFicaComoVeio()
{
    // Nada é trocado nem aparado: o NBSP das pontas e o "ã" ficam.
    const QString digitado = ponto(0xA0) + QStringLiteral("S") + ponto(0xE3) + QStringLiteral("o Jo") + ponto(0xE3) +
                             QStringLiteral("o");
    const auto campo = CampoLeiaute::de(digitado, "xLgr");
    QVERIFY(campo.has_value());
    QCOMPARE(campo->texto(), digitado);
}

void TstTextoLeiaute::seteTiposDoTSStringConferemOAlfabeto()
{
    // Os 7 aliases do domínio que derivam de TSString no XSD: xLgr, nro, xCpl, xBairro, xMotivo, email e verAplic.
    const QString motivo = QStringLiteral("Erro de digitacao");
    const QString espaco = QStringLiteral(" ");
    const QString curvo = ponto(0x2019);
    QCOMPARE(teste::codigoDoErro(nb::Logradouro::de(QStringLiteral("Rua ") + curvo, "xLgr")), QStringLiteral("texto.caractere"));
    QCOMPARE(teste::codigoDoErro(nb::Logradouro::de(QStringLiteral("Rua A") + espaco, "xLgr")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(nb::NumeroEndereco::de(QStringLiteral("10") + curvo, "nro")), QStringLiteral("texto.caractere"));
    QCOMPARE(teste::codigoDoErro(nb::NumeroEndereco::de(espaco + QStringLiteral("10"), "nro")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(nb::ComplementoEndereco::de(QStringLiteral("Sala") + curvo, "xCpl")), QStringLiteral("texto.caractere"));
    QCOMPARE(teste::codigoDoErro(nb::ComplementoEndereco::de(QStringLiteral("Sala 2") + espaco, "xCpl")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(nb::Bairro::de(QStringLiteral("Centro") + curvo, "xBairro")), QStringLiteral("texto.caractere"));
    QCOMPARE(teste::codigoDoErro(nb::Bairro::de(QStringLiteral("Centro") + espaco, "xBairro")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(nb::MotivoSubstituicao::de(motivo + curvo, "xMotivo")), QStringLiteral("texto.caractere"));
    QCOMPARE(teste::codigoDoErro(nb::MotivoSubstituicao::de(motivo + espaco, "xMotivo")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(nb::EmailPessoa::de(QStringLiteral("a@b.com") + curvo, "email")), QStringLiteral("texto.caractere"));
    QCOMPARE(teste::codigoDoErro(nb::EmailPessoa::de(QStringLiteral("a@b.com") + espaco, "email")), QStringLiteral("texto.espaco_na_ponta"));
    QCOMPARE(teste::codigoDoErro(nb::VerAplic::de(QStringLiteral("NotaLivre") + curvo, "verAplic")), QStringLiteral("texto.caractere"));
    QCOMPARE(teste::codigoDoErro(nb::VerAplic::de(QStringLiteral("NotaLivre 0.1") + espaco, "verAplic")), QStringLiteral("texto.espaco_na_ponta"));
}

void TstTextoLeiaute::tiposSemPatternAceitamQualquerCaractere()
{
    // xs:string sem pattern no XSD (xNome, IM, NIF, xCidade, xEstProvReg e cEndPost): o domínio não recusa o que o leiaute
    // aceita. Apóstrofo curvo e espaço no fim passam; só o tamanho conta.
    const QString texto = QStringLiteral("D") + ponto(0x2019) + QStringLiteral("Avila ");
    QVERIFY(nb::NomeRazaoSocial::de(texto, "xNome").has_value());
    QVERIFY(nb::InscricaoMunicipal::de(texto, "IM").has_value());
    QVERIFY(nb::Nif::de(texto, "NIF").has_value());
    QVERIFY(nb::Cidade::de(texto, "xCidade").has_value());
    QVERIFY(nb::EstadoProvRegiao::de(texto, "xEstProvReg").has_value());
    QVERIFY(nb::CodigoEndPostal::de(QStringLiteral("A") + ponto(0x2019) + QStringLiteral(" "), "cEndPost").has_value());
}

QTEST_GUILESS_MAIN(TstTextoLeiaute)
#include "tst_texto_leiaute.moc"
