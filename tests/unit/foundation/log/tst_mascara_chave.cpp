// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Máscara da chave de acesso e do Id da NFS-e no Log (NL-2.8, DA-2.21, ADR-0005 item b). INFERÊNCIA do leiaute: as 14
// posições da inscrição da chave de 50 (10 a 23, 1-based) e do Id de 53 (13 a 26), que é "NFS" mais a chave.
// Slot por CTest (foundation.log.<slot>). Falha é QVERIFY, nunca exceção (L-35).
#include <nl/foundation/log/log.h>

#include <QtTest>

#include <sstream>
#include <string>
#include <vector>

using nl::foundation::log::Campo;
using nl::foundation::log::DeclaracaoCampo;
using nl::foundation::log::Log;
using nl::foundation::log::Nivel;
using nl::foundation::log::Regra;

namespace {

// Chave de 50 posições: 9 dígitos, 14 caracteres [0-9A-Z] (a inscrição) e 27 dígitos. Inscrição em 10 a 23.
const std::string kChave = "123456789" "ABCDEFGHIJKLMN" "012345678901234567890123456";
const std::string kInscricao = "ABCDEFGHIJKLMN";

Log novoLog(std::ostringstream& destino)
{
    return Log(destino, "sefin", {DeclaracaoCampo{"chaveAcesso", Regra::InscricaoMascarada},
                                  DeclaracaoCampo{"idNfse", Regra::InscricaoMascarada}},
               {"sefin.nfse.emitida"});
}

} // namespace

class TesteMascaraChave : public QObject {
    Q_OBJECT

private slots:
    void chaveDeAcessoLogadaComInscricaoMascarada() {
        std::ostringstream destino;
        Log log = novoLog(destino);
        auto resultado = log.registrar(Nivel::Info, "sefin.nfse.emitida", {Campo{"chaveAcesso", kChave}});
        QVERIFY2(resultado.has_value(), "o Log tem de conseguir gravar");
        const std::string saida = destino.str();
        QVERIFY2(saida.find(kInscricao) == std::string::npos, "a inscrição da chave não pode aparecer no log");
        QVERIFY2(saida.find("123456789##############012345678901234567890123456") != std::string::npos,
                 "a chave tem de sair com as 14 posições da inscrição trocadas por '#'");
    }

    void idNfseLogadoComInscricaoMascarada() {
        std::ostringstream destino;
        Log log = novoLog(destino);
        const std::string idNfse = "NFS" + kChave;
        auto resultado = log.registrar(Nivel::Info, "sefin.nfse.emitida", {Campo{"idNfse", idNfse}});
        QVERIFY2(resultado.has_value(), "o Log tem de conseguir gravar");
        const std::string saida = destino.str();
        QVERIFY2(saida.find(kInscricao) == std::string::npos, "a inscrição do Id da NFS-e não pode aparecer no log");
        QVERIFY2(saida.find("NFS123456789##############012345678901234567890123456") != std::string::npos,
                 "o Id da NFS-e tem de sair com as 14 posições da inscrição (13 a 26) trocadas por '#'");
    }

    void chaveForaDoLeiauteDescartada() {
        std::ostringstream destino;
        Log log = novoLog(destino);
        const std::string minuscula = "123456789" "abcdefghijklmn" "012345678901234567890123456";
        const std::string curta = kChave.substr(0, 49);
        QVERIFY(log.registrar(Nivel::Info, "sefin.nfse.emitida", {Campo{"chaveAcesso", minuscula}}).has_value());
        QVERIFY(log.registrar(Nivel::Info, "sefin.nfse.emitida", {Campo{"chaveAcesso", curta}}).has_value());
        const std::string saida = destino.str();
        QVERIFY2(saida.find("abcdefghijklmn") == std::string::npos, "chave fora do leiaute não pode vazar");
        QVERIFY2(saida.find(kInscricao) == std::string::npos, "chave curta não pode vazar a inscrição");
        QCOMPARE(log.descartados(), std::size_t{2});
    }
};

QTEST_APPLESS_MAIN(TesteMascaraChave)
#include "tst_mascara_chave.moc"
