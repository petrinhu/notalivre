// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de unidade do catálogo e do plano do migrador (NL-3.1b, parte b1): sem servidor. Os casos de borda do nome, da
// sequência e do banco à frente seguem a NL-I.351.

#include <nl/base/erro.h>
#include <nl/foundation/sql/migracao.h>

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QtTest>

#include <vector>

namespace sql = nl::foundation::sql;

namespace {

sql::ArquivoMigracao arquivo(const char* nome, const char* conteudo)
{
    return sql::ArquivoMigracao{QString::fromUtf8(nome), QByteArray(conteudo)};
}

QString codigoDe(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

// Catálogo de duas migrações válidas, para os testes do plano.
std::vector<sql::Migracao> catalogoDeDuas()
{
    auto catalogo = sql::catalogoDe({arquivo("V0001__emissor.sql", "CREATE TABLE a (b INT)"),
                                     arquivo("V0002__nota.sql", "CREATE TABLE c (d INT)")});
    return catalogo.has_value() ? *catalogo : std::vector<sql::Migracao>{};
}

} // namespace

class TstMigracao : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void catalogoOrdenaPorVersao();
    void catalogoCalculaSha256DosBytes();
    void catalogoVazioRecusado();
    void nomeForaDoPadraoRecusado();
    void versaoRepetidaRecusada();
    void versaoComBuracoRecusada();
    void catalogoQueNaoComecaEmUmRecusado();
    void bancoVazioAplicaTudo();
    void bancoEmDiaNaoAplicaNada();
    void bancoAtrasadoAplicaSoAsQueFaltam();
    void schemaMaisNovoRecusadoSemPlano();
    void checksumDivergenteRecusado();
    void aplicadasComBuracoRecusadas();
};

void TstMigracao::catalogoOrdenaPorVersao()
{
    const auto catalogo = sql::catalogoDe({arquivo("V0002__nota.sql", "B"), arquivo("V0001__emissor.sql", "A")});
    QVERIFY(catalogo.has_value());
    QCOMPARE(catalogo->size(), std::size_t{2});
    QCOMPARE(catalogo->at(0).versao, 1);
    QCOMPARE(catalogo->at(0).descricao, QStringLiteral("emissor"));
    QCOMPARE(catalogo->at(0).sql, QByteArray("A"));
    QCOMPARE(catalogo->at(1).versao, 2);
    QCOMPARE(catalogo->at(1).descricao, QStringLiteral("nota"));
}

