// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do adaptador de cofre do A1 (NL-2.2, DA-2.30). Cada slot roda por nome no CTest (mid.cofre.<slot>).
// O registro de auditoria é o Log da NL-2.7 em memória (ostringstream), com a política de PII real do Log.
// Cada teste com caso-controle: o caminho saudável precisa passar, senão o vermelho viraria falso verde.

#include <nl/foundation/log/log.h>
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/mid/cofre/adaptador_credencial.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtTest>

#include <expected>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

using nl::foundation::log::Log;
using nl::foundation::pkcs12::Ancoras;
using nl::foundation::pkcs12::CredencialX509;
using nl::foundation::pkcs12::ErroCredencial;
using nl::foundation::pkcs12::ErroAncora;
using nl::foundation::pkcs12::SenhaSegura;
using nl::mid::cofre::AdaptadorCredencial;

namespace {

using Resultado = std::expected<CredencialX509, ErroCredencial>;

const char* const CNPJ_OK = "12345678000195";
const char* const EVENTO_EKU_AUSENTE = "credencial.eku_ausente";

std::string descreverErro(const Resultado& resultado) {
    return resultado ? std::string("sucesso") : std::string(nl::foundation::pkcs12::codigo(resultado.error()));
}

QByteArray fixture(const QString& nome) {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + nome);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

SenhaSegura senhaDaFixture() {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return SenhaSegura::de(QByteArray());
    }
    return SenhaSegura::de(arquivo.readLine().trimmed());
}

std::expected<Ancoras, ErroAncora> ancoraDeTeste() {
    return Ancoras::dePem(fixture(QStringLiteral("ac.pem")));
}

// Log de auditoria em memória, com o evento catalogado. Quem chama decide se o fluxo começa com badbit.
Log novoLog(std::ostringstream& saida) {
    return Log(saida, "notalivre", {}, {EVENTO_EKU_AUSENTE});
}

} // namespace

class TesteAdaptadorCredencial : public QObject {
    Q_OBJECT

private slots:
    void credencialSemEkuRegistraEventoNoLog();
    void credencialComEkuNaoRegistraEvento();
    void falhaDoRegistroRecusaCredencialSemEku();
};

void TesteAdaptadorCredencial::credencialSemEkuRegistraEventoNoLog()
{
    std::ostringstream saida;
    Log log = novoLog(saida);
    AdaptadorCredencial adaptador(log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = adaptador.carregar(fixture(QStringLiteral("eku_ausente.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QVERIFY(resultado->ekuAusente());
    const std::string texto = saida.str();
    QVERIFY2(texto.find(EVENTO_EKU_AUSENTE) != std::string::npos, "evento credencial.eku_ausente não foi gravado");
    QVERIFY2(texto.find(CNPJ_OK) == std::string::npos, "o evento não pode trazer o CNPJ");
}

void TesteAdaptadorCredencial::credencialComEkuNaoRegistraEvento()
{
    // Controle do evento: uma credencial com EKU clientAuth é aceita e não gera o evento de EKU ausente.
    std::ostringstream saida;
    Log log = novoLog(saida);
    AdaptadorCredencial adaptador(log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = adaptador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QVERIFY(!resultado->ekuAusente());
    QVERIFY2(saida.str().find(EVENTO_EKU_AUSENTE) == std::string::npos, "credencial com EKU não pode gerar o evento");
}

void TesteAdaptadorCredencial::falhaDoRegistroRecusaCredencialSemEku()
{
    // Controle primeiro: com o registro saudável, a credencial sem EKU é aceita. Só então o registro em falha.
    // Sem o controle, um adaptador que sempre recusa passaria no caso de falha.
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    {
        std::ostringstream saudavel;
        Log log = novoLog(saudavel);
        AdaptadorCredencial adaptador(log);
        auto senha = senhaDaFixture();
        auto resultado = adaptador.carregar(fixture(QStringLiteral("eku_ausente.pfx")), senha, *ancoras);
        QVERIFY2(resultado.has_value(), "controle: com registro saudável a credencial sem EKU deve ser aceita");
    }
    {
        std::ostringstream falho;
        falho.setstate(std::ios::badbit);
        Log log = novoLog(falho);
        AdaptadorCredencial adaptador(log);
        auto senha = senhaDaFixture();
        auto resultado = adaptador.carregar(fixture(QStringLiteral("eku_ausente.pfx")), senha, *ancoras);
        QVERIFY(!resultado.has_value());
        QCOMPARE(resultado.error(), ErroCredencial::FalhaInterna);
    }
}

QTEST_APPLESS_MAIN(TesteAdaptadorCredencial)
#include "tst_adaptador_credencial.moc"
