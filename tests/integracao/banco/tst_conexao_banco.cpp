// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste de integração com o banco (NL-3.1h, critério 4 de TLS, C4'): quatro conexões por par, uma por slot. Cada slot
// registra a sua linha NL-TLS antes das asserções e afirma o resultado da tabela da classe do servidor. Falta de
// variável ou de senha faz o teste falhar com mensagem; nunca pula.
// Classe fixada por coluna (servidores_banco.txt): (sim, disco), (sim, memoria) ou (nao, nenhum). Outra combinação falha.
#include <QtTest>

#include "ambiente_banco.h"
#include "conexao_banco.h"
#include "impressao_banco.h"
#include "modo_tls.h"
#include "registro_tls.h"
#include "tls_da_sessao.h"
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QSet>

#include <optional>

namespace {

enum class Classe { Disco, Memoria, Nenhum };

// Classe do servidor pela coluna fixada (tls e certificado). Qualquer outra combinação devolve nada.
std::optional<Classe> classeDoServidor(const DadosBanco &dados)
{
    if (dados.servidorTls == QStringLiteral("sim") && dados.certificado == QStringLiteral("disco")) {
        return Classe::Disco;
    }
    if (dados.servidorTls == QStringLiteral("sim") && dados.certificado == QStringLiteral("memoria")) {
        return Classe::Memoria;
    }
    if (dados.servidorTls == QStringLiteral("nao") && dados.certificado == QStringLiteral("nenhum")) {
        return Classe::Nenhum;
    }
    return std::nullopt;
}

// Sessão com TLS nos dois lados: o cliente tem cifra, o servidor tem a mesma cifra, nenhum dos dois é vazio nem erro.
bool mesmaCifraNosDoisLados(const QString &cliente, const QString &servidor)
{
    return cliente != QStringLiteral("nulo") && servidor != QStringLiteral("vazio")
        && servidor != QStringLiteral("ERRO") && cliente == servidor;
}

// SELECT 1 na sessão aberta: verdadeiro só se a consulta roda e devolve resultado.
bool selectUmOk(MYSQL *conexao)
{
    if (mysql_query(conexao, "SELECT 1") != 0) {
        return false;
    }
    MYSQL_RES *resultado = mysql_store_result(conexao);
    if (resultado == nullptr) {
        return false;
    }
    mysql_free_result(resultado);
    return true;
}

// Arranjo comum dos quatro slots (L-33, regra de 3): dados do ambiente, classe do servidor e senha do par. A primeira falta
// vai em "falta", com a mesma mensagem que cada slot dava antes; vazia quando o arranjo está completo.
struct ArranjoBanco {
    std::optional<DadosBanco> dados;
    std::optional<Classe> classe;
    std::optional<QByteArray> senha;
    QByteArray falta;
};

ArranjoBanco arranjoDoBanco()
{
    ArranjoBanco arranjo;
    arranjo.dados = dadosDoAmbiente();
    if (!arranjo.dados) {
        arranjo.falta = QByteArrayLiteral("NL_BANCO_* ausente ou inválido: o teste do banco falha, nunca pula");
        return arranjo;
    }
    arranjo.classe = classeDoServidor(*arranjo.dados);
    if (!arranjo.classe) {
        arranjo.falta = QByteArrayLiteral("classe do servidor fora da tabela (servidorTls e certificado): PARADA");
        return arranjo;
    }
    arranjo.senha = senhaDoArquivo(arranjo.dados->arquivoSenha);
    if (!arranjo.senha) {
        arranjo.falta = QByteArrayLiteral("arquivo de senha do par ausente ou vazio");
    }
    return arranjo;
}

// Caminho real do libmariadb mapeado neste processo (/proc/self/maps, canônico), ou "nd" se não houver exatamente um.
QString bibliotecaCarregada()
{
    QFile mapas(QStringLiteral("/proc/self/maps"));
    if (!mapas.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("nd");
    }
    QSet<QString> caminhos;
    const QList<QByteArray> linhas = mapas.readAll().split('\n');
    for (const QByteArray &linha : linhas) {
        const qsizetype barra = linha.indexOf('/');
        if (barra >= 0 && linha.contains("/libmariadb.so")) {
            caminhos.insert(QFileInfo(QString::fromUtf8(linha.mid(barra))).canonicalFilePath());
        }
    }
    return caminhos.size() == 1 ? *caminhos.cbegin() : QStringLiteral("nd");
}

} // namespace

