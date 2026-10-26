// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários da chave de acesso mínima da NFS-e (NL-1.3). Regra pura, sem I/O.
// Formato: TSIdNFSe sem o literal "NFS" (tiposSimples_v1.01.xsd:41, NFS[0-9]{9}[0-9A-Z]{14}[0-9]{27}): 50 posições.

#include <nl/back/dps/chave_acesso.h>

#include <QtTest>

using nl::back::ChaveAcesso;

namespace {

QString codigo(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstChaveAcesso : public QObject
{
    Q_OBJECT

private slots:
    void chaveComCinquentaPosicoesAceita();
    void chaveComPrefixoNfsRejeitada();
    void chaveComUmaPosicaoAMaisRejeitada();
    void chaveComUmaPosicaoAMenosRejeitada();
    void chaveComLetraNaParteDeDigitosRejeitada();
};

void TstChaveAcesso::chaveComCinquentaPosicoesAceita()
{
    // Nove dígitos, catorze alfanuméricos (inscrição com CNPJ alfanumérico 12ABC34501DE35), vinte e sete dígitos.
    const auto chave = ChaveAcesso::de(u"123456789" u"12ABC34501DE35" u"123456789012345678901234567");
    QVERIFY(chave.has_value());
    QCOMPARE(chave->texto().size(), qsizetype{50});
    QCOMPARE(chave->texto(), QStringLiteral("12345678912ABC34501DE35123456789012345678901234567"));
}

void TstChaveAcesso::chaveComPrefixoNfsRejeitada()
{
    // O literal "NFS" é do identificador da NFS-e, não da chave: a forma de 50 posições não o tem (xsd:41).
    // Entrada com 50 posições (tamanho certo), mas com NFS nas três primeiras: a regra posicional tem de recusar (chave.formato).
    const auto chave = ChaveAcesso::de(u"NFS" u"12345678" u"12ABC34501DE35" u"1234567890123456789012345");
    QCOMPARE(QStringView{u"NFS" u"12345678" u"12ABC34501DE35" u"1234567890123456789012345"}.size(), qsizetype{50});
    QVERIFY(!chave.has_value());
    QCOMPARE(codigo(chave.error()), QStringLiteral("chave.formato"));
}

void TstChaveAcesso::chaveComUmaPosicaoAMaisRejeitada()
{
    const auto chave = ChaveAcesso::de(u"123456789" u"12ABC34501DE35" u"1234567890123456789012345678");
    QVERIFY(!chave.has_value());
    QCOMPARE(codigo(chave.error()), QStringLiteral("chave.tamanho"));
}

void TstChaveAcesso::chaveComUmaPosicaoAMenosRejeitada()
{
    const auto chave = ChaveAcesso::de(u"123456789" u"12ABC34501DE35" u"12345678901234567890123456");
    QVERIFY(!chave.has_value());
    QCOMPARE(codigo(chave.error()), QStringLiteral("chave.tamanho"));
}

void TstChaveAcesso::chaveComLetraNaParteDeDigitosRejeitada()
{
    // A letra A na posição 0 cai na faixa de nove dígitos [0-9]{9}, que não aceita letra.
    const auto chave = ChaveAcesso::de(u"A23456789" u"12ABC34501DE35" u"123456789012345678901234567");
    QVERIFY(!chave.has_value());
    QCOMPARE(codigo(chave.error()), QStringLiteral("chave.formato"));
}

QTEST_GUILESS_MAIN(TstChaveAcesso)
#include "tst_chave_acesso.moc"
