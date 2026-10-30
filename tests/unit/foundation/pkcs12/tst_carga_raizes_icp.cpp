// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da carga das raízes da ICP-Brasil como âncoras (NL-2.13, DA-2.47). Um slot por teste no CTest (pkcs12.raizes.<slot>).
// O repositório real vem de NL_ICP_RAIZES_DIR e NL_ICP_RAIZES_MANIFESTO; a AC de teste e o .pfx vêm de NL_CERTIFICADOS_DIR.
// Raízes sintéticas (fabrica_raiz_teste.h) só provam vencimento e finalidade; nunca viram âncora de produção.
// A conferência de ancoras de produção (ancorasPadraoDeProducao) fica para o Mid (DA-2.44) e não está aqui.

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carga_raizes_icp.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/erro_raiz_icp.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include "fabrica_raiz_teste.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

#include <chrono>
#include <cstddef>
#include <ctime>
#include <expected>
#include <string>
#include <vector>

using namespace nl::foundation::pkcs12;

namespace {

using Instante = std::chrono::system_clock::time_point;
using Carga = std::expected<CargaRaizesIcp, ErroCarga>;

// Instantes em UTC. 2026-10-30 é a data da conferência; 2030-01-01 passa do notAfter da v5 (02/03/2029).
Instante instante(int ano, unsigned mes, unsigned dia) {
    return std::chrono::system_clock::from_time_t(
        std::chrono::system_clock::to_time_t(std::chrono::sys_days{std::chrono::year{ano} / std::chrono::month{mes} / std::chrono::day{dia}}));
}

Instante agoraDeTeste() {
    return instante(2026, 10, 30);
}

QString dirRaizes() {
    return qEnvironmentVariable("NL_ICP_RAIZES_DIR");
}

QString manifestoRaizes() {
    return qEnvironmentVariable("NL_ICP_RAIZES_MANIFESTO");
}

QByteArray fixture(const QString& nome) {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + nome);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

SenhaSegura senhaDaFixture() {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return SenhaSegura::de(QByteArray());
    }
    return SenhaSegura::de(arquivo.readLine().trimmed());
}

std::string descreverCarga(const Carga& carga) {
    if (carga) {
        return "sucesso";
    }
    return std::string(codigo(carga.error().motivo)) + " em " + carga.error().arquivo;
}

std::time_t epoca(int ano, unsigned mes, unsigned dia) {
    return std::chrono::system_clock::to_time_t(instante(ano, mes, dia));
}

} // namespace

class TesteCargaRaizesIcp : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    void raizesIcpCarregadasDoArquivoConferido();
    void hashDivergenteRecusaACarga();
    void cadeiaDeTesteNaoValidaContraAsRaizesIcp();
    void raizVencidaNaoEntraComoAncora();
    void raizForaDoPropositoNaoEntraComoAncora();
    void arquivoForaDoManifestoNaoEntra();
    void manifestoVazioRecusado();
};

void TesteCargaRaizesIcp::raizesIcpCarregadasDoArquivoConferido()
{
    // As seis raízes do repositório (v4, v5, v6, v7, v12, v13), todas com finalidade geral e dentro da validade.
    Carga carga = carregarRaizesIcp(dirRaizes(), manifestoRaizes(), "geral", agoraDeTeste());
    QVERIFY2(carga.has_value(), descreverCarga(carga).c_str());
    QCOMPARE(carga->ancoras.quantidade(), std::size_t{6});
    QVERIFY(carga->descartes.empty());
}

void TesteCargaRaizesIcp::hashDivergenteRecusaACarga()
{
    // Uma cópia do repositório com um byte trocado em uma raiz: a carga inteira é recusada, e o erro nomeia o arquivo.
    QTemporaryDir copia;
    QVERIFY(copia.isValid());
    QDir origem(dirRaizes());
    for (const QString& nome : origem.entryList({QStringLiteral("*.der")}, QDir::Files)) {
        QVERIFY(QFile::copy(origem.filePath(nome), copia.filePath(nome)));
    }
    QFile alvo(copia.filePath(QStringLiteral("ICP-Brasilv12.der")));
    QVERIFY(alvo.open(QIODevice::ReadWrite));
    QByteArray bytes = alvo.readAll();
    QVERIFY(bytes.size() > 0);
    bytes[bytes.size() / 2] = static_cast<char>(bytes[bytes.size() / 2] ^ 0x01);
    alvo.seek(0);
    QVERIFY(alvo.write(bytes) == bytes.size());
    alvo.close();

    Carga carga = carregarRaizesIcp(copia.path(), manifestoRaizes(), "geral", agoraDeTeste());
    QVERIFY2(!carga.has_value(), "carga aceita com hash divergente");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::HashDivergente, std::string(codigo(carga.error().motivo)).c_str());
    QCOMPARE(QString::fromStdString(carga.error().arquivo), QStringLiteral("ICP-Brasilv12.der"));
}

