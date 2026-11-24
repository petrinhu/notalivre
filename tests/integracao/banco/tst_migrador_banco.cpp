// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de integração do migrador no banco (NL-3.1b, parte b2): 12 slots, um por comportamento, contra o servidor do
// par. Cada slot abre uma sessão pelo Executor e imprime a sua linha NL-TLS antes de qualquer asserção; a segunda sessão,
// quando o slot precisa dela, abre sem linha NL-TLS, porque o harness conta uma por slot. No R, migrar é esboço e cada
// slot falha por sql.nao_implementado. Os auxiliares de arranjo repetem os de tst_executor_banco.cpp, que é de outra
// fatia (NL-3.1l): a unificação num cabeçalho comum fica para depois do merge dela.

#include "ambiente_banco.h"
#include "impressao_banco.h"
#include "registro_conector.h"

#include <nl/base/erro.h>
#include <nl/foundation/sql/consulta.h>
#include <nl/foundation/sql/dados_conexao.h>
#include <nl/foundation/sql/executor.h>
#include <nl/foundation/sql/migrador.h>
#include <nl/foundation/sql/parametro.h>

#include <mysql.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QString>
#include <QtTest>

#include <array>
#include <expected>
#include <initializer_list>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace sql = nl::foundation::sql;

namespace {

// Catálogos de teste. O DDL é literal: o migrador não executa SQL que venha de fora do binário.
constexpr std::array<sql::MigracaoEmbutida, 1> kSoV1{{
    {"V0001__cria_a.sql", sql::Ddl::de("CREATE TABLE nl_teste_mig_a (x INT)")},
}};
constexpr std::array<sql::MigracaoEmbutida, 2> kV1EV2{{
    {"V0001__cria_a.sql", sql::Ddl::de("CREATE TABLE nl_teste_mig_a (x INT)")},
    {"V0002__poe_y.sql", sql::Ddl::de("ALTER TABLE nl_teste_mig_a ADD COLUMN y INT")},
}};
constexpr std::array<sql::MigracaoEmbutida, 1> kV1Alterada{{
    {"V0001__cria_a.sql", sql::Ddl::de("CREATE TABLE nl_teste_mig_a (x BIGINT)")},
}};
constexpr std::array<sql::MigracaoEmbutida, 2> kV2QueFalha{{
    {"V0001__cria_a.sql", sql::Ddl::de("CREATE TABLE nl_teste_mig_a (x INT)")},
    {"V0002__poe_y.sql", sql::Ddl::de("ALTER TABLE nl_teste_mig_inexistente ADD COLUMN y INT")},
}};
constexpr std::array<sql::MigracaoEmbutida, 1> kNomeInvalido{{
    {"V1__cria_a.sql", sql::Ddl::de("CREATE TABLE nl_teste_mig_a (x INT)")},
}};

constexpr QByteArrayView kPrefixoDaTrava = "nl_mig_";

// Classe pelo arquivo de tabela, como em tst_executor_banco.cpp: A é o MySQL com disco (fixado); B é o MariaDB sem TLS
// (aberto); C é o MariaDB com TLS em memória (aberto).
enum class Classe { A, B, C };

std::optional<Classe> classeDo(const DadosBanco &dados)
{
    if (dados.servidorTls == QStringLiteral("sim") && dados.certificado == QStringLiteral("disco")) {
        return Classe::A;
    }
    if (dados.servidorTls == QStringLiteral("nao") && dados.certificado == QStringLiteral("nenhum")) {
        return Classe::B;
    }
    if (dados.servidorTls == QStringLiteral("sim") && dados.certificado == QStringLiteral("memoria")) {
        return Classe::C;
    }
    return std::nullopt;
}

struct Arranjo {
    std::optional<DadosBanco> dados;
    std::optional<Classe> classe;
    QByteArray senha;
    QByteArray falta;
};

Arranjo arranjoSemLinha()
{
    Arranjo arranjo;
    arranjo.dados = dadosDoAmbiente();
    if (!arranjo.dados) {
        arranjo.falta = QByteArrayLiteral("NL_BANCO_* ausente ou inválido: o teste do banco falha, nunca pula");
        return arranjo;
    }
    arranjo.classe = classeDo(*arranjo.dados);
    if (!arranjo.classe) {
        arranjo.falta = QByteArrayLiteral("classe do servidor fora da tabela (servidorTls e certificado): PARADA");
        return arranjo;
    }
    const auto senha = senhaDoArquivo(arranjo.dados->arquivoSenha);
    if (!senha) {
        arranjo.falta = QByteArrayLiteral("arquivo de senha do par ausente ou vazio");
        return arranjo;
    }
    arranjo.senha = *senha;
    return arranjo;
}

// Falha antes do executor: o slot imprime a sua NL-TLS antes da asserção que falha, como faz quando o Executor recusa.
void recusaDePreparo()
{
    qInfo("NL-TLS conexao=%s resultado=recusa cliente=nulo servidor=vazio versao=vazio status=nd msg=arranjo erro=0",
          QTest::currentTestFunction());
}

Arranjo arranjoDoSlot()
{
    Arranjo arranjo = arranjoSemLinha();
    if (!arranjo.falta.isEmpty()) {
        recusaDePreparo();
    }
    return arranjo;
}

sql::DadosConexao dadosConexaoDo(const Arranjo &a)
{
    const bool mysql = *a.classe == Classe::A;
    return sql::DadosConexao{
        .host = a.dados->host.toUtf8(),
        .porta = static_cast<int>(a.dados->porta),
        .banco = a.dados->base.toUtf8(),
        .usuario = a.dados->usuario.toUtf8(),
        .senhaNulTerminada = a.senha.constData(),
        .pluginsPermitidos = mysql ? QByteArray("caching_sha2_password") : QByteArray("mysql_native_password"),
        .modo = mysql ? sql::ModoTls(sql::ModoFixado(impressaoParaFixar(*a.dados))) : sql::ModoTls(sql::ModoAberto{}),
    };
}

// Sessão principal do slot: imprime a linha NL-TLS deste slot, antes de qualquer asserção.
std::optional<sql::Executor> abrir(const Arranjo &a)
{
    auto resultado = sql::Executor::conectar(dadosConexaoDo(a));
    if (resultado) {
        const QByteArray cifra = resultado->cifraTlsDaSessao();
        qInfo("NL-TLS conexao=%s resultado=tls cliente=%s servidor=nd versao=nd status=nd msg=nenhuma erro=0",
              QTest::currentTestFunction(), cifra.isEmpty() ? "nulo" : cifra.constData());
        return std::move(*resultado);
    }
    qInfo("NL-TLS conexao=%s resultado=recusa cliente=nulo servidor=vazio versao=vazio status=nd msg=%s erro=0",
          QTest::currentTestFunction(), resultado.error().codigo().c_str());
    return std::nullopt;
}

// Segunda sessão do slot, sem linha NL-TLS (o harness conta uma por slot).
std::optional<sql::Executor> abrirOutra(const Arranjo &a)
{
    auto resultado = sql::Executor::conectar(dadosConexaoDo(a));
    if (!resultado) {
        return std::nullopt;
    }
    return std::move(*resultado);
}

QByteArray codigoDo(const nl::base::Erro &erro)
{
    return QByteArray::fromStdString(erro.codigo());
}

// Primeira coluna da primeira linha como inteiro; -1 se a consulta falhou ou não veio inteiro.
qint64 inteiroDe(const std::expected<sql::Linhas, nl::base::Erro> &linhas)
{
    if (!linhas || linhas->isEmpty() || linhas->front().isEmpty()) {
        return -1;
    }
    const auto *inteiro = std::get_if<qint64>(&linhas->front().front());
    return inteiro != nullptr ? *inteiro : -1;
}

// Banco limpo para o slot: sem nl_migracao e sem a tabela dos catálogos de teste.
bool limpar(sql::Executor &executor)
{
    return executor.executarDdl(sql::Ddl::de("DROP TABLE IF EXISTS nl_migracao")).has_value() &&
           executor.executarDdl(sql::Ddl::de("DROP TABLE IF EXISTS nl_teste_mig_a")).has_value();
}

qint64 tabelaExiste(sql::Executor &executor, QByteArrayView nome)
{
    const std::array parametros{sql::Parametro::texto(nome)};
    return inteiroDe(executor.consultar(
        sql::Consulta::de("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = ?"),
        parametros));
}

qint64 colunaExiste(sql::Executor &executor, QByteArrayView tabela, QByteArrayView coluna)
{
    const std::array parametros{sql::Parametro::texto(tabela), sql::Parametro::texto(coluna)};
    return inteiroDe(executor.consultar(
        sql::Consulta::de("SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() "
                          "AND table_name = ? AND column_name = ?"),
        parametros));
}

qint64 registros(sql::Executor &executor, qint64 sucesso)
{
    const std::array parametros{sql::Parametro::inteiro(sucesso)};
    return inteiroDe(executor.consultar(sql::Consulta::de("SELECT COUNT(*) FROM nl_migracao WHERE sucesso = ?"), parametros));
}

QByteArray checksumGuardado(sql::Executor &executor, qint64 versao)
{
    const std::array parametros{sql::Parametro::inteiro(versao)};
    const auto linhas = executor.consultar(sql::Consulta::de("SELECT checksum FROM nl_migracao WHERE versao = ?"), parametros);
    if (!linhas || linhas->isEmpty() || linhas->front().isEmpty()) {
        return QByteArray();
    }
    const auto *bytes = std::get_if<QByteArray>(&linhas->front().front());
    return bytes != nullptr ? *bytes : QByteArray();
}

qint64 travaLivre(sql::Executor &executor)
{
    const std::array parametros{sql::Parametro::texto(kPrefixoDaTrava)};
    return inteiroDe(executor.consultar(
        sql::Consulta::de("SELECT IS_FREE_LOCK(CONCAT(?, LEFT(SHA2(DATABASE(), 256), 56)))"), parametros));
}

qint64 tomarTrava(sql::Executor &executor)
{
    const std::array parametros{sql::Parametro::texto(kPrefixoDaTrava)};
    return inteiroDe(executor.consultar(
        sql::Consulta::de("SELECT GET_LOCK(CONCAT(?, LEFT(SHA2(DATABASE(), 256), 56)), 0)"), parametros));
}

std::vector<int> versoes(std::initializer_list<int> lista)
{
    return std::vector<int>(lista);
}

} // namespace

