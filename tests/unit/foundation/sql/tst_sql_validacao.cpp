// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de unidade da validação de DadosConexao da Foundation sql (NL-3.1a, bloco U-R): recusa antes de qualquer
// socket, sem servidor. No R, o stub devolve sql.nao_implementado e cada slot falha citando esse código. A costura de
// teste tem de ficar em zero depois de cada recusa.

#include <nl/base/erro.h>
#include <nl/foundation/sql/costura_conexao.h>
#include <nl/foundation/sql/dados_conexao.h>
#include <nl/foundation/sql/validacao.h>

#include <QByteArray>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <string>

namespace sql = nl::foundation::sql;

namespace {

constexpr char senhaDeTeste[] = "senha-de-teste";

sql::DadosConexao dadosValidos()
{
    return sql::DadosConexao{
        .host = QByteArray("127.0.0.1"),
        .porta = 3306,
        .banco = QByteArray("nl"),
        .usuario = QByteArray("nl"),
        .senhaNulTerminada = senhaDeTeste,
        .pluginsPermitidos = QByteArray("mysql_native_password"),
        .modo = sql::ModoAberto{},
    };
}

/// Recusa esperada: a validação falha com o código dado, e a costura de conexão continua em zero.
void conferirRecusa(const sql::DadosConexao& dados, const char* codigoEsperado)
{
    const auto resultado = sql::detalhe::validarDadosConexao(dados);
    QVERIFY2(!resultado.has_value(), "dados inválidos foram aceitos");
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QString::fromLatin1(codigoEsperado));
    QCOMPARE(sql::costura::chamadasDeConexao(), 0);
}

} // namespace

class TstSqlValidacao : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void hostForaDoLoopbackRecusado();
    void portaForaDaFaixaRecusada();
    void senhaNulaRecusada();
    void pluginsVazioRecusado();
    void listaComClearOuDialogRecusada();
    void pluginDesconhecidoRecusado();
    void modoAbertoComPluginSha2Recusado();
    void modoFixadoComImpressaoInvalidaRecusado();
};

void TstSqlValidacao::hostForaDoLoopbackRecusado()
{
    for (const char* host : {"192.0.2.1", "localhost", ""}) {
        auto dados = dadosValidos();
        dados.host = QByteArray(host);
        conferirRecusa(dados, "sql.host_fora_do_loopback");
    }
}

void TstSqlValidacao::portaForaDaFaixaRecusada()
{
    for (const int porta : {0, -1, 65536}) {
        auto dados = dadosValidos();
        dados.porta = porta;
        conferirRecusa(dados, "sql.porta_invalida");
    }
}

void TstSqlValidacao::senhaNulaRecusada()
{
    auto dados = dadosValidos();
    dados.senhaNulTerminada = nullptr;
    conferirRecusa(dados, "sql.senha_ausente");
}

void TstSqlValidacao::pluginsVazioRecusado()
{
    auto dados = dadosValidos();
    dados.pluginsPermitidos = QByteArray();
    conferirRecusa(dados, "sql.plugins_vazio");
}

void TstSqlValidacao::listaComClearOuDialogRecusada()
{
    for (const char* lista : {"mysql_clear_password", "dialog", "mysql_native_password,dialog"}) {
        auto dados = dadosValidos();
        dados.pluginsPermitidos = QByteArray(lista);
        conferirRecusa(dados, "sql.plugin_proibido");
    }
}

void TstSqlValidacao::pluginDesconhecidoRecusado()
{
    for (const char* lista : {"mysql_native_password_x", "caching_sha2"}) {
        auto dados = dadosValidos();
        dados.pluginsPermitidos = QByteArray(lista);
        conferirRecusa(dados, "sql.plugin_desconhecido");
    }
}

void TstSqlValidacao::modoAbertoComPluginSha2Recusado()
{
    for (const char* lista : {"caching_sha2_password", "sha256_password"}) {
        auto dados = dadosValidos();
        dados.pluginsPermitidos = QByteArray(lista);
        dados.modo = sql::ModoAberto{};
        conferirRecusa(dados, "sql.plugin_sha2_no_modo_aberto");
    }
}

void TstSqlValidacao::modoFixadoComImpressaoInvalidaRecusado()
{
    QByteArray comG(64, 'a');
    comG[63] = 'g';
    for (const QByteArray& impressao : {QByteArray(), QByteArray(63, 'a'), comG}) {
        auto dados = dadosValidos();
        dados.modo = sql::ModoFixado(impressao);
        conferirRecusa(dados, "sql.impressao_invalida");
    }
}

QTEST_GUILESS_MAIN(TstSqlValidacao)
#include "tst_sql_validacao.moc"
