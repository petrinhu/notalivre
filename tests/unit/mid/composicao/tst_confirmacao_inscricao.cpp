// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da confirmação forte da inscrição (NL-2.9, item 2 da triagem do CISO, DA-2.14). Função pura, sem arquivo nem rede.
#include <nl/foundation/pkcs12/inscricao.h>
#include <nl/mid/composicao/confirmacao_inscricao.h>

#include <QString>
#include <QtTest>

namespace {

// CNPJ sintético de teste (DV válido), e outro CNPJ sintético de teste, usados como A1 e emitente nas duas recusas.
const QString kCnpjA1 = QStringLiteral("11222333000181");
const QString kCnpjOutro = QStringLiteral("11444777000161");

nl::foundation::pkcs12::Inscricao inscricaoA1()
{
    return *nl::foundation::pkcs12::Inscricao::cnpj(kCnpjA1);
}

} // namespace

class TstConfirmacaoInscricao : public QObject
{
    Q_OBJECT

private slots:
    // Digitado com pontuação, igual ao A1 e ao emitente, passa depois da normalização do Back.
    void confirmacaoIgualNormalizadaAceita();
    // Digitado diferente do A1 é recusado antes de qualquer assinatura.
    void confirmacaoDiferenteDoA1Recusada();
    // Digitado igual ao A1, mas diferente do emitente da DPS, é recusado.
    void confirmacaoDiferenteDoEmitenteRecusada();
};

void TstConfirmacaoInscricao::confirmacaoIgualNormalizadaAceita()
{
    const auto aceita = nl::mid::composicao::conferirConfirmacao(QStringLiteral("11.222.333/0001-81"), inscricaoA1(), kCnpjA1);
    QVERIFY2(aceita.has_value(), "a confirmação igual, com pontuação, tem de passar depois da normalização");
}

void TstConfirmacaoInscricao::confirmacaoDiferenteDoA1Recusada()
{
    const auto recusa = nl::mid::composicao::conferirConfirmacao(kCnpjOutro, inscricaoA1(), kCnpjA1);
    QVERIFY2(!recusa.has_value(), "confirmação diferente do A1 tem de ser recusada");
    QCOMPARE(QString::fromStdString(recusa.error().codigo()), QStringLiteral("confirmacao.diferente_do_a1"));
}

void TstConfirmacaoInscricao::confirmacaoDiferenteDoEmitenteRecusada()
{
    const auto recusa = nl::mid::composicao::conferirConfirmacao(kCnpjA1, inscricaoA1(), kCnpjOutro);
    QVERIFY2(!recusa.has_value(), "confirmação diferente do emitente da DPS tem de ser recusada");
    QCOMPARE(QString::fromStdString(recusa.error().codigo()), QStringLiteral("confirmacao.diferente_do_emitente"));
}

QTEST_APPLESS_MAIN(TstConfirmacaoInscricao)
#include "tst_confirmacao_inscricao.moc"
