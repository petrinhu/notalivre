// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Falhas do envio (NL-3.C, plano 3.3 e 4.2): a causa de cada falha de transporte é tipada e a decisão do envio não usa
// texto. Tabela pura, sem rede (os reenvios são só o parâmetro).
#include <nl/back/ports/causa_falha_sefin.h>
#include <nl/back/ports/gateway_sefin.h>
#include <nl/foundation/transporte/transporte_https.h>
#include <nl/mid/sefin/cliente/falhas_do_envio.h>

#include <QtTest>

#include <vector>

namespace bk = nl::back;
namespace tr = nl::foundation::transporte;
namespace cl = nl::mid::sefin::cliente;

namespace {

tr::ErroTransporte erroDe(tr::TipoErro tipo, tr::CausaErro causa, std::vector<int> codigosSsl)
{
    tr::ErroTransporte erro;
    erro.tipo = tipo;
    erro.causa = causa;
    erro.codigosSsl = std::move(codigosSsl);
    return erro;
}

} // namespace

class TesteFalhasDoEnvio : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void causaDoTransporteCobreTodasAsCausas();
    void semBytesTlsDeixaDeSerTransitoria();
    void semBytesRedeContinuaTransitoria();
    void semBytesConfiguracaoViraConfiguracaoDaConexao();
    void falhaDeRedeNaConsultaLevaACausa();
    void semBytesNoReenvioViraNaoConcluido();
    void indeterminadoDepoisDaConsultaLevaACausa();
};

void TesteFalhasDoEnvio::initTestCase()
{
}

void TesteFalhasDoEnvio::cleanupTestCase()
{
}

void TesteFalhasDoEnvio::causaDoTransporteCobreTodasAsCausas()
{
    struct Caso {
        tr::CausaErro causa;
        std::vector<int> codigosSsl;
        bk::CausaFalhaSefin esperada;
        const char* nome;
    };
    const std::vector<Caso> casos{
        {tr::CausaErro::Tls, {}, bk::CausaFalhaSefin::CertificadoDoEmissorProvavel, "tls sem codigosSsl"},
        {tr::CausaErro::Tls, {10}, bk::CausaFalhaSefin::ServidorNaoConfirmado, "tls com codigosSsl"},
        {tr::CausaErro::Timeout, {}, bk::CausaFalhaSefin::Rede, "timeout sem codigosSsl"},
        {tr::CausaErro::Timeout, {10}, bk::CausaFalhaSefin::Rede, "timeout com codigosSsl"},
        {tr::CausaErro::Conexao, {}, bk::CausaFalhaSefin::Rede, "conexao sem codigosSsl"},
        {tr::CausaErro::Conexao, {10}, bk::CausaFalhaSefin::Rede, "conexao com codigosSsl"},
        {tr::CausaErro::Protocolo, {}, bk::CausaFalhaSefin::Rede, "protocolo sem codigosSsl"},
        {tr::CausaErro::Protocolo, {10}, bk::CausaFalhaSefin::Rede, "protocolo com codigosSsl"},
        {tr::CausaErro::Configuracao, {}, bk::CausaFalhaSefin::ConfiguracaoDaConexao, "configuracao sem codigosSsl"},
        {tr::CausaErro::Configuracao, {10}, bk::CausaFalhaSefin::ConfiguracaoDaConexao, "configuracao com codigosSsl"},
    };
    for (const Caso& caso : casos) {
        const bk::CausaFalhaSefin obtida = cl::causaDoTransporte(erroDe(tr::TipoErro::Definitivo, caso.causa, caso.codigosSsl));
        QVERIFY2(obtida == caso.esperada, caso.nome);
    }
}

void TesteFalhasDoEnvio::semBytesTlsDeixaDeSerTransitoria()
{
    // Emenda do CISO (ciso.md:30): recusa de TLS com zero bytes é definitiva, com a causa certa.
    const bk::FalhaSefin semCodigosSsl = cl::falhaSemBytesEscritos(erroDe(tr::TipoErro::Definitivo, tr::CausaErro::Tls, {}), false);
    QVERIFY2(semCodigosSsl.motivo == bk::MotivoFalhaSefin::Definitiva, "tls sem codigosSsl: motivo");
    QVERIFY2(semCodigosSsl.causa == bk::CausaFalhaSefin::CertificadoDoEmissorProvavel, "tls sem codigosSsl: causa");

    const bk::FalhaSefin comCodigosSsl = cl::falhaSemBytesEscritos(erroDe(tr::TipoErro::Definitivo, tr::CausaErro::Tls, {10}), false);
    QVERIFY2(comCodigosSsl.motivo == bk::MotivoFalhaSefin::Definitiva, "tls com codigosSsl: motivo");
    QVERIFY2(comCodigosSsl.causa == bk::CausaFalhaSefin::ServidorNaoConfirmado, "tls com codigosSsl: causa");
}

