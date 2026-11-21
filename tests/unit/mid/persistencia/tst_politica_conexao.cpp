// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de unidade da política de conexão do Mid (NL-3.1a, bloco M-R): sem servidor. No R, o stub devolve
// persistencia.nao_implementado e cada slot falha citando esse código.

#include <nl/base/erro.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/foundation/sql/dados_conexao.h>
#include <nl/mid/persistencia/politica_conexao.h>

#include <QByteArray>
#include <QString>
#include <QtTest>

#include <variant>

namespace mid = nl::mid::persistencia;
namespace sql = nl::foundation::sql;
namespace pkcs12 = nl::foundation::pkcs12;

namespace {

mid::ServidorDeclarado servidor(mid::FamiliaServidor familia, bool exigeTls = false,
                                const QByteArray& impressao = QByteArray(64, 'a'))
{
    mid::ServidorDeclarado declarado;
    declarado.familia = familia;
    declarado.exigeTransporteSeguro = exigeTls;
    declarado.impressaoSha256Hex = impressao;
    declarado.host = QByteArray("127.0.0.1");
    declarado.porta = 3306;
    declarado.banco = QByteArray("nl");
    declarado.usuario = QByteArray("nl");
    return declarado;
}

} // namespace

class TstPoliticaConexao : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void mysqlVaiAoModoFixado();
    void mysqlSemImpressaoRecusado();
    void mariadbVaiAoModoAberto();
    void mariadbComTransporteSeguroVaiAoFixado();
    void listaDoMysqlSoCachingSha2();
    void listaDoMariadbSemPluginSha2();
    void senhaVemDaSenhaSeguraSemCopia();
};

void TstPoliticaConexao::mysqlVaiAoModoFixado()
{
    const auto senha = pkcs12::SenhaSegura::de(QByteArray("senha-de-teste"));
    const auto resultado = mid::dadosDeConexao(servidor(mid::FamiliaServidor::Mysql), senha);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    QVERIFY(std::holds_alternative<sql::ModoFixado>(resultado->modo));
}

void TstPoliticaConexao::mysqlSemImpressaoRecusado()
{
    const auto senha = pkcs12::SenhaSegura::de(QByteArray("senha-de-teste"));
    const auto resultado = mid::dadosDeConexao(servidor(mid::FamiliaServidor::Mysql, false, QByteArray()), senha);
    QVERIFY(!resultado.has_value());
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("persistencia.impressao_ausente"));
}

void TstPoliticaConexao::mariadbVaiAoModoAberto()
{
    const auto senha = pkcs12::SenhaSegura::de(QByteArray("senha-de-teste"));
    const auto resultado = mid::dadosDeConexao(servidor(mid::FamiliaServidor::Mariadb), senha);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    QVERIFY(std::holds_alternative<sql::ModoAberto>(resultado->modo));
}

void TstPoliticaConexao::mariadbComTransporteSeguroVaiAoFixado()
{
    const auto senha = pkcs12::SenhaSegura::de(QByteArray("senha-de-teste"));
    const auto resultado = mid::dadosDeConexao(servidor(mid::FamiliaServidor::Mariadb, true), senha);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    QVERIFY(std::holds_alternative<sql::ModoFixado>(resultado->modo));
}

void TstPoliticaConexao::listaDoMysqlSoCachingSha2()
{
    const auto senha = pkcs12::SenhaSegura::de(QByteArray("senha-de-teste"));
    const auto resultado = mid::dadosDeConexao(servidor(mid::FamiliaServidor::Mysql), senha);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    QCOMPARE(resultado->pluginsPermitidos, QByteArray("caching_sha2_password"));
}

void TstPoliticaConexao::listaDoMariadbSemPluginSha2()
{
    const auto senha = pkcs12::SenhaSegura::de(QByteArray("senha-de-teste"));
    const auto resultado = mid::dadosDeConexao(servidor(mid::FamiliaServidor::Mariadb), senha);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    QCOMPARE(resultado->pluginsPermitidos, QByteArray("mysql_native_password"));
}

void TstPoliticaConexao::senhaVemDaSenhaSeguraSemCopia()
{
    const auto senha = pkcs12::SenhaSegura::de(QByteArray("senha-de-teste"));
    const auto resultado = mid::dadosDeConexao(servidor(mid::FamiliaServidor::Mariadb), senha);
    QVERIFY2(resultado.has_value(), resultado.has_value() ? "" : resultado.error().codigo().c_str());
    QCOMPARE(resultado->senhaNulTerminada, senha.comoC());
}

QTEST_GUILESS_MAIN(TstPoliticaConexao)
#include "tst_politica_conexao.moc"