void TesteCargaRaizesIcp::cadeiaDeTesteNaoValidaContraAsRaizesIcp()
{
    // Controle positivo: o .pfx de teste fecha cadeia com a AC de teste. Depois, com as raízes do ITI, a mesma
    // cadeia tem de ser recusada: a AC de teste não está entre as âncoras de produção.
    const QByteArray pfx = fixture(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY(!pfx.isEmpty());

    auto ancorasTeste = Ancoras::dePem(fixture(QStringLiteral("ac.pem")));
    QVERIFY2(ancorasTeste.has_value(), "ac.pem nao lido");
    SenhaSegura senhaTeste = senhaDaFixture();
    CarregadorPkcs12 carregadorTeste;
    auto controle = carregadorTeste.carregar(pfx, senhaTeste, *ancorasTeste);
    QVERIFY2(controle.has_value(), "controle com a AC de teste deveria carregar");

    Carga carga = carregarRaizesIcp(dirRaizes(), manifestoRaizes(), "geral", agoraDeTeste());
    QVERIFY2(carga.has_value(), descreverCarga(carga).c_str());
    SenhaSegura senhaProducao = senhaDaFixture();
    CarregadorPkcs12 carregadorProducao;
    auto resultado = carregadorProducao.carregar(pfx, senhaProducao, carga->ancoras);
    QVERIFY2(!resultado.has_value(), "cadeia de teste aceita pelas raízes do ITI");
    QVERIFY2(resultado.error() == ErroCredencial::CadeiaInvalida, std::string(codigo(resultado.error())).c_str());
}

void TesteCargaRaizesIcp::raizVencidaNaoEntraComoAncora()
{
    // Em 2030-01-01 a v5 (notAfter 02/03/2029) sai do conjunto, e as outras cinco continuam: a carga não é recusada.
    Carga carga = carregarRaizesIcp(dirRaizes(), manifestoRaizes(), "geral", instante(2030, 1, 1));
    QVERIFY2(carga.has_value(), descreverCarga(carga).c_str());
    QCOMPARE(carga->ancoras.quantidade(), std::size_t{5});
    QCOMPARE(carga->descartes.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(carga->descartes.front().arquivo), QStringLiteral("ICP-Brasilv5.der"));
    QVERIFY(carga->descartes.front().motivo == MotivoDescarte::Vencida);
}

void TesteCargaRaizesIcp::raizForaDoPropositoNaoEntraComoAncora()
{
    // Duas raízes sintéticas válidas: uma com finalidade geral (entra) e outra declarada como SSL (fica de fora).
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QByteArray geral = teste::raizSinteticaDer("Raiz geral de teste", epoca(2020, 1, 1), epoca(2040, 1, 1));
    const QByteArray ssl = teste::raizSinteticaDer("Raiz SSL de teste", epoca(2020, 1, 1), epoca(2040, 1, 1));
    QVERIFY(!geral.isEmpty() && !ssl.isEmpty());
    const QString manifesto = teste::escreverRepositorioTeste(
        tmp.path(), {{QStringLiteral("raiz_geral.der"), geral, QStringLiteral("geral")},
                     {QStringLiteral("raiz_ssl.der"), ssl, QStringLiteral("ssl")}});
    QVERIFY(!manifesto.isEmpty());

    Carga carga = carregarRaizesIcp(tmp.path(), manifesto, "geral", agoraDeTeste());
    QVERIFY2(carga.has_value(), descreverCarga(carga).c_str());
    QCOMPARE(carga->ancoras.quantidade(), std::size_t{1});
    QCOMPARE(carga->descartes.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(carga->descartes.front().arquivo), QStringLiteral("raiz_ssl.der"));
    QVERIFY(carga->descartes.front().motivo == MotivoDescarte::PropositoDiferente);
}

void TesteCargaRaizesIcp::arquivoForaDoManifestoNaoEntra()
{
    // Um .der no diretório que não está no SHA256SUMS não vira âncora, mesmo sendo um certificado válido.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QByteArray geral = teste::raizSinteticaDer("Raiz listada de teste", epoca(2020, 1, 1), epoca(2040, 1, 1));
    const QByteArray fora = teste::raizSinteticaDer("Raiz fora do manifesto", epoca(2020, 1, 1), epoca(2040, 1, 1));
    QVERIFY(!geral.isEmpty() && !fora.isEmpty());
    const QString manifesto = teste::escreverRepositorioTeste(
        tmp.path(), {{QStringLiteral("raiz_listada.der"), geral, QStringLiteral("geral")}});
    QVERIFY(!manifesto.isEmpty());
    QFile extra(tmp.filePath(QStringLiteral("raiz_fora.der")));
    QVERIFY(extra.open(QIODevice::WriteOnly));
    QVERIFY(extra.write(fora) == fora.size());
    extra.close();

    Carga carga = carregarRaizesIcp(tmp.path(), manifesto, "geral", agoraDeTeste());
    QVERIFY2(carga.has_value(), descreverCarga(carga).c_str());
    QCOMPARE(carga->ancoras.quantidade(), std::size_t{1});
    QVERIFY(carga->descartes.empty());
}

void TesteCargaRaizesIcp::manifestoVazioRecusado()
{
    // Manifesto sem nenhuma linha de hash: piso não-vazio (L-36), recusa com ManifestoVazio, nunca conjunto vazio.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QFile manifesto(tmp.filePath(QStringLiteral("SHA256SUMS")));
    QVERIFY(manifesto.open(QIODevice::WriteOnly));
    manifesto.write("# só comentário, nenhum hash\n");
    manifesto.close();

    Carga carga = carregarRaizesIcp(dirRaizes(), manifesto.fileName(), "geral", agoraDeTeste());
    QVERIFY2(!carga.has_value(), "manifesto vazio aceito");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::ManifestoVazio, std::string(codigo(carga.error().motivo)).c_str());
}

QTEST_APPLESS_MAIN(TesteCargaRaizesIcp)
#include "tst_carga_raizes_icp.moc"