class TesteConexaoBanco : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void sessaoPadrao();
    void modoAberto();
    void modoFixadoImpressaoCerta();
    void modoFixadoImpressaoTrocada();
};

// G-R do CISO (emenda 1, 3.2): o Connector/C carregado na execução é o do cabeçalho compilado, e o arquivo real dele mora
// no prefixo do build. A linha NL-CONECTOR sai antes das asserções, uma por processo; o harness a conta como a NL-TLS.
void TesteConexaoBanco::initTestCase()
{
    const QString cabecalho = QStringLiteral(MARIADB_PACKAGE_VERSION);
    const QString execucao = QString::fromUtf8(mysql_get_client_info());
    const QString biblioteca = bibliotecaCarregada();
    const QString prefixo = QFileInfo(QStringLiteral(NL_MARIADB_PREFIXO_DO_BUILD)).canonicalFilePath();
    const QByteArray cabecalhoBytes = cabecalho.toUtf8();
    const QByteArray execucaoBytes = execucao.toUtf8();
    const QByteArray bibliotecaBytes = biblioteca.toUtf8();
    const QByteArray prefixoBytes = prefixo.isEmpty() ? QByteArrayLiteral("nd") : prefixo.toUtf8();
    qInfo("NL-CONECTOR cabecalho=%s execucao=%s biblioteca=%s prefixo=%s", cabecalhoBytes.constData(),
          execucaoBytes.constData(), bibliotecaBytes.constData(), prefixoBytes.constData());
    QVERIFY2(execucao == cabecalho, "libmariadb carregado com versão diferente da do cabeçalho compilado: PARADA ao CISO");
    QVERIFY2(!prefixo.isEmpty() && biblioteca.startsWith(prefixo + QLatin1Char('/')),
             "libmariadb carregado fora do prefixo do build: PARADA ao CISO");
}

// (a) Sem opção de TLS (registro da biblioteca): sessão com a mesma cifra nos dois lados, ou recusa 2026. Sem TLS no
// servidor, a recusa tem de dizer "SSL is required". Com TLS no servidor, só passa a sessão aberta com status 1 (só o bit
// TRUST), e qualquer recusa é PARADA ao CISO (C2 da emenda 1).
void TesteConexaoBanco::sessaoPadrao()
{
    const auto [dados, classe, senha, falta] = arranjoDoBanco();
    QVERIFY2(falta.isEmpty(), falta.constData());

    const auto resultado = conectar(*dados, *senha, ModoTls::Padrao, QByteArray());
    registrarTls("sessaoPadrao", resultado);
    if (!resultado.conexao) {
        QVERIFY2(*classe == Classe::Nenhum, "(a) recusa no padrão com TLS no servidor: PARADA ao CISO (status e marca na NL-TLS)");
        QCOMPARE(resultado.erro, 2026u);
        QVERIFY2(resultado.mensagem.contains(QStringLiteral("SSL is required")),
                 "(a) sem TLS no servidor: a recusa tem de dizer SSL is required");
        return;
    }
    QVERIFY2(*classe != Classe::Nenhum, "(a) sessão aberta numa classe sem TLS no servidor: PARADA");
    QVERIFY2(mesmaCifraNosDoisLados(tlsDoCliente(resultado.conexao.get()), tlsDoServidor(resultado.conexao.get())),
             "(a) os dois lados não têm a mesma cifra de TLS");
    QVERIFY2(statusDaVerificacao(resultado.conexao.get()) == QStringLiteral("1"),
             "(a) padrão com TLS: o status tem de ser 1, só o bit TRUST (C2 do CISO): PARADA ao CISO");
    QVERIFY2(selectUmOk(resultado.conexao.get()), "SELECT 1 falhou na sessão do par");
}

