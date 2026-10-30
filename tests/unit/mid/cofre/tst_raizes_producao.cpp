// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do conjunto de produção das âncoras do A1 (NL-2.13, DA-2.44, DA-2.48). Um slot no CTest: mid.cofre.<nome>.
// Lê o repositório real de NL_ICP_RAIZES_DIR e NL_ICP_RAIZES_MANIFESTO pelo adaptador do cofre, não pela Foundation direta.

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carga_raizes_icp.h>
#include <nl/mid/cofre/raizes_producao.h>

#include <openssl/asn1.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>

#include <QString>
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

QString manifestoRaizes() {
    return qEnvironmentVariable("NL_ICP_RAIZES_MANIFESTO");
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
};

void TesteRaizesProducao::ancorasPadraoDeProducaoSaoAsRaizesIcp()
{
    // Produção = seis raízes (v4, v5, v6, v7, v12 e v13). Sem v10 (SSL) nem v11 (assinatura de código), e sem nada
    // além do manifesto. O nome comum confere com o que o ITI publica em cada certificado.
    auto carga = carregarRaizesDeProducao(dirRaizes(), manifestoRaizes(), agoraDeTeste());
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

QTEST_APPLESS_MAIN(TesteRaizesProducao)
#include "tst_raizes_producao.moc"
