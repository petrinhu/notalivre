// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes das travas de `notalivre emitir` antes da senha (NL-2.9, DA-2.5, spec 4.12): Produção bloqueada e CNPJ digitado.
// Estes casos recusam antes de ler a senha, então não dependem de terminal. Cada teste aponta XDG_CONFIG_HOME para uma
// configuração temporária com Produção falsa (padrão do app).
#include "comando_emitir.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Configuração temporária com Produção falsa, apontada por XDG_CONFIG_HOME. A pasta fica viva enquanto o objeto existir.
class ConfigTemporaria
{
public:
    ConfigTemporaria()
    {
        QDir().mkpath(pasta_.filePath(QStringLiteral("notalivre")));
        QFile arquivo(pasta_.filePath(QStringLiteral("notalivre/config.ini")));
        static_cast<void>(arquivo.open(QIODevice::WriteOnly | QIODevice::Text));
        arquivo.write("[sefin]\nurl = https://127.0.0.1:9443\nancoras = /tmp/ac.pem\n"
                      "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
                      "[emissao]\nproducao_liberada = false\n");
        arquivo.close();
        qputenv("XDG_CONFIG_HOME", pasta_.path().toLocal8Bit());
    }

private:
    QTemporaryDir pasta_;
};

} // namespace

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
    ConfigTemporaria config;
    const QStringList argumentos{"emitir", "dps.json", "--serie", "1", "--numero", "1", "--ambiente", "producao",
                                 "--cnpj-confirmado", "12345678000195"};
    QCOMPARE(nl::cli::executarEmitir(argumentos), 1);
}

void TstComandoEmitir::producaoSemCnpjDigitadoSaiAntesDaSenha()
{
    ConfigTemporaria config;
    const QStringList argumentos{"emitir", "dps.json", "--serie", "1", "--numero", "1", "--ambiente", "producao"};
    QCOMPARE(nl::cli::executarEmitir(argumentos), 1);
}

QTEST_APPLESS_MAIN(TstComandoEmitir)
#include "tst_comando_emitir.moc"
