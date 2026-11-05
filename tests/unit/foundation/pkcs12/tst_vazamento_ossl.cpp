// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Vazamento do contexto OpenSSL no carregamento do .pfx (NL-3.0h, causa B). Processo próprio: o carregador abre o .pfx
// moderno e o legado, e nada do provider deve sobrar vivo depois. A checagem é in-process, pelo LeakSanitizer: o CTest
// não reprova por leak quando a regex de PASS decide o resultado (medido: o processo sai com 1 e o CTest marca Passed).
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtTest>

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/lsan_interface.h>
#endif

namespace {

using namespace nl::foundation::pkcs12;

QByteArray lerFixture(const QString& nome)
{
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + nome);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

SenhaSegura senhaDaFixture()
{
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return SenhaSegura::de(QByteArray());
    }
    return SenhaSegura::de(arquivo.readLine().trimmed());
}

// Quantos blocos o LeakSanitizer acha agora. Fora do ASan não há o que medir: devolve 0, e o teste vale só pelo carregar.
int vazamentosAgora()
{
#if defined(__SANITIZE_ADDRESS__)
    return __lsan_do_recoverable_leak_check();
#else
    return 0;
#endif
}

} // namespace

class TesteVazamentoOssl : public QObject {
    Q_OBJECT

private slots:
    void decifrarNaoDeixaProvedorVivo();
};

void TesteVazamentoOssl::decifrarNaoDeixaProvedorVivo()
{
    auto ancoras = Ancoras::dePem(lerFixture(QStringLiteral("ac.pem")));
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    {
        CarregadorPkcs12 carregador;
        auto senhaModerno = senhaDaFixture();
        auto moderno = carregador.carregar(lerFixture(QStringLiteral("cnpj_ok.pfx")), senhaModerno, *ancoras);
        QVERIFY2(moderno.has_value(), "o .pfx moderno tem de abrir");
        // O legado passa pela segunda tentativa, com o provider legacy no contexto próprio.
        auto senhaLegado = senhaDaFixture();
        auto legado = carregador.carregar(lerFixture(QStringLiteral("cnpj_ok_legado.pfx")), senhaLegado, *ancoras);
        QVERIFY2(legado.has_value(), "o .pfx legado tem de abrir");
    }
    QCOMPARE(vazamentosAgora(), 0);
}

QTEST_APPLESS_MAIN(TesteVazamentoOssl)
#include "tst_vazamento_ossl.moc"
