// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de integração do executor SQL da Foundation sql (NL-3.1a, bloco I): 27 slots, um por comportamento, contra o
// servidor do par. Cada slot abre a sessão pelo Executor, imprime a sua linha NL-TLS antes de qualquer asserção, e
// afirma o resultado da classe do servidor (tabela 1.3 da ordem v1). No R, o Executor é stub: cada slot falha por
// sql.nao_implementado. A sessão de teste por texto (API C, código de teste fora do portão S1) só prepara o cenário.

#include "ambiente_banco.h"
#include "conexao_banco.h"
#include "impressao_banco.h"
#include "modo_tls.h"
#include "registro_conector.h"
#include "tls_da_sessao.h"

#include <nl/base/erro.h>
#include <nl/foundation/sql/consulta.h>
#include <nl/foundation/sql/costura_conexao.h>
#include <nl/foundation/sql/dados_conexao.h>
#include <nl/foundation/sql/executor.h>
#include <nl/foundation/sql/parametro.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QSet>
#include <QString>
#include <QRandomGenerator>
#include <QStringList>
#include <QTemporaryDir>
#include <QtLogging>
#include <QtTest>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <optional>
#include <utility>
#include <utility>
#include <variant>

namespace sql = nl::foundation::sql;

namespace {

// Classe pelo arquivo de tabela (servidorTls e certificado): A é o MySQL com disco (fixado); B é o MariaDB sem TLS
// (aberto); C é o MariaDB com TLS em memória (aberto). Outra combinação é PARADA.
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

// Arranjo comum: dados do ambiente, classe e senha do par. A senha é um QByteArray, e o seu constData() é NUL-terminado.
struct Arranjo {
    std::optional<DadosBanco> dados;
    std::optional<Classe> classe;
    QByteArray senha;
    QByteArray falta;
};

Arranjo arranjoDoSlot()
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

// Modo do executor pela família: MySQL (classe A) vai ao fixado com caching_sha2; MariaDB vai ao aberto com native.
sql::ModoTls modoDaFamilia(const Arranjo &a)
{
    if (*a.classe == Classe::A) {
        return sql::ModoTls(sql::ModoFixado(impressaoParaFixar(*a.dados)));
    }
    return sql::ModoTls(sql::ModoAberto{});
}

QByteArray pluginsDaFamilia(const Arranjo &a)
{
    return *a.classe == Classe::A ? QByteArray("caching_sha2_password") : QByteArray("mysql_native_password");
}

sql::DadosConexao dadosConexaoDo(const Arranjo &a, const QByteArray &plugins, const sql::ModoTls &modo)
{
    return sql::DadosConexao{
        .host = a.dados->host.toUtf8(),
        .porta = static_cast<int>(a.dados->porta),
        .banco = a.dados->base.toUtf8(),
        .usuario = a.dados->usuario.toUtf8(),
        .senhaNulTerminada = a.senha.constData(),
        .pluginsPermitidos = plugins,
        .modo = modo,
    };
}

// Resultado de abrir o executor: a sessão (se abriu) e o código de erro (se não abriu).
struct Abertura {
    std::optional<sql::Executor> executor;
    QByteArray codigo;
};

// Abre pelo Executor e imprime a linha NL-TLS deste slot, antes de qualquer asserção do slot.
Abertura abrir(const char *slot, const Arranjo &a, const QByteArray &plugins, const sql::ModoTls &modo)
{
    Abertura abertura;
    auto resultado = sql::Executor::conectar(dadosConexaoDo(a, plugins, modo));
    if (resultado) {
        const QByteArray cifra = resultado->cifraTlsDaSessao();
        const QByteArray cliente = cifra.isEmpty() ? QByteArrayLiteral("nulo") : cifra;
        qInfo("NL-TLS conexao=%s resultado=tls cliente=%s servidor=nd versao=nd status=nd msg=nenhuma erro=0", slot,
              cliente.constData());
        abertura.executor = std::move(*resultado);
    } else {
        abertura.codigo = QByteArray::fromStdString(resultado.error().codigo());
        qInfo("NL-TLS conexao=%s resultado=recusa cliente=nulo servidor=vazio versao=vazio status=nd msg=%s erro=0",
              slot, abertura.codigo.constData());
    }
    return abertura;
}

// Sessão de teste por texto (API C, código de teste): abre no modo da família, sem o Executor.
ConexaoBanco sessaoDeTeste(const Arranjo &a)
{
    const ModoTls modo = *a.classe == Classe::A ? ModoTls::Fixado : ModoTls::Aberto;
    const QByteArray impressao = *a.classe == Classe::A ? impressaoParaFixar(*a.dados) : QByteArray();
    return conectar(*a.dados, a.senha, modo, impressao).conexao;
}

// Valor de uma coluna da primeira linha, como texto (ou "nulo" para NULL). Vazio se a consulta não devolver linha.
QByteArray primeiraColuna(const sql::Linhas &linhas)
{
    if (linhas.isEmpty() || linhas.front().isEmpty()) {
        return QByteArray();
    }
    const sql::Valor &valor = linhas.front().front();
    if (std::holds_alternative<sql::Nulo>(valor)) {
        return QByteArrayLiteral("nulo");
    }
    if (const auto *inteiro = std::get_if<qint64>(&valor)) {
        return QByteArray::number(*inteiro);
    }
    if (const auto *bytes = std::get_if<QByteArray>(&valor)) {
        return *bytes;
    }
    return std::get<QDate>(valor).toString(Qt::ISODate).toUtf8();
}

QByteArray codigoDo(const nl::base::Erro &erro)
{
    return QByteArray::fromStdString(erro.codigo());
}

// Texto do Erro para a asserção: código e mensagem. Usuário, base, host e senha da conexão saem da mensagem, trocados
// por marcas, para que o log do teste não leve dado da conta (v1k).
QByteArray textoDoErro(const nl::base::Erro &erro, const Arranjo &a)
{
    QString texto = QString::fromStdString(erro.codigo()) + QStringLiteral(" ") + QString::fromStdString(erro.mensagem());
    if (a.dados) {
        if (!a.dados->usuario.isEmpty()) texto.replace(a.dados->usuario, QStringLiteral("<usuario>"));
        if (!a.dados->base.isEmpty()) texto.replace(a.dados->base, QStringLiteral("<base>"));
        if (!a.dados->host.isEmpty()) texto.replace(a.dados->host, QStringLiteral("<host>"));
    }
    if (!a.senha.isEmpty()) texto.replace(QString::fromUtf8(a.senha), QStringLiteral("<senha>"));
    return texto.toUtf8();
}

} // namespace

