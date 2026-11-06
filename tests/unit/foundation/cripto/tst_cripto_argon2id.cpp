// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do Argon2id da cripto (NL-3.2b, ADR-0006). O vetor literal de criação foi produzido UMA vez pelo oráculo
// independente, o OpenSSL, sem o código sob teste:
//   openssl kdf -keylen 32 -kdfopt pass:<entrada-de-fixture> -kdfopt hexsalt:000102030405060708090a0b0c0d0e0f
//     -kdfopt iter:3 -kdfopt memcost:65536 -kdfopt lanes:1 -kdfopt threads:1 -binary ARGON2ID | od -An -tx1
// Entrada de fixture (texto): vetor-argon2-nl32b. Não é senha: é entrada de vetor, sem valor de segredo.
// Saída na imagem Fedora anterior (aposentada, ver tools/portoes/distro_aposentada.txt), OpenSSL 3.5.9, em 04/11/2026 (entrada de fixture não sensível, sem palavra de senha): c4 61 cf 7c 69 77 02 84 99 1d 71 ab fb a4 44 86
// df 1a 36 92 22 48 e7 ad 70 7d 72 03 48 f7 0f e3.
// Vetor público (argon2idVetorReferenciaPhc): PyNaCl, índice 2, gerado pelo CLI da PHC: https://github.com/pyca/pynacl/blob/29b07709fee9c75109b9c98a4485f5080c261224/tests/data/raw_argon2id_hashes.json#L23-L30. A suíte do libsodium não tem vetor positivo acima do piso.

#include <nl/foundation/cripto/argon2id.h>

#include <QByteArray>
#include <QElapsedTimer>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <span>

namespace cr = nl::foundation::cripto;

