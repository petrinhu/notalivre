// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da linha de comando de `notalivre emitir` (NL-2.9): senha nunca por argumento, série e número obrigatórios.
#include "argumentos_emitir.h"

#include <QString>
#include <QStringList>
#include <QtTest>

class TstArgumentosEmitir : public QObject
{
    Q_OBJECT

private slots:
    // Linha completa e correta é lida, com série e número da linha de comando.
    void linhaCompletaLida();
    // Qualquer opção com "senha" é recusada, mesmo com valor: a senha não vai por argumento.
    void senhaPorArgumentoRecusada();
    // Série ou número ausentes, não numéricos ou fora do tamanho são recusados.
    void serieENumeroObrigatoriosENumericos();
    // NL-2.10b, BAIXA-7: só dígitos ASCII 0 a 9 entram no número; dígito de outro alfabeto sai como argumento inválido.
    void digitoUnicodeNoNumeroRecusado();
};

void TstArgumentosEmitir::linhaCompletaLida()
{
    const auto lidos = nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--serie", "1", "--numero", "42",
                                                                 "--ambiente", "homologacao"});
    QVERIFY2(lidos.has_value(), "a linha completa tem de ser aceita");
    QCOMPARE(lidos->serie, QStringLiteral("1"));
    QCOMPARE(lidos->numero, QStringLiteral("42"));
    QCOMPARE(lidos->ambiente, QStringLiteral("homologacao"));
}

void TstArgumentosEmitir::senhaPorArgumentoRecusada()
{
    const auto comSenha = nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--serie", "1", "--numero", "42",
                                                                   "--ambiente", "homologacao", "--senha", "x"});
    QVERIFY2(!comSenha.has_value(), "--senha tem de ser recusado");
    QVERIFY(comSenha.error().contains(QStringLiteral("terminal")));
    QVERIFY(!nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--senhaMestra", "x"}).has_value());
}

void TstArgumentosEmitir::serieENumeroObrigatoriosENumericos()
{
    QVERIFY(!nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--numero", "42", "--ambiente", "homologacao"}).has_value());
    QVERIFY(!nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--serie", "1", "--ambiente", "homologacao"}).has_value());
    QVERIFY(!nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--serie", "A", "--numero", "42", "--ambiente", "homologacao"}).has_value());
    QVERIFY(!nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--serie", "123456", "--numero", "42", "--ambiente", "homologacao"}).has_value());
}

void TstArgumentosEmitir::digitoUnicodeNoNumeroRecusado()
{
    // U+FF14 (fullwidth 4) passa em QChar::isDigit, mas não é dígito ASCII: o número tem de ser recusado.
    const QString numeroUnicode = QStringLiteral("\uFF14\uFF12");
    QVERIFY(!nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--serie", "1", "--numero", numeroUnicode, "--ambiente", "homologacao"}).has_value());
    QVERIFY(!nl::cli::lerArgumentosEmitir(QStringList{"emitir", "dps.json", "--serie", "\u0661", "--numero", "42", "--ambiente", "homologacao"}).has_value());
}

QTEST_APPLESS_MAIN(TstArgumentosEmitir)
#include "tst_argumentos_emitir.moc"