// Falha antes do executor (ambiente ou preparo do cenário): o slot imprime a sua NL-TLS antes da asserção que falha,
// como faz quando o Executor recusa. O nome do slot vem do QTest, e o motivo não tem espaço.
namespace {
void recusaDePreparo(const char *motivo)
{
    qInfo("NL-TLS conexao=%s resultado=recusa cliente=nulo servidor=vazio versao=vazio status=nd msg=%s erro=0",
          QTest::currentTestFunction(), motivo);
}
} // namespace

// Helpers da costura de teste (v1j): leem a sessão do conector pela API C. O produto não tem nenhuma delas.
namespace {
unsigned long capacidadesDoCliente(const sql::Executor &executor)
{
    unsigned long capacidades = 0;
    mariadb_get_infov(nl::foundation::sql::costura::sessaoDoConector(executor), MARIADB_CONNECTION_CLIENT_CAPABILITIES,
                      &capacidades);
    return capacidades;
}

int opcaoLocalInfile(const sql::Executor &executor)
{
    unsigned int valor = 1;
    mysql_get_optionv(nl::foundation::sql::costura::sessaoDoConector(executor), MYSQL_OPT_LOCAL_INFILE, &valor);
    return static_cast<int>(valor);
}

// Quantos dos primeiros `tamanho` bytes da cópia da senha feita pelo conector não são zero (NL-3.1l; parecer do CISO,
// R4): o teste confere todos os bytes, não só o primeiro. Sessão ou ponteiro nulo devolve -1.
int bytesNaoZeradosDaSenhaDoConector(const sql::Executor &executor, qsizetype tamanho)
{
    const st_mysql *sessao = nl::foundation::sql::costura::sessaoDoConector(executor);
    if (sessao == nullptr || sessao->passwd == nullptr) {
        return -1;
    }
    int naoZerados = 0;
    for (qsizetype i = 0; i < tamanho; ++i) {
        if (sessao->passwd[i] != '\0') {
            ++naoZerados;
        }
    }
    return naoZerados;
}

bool ponteiroDaSenhaNulo(const sql::Executor &executor)
{
    const st_mysql *sessao = nl::foundation::sql::costura::sessaoDoConector(executor);
    return sessao == nullptr || sessao->passwd == nullptr;
}
} // namespace

