// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do conjunto de produção das âncoras do A1 (NL-2.13, DA-2.44, DA-2.48, CISO 20:20:03 item 1). Um slot no CTest:
// mid.cofre.<nome>. O conjunto é fixado no binário: o SHA-256 do SHA256SUMS embutido pelo CMake (NL_ICP_RAIZES_MANIFESTO_SHA256)
// é a prova de que o diretório lido é o conjunto de produção, e não só "o que o ambiente apontou".

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carga_raizes_icp.h>
#include <nl/foundation/pkcs12/erro_raiz_icp.h>
#include <nl/mid/cofre/raizes_producao.h>

#include <openssl/asn1.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

#include <chrono>
#include <cstddef>
#include <expected>
#include <set>
#include <string>

using namespace nl::foundation::pkcs12;
using namespace nl::mid::cofre;

namespace {

std::chrono::system_clock::time_point agoraDeTeste() {
    // 2026-10-30 12:00 UTC: data da conferência do repositório.
    return std::chrono::system_clock::from_time_t(1793361600);
}

QString dirRaizes() {
    return qEnvironmentVariable("NL_ICP_RAIZES_DIR");
}

// SHA-256 do arquivo, em hexadecimal minúsculo (o mesmo formato do embutido pelo CMake).
QString sha256DoArquivo(const QString& caminho) {
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromLatin1(QCryptographicHash::hash(arquivo.readAll(), QCryptographicHash::Sha256).toHex());
}

// Nome comum (CN) do certificado, para comparar com os nomes das raízes de produção.
std::string nomeComum(x509_st* certificado) {
    X509_NAME* nome = X509_get_subject_name(certificado);
    const int indice = X509_NAME_get_index_by_NID(nome, NID_commonName, -1);
    if (indice < 0) {
        return {};
    }
    X509_NAME_ENTRY* entrada = X509_NAME_get_entry(nome, indice);
    unsigned char* utf8 = nullptr;
    const int tamanho = ASN1_STRING_to_UTF8(&utf8, X509_NAME_ENTRY_get_data(entrada));
    if (tamanho < 0 || utf8 == nullptr) {
        return {};
    }
    std::string texto(reinterpret_cast<const char*>(utf8), static_cast<std::size_t>(tamanho));
    OPENSSL_free(utf8);
    return texto;
}

} // namespace

class TesteRaizesProducao : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    void ancorasPadraoDeProducaoSaoAsRaizesIcp();
    void manifestoComHashDiferenteDoEmbutidoRecusado();
};

void TesteRaizesProducao::ancorasPadraoDeProducaoSaoAsRaizesIcp()
{
    // Prova do conjunto fixado: o SHA256SUMS lido do diretório tem o mesmo SHA-256 que o embutido no binário.
    // Sem isso, o teste provaria só o caminho do ambiente.
    const QString dir = dirRaizes();
    QVERIFY2(sha256DoArquivo(dir + QStringLiteral("/SHA256SUMS")) == QString::fromLatin1(NL_ICP_RAIZES_MANIFESTO_SHA256),
             "SHA256SUMS do diretório não é o embutido no build");

    // Produção = seis raízes (v4, v5, v6, v7, v12 e v13). Sem v10 (SSL) nem v11 (assinatura de código).
    auto carga = carregarRaizesDeProducao(dir, agoraDeTeste());
    QVERIFY2(carga.has_value(), "carga de produção recusada");
    QCOMPARE(carga->ancoras.quantidade(), std::size_t{6});

    std::set<std::string> nomes;
    for (x509_st* certificado : carga->ancoras.certificados()) {
        nomes.insert(nomeComum(certificado));
    }
    QCOMPARE(nomes.size(), std::size_t{6});
    for (const char* esperado : {"Autoridade Certificadora Raiz Brasileira v4", "Autoridade Certificadora Raiz Brasileira v5",
                                 "Autoridade Certificadora Raiz Brasileira v6", "Autoridade Certificadora Raiz Brasileira v7",
                                 "Autoridade Certificadora Raiz Brasileira v12", "Autoridade Certificadora Raiz Brasileira v13"}) {
        QVERIFY2(nomes.count(esperado) == 1, esperado);
    }
    QVERIFY2(nomes.count("Autoridade Certificadora Raiz Brasileira v10") == 0, "v10 (SSL) não pode ser âncora de produção");
    QVERIFY2(nomes.count("Autoridade Certificadora Raiz Brasileira v11") == 0, "v11 (assinatura de código) não pode ser âncora");
}

void TesteRaizesProducao::manifestoComHashDiferenteDoEmbutidoRecusado()
{
    // Uma cópia do conjunto com um comentário a mais no SHA256SUMS: as raízes são as mesmas, o manifesto não é o fixado.
    // A carga recusa antes de ler qualquer raiz (ManifestoNaoFixado).
    QTemporaryDir copia;
    QVERIFY(copia.isValid());
    QDir origem(dirRaizes());
    for (const QString& nome : origem.entryList({QStringLiteral("*.der"), QStringLiteral("SHA256SUMS")}, QDir::Files)) {
        QVERIFY(QFile::copy(origem.filePath(nome), copia.filePath(nome)));
    }
    QFile manifesto(copia.filePath(QStringLiteral("SHA256SUMS")));
    QVERIFY(manifesto.open(QIODevice::Append | QIODevice::WriteOnly));
    manifesto.write("# alterado\n");
    manifesto.close();

    auto carga = carregarRaizesDeProducao(copia.path(), agoraDeTeste());
    QVERIFY2(!carga.has_value(), "manifesto com hash diferente do embutido foi aceito");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::ManifestoNaoFixado, std::string(codigo(carga.error().motivo)).c_str());

    // O diretório de teste da AC apontado para a produção: não há SHA256SUMS ali, então a carga falha fechada.
    auto outra = carregarRaizesDeProducao(qEnvironmentVariable("NL_CERTIFICADOS_DIR"), agoraDeTeste());
    QVERIFY2(!outra.has_value(), "diretório da AC de teste aceito como produção");
    QVERIFY2(outra.error().motivo == ErroRaizIcp::ManifestoAusente, std::string(codigo(outra.error().motivo)).c_str());
}

QTEST_APPLESS_MAIN(TesteRaizesProducao)
#include "tst_raizes_producao.moc"
