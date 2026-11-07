// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do gerador aleatório da cripto (NL-3.2b). Propriedade com teto declarado: 100000 nonces de 96 bits, sem repetição.

#include <nl/foundation/cripto/aleatorio.h>
#include <nl/foundation/cripto/tamanhos.h>

#include <QByteArray>
#include <QHash>
#include <QtTest>

#include <cstddef>
#include <span>

namespace cr = nl::foundation::cripto;

class TstCriptoAleatorio : public QObject
{
    Q_OBJECT

private slots:
    void nonceNuncaRepete();
};

void TstCriptoAleatorio::nonceNuncaRepete()
{
    constexpr int sorteios = 100000;
    QHash<QByteArray, int> vistos;
    vistos.reserve(sorteios);

    for (int indice = 0; indice < sorteios; ++indice) {
        QByteArray nonce(cr::kTamanhoNonce, '\0');
        const auto resultado = cr::bytesAleatorios(
            std::span<unsigned char>(reinterpret_cast<unsigned char*>(nonce.data()), static_cast<std::size_t>(nonce.size())));
        QVERIFY2(resultado.has_value(), "o gerador do sistema não pode falhar");
        QVERIFY2(!vistos.contains(nonce), qPrintable(QStringLiteral("nonce repetido no sorteio %1").arg(indice)));
        vistos.insert(nonce, indice);
    }
    QCOMPARE(vistos.size(), sorteios);
}

QTEST_GUILESS_MAIN(TstCriptoAleatorio)
#include "tst_cripto_aleatorio.moc"