// Captura das mensagens do Qt durante o slot (v1n, m25). Cada mensagem segue para o manipulador anterior, para que o log
// do harness (NL-TLS, NL-CONECTOR e o prefixo do QTest) continue igual.
namespace {
QStringList mensagensCapturadas;
QtMessageHandler manipuladorAnterior = nullptr;

void capturarMensagem(QtMsgType tipo, const QMessageLogContext &contexto, const QString &mensagem)
{
    mensagensCapturadas << mensagem;
    if (manipuladorAnterior != nullptr) {
        manipuladorAnterior(tipo, contexto, mensagem);
    } else {
        // Sem handler anterior, a mensagem sai no stderr no mesmo formato do Qt: nenhuma linha NL-TLS some.
        std::fprintf(stderr, "%s\n", qPrintable(qFormatLogMessage(tipo, contexto, mensagem)));
    }
}

// Instala a captura no começo do slot e devolve o manipulador anterior ao fim, mesmo se o slot sair cedo.
struct CapturaDeMensagens {
    CapturaDeMensagens() { manipuladorAnterior = qInstallMessageHandler(capturarMensagem); }
    ~CapturaDeMensagens() { qInstallMessageHandler(manipuladorAnterior); }
};
} // namespace

class TesteExecutorBanco : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void conexaoNoModoDaFamilia();
    void conexaoLocalSemTls();
    void modoFixadoImpressaoCertaAbre();
    void modoFixadoImpressaoTrocadaRecusa();
    void impressaoTrocadaComVariavelDeAmbienteRecusa();
    void metodoForaDaListaRecusado();
    void modoFixadoAutenticaComCacheVazio();
    void parametroComAspasNaoInjeta();
    void blobTodosOsBytesIdaEVolta();
    void textoComNulNoMeioIdaEVolta();
    void textoComTamanhoExatoDoBuffer();
    void textoMaiorQueOBufferVoltaInteiro();
    void utf8mb4QuatroBytesIdaEVolta();
    void valorLongoRecusadoNoModoEstrito();
    void quantidadeDeParametrosDiferenteRecusada();
    void rePrepareSemNovoVinculoRecusado();
    void multiStatementsDesligado();
    void localInfileDesligado();
    void arquivoDeOpcoesIgnorado();
    void quedaDeConexaoViraErro();
    void transacaoComFalhaDesfaz();
    void transacaoComSucessoGrava();
    void ddlEmTransacaoRecusado();
    void transacaoAninhadaRecusada();
    void senhaNaoApareceEmErroNemLog();
    void dataNaoMudaComFusoDaSessao();
    void senhaDoConectorZeradaDepoisDeConectar();
};

// Registro do Connector/C deste processo (G-R do CISO): uma linha NL-CONECTOR por processo, antes das asserções.
void TesteExecutorBanco::initTestCase()
{
    const QString cabecalho = QStringLiteral(MARIADB_PACKAGE_VERSION);
    const QString execucao = QString::fromUtf8(mysql_get_client_info());
    registrarNlConector(cabecalho, execucao, QStringLiteral("nd"), QStringLiteral(NL_MARIADB_PREFIXO_DO_BUILD));
    QVERIFY2(execucao == cabecalho, "libmariadb carregado com versão diferente da do cabeçalho compilado: PARADA ao CISO");
}

// Sessão no modo da família, com a linha NL-INIT (literal do MYSQL_INIT_COMMAND) e a NL-SHOW (marcador ? no SHOW), ambas
// impressas pela sessão de teste antes do executor. No R, o executor é stub e a abertura falha.
void TesteExecutorBanco::conexaoNoModoDaFamilia()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());

    auto sessao = sessaoDeTeste(a);
    const char init[] = "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION', time_zone='+00:00', "
                        "collation_connection='utf8mb4_bin'";
    const bool aceitoInit = sessao && mysql_query(sessao.get(), init) == 0;
    qInfo("NL-INIT aceito=%d erro=%u", aceitoInit ? 1 : 0, sessao ? mysql_errno(sessao.get()) : 0u);
    bool aceitoShow = false;
    if (sessao) {
        MYSQL_STMT *stmt = mysql_stmt_init(sessao.get());
        aceitoShow = stmt != nullptr && mysql_stmt_prepare(stmt, "SHOW SESSION STATUS LIKE ?", -1) == 0;
        if (stmt != nullptr) {
            mysql_stmt_close(stmt);
        }
    }
    qInfo("NL-SHOW aceito=%d", aceitoShow ? 1 : 0);

    auto ab = abrir("conexaoNoModoDaFamilia", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    QVERIFY2(!ab.executor->cifraTlsDaSessao().isEmpty() == (*a.classe == Classe::A),
             "a cifra de TLS do cliente só existe na classe A (MySQL, fixado): PARADA ao CISO");
}

