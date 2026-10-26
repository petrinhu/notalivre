// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do tipo Erro da Foundation base (NL-1.12). Regra pura: sem I/O, sem Qt GUI.

#include <nl/base/erro.h>

#include <optional>
#include <string>

#include <QtTest>

class TstErro : public QObject
{
    Q_OBJECT

private slots:
    void erroGuardaCodigoCampoEMensagem();
    void errosIguaisPorCodigoECampo();
    void ambosSemCampoSaoIguais();
    void campoVazioDifereDeAusente();
};

void TstErro::erroGuardaCodigoCampoEMensagem()
{
    const nl::base::Erro comCampo{"cnpj.formato", std::string{"Dps.Prestador.CNPJ"}, "CNPJ com tamanho inválido"};

    QCOMPARE(QString::fromStdString(comCampo.codigo()), QStringLiteral("cnpj.formato"));
    QCOMPARE(comCampo.campo().has_value(), true);
    QCOMPARE(QString::fromStdString(comCampo.campo().value_or(std::string{})), QStringLiteral("Dps.Prestador.CNPJ"));
    QCOMPARE(QString::fromStdString(comCampo.mensagem()), QString::fromUtf8("CNPJ com tamanho inválido"));

    const nl::base::Erro semCampo{"E0188", std::nullopt, "Erro sem campo do leiaute"};

    QCOMPARE(QString::fromStdString(semCampo.codigo()), QStringLiteral("E0188"));
    QCOMPARE(semCampo.campo().has_value(), false);
    QCOMPARE(QString::fromStdString(semCampo.mensagem()), QStringLiteral("Erro sem campo do leiaute"));
}

void TstErro::errosIguaisPorCodigoECampo()
{
    const nl::base::Erro base{"cnpj.formato", std::string{"Dps.Prestador.CNPJ"}, "mensagem original"};
    const nl::base::Erro mesmoCodigoECampo{"cnpj.formato", std::string{"Dps.Prestador.CNPJ"}, "outra mensagem"};
    const nl::base::Erro outroCodigo{"cnpj.dv", std::string{"Dps.Prestador.CNPJ"}, "mensagem original"};
    const nl::base::Erro outroCampo{"cnpj.formato", std::string{"Dps.Tomador.CNPJ"}, "mensagem original"};
    const nl::base::Erro semCampo{"cnpj.formato", std::nullopt, "mensagem original"};

    QCOMPARE(base == mesmoCodigoECampo, true);
    QCOMPARE(base == outroCodigo, false);
    QCOMPARE(base == outroCampo, false);
    QCOMPARE(base == semCampo, false);
    QCOMPARE(semCampo == base, false);
}

void TstErro::ambosSemCampoSaoIguais()
{
    const nl::base::Erro semCampo{"cnpj.formato", std::nullopt, "mensagem original"};
    const nl::base::Erro semCampoOutraMensagem{"cnpj.formato", std::nullopt, "outra mensagem"};
    const nl::base::Erro semCampoOutroCodigo{"cnpj.dv", std::nullopt, "mensagem original"};

    QCOMPARE(semCampo == semCampoOutraMensagem, true);
    QCOMPARE(semCampo == semCampoOutroCodigo, false);
}

void TstErro::campoVazioDifereDeAusente()
{
    const nl::base::Erro campoVazio{"cnpj.formato", std::string{}, "mensagem original"};
    const nl::base::Erro semCampo{"cnpj.formato", std::nullopt, "mensagem original"};

    QCOMPARE(campoVazio.campo().has_value(), true);
    QCOMPARE(campoVazio == semCampo, false);
    QCOMPARE(semCampo == campoVazio, false);
}

QTEST_GUILESS_MAIN(TstErro)
#include "tst_erro.moc"