class TesteMigradorBanco : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void bancoVazioAplicaTudoERegistra();
    void segundaExecucaoNaoAplicaNada();
    void versaoNovaAplicaSoAQueFalta();
    void bancoAFrenteRecusaSemEscrita();
    void checksumDivergenteRecusaSemEscrita();
    void ddlQueFalhaDeixaMarcaETravaAsSeguintes();
    void travaOcupadaRecusaSemEscrita();
    void travaLiberadaQuandoASessaoCai();
    void travaLiberadaAoSair();
    void catalogoInvalidoNaoTocaOBanco();
    void linhaIlegivelRecusaSemEscrita();
    void marcaQueFalhaNaoRodaODdl();
};

// Registro do Connector/C deste processo (G-R do CISO): uma linha NL-CONECTOR por processo, antes das asserções.
void TesteMigradorBanco::initTestCase()
{
    const QString cabecalho = QStringLiteral(MARIADB_PACKAGE_VERSION);
    const QString execucao = QString::fromUtf8(mysql_get_client_info());
    registrarNlConector(cabecalho, execucao, QStringLiteral("nd"), QStringLiteral(NL_MARIADB_PREFIXO_DO_BUILD));
    QVERIFY2(execucao == cabecalho, "libmariadb carregado com versão diferente da do cabeçalho compilado: PARADA ao CISO");
}