void TesteExecutorBanco::conexaoLocalSemTls()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("conexaoLocalSemTls", a, QByteArrayLiteral("mysql_native_password"),
                          sql::ModoTls(sql::ModoAberto{}));
    if (*a.classe == Classe::A) {
        QVERIFY2(!ab.executor.has_value(), "classe A abriu sem TLS no aberto com a lista native: PARADA ao CISO");
        QCOMPARE(ab.codigo, QByteArrayLiteral("sql.conexao_recusada"));
        return;
    }
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    QVERIFY2(ab.executor->cifraTlsDaSessao().isEmpty(), "modo aberto com TLS na sessão: PARADA ao CISO");
}

void TesteExecutorBanco::modoFixadoImpressaoCertaAbre()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("modoFixadoImpressaoCertaAbre", a, pluginsDaFamilia(a),
                          sql::ModoTls(sql::ModoFixado(impressaoParaFixar(*a.dados))));
    if (*a.classe == Classe::B) {
        QVERIFY2(!ab.executor.has_value(), "classe B abriu no fixado sem TLS no servidor: PARADA ao CISO");
        QCOMPARE(ab.codigo, QByteArrayLiteral("sql.tls_recusado"));
        return;
    }
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    // Classes A e C: o modo fixado abre com TLS na sessão (tabela 1.3; NL-3.1l, parecer do CISO, R5).
    QVERIFY2(!ab.executor->cifraTlsDaSessao().isEmpty(), "modo fixado abriu sem TLS na sessão: PARADA ao CISO");
}

void TesteExecutorBanco::modoFixadoImpressaoTrocadaRecusa()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("modoFixadoImpressaoTrocadaRecusa", a, pluginsDaFamilia(a),
                          sql::ModoTls(sql::ModoFixado(impressaoTrocada(impressaoParaFixar(*a.dados)))));
    QVERIFY2(!ab.executor.has_value(), "impressão trocada abriu sessão: PARADA ao CISO");
    QCOMPARE(ab.codigo, QByteArrayLiteral("sql.tls_recusado"));
}

void TesteExecutorBanco::impressaoTrocadaComVariavelDeAmbienteRecusa()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    qputenv("MARIADB_TLS_DISABLE_PEER_VERIFICATION", "1");
    auto ab = abrir("impressaoTrocadaComVariavelDeAmbienteRecusa", a, pluginsDaFamilia(a),
                          sql::ModoTls(sql::ModoFixado(impressaoTrocada(impressaoParaFixar(*a.dados)))));
    qunsetenv("MARIADB_TLS_DISABLE_PEER_VERIFICATION");
    QVERIFY2(!ab.executor.has_value(), "impressão trocada abriu com a variável no ambiente: PARADA ao CISO");
    QCOMPARE(ab.codigo, QByteArrayLiteral("sql.tls_recusado"));
}

void TesteExecutorBanco::metodoForaDaListaRecusado()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    // Classe A: fixado com a lista native (fora do caching_sha2). Classes B e C: aberto com client_ed25519.
    auto ab = *a.classe == Classe::A
        ? abrir("metodoForaDaListaRecusado", a, QByteArrayLiteral("mysql_native_password"),
                sql::ModoTls(sql::ModoFixado(impressaoParaFixar(*a.dados))))
        : abrir("metodoForaDaListaRecusado", a, QByteArrayLiteral("client_ed25519"),
                sql::ModoTls(sql::ModoAberto{}));
    QVERIFY2(!ab.executor.has_value(), "método fora da lista abriu sessão: PARADA ao CISO");
    QCOMPARE(ab.codigo, QByteArrayLiteral("sql.conexao_recusada"));
}

