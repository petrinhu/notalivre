// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de xDescServ (NL-1.4, DA-1.2): não vazio e com caractere que não seja whitespace do XSD; o limite é da versão (NL-1.6).

#include <nl/back/dps/descricao_servico.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::DescricaoServico;

class TstDescricaoServico : public QObject
{
    Q_OBJECT

private slots:
    void descricaoComTextoAceita();
    void descricaoVaziaRejeitada();
    void descricaoSoComEspacosRejeitada();
    void espacoNaoQuebravelContaComoTexto();
    void limiteDeTamanhoFicaNaVersao();
};

void TstDescricaoServico::descricaoComTextoAceita()
{
    const auto descricao = DescricaoServico::de(u"Desenvolvimento de software");
    QVERIFY(descricao.has_value());
    QCOMPARE(descricao->texto(), QStringLiteral("Desenvolvimento de software"));
}

void TstDescricaoServico::descricaoVaziaRejeitada()
{
    QCOMPARE(teste::codigoDoErro(DescricaoServico::de(u"")), QStringLiteral("servico.xDescServ.vazia"));
}

void TstDescricaoServico::descricaoSoComEspacosRejeitada()
{
    // Pattern do TSDesc2000 (tiposSimples_v1.01.xsd:1443): [\s\S]*[^\s][\s\S]*. Whitespace do XSD: espaço, tab, CR e LF.
    QCOMPARE(teste::codigoDoErro(DescricaoServico::de(u"   ")), QStringLiteral("servico.xDescServ.sem_texto"));
    QCOMPARE(teste::codigoDoErro(DescricaoServico::de(u"\t \r\n")), QStringLiteral("servico.xDescServ.sem_texto"));
}

void TstDescricaoServico::espacoNaoQuebravelContaComoTexto()
{
    // U+00A0 não é whitespace no XML Schema, então o oráculo aceita. O domínio segue o XSD, não o QChar::isSpace.
    QVERIFY(DescricaoServico::de(u" ").has_value());
}

void TstDescricaoServico::limiteDeTamanhoFicaNaVersao()
{
    // O domínio não corta em 2000: o limite do leiaute de 20/10 é aplicado pela versão do codec (NL-1.6).
    const QString longa(2500, QChar{'a'});
    QVERIFY(DescricaoServico::de(longa).has_value());
}

QTEST_GUILESS_MAIN(TstDescricaoServico)
#include "tst_descricao_servico.moc"