void TesteFalhasDoEnvio::semBytesRedeContinuaTransitoria()
{
    const tr::CausaErro causas[] = {tr::CausaErro::Timeout, tr::CausaErro::Conexao, tr::CausaErro::Protocolo};
    for (tr::CausaErro causa : causas) {
        const bk::FalhaSefin falha = cl::falhaSemBytesEscritos(erroDe(tr::TipoErro::Transitorio, causa, {}), false);
        QVERIFY2(falha.motivo == bk::MotivoFalhaSefin::Transitoria, "rede: motivo");
        QVERIFY2(falha.causa == bk::CausaFalhaSefin::Rede, "rede: causa");
    }
}

void TesteFalhasDoEnvio::semBytesConfiguracaoViraConfiguracaoDaConexao()
{
    const bk::FalhaSefin falha =
        cl::falhaSemBytesEscritos(erroDe(tr::TipoErro::Definitivo, tr::CausaErro::Configuracao, {}), false);
    QVERIFY2(falha.motivo == bk::MotivoFalhaSefin::Definitiva, "configuracao: motivo");
    QVERIFY2(falha.causa == bk::CausaFalhaSefin::ConfiguracaoDaConexao, "configuracao: causa");
}

void TesteFalhasDoEnvio::falhaDeRedeNaConsultaLevaACausa()
{
    const bk::FalhaSefin transitoria =
        cl::falhaDeRedeNaConsulta(erroDe(tr::TipoErro::Transitorio, tr::CausaErro::Timeout, {}));
    QVERIFY2(transitoria.motivo == bk::MotivoFalhaSefin::Transitoria, "transitorio: motivo");
    QVERIFY2(transitoria.causa == bk::CausaFalhaSefin::Rede, "transitorio: causa");
    QVERIFY2(transitoria.codigoE.empty(), "transitorio: codigoE");
    QVERIFY2(transitoria.mensagem.empty(), "transitorio: mensagem");

    const bk::FalhaSefin definitiva =
        cl::falhaDeRedeNaConsulta(erroDe(tr::TipoErro::Definitivo, tr::CausaErro::Configuracao, {}));
    QVERIFY2(definitiva.motivo == bk::MotivoFalhaSefin::Definitiva, "definitivo: motivo");
    QVERIFY2(definitiva.causa == bk::CausaFalhaSefin::ConfiguracaoDaConexao, "definitivo: causa");
    QVERIFY2(definitiva.codigoE.empty(), "definitivo: codigoE");
}

void TesteFalhasDoEnvio::semBytesNoReenvioViraNaoConcluido()
{
    // R-10 b (CISO): com o reenvio, a Sefin pode já ter a DPS; zero bytes vira transitória sem número, nunca causa de recusa.
    const tr::CausaErro causas[] = {tr::CausaErro::Tls, tr::CausaErro::Timeout, tr::CausaErro::Conexao, tr::CausaErro::Protocolo,
                                    tr::CausaErro::Configuracao};
    const std::vector<std::vector<int>> codigosSsl{{}, {10}};
    for (tr::CausaErro causa : causas) {
        for (const std::vector<int>& ssl : codigosSsl) {
            const bk::FalhaSefin falha = cl::falhaSemBytesEscritos(erroDe(tr::TipoErro::Definitivo, causa, ssl), true);
            QVERIFY2(falha.motivo == bk::MotivoFalhaSefin::Transitoria, "reenvio: motivo");
            QVERIFY2(falha.causa == bk::CausaFalhaSefin::NaoGerada, "reenvio: causa");
        }
    }
}

void TesteFalhasDoEnvio::indeterminadoDepoisDaConsultaLevaACausa()
{
    const bk::CausaFalhaSefin causas[] = {
        bk::CausaFalhaSefin::Nenhuma,
        bk::CausaFalhaSefin::CertificadoDoEmissorProvavel,
        bk::CausaFalhaSefin::ServidorNaoConfirmado,
        bk::CausaFalhaSefin::Rede,
        bk::CausaFalhaSefin::ConfiguracaoDaConexao,
        bk::CausaFalhaSefin::PreparacaoDoEnvio,
        bk::CausaFalhaSefin::RespostaSemCodigoValido,
        bk::CausaFalhaSefin::SefinIndisponivel,
        bk::CausaFalhaSefin::NaoGerada,
    };
    for (bk::CausaFalhaSefin causa : causas) {
        const bk::FalhaSefin consulta{bk::MotivoFalhaSefin::Transitoria, {}, {}, causa};
        const bk::FalhaSefin falha = cl::indeterminadoDepoisDaConsulta(consulta);
        QVERIFY2(falha.motivo == bk::MotivoFalhaSefin::Indeterminado, "indeterminado: motivo");
        QVERIFY2(falha.causa == causa, "indeterminado: causa da consulta");
        QVERIFY2(falha.codigoE.empty(), "indeterminado: codigoE");
    }
}

QTEST_APPLESS_MAIN(TesteFalhasDoEnvio)
#include "tst_falhas_do_envio.moc"