// Banco vazio: aplica V1 e V2, em ordem, e registra as duas com sucesso 1 e o SHA-256 dos bytes executados.
void TesteMigradorBanco::bancoVazioAplicaTudoERegistra()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));

    const auto resultado = sql::migrar(*executor, kV1EV2);
    QVERIFY2(resultado.has_value(), resultado ? "" : resultado.error().codigo().c_str());
    QCOMPARE(resultado->aplicadas, versoes({1, 2}));
    QCOMPARE(registros(*executor, 1), qint64{2});
    QCOMPARE(registros(*executor, 0), qint64{0});
    QCOMPARE(colunaExiste(*executor, "nl_teste_mig_a", "y"), qint64{1});
    QCOMPARE(checksumGuardado(*executor, 1),
             QCryptographicHash::hash(QByteArrayLiteral("CREATE TABLE nl_teste_mig_a (x INT)"), QCryptographicHash::Sha256));
}

// Banco já na versão do catálogo: nada é aplicado e os registros não mudam.
void TesteMigradorBanco::segundaExecucaoNaoAplicaNada()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));

    QVERIFY(sql::migrar(*executor, kV1EV2).has_value());
    const auto segunda = sql::migrar(*executor, kV1EV2);
    QVERIFY2(segunda.has_value(), segunda ? "" : segunda.error().codigo().c_str());
    QVERIFY(segunda->aplicadas.empty());
    QCOMPARE(registros(*executor, 1), qint64{2});
}

