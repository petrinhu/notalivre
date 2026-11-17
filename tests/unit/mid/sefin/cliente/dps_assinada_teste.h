// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: a DPS de referência assinada pelo xmlsec1.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_DPS_ASSINADA_TESTE_H
#define NL_TESTE_DPS_ASSINADA_TESTE_H

#include <nl/back/dps/dps_assinada.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/inscricao.h>
#include <nl/back/dps/id_dps.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <cstdint>
#include <string>

namespace bk = nl::back;
namespace fp = nl::foundation::pkcs12;

namespace {

// DPS de referência assinada pelo xmlsec1 (SHA-256, quatro imagens). TROCAR pelo AssinadorXml::assinarEnveloped quando a NL-2.3 entrar na onda-2.
std::expected<bk::DpsAssinada, std::string> dpsAssinada(const fp::CredencialX509& cred)
{
    QFile arquivo(qEnvironmentVariable("NL_FIXTURES_DPS") + QStringLiteral("/valida_dps_mei_minima.xml"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(std::string("fixture da DPS"));
    }
    const QByteArray base = arquivo.readAll();
    const QString id = QStringLiteral("DPS430510821122233300018100001000000000000001");
    const QByteArray assinatura =
        QByteArrayLiteral("<Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\"><SignedInfo>"
                          "<CanonicalizationMethod Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315\"/>"
                          "<SignatureMethod Algorithm=\"http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\"/>"
                          "<Reference URI=\"#") + id.toUtf8() +
        QByteArrayLiteral("\"><Transforms>"
                          "<Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"/>"
                          "<Transform Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315\"/></Transforms>"
                          "<DigestMethod Algorithm=\"http://www.w3.org/2001/04/xmlenc#sha256\"/>"
                          "<DigestValue></DigestValue></Reference></SignedInfo><SignatureValue></SignatureValue>"
                          "<KeyInfo><X509Data></X509Data></KeyInfo></Signature>");
    const int fim = base.lastIndexOf("</DPS>");
    if (fim < 0) {
        return std::unexpected(std::string("fixture sem </DPS>"));
    }
    QTemporaryDir dir;
    if (!dir.isValid()) {
        return std::unexpected(std::string("sem pasta temporária"));
    }
    const QString chavePem = dir.filePath(QStringLiteral("chave.pem"));
    const QString certPem = dir.filePath(QStringLiteral("cert.pem"));
    const QString modelo = dir.filePath(QStringLiteral("modelo.xml"));
    const QString saida = dir.filePath(QStringLiteral("assinado.xml"));
    BIO* bioChave = BIO_new(BIO_s_mem());
    BIO* bioCert = BIO_new(BIO_s_mem());
    if (bioChave == nullptr || bioCert == nullptr ||
        PEM_write_bio_PKCS8PrivateKey(bioChave, cred.chavePrivada(), nullptr, nullptr, 0, nullptr, nullptr) != 1 ||
        PEM_write_bio_X509(bioCert, cred.certificado()) != 1) {
        BIO_free(bioChave);
        BIO_free(bioCert);
        return std::unexpected(std::string("material PEM"));
    }
    char* dadosChave = nullptr;
    char* dadosCert = nullptr;
    const long nChave = BIO_get_mem_data(bioChave, &dadosChave);
    const long nCert = BIO_get_mem_data(bioCert, &dadosCert);
    QFile fChave(chavePem);
    QFile fCert(certPem);
    QFile fModelo(modelo);
    if (!fChave.open(QIODevice::WriteOnly) || !fCert.open(QIODevice::WriteOnly) || !fModelo.open(QIODevice::WriteOnly)) {
        BIO_free(bioChave);
        BIO_free(bioCert);
        return std::unexpected(std::string("gravação dos arquivos de teste"));
    }
    fChave.write(dadosChave, nChave);
    fCert.write(dadosCert, nCert);
    fModelo.write(base.left(fim) + assinatura + base.mid(fim));
    fChave.close();
    fCert.close();
    fModelo.close();
    BIO_free(bioChave);
    BIO_free(bioCert);

    QProcess versao;
    versao.start(qEnvironmentVariable("NL_XMLSEC1"), {QStringLiteral("--version")});
    versao.waitForFinished(30000);
    const bool pelo13 = QRegularExpression(QStringLiteral("xmlsec1 1\\.(\\d+)"))
                            .match(QString::fromUtf8(versao.readAllStandardOutput() + versao.readAllStandardError()))
                            .captured(1).toInt() >= 3;
    QStringList args{QStringLiteral("--sign")};
    if (pelo13) {
        args << QStringLiteral("--lax-key-search");
    }
    args << QStringLiteral("--id-attr:Id") << QStringLiteral("infDPS") << QStringLiteral("--privkey-pem")
         << (chavePem + QLatin1Char(',') + certPem) << QStringLiteral("--output") << saida << modelo;
    QProcess xmlsec;
    xmlsec.start(qEnvironmentVariable("NL_XMLSEC1"), args);
    if (!xmlsec.waitForFinished(120000) || xmlsec.exitCode() != 0) {
        return std::unexpected(std::string("xmlsec1 não assinou a DPS de referência"));
    }
    QFile lido(saida);
    if (!lido.open(QIODevice::ReadOnly)) {
        return std::unexpected(std::string("saída do xmlsec1 ausente"));
    }
    auto municipio = bk::CodigoMunicipio::de(QStringLiteral("4305108"));
    auto cnpj = bk::Cnpj::de(QStringLiteral("11222333000181"));
    auto serie = bk::Serie::de(1);
    auto numero = bk::NumeroDps::de(std::int64_t{1});
    if (!municipio || !cnpj || !serie || !numero) {
        return std::unexpected(std::string("identificadores"));
    }
    auto idDps = bk::IdDps::montar(*municipio, bk::Inscricao{*cnpj}, *serie, *numero);
    if (!idDps) {
        return std::unexpected(std::string("id"));
    }
    return bk::DpsAssinada{*idDps, lido.readAll()};
}

} // namespace

#endif // NL_TESTE_DPS_ASSINADA_TESTE_H
