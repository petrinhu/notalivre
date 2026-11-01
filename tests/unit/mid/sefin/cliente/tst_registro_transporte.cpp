// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do evento sefin.transporte_falhou (NL-2.10e, E4, DA-2.75). O log usa as listas de produção (allowlist_log_sefin): se o
// evento ou um campo não estiver na allowlist, a asserção falha.
#include <nl/mid/sefin/cliente/allowlist_log_sefin.h>
#include <nl/mid/sefin/cliente/registro_transporte.h>

#include <nl/foundation/log/log.h>

#include <sstream>

#include <QDir>
#include <QFile>

#include <QtTest>

namespace fl = nl::foundation::log;
namespace mc = nl::mid::sefin::cliente;
namespace ft = nl::foundation::transporte;

// NL-2.10e (R-B2): grava a saída do evento na pasta que o portão de PII varre (NL_LOGS_PII_CLI_DIR), para o portão pegar um campo que
// vaze CNPJ. Sem a variável, não faz nada.
void gravarParaPortaoDePii(const std::string& saida, const char* nome)
{
    const QString pasta = qEnvironmentVariable("NL_LOGS_PII_CLI_DIR");
    if (pasta.isEmpty()) {
        return;
    }
    QDir().mkpath(pasta);
    QFile arquivo(pasta + QLatin1Char('/') + QLatin1String(nome));
    if (arquivo.open(QIODevice::WriteOnly | QIODevice::Append)) {
        arquivo.write(saida.data(), static_cast<qint64>(saida.size()));
    }
}

class TstRegistroTransporte : public QObject
{
    Q_OBJECT

private slots:
    // O evento sai com causa, codigoQt, handshake, bytesEscritos e etapa, todos de conjunto fechado.
    void eventoTemSoCamposFechados();
    // Etapa ou causa fora do conjunto vira "outro": nada do texto livre entra no log.
    void valorForaDoConjuntoVira_outro();
    // E4 complemento (DA-2.75): codigosSsl (inteiros), versaoTls e protocoloNegociado saem como conjunto fechado.
    void camposNovosSaoFechados();
    // E4 complemento: versão TLS ou protocolo fora do conjunto vira "outro", sem o texto livre.
    void versaoForaDoConjuntoVira_outro();
    // NL-2.10e (L2): no máximo 16 códigos SSL no evento.
    void codigosSslLimitadosEm16();
};

void TstRegistroTransporte::camposNovosSaoFechados()
{
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    mc::registrarTransporteFalhou(log, ft::CausaErro::Tls, 4, true, 0, "POST", {1, 4}, "tls1.3", "h2");
    const std::string saida = destino.str();
    gravarParaPortaoDePii(saida, "transporte_falhou.log");
    QVERIFY2(saida.find("\"codigosSsl\":\"1,4\"") != std::string::npos, "os códigos SSL tem de sair como inteiros separados por vírgula");
    QVERIFY2(saida.find("\"versaoTls\":\"tls1.3\"") != std::string::npos, "a versão TLS tem de sair do conjunto fechado");
    QVERIFY2(saida.find("\"protocoloNegociado\":\"h2\"") != std::string::npos, "o protocolo negociado tem de sair do conjunto fechado");
}

void TstRegistroTransporte::versaoForaDoConjuntoVira_outro()
{
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    mc::registrarTransporteFalhou(log, ft::CausaErro::Tls, 4, true, 0, "POST", {}, "SSLv3 texto livre", "http/1.1");
    const std::string saida = destino.str();
    gravarParaPortaoDePii(saida, "transporte_falhou.log");
    QVERIFY2(saida.find("\"versaoTls\":\"outro\"") != std::string::npos, "versão fora do conjunto tem de virar outro");
    QVERIFY2(saida.find("texto livre") == std::string::npos, "o texto livre da versão não pode sair no log");
}

void TstRegistroTransporte::eventoTemSoCamposFechados()
{
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    mc::registrarTransporteFalhou(log, ft::CausaErro::Tls, 4, true, 0, "POST");
    const std::string saida = destino.str();
    gravarParaPortaoDePii(saida, "transporte_falhou.log");
    QVERIFY2(saida.find("sefin.transporte_falhou") != std::string::npos, "o evento de falha de transporte tem de sair no log");
    QVERIFY2(saida.find("\"causa\":\"tls\"") != std::string::npos, "a causa tem de sair como valor do conjunto fechado");
    QVERIFY2(saida.find("\"codigoQt\":\"4\"") != std::string::npos, "o código Qt tem de sair como inteiro");
    QVERIFY2(saida.find("\"handshake\":\"sim\"") != std::string::npos, "handshake tem de sair como sim ou nao");
    QVERIFY2(saida.find("\"bytesEscritos\":\"0\"") != std::string::npos, "bytesEscritos tem de sair como inteiro");
    QVERIFY2(saida.find("\"etapa\":\"POST\"") != std::string::npos, "a etapa tem de sair como POST");
}

void TstRegistroTransporte::valorForaDoConjuntoVira_outro()
{
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    mc::registrarTransporteFalhou(log, ft::CausaErro::Conexao, -1, false, 12, "GET texto livre do qt");
    const std::string saida = destino.str();
    gravarParaPortaoDePii(saida, "transporte_falhou.log");
    QVERIFY2(saida.find("\"etapa\":\"outro\"") != std::string::npos, "etapa fora do conjunto tem de virar outro");
    QVERIFY2(saida.find("texto livre") == std::string::npos, "o texto livre não pode sair no log");
}

// NL-2.10e (L2, DA-2.75): no máximo 16 códigos SSL no evento. Com 20 códigos, saem só os 16 primeiros: a string tem de fechar aspas no 16.
void TstRegistroTransporte::codigosSslLimitadosEm16()
{
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    const std::vector<int> vinte(20, 7);
    mc::registrarTransporteFalhou(log, ft::CausaErro::Tls, 4, true, 0, "POST", vinte, "tls1.3", "h2");
    const std::string saida = destino.str();
    gravarParaPortaoDePii(saida, "transporte_falhou.log");
    const std::string esperado = "\"codigosSsl\":\"7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7\"";
    QVERIFY2(saida.find(esperado) != std::string::npos, "com mais de 16 códigos SSL, só os 16 primeiros podem sair");
}

QTEST_APPLESS_MAIN(TstRegistroTransporte)
#include "tst_registro_transporte.moc"