// Troca a senha da própria conta (mesma senha) pela sessão de teste, para esvaziar o cache de autenticação; depois o
// executor abre no modo da família. Só a classe A tem cache do caching_sha2.
void TesteExecutorBanco::modoFixadoAutenticaComCacheVazio()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto sessao = sessaoDeTeste(a);
    if (!sessao) {
        recusaDePreparo("sessao_de_teste");
    }
    QVERIFY2(sessao != nullptr, "sessão de teste não abriu para trocar a senha da conta");
    QByteArray escapada(2 * a.senha.size() + 1, '\0');
    const unsigned long tamanho = mysql_real_escape_string(sessao.get(), escapada.data(), a.senha.constData(),
                                                           static_cast<unsigned long>(a.senha.size()));
    escapada.resize(static_cast<qsizetype>(tamanho));
    // Classe A (MySQL, caching_sha2): ALTER USER. Classes B e C (MariaDB, native): SET PASSWORD, que a conta pode usar
    // sem privilégio novo (v1i, CTO). A forma sai na linha NL-TROCA, sempre.
    const bool classeA = *a.classe == Classe::A;
    const QByteArray troca = classeA
        ? QByteArrayLiteral("ALTER USER CURRENT_USER() IDENTIFIED BY '") + escapada + "'"
        : QByteArrayLiteral("SET PASSWORD = PASSWORD('") + escapada + "')";
    const bool trocou = mysql_query(sessao.get(), troca.constData()) == 0;
    qInfo("NL-TROCA forma=%s erro=%u", classeA ? "alter_user" : "set_password", mysql_errno(sessao.get()));
    if (!trocou) {
        recusaDePreparo("troca_senha");
    }
    QVERIFY2(trocou, "a conta de teste não pôde trocar a própria senha: PARADA ao CTO (NL-TROCA, erro do servidor)");
    sessao.reset();
    auto ab = abrir("modoFixadoAutenticaComCacheVazio", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
}

// Parâmetro com payload de injeção: volta igual, a tabela continua e tem uma linha.
void TesteExecutorBanco::parametroComAspasNaoInjeta()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("parametroComAspasNaoInjeta", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    auto &executor = *ab.executor;
    {
        const auto ddl = executor.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_aspas (x VARCHAR(64))"));
        QVERIFY2(ddl.has_value(), ddl.has_value() ? "" : textoDoErro(ddl.error(), a).constData());
    }
    for (const char *payload : {"x'); DROP TABLE t; --", "' OR '1'='1"}) {
        QVERIFY(executor.executar(sql::Consulta::de("INSERT INTO nl_aspas (x) VALUES (?)"),
                                  std::array{sql::Parametro::texto(QByteArray(payload))}).has_value());
        const auto linhas = executor.consultar(sql::Consulta::de("SELECT x FROM nl_aspas WHERE x = ?"),
                                               std::array{sql::Parametro::texto(QByteArray(payload))});
        QVERIFY(linhas.has_value());
        QCOMPARE(primeiraColuna(*linhas), QByteArray(payload));
    }
}

void TesteExecutorBanco::blobTodosOsBytesIdaEVolta()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("blobTodosOsBytesIdaEVolta", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    QByteArray todos;
    for (int i = 0; i < 256; ++i) {
        todos.append(static_cast<char>(i));
    }
    auto &executor = *ab.executor;
    {
        const auto ddl = executor.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_blob (b BLOB)"));
        QVERIFY2(ddl.has_value(), ddl.has_value() ? "" : textoDoErro(ddl.error(), a).constData());
    }
    for (int rodada = 0; rodada < 4; ++rodada) {
        QVERIFY(executor.executar(sql::Consulta::de("INSERT INTO nl_blob (b) VALUES (?)"),
                                  std::array{sql::Parametro::bytes(todos)}).has_value());
    }
    QVERIFY(executor.executar(sql::Consulta::de("INSERT INTO nl_blob (b) VALUES (?)"),
                              std::array{sql::Parametro::bytes(QByteArray())}).has_value());
    const auto linhas = executor.consultar(sql::Consulta::de("SELECT b FROM nl_blob ORDER BY b"), {});
    QVERIFY(linhas.has_value());
    QCOMPARE(linhas->size(), qsizetype(5));
}

void TesteExecutorBanco::textoComNulNoMeioIdaEVolta()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("textoComNulNoMeioIdaEVolta", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const QByteArray comNul("a\0b", 3);
    const auto linhas = ab.executor->consultar(sql::Consulta::de("SELECT ?"),
                                               std::array{sql::Parametro::texto(comNul)});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), comNul);
}

void TesteExecutorBanco::textoComTamanhoExatoDoBuffer()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("textoComTamanhoExatoDoBuffer", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    for (const int tamanho : {255, 256, 257}) {
        const QByteArray texto(tamanho, 'x');
        const auto linhas = ab.executor->consultar(sql::Consulta::de("SELECT ?"),
                                                   std::array{sql::Parametro::texto(texto)});
        QVERIFY(linhas.has_value());
        QCOMPARE(primeiraColuna(*linhas), texto);
    }
}