// Banco em V1 e catálogo com V2: aplica só a V2.
void TesteMigradorBanco::versaoNovaAplicaSoAQueFalta()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));

    QVERIFY(sql::migrar(*executor, kSoV1).has_value());
    QCOMPARE(colunaExiste(*executor, "nl_teste_mig_a", "y"), qint64{0});
    const auto resultado = sql::migrar(*executor, kV1EV2);
    QVERIFY2(resultado.has_value(), resultado ? "" : resultado.error().codigo().c_str());
    QCOMPARE(resultado->aplicadas, versoes({2}));
    QCOMPARE(colunaExiste(*executor, "nl_teste_mig_a", "y"), qint64{1});
}

// Banco em V2 e catálogo só com V1 (app mais velho que o banco): recusa, e nada muda no banco.
void TesteMigradorBanco::bancoAFrenteRecusaSemEscrita()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));

    QVERIFY(sql::migrar(*executor, kV1EV2).has_value());
    const auto resultado = sql::migrar(*executor, kSoV1);
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("migracao.schema_mais_novo"));
    QCOMPARE(registros(*executor, 1), qint64{2});
    QCOMPARE(registros(*executor, 0), qint64{0});
    QCOMPARE(colunaExiste(*executor, "nl_teste_mig_a", "y"), qint64{1});
}

// A V1 aplicada mudou no catálogo: recusa pelo checksum, e o checksum guardado continua o da V1 original.
void TesteMigradorBanco::checksumDivergenteRecusaSemEscrita()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));

    QVERIFY(sql::migrar(*executor, kSoV1).has_value());
    const auto resultado = sql::migrar(*executor, kV1Alterada);
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("migracao.checksum_divergente"));
    QCOMPARE(registros(*executor, 1), qint64{1});
    QCOMPARE(checksumGuardado(*executor, 1),
             QCryptographicHash::hash(QByteArrayLiteral("CREATE TABLE nl_teste_mig_a (x INT)"), QCryptographicHash::Sha256));
}

// Falha de banco no meio (NL-I.351): o DDL da V2 falha, a V2 fica com a marca 0, e a execução seguinte recusa por
// migração interrompida, sem aplicar nada.
void TesteMigradorBanco::ddlQueFalhaDeixaMarcaETravaAsSeguintes()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));

    const auto falha = sql::migrar(*executor, kV2QueFalha);
    QVERIFY(!falha.has_value());
    QVERIFY2(codigoDo(falha.error()).startsWith("sql."), codigoDo(falha.error()).constData());
    QCOMPARE(registros(*executor, 1), qint64{1});
    QCOMPARE(registros(*executor, 0), qint64{1});
    const auto depois = sql::migrar(*executor, kV1EV2);
    QVERIFY(!depois.has_value());
    QCOMPARE(codigoDo(depois.error()), QByteArrayLiteral("migracao.interrompida"));
    QCOMPARE(colunaExiste(*executor, "nl_teste_mig_a", "y"), qint64{0});
}

// Outra sessão segura a trava: recusa depois da espera, sem criar a nl_migracao.
void TesteMigradorBanco::travaOcupadaRecusaSemEscrita()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));
    auto outra = abrirOutra(a);
    QVERIFY(outra.has_value());
    QCOMPARE(tomarTrava(*outra), qint64{1});

    const auto resultado = sql::migrar(*executor, kV1EV2);
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("migracao.trava_ocupada"));
    QCOMPARE(tabelaExiste(*executor, "nl_migracao"), qint64{0});
    QCOMPARE(tabelaExiste(*executor, "nl_teste_mig_a"), qint64{0});
}

