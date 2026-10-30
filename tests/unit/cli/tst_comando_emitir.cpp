// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes das travas de `notalivre emitir` antes da senha (NL-2.9, DA-2.5, spec 4.12): Produção bloqueada e CNPJ digitado.
// Estes casos recusam antes de ler a senha, então não dependem de terminal.
#include "comando_emitir.h"

#include <QtTest>

class TstComandoEmitir : public QObject
{
    Q_OBJECT

private slots:
    // Produção sem liberação por configuração sai com rc 1, antes de qualquer leitura de senha.
    void producaoBloqueadaSaiAntesDaSenha();
    // Produção exige o CNPJ digitado como confirmação.
    void producaoSemCnpjDigitadoSaiAntesDaSenha();
};

void TstComandoEmitir::producaoBloqueadaSaiAntesDaSenha()
{
    const QStringList argumentos{"emitir", "dps.json", "--serie", "1", "--numero", "1", "--ambiente", "producao",
                                 "--cnpj-confirmado", "12345678000195"};
    QCOMPARE(nl::cli::executarEmitir(argumentos), 1);
}

void TstComandoEmitir::producaoSemCnpjDigitadoSaiAntesDaSenha()
{
    const QStringList argumentos{"emitir", "dps.json", "--serie", "1", "--numero", "1", "--ambiente", "producao"};
    QCOMPARE(nl::cli::executarEmitir(argumentos), 1);
}

QTEST_APPLESS_MAIN(TstComandoEmitir)
#include "tst_comando_emitir.moc"