void TesteExecutorBanco::textoMaiorQueOBufferVoltaInteiro()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("textoMaiorQueOBufferVoltaInteiro", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const QByteArray grande(1 << 20, 'y');
    auto &executor = *ab.executor;
    {
        const auto ddl = executor.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_grande (t MEDIUMTEXT, b MEDIUMBLOB)"));
        QVERIFY2(ddl.has_value(), ddl.has_value() ? "" : textoDoErro(ddl.error(), a).constData());
    }
    QVERIFY(executor.executar(sql::Consulta::de("INSERT INTO nl_grande (t, b) VALUES (?, ?)"),
                              std::array{sql::Parametro::texto(grande), sql::Parametro::bytes(grande)}).has_value());
    const auto linhas = executor.consultar(sql::Consulta::de("SELECT t FROM nl_grande"), {});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), grande);
}

void TesteExecutorBanco::utf8mb4QuatroBytesIdaEVolta()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("utf8mb4QuatroBytesIdaEVolta", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const QByteArray quatroBytes("\xF0\x9F\x98\x80", 4);
    const auto linhas = ab.executor->consultar(sql::Consulta::de("SELECT ?"),
                                               std::array{sql::Parametro::texto(quatroBytes)});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), quatroBytes);
}

void TesteExecutorBanco::valorLongoRecusadoNoModoEstrito()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("valorLongoRecusadoNoModoEstrito", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    auto &executor = *ab.executor;
    {
        const auto ddl = executor.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_curto (v VARCHAR(4))"));
        QVERIFY2(ddl.has_value(), ddl.has_value() ? "" : textoDoErro(ddl.error(), a).constData());
    }
    const auto insercao = executor.executar(sql::Consulta::de("INSERT INTO nl_curto (v) VALUES (?)"),
                                            std::array{sql::Parametro::texto(QByteArray("12345"))});
    QVERIFY(!insercao.has_value());
    const auto linhas = executor.consultar(sql::Consulta::de("SELECT v FROM nl_curto"), {});
    QVERIFY(linhas.has_value());
    QCOMPARE(linhas->size(), qsizetype(0));
}

void TesteExecutorBanco::quantidadeDeParametrosDiferenteRecusada()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("quantidadeDeParametrosDiferenteRecusada", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const auto dois = ab.executor->consultar(sql::Consulta::de("SELECT ?, ?"),
                                             std::array{sql::Parametro::inteiro(1)});
    QVERIFY(!dois.has_value());
    QCOMPARE(codigoDo(dois.error()), QByteArrayLiteral("sql.vinculo_quantidade"));
    const auto tres = ab.executor->consultar(sql::Consulta::de("SELECT ?"),
                                             std::array{sql::Parametro::inteiro(1), sql::Parametro::inteiro(2),
                                                        sql::Parametro::inteiro(3)});
    QVERIFY(!tres.has_value());
    QCOMPARE(codigoDo(tres.error()), QByteArrayLiteral("sql.vinculo_quantidade"));
}

// Reexecução de um preparado sem novo vínculo: a costura do motor reexecuta e tem de devolver sql.sem_vinculo.
void TesteExecutorBanco::rePrepareSemNovoVinculoRecusado()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("rePrepareSemNovoVinculoRecusado", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const auto reexecutado = nl::foundation::sql::costura::reexecutarPreparadoSemVinculo(*ab.executor);
    QVERIFY(!reexecutado.has_value());
    QCOMPARE(codigoDo(reexecutado.error()), QByteArrayLiteral("sql.sem_vinculo"));
}

void TesteExecutorBanco::multiStatementsDesligado()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("multiStatementsDesligado", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    QCOMPARE(capacidadesDoCliente(*ab.executor) & (1UL << 16), 0UL);
}

void TesteExecutorBanco::localInfileDesligado()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("localInfileDesligado", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    QCOMPARE(opcaoLocalInfile(*ab.executor), 0);                          // opção de sessão (mysql_get_optionv)
    QCOMPARE(capacidadesDoCliente(*ab.executor) & (1UL << 7), 0UL);       // capacidade anunciada (mariadb_get_infov)
}