// Liberação da trava por queda da sessão (NL-I.233): a sessão que segura a trava é fechada pelo servidor (wait_timeout de
// 1 s, sem o cliente fechar), e o migrador da outra sessão consegue a trava dentro da espera e aplica.
void TesteMigradorBanco::travaLiberadaQuandoASessaoCai()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));
    auto outra = abrirOutra(a);
    QVERIFY(outra.has_value());
    QCOMPARE(tomarTrava(*outra), qint64{1});
    QVERIFY(outra->executar(sql::Consulta::de("SET SESSION wait_timeout = 1"), std::span<const sql::Parametro>{}).has_value());

    const auto resultado = sql::migrar(*executor, kV1EV2);
    QVERIFY2(resultado.has_value(), resultado ? "" : resultado.error().codigo().c_str());
    QCOMPARE(resultado->aplicadas, versoes({1, 2}));
}

// A trava sai em todo caminho de volta: depois do sucesso e depois de uma recusa do plano.
void TesteMigradorBanco::travaLiberadaAoSair()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));
    auto outra = abrirOutra(a);
    QVERIFY(outra.has_value());

    QVERIFY(sql::migrar(*executor, kV1EV2).has_value());
    QCOMPARE(travaLivre(*outra), qint64{1});
    QVERIFY(!sql::migrar(*executor, kSoV1).has_value());
    QCOMPARE(travaLivre(*outra), qint64{1});
}

// Catálogo com nome fora do padrão: recusa antes da trava e da tabela, e o banco não muda.
void TesteMigradorBanco::catalogoInvalidoNaoTocaOBanco()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));

    const auto resultado = sql::migrar(*executor, kNomeInvalido);
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("migracao.nome_invalido"));
    QCOMPARE(tabelaExiste(*executor, "nl_migracao"), qint64{0});
    QCOMPARE(tabelaExiste(*executor, "nl_teste_mig_a"), qint64{0});
}

// Linha ilegível em nl_migracao (parecer do CISO, I-b): quem escreve no banco deixou a V1 com sucesso NULL, numa tabela de
// coluna anulável que o CREATE TABLE IF NOT EXISTS do migrador mantém. Recusa por migração interrompida, sem aplicar nada.
void TesteMigradorBanco::linhaIlegivelRecusaSemEscrita()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));
    QVERIFY(executor->executarDdl(sql::Ddl::de(
        "CREATE TABLE nl_migracao (versao INT NOT NULL PRIMARY KEY, descricao VARCHAR(255) NOT NULL, "
        "checksum BINARY(32) NOT NULL, aplicada_em DATETIME(6) NOT NULL, sucesso TINYINT NULL) "
        "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin")).has_value());
    const QByteArray checksum =
        QCryptographicHash::hash(QByteArrayLiteral("CREATE TABLE nl_teste_mig_a (x INT)"), QCryptographicHash::Sha256);
    const std::array linha{sql::Parametro::texto("cria_a"), sql::Parametro::bytes(checksum)};
    QVERIFY(executor->executar(sql::Consulta::de("INSERT INTO nl_migracao (versao, descricao, checksum, aplicada_em, sucesso) "
                                                 "VALUES (1, ?, ?, UTC_TIMESTAMP(6), NULL)"),
                               linha).has_value());

    const auto resultado = sql::migrar(*executor, kV1EV2);
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("migracao.interrompida"));
    QCOMPARE(tabelaExiste(*executor, "nl_teste_mig_a"), qint64{0});
}

// A marca que não grava não deixa o DDL rodar (parecer do CISO, I-c): a nl_migracao tem CHECK (sucesso = 1), então o
// INSERT da marca com sucesso 0 falha no servidor. Recusa com o erro de execução do executor, e a V1 não roda.
void TesteMigradorBanco::marcaQueFalhaNaoRodaODdl()
{
    const auto a = arranjoDoSlot();
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto executor = abrir(a);
    QVERIFY(executor.has_value());
    QVERIFY(limpar(*executor));
    QVERIFY(executor->executarDdl(sql::Ddl::de(
        "CREATE TABLE nl_migracao (versao INT NOT NULL PRIMARY KEY, descricao VARCHAR(255) NOT NULL, "
        "checksum BINARY(32) NOT NULL, aplicada_em DATETIME(6) NOT NULL, sucesso TINYINT NOT NULL, CHECK (sucesso = 1)) "
        "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin")).has_value());

    const auto resultado = sql::migrar(*executor, kSoV1);
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("sql.execucao"));
    QCOMPARE(tabelaExiste(*executor, "nl_teste_mig_a"), qint64{0});
}

QTEST_GUILESS_MAIN(TesteMigradorBanco)
#include "tst_migrador_banco.moc"
