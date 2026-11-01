// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do evento sefin.transporte_falhou (NL-2.10e, E4, DA-2.75). O log usa as listas de produção (allowlist_log_sefin): se o
// evento ou um campo não estiver na allowlist, a asserção falha.
#include <nl/mid/sefin/cliente/allowlist_log_sefin.h>
#include <nl/mid/sefin/cliente/registro_transporte.h>

#include <nl/foundation/log/log.h>

#include <sstream>

#include <QtTest>

namespace fl = nl::foundation::log;
namespace mc = nl::mid::sefin::cliente;
namespace ft = nl::foundation::transporte;

class TstRegistroTransporte : public QObject
{
    Q_OBJECT

private slots:
    // O evento sai com causa, codigoQt, handshake, bytesEscritos e etapa, todos de conjunto fechado.
    void eventoTemSoCamposFechados();
    // Etapa ou causa fora do conjunto vira "outro": nada do texto livre entra no log.
    void valorForaDoConjuntoVira_outro();
};

void TstRegistroTransporte::eventoTemSoCamposFechados()
{
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    mc::registrarTransporteFalhou(log, ft::CausaErro::Tls, 4, true, 0, "POST");
    const std::string saida = destino.str();
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
    QVERIFY2(saida.find("\"etapa\":\"outro\"") != std::string::npos, "etapa fora do conjunto tem de virar outro");
    QVERIFY2(saida.find("texto livre") == std::string::npos, "o texto livre não pode sair no log");
}

QTEST_APPLESS_MAIN(TstRegistroTransporte)
#include "tst_registro_transporte.moc"