// HOME temporário com um .my.cnf que tenta um init-command: a variável de sessão tem de voltar NULL.
void TesteExecutorBanco::arquivoDeOpcoesIgnorado()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    QTemporaryDir casa;
    if (!casa.isValid()) {
        recusaDePreparo("casa_temporaria");
    }
    QVERIFY(casa.isValid());
    QFile cnf(QDir(casa.path()).filePath(QStringLiteral(".my.cnf")));
    if (!cnf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        recusaDePreparo("arquivo_cnf");
    }
    QVERIFY(cnf.isOpen());
    cnf.write("[client]\ninit-command=SET @nl_arquivo_de_opcoes=1\n");
    cnf.close();
    qputenv("HOME", casa.path().toUtf8());
    auto ab = abrir("arquivoDeOpcoesIgnorado", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const auto linhas = ab.executor->consultar(sql::Consulta::de("SELECT @nl_arquivo_de_opcoes"), {});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), QByteArrayLiteral("nulo"));
}

// KILL pela sessão de teste no CONNECTION_ID que o executor leu: a chamada seguinte tem de ser sql.conexao_perdida.
void TesteExecutorBanco::quedaDeConexaoViraErro()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("quedaDeConexaoViraErro", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const auto id = ab.executor->consultar(sql::Consulta::de("SELECT CONNECTION_ID()"), {});
    QVERIFY(id.has_value());
    auto sessao = sessaoDeTeste(a);
    QVERIFY(sessao != nullptr);
    const QByteArray kill = QByteArrayLiteral("KILL ") + primeiraColuna(*id);
    QVERIFY2(mysql_query(sessao.get(), kill.constData()) == 0, "KILL pela sessão de teste falhou");
    const auto depois = ab.executor->consultar(sql::Consulta::de("SELECT 1"), {});
    QVERIFY(!depois.has_value());
    QCOMPARE(codigoDo(depois.error()), QByteArrayLiteral("sql.conexao_perdida"));
}

void TesteExecutorBanco::transacaoComFalhaDesfaz()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("transacaoComFalhaDesfaz", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    auto &executor = *ab.executor;
    {
        const auto ddl = executor.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_tx (x INT)"));
        QVERIFY2(ddl.has_value(), ddl.has_value() ? "" : textoDoErro(ddl.error(), a).constData());
    }
    const auto resultado = executor.transacao([](sql::Executor &e) -> std::expected<void, nl::base::Erro> {
        if (!e.executar(sql::Consulta::de("INSERT INTO nl_tx (x) VALUES (1)"), {}).has_value()) {
            return std::unexpected(nl::base::Erro("sql.execucao", std::nullopt, "falha de teste"));
        }
        return std::unexpected(nl::base::Erro("sql.transacao", std::nullopt, "corpo recusado de propósito"));
    });
    QVERIFY(!resultado.has_value());
    const auto linhas = executor.consultar(sql::Consulta::de("SELECT COUNT(*) FROM nl_tx"), {});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), QByteArrayLiteral("0"));
}

void TesteExecutorBanco::transacaoComSucessoGrava()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("transacaoComSucessoGrava", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    auto &executor = *ab.executor;
    {
        const auto ddl = executor.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_tx2 (x INT)"));
        QVERIFY2(ddl.has_value(), ddl.has_value() ? "" : textoDoErro(ddl.error(), a).constData());
    }
    const auto resultado = executor.transacao([](sql::Executor &e) -> std::expected<void, nl::base::Erro> {
        if (!e.executar(sql::Consulta::de("INSERT INTO nl_tx2 (x) VALUES (1)"), {}).has_value()) {
            return std::unexpected(nl::base::Erro("sql.execucao", std::nullopt, "falha de teste"));
        }
        return {};
    });
    QVERIFY(resultado.has_value());
    const auto linhas = executor.consultar(sql::Consulta::de("SELECT COUNT(*) FROM nl_tx2"), {});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), QByteArrayLiteral("1"));
}

void TesteExecutorBanco::ddlEmTransacaoRecusado()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("ddlEmTransacaoRecusado", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    auto &executor = *ab.executor;
    const auto resultado = executor.transacao([](sql::Executor &e) -> std::expected<void, nl::base::Erro> {
        return e.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_ddl (x INT)"));
    });
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("sql.ddl_em_transacao"));
}

void TesteExecutorBanco::transacaoAninhadaRecusada()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("transacaoAninhadaRecusada", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    auto &executor = *ab.executor;
    const auto resultado = executor.transacao([](sql::Executor &e) -> std::expected<void, nl::base::Erro> {
        return e.transacao([](sql::Executor &) -> std::expected<void, nl::base::Erro> { return {}; });
    });
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDo(resultado.error()), QByteArrayLiteral("sql.transacao_aninhada"));
}

