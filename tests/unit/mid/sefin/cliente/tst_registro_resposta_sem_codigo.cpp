// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do evento sefin.resposta_sem_codigo (NL-3.L, passo 15). O log usa as listas de produção (allowlist_log_sefin): se o
// evento ou um campo não estiver na allowlist, a asserção falha. O tipo de conteúdo sai de um conjunto fechado e o corpo nunca.
#include <nl/mid/sefin/cliente/allowlist_log_sefin.h>
#include <nl/mid/sefin/cliente/registro_resposta_sem_codigo.h>

#include <nl/foundation/log/log.h>

#include <sstream>
#include <string>

#include <QDir>
#include <QFile>

#include <QtTest>

namespace fl = nl::foundation::log;
namespace mc = nl::mid::sefin::cliente;

namespace {

// Grava a saída do evento na pasta que o portão de PII varre (NL_LOGS_PII_CLI_DIR, NL-2.10e R-B2). Sem a variável, não faz nada.
void gravarParaPortaoDePii(const std::string& saida)
{
    const QString pasta = qEnvironmentVariable("NL_LOGS_PII_CLI_DIR");
    if (pasta.isEmpty()) {
        return;
    }
    QDir().mkpath(pasta);
    QFile arquivo(pasta + QStringLiteral("/resposta_sem_codigo.log"));
    if (arquivo.open(QIODevice::WriteOnly | QIODevice::Append)) {
        arquivo.write(saida.data(), static_cast<qint64>(saida.size()));
    }
}

// Saída do log de produção depois de um evento com o status, o Content-Type e o tamanho dados.
std::string saidaDoEvento(int status, QByteArrayView tipoConteudo, qsizetype bytesCorpo)
{
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    mc::registrarRespostaSemCodigo(log, status, tipoConteudo, bytesCorpo);
    gravarParaPortaoDePii(destino.str());
    return destino.str();
}

} // namespace

class TstRegistroRespostaSemCodigo : public QObject
{
    Q_OBJECT

private slots:
    // O evento sai com status, tipoConteudo e bytesCorpo, todos aceitos pela allowlist de produção.
    void eventoTemStatusTipoETamanho();
    // HTML com charset e maiúsculas vira "html"; JSON vira "json": é o que separa porta de entrada de aplicação.
    void tipoConteudoSaiDoConjuntoFechado();
    // Content-Type fora do conjunto vira "outro", sem nada do texto do cabeçalho.
    void tipoConteudoForaDoConjuntoViraOutro();
    // Sem Content-Type, "ausente"; tamanho negativo vira 0.
    void semTipoConteudoViraAusente();
};

void TstRegistroRespostaSemCodigo::eventoTemStatusTipoETamanho()
{
    const std::string saida = saidaDoEvento(403, "text/html; charset=utf-8", 139);
    QVERIFY2(saida.find("sefin.resposta_sem_codigo") != std::string::npos, "o evento tem de sair no log");
    QVERIFY2(saida.find("evento.invalido") == std::string::npos, "o evento tem de estar no catálogo de produção");
    QVERIFY2(saida.find("\"status\":\"403\"") != std::string::npos, "o status tem de sair como inteiro");
    QVERIFY2(saida.find("\"tipoConteudo\":\"html\"") != std::string::npos, "o tipo de conteúdo tem de sair do conjunto fechado");
    QVERIFY2(saida.find("\"bytesCorpo\":\"139\"") != std::string::npos, "o tamanho do corpo tem de sair como inteiro");
}

void TstRegistroRespostaSemCodigo::tipoConteudoSaiDoConjuntoFechado()
{
    QCOMPARE(QString::fromStdString(mc::tipoConteudoParaLog("Text/HTML; charset=UTF-8")), QStringLiteral("html"));
    QCOMPARE(QString::fromStdString(mc::tipoConteudoParaLog("application/json")), QStringLiteral("json"));
    QCOMPARE(QString::fromStdString(mc::tipoConteudoParaLog("application/problem+json")), QStringLiteral("json"));
    QCOMPARE(QString::fromStdString(mc::tipoConteudoParaLog("text/xml")), QStringLiteral("xml"));
    QCOMPARE(QString::fromStdString(mc::tipoConteudoParaLog("text/plain")), QStringLiteral("texto"));
}

void TstRegistroRespostaSemCodigo::tipoConteudoForaDoConjuntoViraOutro()
{
    const std::string saida = saidaDoEvento(400, "application/x-cpf-12345678909", 10);
    QVERIFY2(saida.find("\"tipoConteudo\":\"outro\"") != std::string::npos, "tipo fora do conjunto tem de virar outro");
    QVERIFY2(saida.find("12345678909") == std::string::npos, "nada do texto do cabeçalho pode sair no log");
}

void TstRegistroRespostaSemCodigo::semTipoConteudoViraAusente()
{
    const std::string saida = saidaDoEvento(401, QByteArrayView{}, -5);
    QVERIFY2(saida.find("\"tipoConteudo\":\"ausente\"") != std::string::npos, "sem Content-Type, o tipo é ausente");
    QVERIFY2(saida.find("\"bytesCorpo\":\"0\"") != std::string::npos, "tamanho negativo tem de virar 0");
}

QTEST_APPLESS_MAIN(TstRegistroRespostaSemCodigo)
#include "tst_registro_resposta_sem_codigo.moc"
