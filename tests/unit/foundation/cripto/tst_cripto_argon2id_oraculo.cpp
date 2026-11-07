// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Oráculo independente do Argon2id (NL-3.2b, ADR-0006, verificação, e ciso item 6): a derivação do libsodium tem de
// igualar a do `openssl kdf` com p=1 e sem segredo nem dado associado. Slot só com build nativo: o build Flatpak não
// traz o binário openssl (flatpak/testes_fora_do_flatpak.txt). OpenSSL 3.0 (mint22.3) não tem ARGON2ID: nesse caso o
// teste declara pulo, e a lista de pulos da distro tem de conter a linha.

#include <nl/foundation/cripto/argon2id.h>

#include <QByteArray>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <span>

namespace cr = nl::foundation::cripto;

namespace {

const QByteArray kSenhaDeTeste("vetor-argon2-nl32b");
const QByteArray kSaltFixo = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");

// Versão do OpenSSL na saída de `openssl version`, por exemplo "OpenSSL 3.5.9 29 Sep 2026".
bool opensslTemArgon2id(const QString& versao)
{
    static const QRegularExpression padrao(QStringLiteral("OpenSSL (\\d+)\\.(\\d+)"));
    const auto casamento = padrao.match(versao);
    if (!casamento.hasMatch()) {
        return false;
    }
    const int maior = casamento.captured(1).toInt();
    const int menor = casamento.captured(2).toInt();
    return maior > 3 || (maior == 3 && menor >= 2);
}

} // namespace

class TstCriptoArgon2idOraculo : public QObject
{
    Q_OBJECT

private slots:
    void argon2idOraculoOpenssl();
};

void TstCriptoArgon2idOraculo::argon2idOraculoOpenssl()
{
    QProcess versao;
    versao.start(QStringLiteral("openssl"), {QStringLiteral("version")});
    QVERIFY2(versao.waitForFinished(10000) && versao.exitCode() == 0, "o binário openssl precisa estar no contêiner");
    const QString textoVersao = QString::fromUtf8(versao.readAllStandardOutput()).trimmed();

    QProcess kdf;
    kdf.start(QStringLiteral("openssl"),
              {QStringLiteral("kdf"), QStringLiteral("-keylen"), QStringLiteral("32"),
               QStringLiteral("-kdfopt"), QStringLiteral("pass:") + QString::fromUtf8(kSenhaDeTeste),
               QStringLiteral("-kdfopt"), QStringLiteral("hexsalt:") + QString::fromLatin1(kSaltFixo.toHex()),
               QStringLiteral("-kdfopt"), QStringLiteral("iter:6"),
               QStringLiteral("-kdfopt"), QStringLiteral("memcost:131072"),
               QStringLiteral("-kdfopt"), QStringLiteral("lanes:1"),
               QStringLiteral("-kdfopt"), QStringLiteral("threads:1"),
               QStringLiteral("-binary"), QStringLiteral("ARGON2ID")});
    QVERIFY2(kdf.waitForFinished(60000), "o openssl kdf não terminou no prazo");
    if (kdf.exitCode() != 0) {
        // Pulo só quando a versão é a que não tem o algoritmo; em qualquer outra, a falha do oráculo reprova.
        if (!opensslTemArgon2id(textoVersao)) {
            QSKIP(qPrintable(QStringLiteral("OpenSSL sem ARGON2ID (%1), oráculo declarado como pulo").arg(textoVersao)));
        }
        QFAIL(qPrintable(QStringLiteral("openssl kdf falhou com OpenSSL com ARGON2ID: %1")
                             .arg(QString::fromUtf8(kdf.readAllStandardError()).trimmed())));
    }
    const QByteArray esperado = kdf.readAllStandardOutput();
    QCOMPARE(esperado.size(), 32);

    QByteArray obtido(32, '\0');
    const cr::ParametrosArgon2id parametros{cr::kMemlimitCriacao, cr::kOpslimitCriacao, kSaltFixo};
    const auto resultado = cr::argon2id(kSenhaDeTeste, parametros,
        std::span<unsigned char>(reinterpret_cast<unsigned char*>(obtido.data()), static_cast<std::size_t>(obtido.size())));
    QVERIFY2(resultado.has_value(), "a derivação do libsodium não pode falhar");
    QCOMPARE(obtido, esperado);
}

QTEST_GUILESS_MAIN(TstCriptoArgon2idOraculo)
#include "tst_cripto_argon2id_oraculo.moc"