// Marca única na senha: nenhuma mensagem de erro do executor a contém (a mensagem é apresentação; o log também).
void TesteExecutorBanco::senhaNaoApareceEmErroNemLog()
{
    CapturaDeMensagens captura; // v1n: todas as mensagens do slot, do início ao fim
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    const QString marca = QString::fromUtf8(a.senha);

    // Falha de conexão com uma senha-marca (nl-marca- mais 16 hex aleatórios): a mensagem do erro não contém a marca.
    const QString hex = QStringLiteral("%1").arg(QRandomGenerator::global()->generate64(), 16, 16, QLatin1Char('0'));
    const QString senhaMarca = QStringLiteral("nl-marca-") + hex;
    sql::DadosConexao dadosRuins = dadosConexaoDo(a, pluginsDaFamilia(a), modoDaFamilia(a));
    const QByteArray senhaMarcaBytes = senhaMarca.toUtf8();
    dadosRuins.senhaNulTerminada = senhaMarcaBytes.constData();
    const auto falhou = sql::Executor::conectar(dadosRuins);
    QVERIFY(!falhou.has_value());
    QVERIFY(!QString::fromStdString(falhou.error().mensagem()).contains(senhaMarca));

    // Trecho de hoje: conexão certa e consulta que falha, sem a senha na mensagem.
    auto ab = abrir("senhaNaoApareceEmErroNemLog", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    const auto erro = ab.executor->consultar(sql::Consulta::de("SELECT * FROM tabela_que_nao_existe_nl"), {});
    QVERIFY(!erro.has_value());
    QVERIFY(!QString::fromStdString(erro.error().mensagem()).contains(marca));

    // Log capturado: nenhuma mensagem do slot contém a senha-marca nem a senha certa.
    for (const QString &mensagem : std::as_const(mensagensCapturadas)) {
        QVERIFY2(!mensagem.contains(senhaMarca), "a senha-marca apareceu no log");
        QVERIFY2(!mensagem.contains(marca), "a senha certa apareceu no log");
    }
}

// DATE e BIGINT gravados pelo executor, lidos por uma sessão de teste com fusos diferentes: iguais nos dois.
void TesteExecutorBanco::dataNaoMudaComFusoDaSessao()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("dataNaoMudaComFusoDaSessao", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    auto &executor = *ab.executor;
    {
        const auto ddl = executor.executarDdl(sql::Ddl::de("CREATE TEMPORARY TABLE nl_data (d DATE, n BIGINT)"));
        QVERIFY2(ddl.has_value(), ddl.has_value() ? "" : textoDoErro(ddl.error(), a).constData());
    }
    QVERIFY(executor.executar(sql::Consulta::de("INSERT INTO nl_data (d, n) VALUES (?, ?)"),
                              std::array{sql::Parametro::data(QDate(2026, 11, 21)), sql::Parametro::inteiro(7)})
                .has_value());
    const auto linhas = executor.consultar(sql::Consulta::de("SELECT d FROM nl_data"), {});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), QByteArrayLiteral("2026-11-21"));
}

// A senha do conector é zerada depois de conectar (a cópia MYSQL::passwd): ponteiro não nulo, primeiro byte zero.
// Depois disso, na mesma conexão, SELECT ? com 7 volta 7.
void TesteExecutorBanco::senhaDoConectorZeradaDepoisDeConectar()
{
    const auto a = arranjoDoSlot();
    if (!a.falta.isEmpty()) {
        recusaDePreparo("arranjo");
    }
    QVERIFY2(a.falta.isEmpty(), a.falta.constData());
    auto ab = abrir("senhaDoConectorZeradaDepoisDeConectar", a, pluginsDaFamilia(a), modoDaFamilia(a));
    QVERIFY2(ab.executor.has_value(), ab.codigo.constData());
    qInfo("NL-SENHA-PTR nulo=%d", ponteiroDaSenhaNulo(*ab.executor) ? 1 : 0);
    QVERIFY(!ponteiroDaSenhaNulo(*ab.executor));
    QVERIFY(!a.senha.isEmpty());
    QCOMPARE(bytesNaoZeradosDaSenhaDoConector(*ab.executor, a.senha.size()), 0);
    const auto linhas = ab.executor->consultar(sql::Consulta::de("SELECT ?"),
                                               std::array{sql::Parametro::inteiro(7)});
    QVERIFY(linhas.has_value());
    QCOMPARE(primeiraColuna(*linhas), QByteArrayLiteral("7"));
}

QTEST_GUILESS_MAIN(TesteExecutorBanco)
#include "tst_executor_banco.moc"