namespace {

const QByteArray kSenhaDeTeste("vetor-argon2-nl32b");
const QByteArray kSaltFixo = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f");
const QByteArray kVetorDeCriacao = QByteArray::fromHex("c461cf7c69770284991d71abfba44486df1a36922248e7ad707d720348f70fe3");
const QByteArray kEntradaVetorPhc = QByteArray::fromHex("5a54676f4b655667");
const QByteArray kSaltVetorPhc = QByteArray::fromHex("6f435364693762364933555966746a36");
const QByteArray kEsperadoVetorPhc = QByteArray::fromHex("823cd743a9378b0013a61b275d8a8d61b4abf465e3c758");

std::span<unsigned char> areaDe(QByteArray& buffer)
{
    return {reinterpret_cast<unsigned char*>(buffer.data()), static_cast<std::size_t>(buffer.size())};
}

cr::ParametrosArgon2id parametrosDeCriacao()
{
    return cr::ParametrosArgon2id{cr::kMemlimitCriacao, cr::kOpslimitCriacao, kSaltFixo};
}

QString codigoDe(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstCriptoArgon2id : public QObject
{
    Q_OBJECT

private slots:
    void argon2idVetorParametrosDeCriacao();
    void argon2idVetorReferenciaPhc();
    void argon2idUsaParametrosRecebidos();
    void argon2idAbaixoDoPisoRecusa();
    void argon2idAcimaDoTetoRecusa();
    void argon2idNoLimiteDoTetoAceita();
    void saltDeTamanhoErradoRecusa();
};

void TstCriptoArgon2id::argon2idVetorParametrosDeCriacao()
{
    QByteArray saida(32, '\0');
    QElapsedTimer cronometro;
    cronometro.start();
    const auto resultado = cr::argon2id(kSenhaDeTeste, parametrosDeCriacao(), areaDe(saida));
    const qint64 umaDerivacao = cronometro.elapsed();
    QVERIFY2(resultado.has_value(), "a derivação de criação não pode falhar");
    QCOMPARE(saida, kVetorDeCriacao);

    // Medição, não portão (ADR-0006, item 7): o tempo sai no log para o relatório da fatia.
    qInfo("medição argon2id: 64 MiB, 3 passes, p=1, uma derivação de %lld ms", static_cast<long long>(umaDerivacao));
}

void TstCriptoArgon2id::argon2idVetorReferenciaPhc()
{
    QByteArray saida(23, '\0');
    const cr::ParametrosArgon2id parametros{67108864, 3, kSaltVetorPhc};
    const auto resultado = cr::argon2id(kEntradaVetorPhc, parametros, areaDe(saida));
    QVERIFY2(resultado.has_value(), "o vetor público da referência PHC não pode falhar");
    QCOMPARE(saida, kEsperadoVetorPhc);
}

void TstCriptoArgon2id::argon2idUsaParametrosRecebidos()
{
    QByteArray dois(32, '\0');
    QByteArray tres(32, '\0');
    const cr::ParametrosArgon2id passesDois{cr::kMemlimitPiso, 2, kSaltFixo};
    const cr::ParametrosArgon2id passesTres{cr::kMemlimitPiso, 3, kSaltFixo};
    QVERIFY(cr::argon2id(kSenhaDeTeste, passesDois, areaDe(dois)).has_value());
    QVERIFY(cr::argon2id(kSenhaDeTeste, passesTres, areaDe(tres)).has_value());
    QVERIFY2(dois != tres, "opslimit 2 e 3 precisam dar saídas diferentes: o código não pode fixar o passe");
}

void TstCriptoArgon2id::argon2idAbaixoDoPisoRecusa()
{
    // Um byte abaixo do piso de memória, e um passe abaixo do piso. Nos dois, a saída não é escrita.
    QByteArray saida(32, '\xAA');
    const cr::ParametrosArgon2id memoriaAbaixo{cr::kMemlimitPiso - 1, cr::kOpslimitCriacao, kSaltFixo};
    const auto memoria = cr::argon2id(kSenhaDeTeste, memoriaAbaixo, areaDe(saida));
    QVERIFY(!memoria.has_value());
    QCOMPARE(codigoDe(memoria.error()), QStringLiteral("cripto.parametro_abaixo_do_piso"));
    QCOMPARE(saida, QByteArray(32, '\xAA'));

    const cr::ParametrosArgon2id passesAbaixo{cr::kMemlimitCriacao, cr::kOpslimitPiso - 1, kSaltFixo};
    const auto passes = cr::argon2id(kSenhaDeTeste, passesAbaixo, areaDe(saida));
    QVERIFY(!passes.has_value());
    QCOMPARE(codigoDe(passes.error()), QStringLiteral("cripto.parametro_abaixo_do_piso"));
    QCOMPARE(saida, QByteArray(32, '\xAA'));
}

void TstCriptoArgon2id::argon2idAcimaDoTetoRecusa()
{
    // Recusa sem alocar: 4 GiB de memória e 17 passes são usados só como valor de entrada. Se alocasse, o teste travaria.
    QByteArray saida(32, '\xAA');
    const cr::ParametrosArgon2id memoriaAcima{cr::kMemlimitTeto + 1, cr::kOpslimitCriacao, kSaltFixo};
    const auto memoria = cr::argon2id(kSenhaDeTeste, memoriaAcima, areaDe(saida));
    QVERIFY(!memoria.has_value());
    QCOMPARE(codigoDe(memoria.error()), QStringLiteral("cripto.parametro_acima_do_teto"));

    const cr::ParametrosArgon2id memoriaGigante{qint64{4} * 1024 * 1024 * 1024, cr::kOpslimitCriacao, kSaltFixo};
    const auto gigante = cr::argon2id(kSenhaDeTeste, memoriaGigante, areaDe(saida));
    QVERIFY(!gigante.has_value());
    QCOMPARE(codigoDe(gigante.error()), QStringLiteral("cripto.parametro_acima_do_teto"));

    const cr::ParametrosArgon2id passesAcima{cr::kMemlimitCriacao, cr::kOpslimitTeto + 1, kSaltFixo};
    const auto passes = cr::argon2id(kSenhaDeTeste, passesAcima, areaDe(saida));
    QVERIFY(!passes.has_value());
    QCOMPARE(codigoDe(passes.error()), QStringLiteral("cripto.parametro_acima_do_teto"));
    QCOMPARE(saida, QByteArray(32, '\xAA'));
}

void TstCriptoArgon2id::argon2idNoLimiteDoTetoAceita()
{
    // Fronteira exata do piso e do teto de passes (L-43: testar a borda, não só um passo além).
    QByteArray saida(32, '\0');
    const cr::ParametrosArgon2id noPiso{cr::kMemlimitPiso, cr::kOpslimitPiso, kSaltFixo};
    QVERIFY2(cr::argon2id(kSenhaDeTeste, noPiso, areaDe(saida)).has_value(), "exatamente no piso precisa aceitar");
    const cr::ParametrosArgon2id noTetoDePasses{cr::kMemlimitPiso, cr::kOpslimitTeto, kSaltFixo};
    QVERIFY2(cr::argon2id(kSenhaDeTeste, noTetoDePasses, areaDe(saida)).has_value(), "exatamente no teto de passes precisa aceitar");
}

void TstCriptoArgon2id::saltDeTamanhoErradoRecusa()
{
    QByteArray saida(32, '\0');
    const cr::ParametrosArgon2id saltCurto{cr::kMemlimitCriacao, cr::kOpslimitCriacao, kSaltFixo.left(15)};
    const auto resultado = cr::argon2id(kSenhaDeTeste, saltCurto, areaDe(saida));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tamanho"));
}

QTEST_GUILESS_MAIN(TstCriptoArgon2id)
#include "tst_cripto_argon2id.moc"