// (b) Modo aberto: sem verificação e sem obrigação. Tem de abrir sem TLS nas três classes. TLS aqui é PARADA.
void TesteConexaoBanco::modoAberto()
{
    const auto [dados, classe, senha, falta] = arranjoDoBanco();
    QVERIFY2(falta.isEmpty(), falta.constData());

    const auto resultado = conectar(*dados, *senha, ModoTls::Aberto, QByteArray());
    registrarTls("modoAberto", resultado);
    QVERIFY2(resultado.conexao != nullptr, "(b) modo aberto: o servidor recusou a conexão (fora da tabela C4')");
    QVERIFY2(tlsDoCliente(resultado.conexao.get()) == QStringLiteral("nulo"),
             "(b) TLS do lado do cliente no modo aberto: PARADA");
    QVERIFY2(tlsDoServidor(resultado.conexao.get()) == QStringLiteral("vazio"),
             "(b) TLS do lado do servidor no modo aberto: PARADA");
    QVERIFY2(selectUmOk(resultado.conexao.get()), "SELECT 1 falhou no modo aberto");
}

// (c+) Modo fixado com a impressão certa: sessão com a mesma cifra nos dois lados (classes com certificado); nas classes
// sem TLS, recusa 2026 com SSL is required.
void TesteConexaoBanco::modoFixadoImpressaoCerta()
{
    const auto [dados, classe, senha, falta] = arranjoDoBanco();
    QVERIFY2(falta.isEmpty(), falta.constData());

    const auto resultado = conectar(*dados, *senha, ModoTls::Fixado, impressaoParaFixar(*dados));
    registrarTls("modoFixadoImpressaoCerta", resultado);
    if (*classe == Classe::Nenhum) {
        QVERIFY2(!resultado.conexao, "(c+) sessão aberta sem TLS no servidor: PARADA");
        QCOMPARE(resultado.erro, 2026u);
        QVERIFY2(resultado.mensagem.contains(QStringLiteral("SSL is required")),
                 "(c+) sem TLS no servidor: a recusa tem de dizer SSL is required");
        return;
    }
    QVERIFY2(resultado.conexao != nullptr, "(c+) impressão certa recusada: fora da tabela C4'");
    QVERIFY2(mesmaCifraNosDoisLados(tlsDoCliente(resultado.conexao.get()), tlsDoServidor(resultado.conexao.get())),
             "(c+) os dois lados não têm a mesma cifra de TLS");
    QVERIFY2(selectUmOk(resultado.conexao.get()), "SELECT 1 falhou na sessão do par");
}

// (c-) Modo fixado com a impressão trocada (um dígito): recusa 2026. Com certificado, a recusa diz
// "Fingerprint validation of peer certificate failed". Sem TLS no servidor, diz "SSL is required". Sessão aberta é PARADA.
void TesteConexaoBanco::modoFixadoImpressaoTrocada()
{
    const auto [dados, classe, senha, falta] = arranjoDoBanco();
    QVERIFY2(falta.isEmpty(), falta.constData());

    const QByteArray errada = impressaoTrocada(impressaoParaFixar(*dados));
    const auto resultado = conectar(*dados, *senha, ModoTls::Fixado, errada);
    registrarTls("modoFixadoImpressaoTrocada", resultado);
    QVERIFY2(!resultado.conexao, "(c-) impressão trocada abriu sessão: PARADA");
    QCOMPARE(resultado.erro, 2026u);
    if (*classe == Classe::Nenhum) {
        QVERIFY2(resultado.mensagem.contains(QStringLiteral("SSL is required")),
                 "(c-) sem TLS no servidor: a recusa tem de dizer SSL is required");
    } else {
        QVERIFY2(resultado.mensagem.contains(QStringLiteral("Fingerprint validation of peer certificate failed")),
                 "(c-) com certificado: a recusa tem de dizer Fingerprint validation of peer certificate failed");
    }
}

QTEST_GUILESS_MAIN(TesteConexaoBanco)
#include "tst_conexao_banco.moc"
