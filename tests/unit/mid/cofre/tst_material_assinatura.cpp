// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da conversão CredencialX509 -> MaterialAssinatura (NL-2.3, DA-2.36). Um slot por CTest (mid.cofre.<slot>).
// Fixtures sintéticas da NL-2.1 em NL_CERTIFICADOS_DIR, carregadas pelo carregador da NL-2.2. Falha é QVERIFY.

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/foundation/xmldsig/assinador_xml.h>
#include <nl/mid/cofre/material_assinatura.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtTest>

namespace pk = nl::foundation::pkcs12;
namespace mc = nl::mid::cofre;

namespace {

QByteArray lerFixture(const char* nome) {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + QString::fromUtf8(nome));
    return arquivo.open(QIODevice::ReadOnly) ? arquivo.readAll() : QByteArray();
}

std::expected<pk::CredencialX509, pk::ErroCredencial> credencialDeTeste() {
    auto ancoras = pk::Ancoras::dePem(lerFixture("ac.pem"));
    if (!ancoras) {
        return std::unexpected(pk::ErroCredencial::CadeiaInvalida);
    }
    QFile senhaArquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    pk::SenhaSegura senha = pk::SenhaSegura::de(senhaArquivo.open(QIODevice::ReadOnly)
                                                    ? QByteArray(senhaArquivo.readLine().trimmed())
                                                    : QByteArray());
    pk::CarregadorPkcs12 carregador;
    return carregador.carregar(lerFixture("cnpj_ok.pfx"), senha, *ancoras);
}

} // namespace

class TesteMaterialAssinatura : public QObject {
    Q_OBJECT

private slots:
    void materialDaCredencialViva() {
        auto credencial = credencialDeTeste();
        QVERIFY2(credencial.has_value(), "a fixture cnpj_ok tem de carregar para o teste de material");
        auto material = mc::materialParaAssinatura(*credencial);
        QVERIFY2(material.has_value(), "credencial viva tem de entregar material");
        QVERIFY2(material->certificado == credencial->certificado(), "o certificado tem de ser o da credencial, sem cópia");
        QVERIFY2(material->chave == credencial->chavePrivada(), "a chave tem de ser a da credencial, sem cópia");
    }

    void credencialDescartadaNaoEntregaMaterial() {
        auto credencial = credencialDeTeste();
        QVERIFY2(credencial.has_value(), "a fixture cnpj_ok tem de carregar para o teste de descarte");
        credencial->descartar();
        auto material = mc::materialParaAssinatura(*credencial);
        QVERIFY2(!material.has_value(), "credencial descartada não pode entregar material");
        QVERIFY2(material.error() == mc::ErroMaterial::CredencialDescartada, "o erro tem de ser o próprio");
    }
};

QTEST_APPLESS_MAIN(TesteMaterialAssinatura)
#include "tst_material_assinatura.moc"
