// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do caso de uso EmitirNota (NL-2.9, DA-2.42) com o port AssinadorDps em memória: nenhum adaptador real aqui.
#include <nl/back/app/emitir_nota.h>
#include <nl/back/ports/assinador_dps.h>

#include <QByteArray>
#include <QString>
#include <QtTest>

#include <expected>
#include <optional>

namespace {

// Fake em memória do port: registra a chamada e devolve o que o teste configurar.
class AssinadorEmMemoria final : public nl::back::AssinadorDps
{
public:
    std::expected<QByteArray, nl::base::Erro> assinar(QByteArrayView xml, QStringView idReferencia) override {
        chamadas_++;
        xmlRecebido_ = QByteArray(xml.data(), static_cast<qsizetype>(xml.size()));
        idRecebido_ = idReferencia.toString();
        if (erro_) {
            return std::unexpected(*erro_);
        }
        return assinado_;
    }

    int chamadas_ = 0;
    QByteArray xmlRecebido_;
    QString idRecebido_;
    QByteArray assinado_ = "<DPS-assinada/>";
    std::optional<nl::base::Erro> erro_;
};

} // namespace

class TesteEmitirNota : public QObject
{
    Q_OBJECT

private slots:
    // O caso de uso entrega a DPS e o Id ao port, e devolve a DPS assinada que o port produziu.
    void assinaPelaPortaComOIdDaDps();
    // O erro do port sobe com o mesmo código: o caso de uso não traduz nem engole.
    void erroDoAssinadorSobeComOMesmoCodigo();
    // Id vazio é recusado antes de chamar o port.
    void idVazioRecusadoSemChamarAssinador();
};

void TesteEmitirNota::assinaPelaPortaComOIdDaDps()
{
    AssinadorEmMemoria assinador;
    nl::back::EmitirNota caso(assinador);
    const QByteArray dps = "<DPS><infDPS Id=\"DPS1\"/></DPS>";
    auto resultado = caso.executar(dps, QStringLiteral("DPS1"));
    QVERIFY2(resultado.has_value(), "a emissão com port que assina tem de dar a DPS assinada");
    QCOMPARE(assinador.chamadas_, 1);
    QCOMPARE(assinador.idRecebido_, QStringLiteral("DPS1"));
    QCOMPARE(assinador.xmlRecebido_, dps);
    QCOMPARE(*resultado, assinador.assinado_);
}

void TesteEmitirNota::erroDoAssinadorSobeComOMesmoCodigo()
{
    AssinadorEmMemoria assinador;
    assinador.erro_ = nl::base::Erro("assinatura.algoritmo_bloqueado", std::nullopt, "algoritmo recusado");
    nl::back::EmitirNota caso(assinador);
    auto resultado = caso.executar("<DPS/>", QStringLiteral("DPS1"));
    QVERIFY2(!resultado.has_value(), "o erro do port tem de chegar ao chamador");
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("assinatura.algoritmo_bloqueado"));
}

void TesteEmitirNota::idVazioRecusadoSemChamarAssinador()
{
    AssinadorEmMemoria assinador;
    nl::back::EmitirNota caso(assinador);
    auto resultado = caso.executar("<DPS/>", QStringLiteral(""));
    QVERIFY2(!resultado.has_value(), "Id vazio tem de ser recusado");
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("emissao.id_vazio"));
    QCOMPARE(assinador.chamadas_, 0);
}

QTEST_APPLESS_MAIN(TesteEmitirNota)
#include "tst_emitir_nota.moc"