void TstMigracao::catalogoCalculaSha256DosBytes()
{
    // Vetor conhecido do SHA-256 para "abc" (FIPS 180-2, apêndice B.1).
    const auto catalogo = sql::catalogoDe({arquivo("V0001__vetor.sql", "abc")});
    QVERIFY(catalogo.has_value());
    QCOMPARE(catalogo->at(0).checksum,
             QByteArray::fromHex("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
}

void TstMigracao::catalogoVazioRecusado()
{
    const auto catalogo = sql::catalogoDe({});
    QVERIFY(!catalogo.has_value());
    QCOMPARE(codigoDe(catalogo.error()), QStringLiteral("migracao.catalogo_vazio"));
}

void TstMigracao::nomeForaDoPadraoRecusado()
{
    const QStringList nomes{QString(), QStringLiteral("V1__a.sql"), QStringLiteral("V0001_a.sql"),
                            QStringLiteral("v0001__a.sql"), QStringLiteral("V0001__A.sql"),
                            QStringLiteral("V0001__a.txt"), QStringLiteral("V0000__a.sql"),
                            QStringLiteral("V0001__.sql"), QStringLiteral("V0001__a b.sql"),
                            QStringLiteral("V00001__a.sql"), QStringLiteral(" V0001__a.sql")};
    for (const QString& nome : nomes) {
        const auto catalogo = sql::catalogoDe({sql::ArquivoMigracao{nome, QByteArray("A")}});
        QVERIFY2(!catalogo.has_value(), qPrintable(nome));
        QCOMPARE(codigoDe(catalogo.error()), QStringLiteral("migracao.nome_invalido"));
    }
}

void TstMigracao::versaoRepetidaRecusada()
{
    const auto catalogo = sql::catalogoDe({arquivo("V0001__a.sql", "A"), arquivo("V0001__b.sql", "B")});
    QVERIFY(!catalogo.has_value());
    QCOMPARE(codigoDe(catalogo.error()), QStringLiteral("migracao.versao_repetida"));
}

void TstMigracao::versaoComBuracoRecusada()
{
    const auto catalogo = sql::catalogoDe({arquivo("V0001__a.sql", "A"), arquivo("V0003__c.sql", "C")});
    QVERIFY(!catalogo.has_value());
    QCOMPARE(codigoDe(catalogo.error()), QStringLiteral("migracao.versao_com_buraco"));
}

void TstMigracao::catalogoQueNaoComecaEmUmRecusado()
{
    const auto catalogo = sql::catalogoDe({arquivo("V0002__b.sql", "B")});
    QVERIFY(!catalogo.has_value());
    QCOMPARE(codigoDe(catalogo.error()), QStringLiteral("migracao.versao_com_buraco"));
}

void TstMigracao::bancoVazioAplicaTudo()
{
    const auto catalogo = catalogoDeDuas();
    QCOMPARE(catalogo.size(), std::size_t{2});
    const auto plano = sql::planoDe(catalogo, {});
    QVERIFY(plano.has_value());
    QCOMPARE(plano->size(), std::size_t{2});
    QCOMPARE(plano->at(0).versao, 1);
    QCOMPARE(plano->at(1).versao, 2);
}

void TstMigracao::bancoEmDiaNaoAplicaNada()
{
    const auto catalogo = catalogoDeDuas();
    QCOMPARE(catalogo.size(), std::size_t{2});
    const auto plano = sql::planoDe(catalogo, {sql::MigracaoAplicada{2, catalogo.at(1).checksum},
                                               sql::MigracaoAplicada{1, catalogo.at(0).checksum}});
    QVERIFY(plano.has_value());
    QVERIFY(plano->empty());
}

void TstMigracao::bancoAtrasadoAplicaSoAsQueFaltam()
{
    const auto catalogo = catalogoDeDuas();
    QCOMPARE(catalogo.size(), std::size_t{2});
    const auto plano = sql::planoDe(catalogo, {sql::MigracaoAplicada{1, catalogo.at(0).checksum}});
    QVERIFY(plano.has_value());
    QCOMPARE(plano->size(), std::size_t{1});
    QCOMPARE(plano->at(0).versao, 2);
}

void TstMigracao::schemaMaisNovoRecusadoSemPlano()
{
    const auto catalogo = catalogoDeDuas();
    QCOMPARE(catalogo.size(), std::size_t{2});
    const auto plano = sql::planoDe(catalogo, {sql::MigracaoAplicada{1, catalogo.at(0).checksum},
                                               sql::MigracaoAplicada{2, catalogo.at(1).checksum},
                                               sql::MigracaoAplicada{3, QByteArray(32, '\x01')}});
    QVERIFY(!plano.has_value());
    QCOMPARE(codigoDe(plano.error()), QStringLiteral("migracao.schema_mais_novo"));
}

void TstMigracao::checksumDivergenteRecusado()
{
    const auto catalogo = catalogoDeDuas();
    QCOMPARE(catalogo.size(), std::size_t{2});
    const auto plano = sql::planoDe(catalogo, {sql::MigracaoAplicada{1, QByteArray(32, '\x02')}});
    QVERIFY(!plano.has_value());
    QCOMPARE(codigoDe(plano.error()), QStringLiteral("migracao.checksum_divergente"));
}

void TstMigracao::aplicadasComBuracoRecusadas()
{
    const auto catalogo = catalogoDeDuas();
    QCOMPARE(catalogo.size(), std::size_t{2});
    const auto soADois = sql::planoDe(catalogo, {sql::MigracaoAplicada{2, catalogo.at(1).checksum}});
    QVERIFY(!soADois.has_value());
    QCOMPARE(codigoDe(soADois.error()), QStringLiteral("migracao.aplicadas_com_buraco"));
    const auto umRepetido = sql::planoDe(catalogo, {sql::MigracaoAplicada{1, catalogo.at(0).checksum},
                                                    sql::MigracaoAplicada{1, catalogo.at(0).checksum}});
    QVERIFY(!umRepetido.has_value());
    QCOMPARE(codigoDe(umRepetido.error()), QStringLiteral("migracao.aplicadas_com_buraco"));
}

QTEST_GUILESS_MAIN(TstMigracao)
#include "tst_migracao.moc"
