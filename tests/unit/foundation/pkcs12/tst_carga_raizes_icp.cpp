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
#include <QCryptographicHash>
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
    void arquivoNaoDeclaradoNoDiretorioRecusado();
    void manifestoVazioRecusado();
    void metaComChaveRepetidaRecusada();

    // Caminhos de falha fechada (CISO, 30/10/2026, item 2): cada um com a sua prova.
    void arquivoAusenteRecusa();
    void certificadoIlegivelRecusa();
    void manifestoAusenteRecusa();
    void manifestoMalformadoRecusa();
    void nenhumaRaizAceitaRecusa();
    void raizAindaNaoValidaNaoEntraComoAncora();
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

void TesteCargaRaizesIcp::arquivoNaoDeclaradoNoDiretorioRecusado()
{
    // Um .der no diretório que o manifesto não declara faz a carga recusar com ArquivoNaoDeclarado (CISO, item 4):
    // em produção o conteúdo do diretório é conhecido por inteiro, então um arquivo a mais é sinal de adulteração.
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
    QVERIFY2(!carga.has_value(), "carga aceita com .der não declarado no diretório");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::ArquivoNaoDeclarado, std::string(codigo(carga.error().motivo)).c_str());
    QCOMPARE(QString::fromStdString(carga.error().arquivo), QStringLiteral("raiz_fora.der"));
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


void TesteCargaRaizesIcp::metaComChaveRepetidaRecusada()
{
    // Um "# meta" com a mesma chave duas vezes (proposito=geral e proposito=ssl) é ambíguo. Ao lado, uma linha boa: o
    // manifesto inteiro é recusado, e a linha boa não entra sozinha (CISO, 30/10/2026, item 3).
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QByteArray repetida = teste::raizSinteticaDer("Raiz com meta repetida", epoca(2020, 1, 1), epoca(2040, 1, 1));
    const QByteArray boa = teste::raizSinteticaDer("Raiz boa ao lado", epoca(2020, 1, 1), epoca(2040, 1, 1));
    QVERIFY(!repetida.isEmpty() && !boa.isEmpty());
    QFile arquivoRepetida(tmp.filePath(QStringLiteral("raiz.der")));
    QVERIFY(arquivoRepetida.open(QIODevice::WriteOnly));
    QVERIFY(arquivoRepetida.write(repetida) == repetida.size());
    arquivoRepetida.close();
    QFile arquivoBoa(tmp.filePath(QStringLiteral("boa.der")));
    QVERIFY(arquivoBoa.open(QIODevice::WriteOnly));
    QVERIFY(arquivoBoa.write(boa) == boa.size());
    arquivoBoa.close();
    const QString hashRepetida = QString::fromLatin1(QCryptographicHash::hash(repetida, QCryptographicHash::Sha256).toHex());
    const QString hashBoa = QString::fromLatin1(QCryptographicHash::hash(boa, QCryptographicHash::Sha256).toHex());
    QFile manifesto(tmp.filePath(QStringLiteral("SHA256SUMS")));
    QVERIFY(manifesto.open(QIODevice::WriteOnly));
    manifesto.write(QStringLiteral("# meta boa.der proposito=geral fonte=teste\n%1  boa.der\n"
                                   "# meta raiz.der proposito=geral proposito=ssl fonte=teste\n%2  raiz.der\n")
                        .arg(hashBoa, hashRepetida).toUtf8());
    manifesto.close();

    Carga carga = carregarRaizesIcp(tmp.path(), manifesto.fileName(), "geral", agoraDeTeste());
    QVERIFY2(!carga.has_value(), "meta com chave repetida aceita com a raiz boa ao lado");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::ManifestoMalformado, std::string(codigo(carga.error().motivo)).c_str());
}

void TesteCargaRaizesIcp::arquivoAusenteRecusa()
{
    // Um .der que o manifesto declara e que não está no diretório: a carga inteira falha, nomeando o arquivo.
    QTemporaryDir copia;
    QVERIFY(copia.isValid());
    QDir origem(dirRaizes());
    for (const QString& nome : origem.entryList({QStringLiteral("*.der")}, QDir::Files)) {
        QVERIFY(QFile::copy(origem.filePath(nome), copia.filePath(nome)));
    }
    QVERIFY(QFile::remove(copia.filePath(QStringLiteral("ICP-Brasilv13.der"))));

    Carga carga = carregarRaizesIcp(copia.path(), manifestoRaizes(), "geral", agoraDeTeste());
    QVERIFY2(!carga.has_value(), "carga aceita com arquivo declarado ausente");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::ArquivoAusente, std::string(codigo(carga.error().motivo)).c_str());
    QCOMPARE(QString::fromStdString(carga.error().arquivo), QStringLiteral("ICP-Brasilv13.der"));
}


void TesteCargaRaizesIcp::certificadoIlegivelRecusa()
{
    // Uma raiz válida e um conteúdo que não é DER de certificado, ambos com hash conferido. A carga inteira tem de ser
    // recusada: a raiz válida não entra sozinha, e o erro nomeia o arquivo ruim (não basta o código de erro).
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QByteArray boa = teste::raizSinteticaDer("Raiz boa ao lado do lixo", epoca(2020, 1, 1), epoca(2040, 1, 1));
    QVERIFY(!boa.isEmpty());
    const QByteArray lixo("isto nao e um certificado X.509");
    const QString manifesto = teste::escreverRepositorioTeste(
        tmp.path(), {{QStringLiteral("ruim.der"), lixo, QStringLiteral("geral")},
                     {QStringLiteral("boa.der"), boa, QStringLiteral("geral")}});
    QVERIFY(!manifesto.isEmpty());

    Carga carga = carregarRaizesIcp(tmp.path(), manifesto, "geral", agoraDeTeste());
    QVERIFY2(!carga.has_value(), "carga aceita com a raiz válida ao lado de um conteúdo não-X.509");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::CertificadoIlegivel, std::string(codigo(carga.error().motivo)).c_str());
    QCOMPARE(QString::fromStdString(carga.error().arquivo), QStringLiteral("ruim.der"));
}


void TesteCargaRaizesIcp::manifestoAusenteRecusa()
{
    // Uma raiz válida no diretório e nenhum SHA256SUMS: sem manifesto não há o que conferir, então a carga é recusada
    // (ManifestoAusente) e não vira um conjunto vazio nem usa o .der sem conferência.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QByteArray boa = teste::raizSinteticaDer("Raiz valida sem manifesto", epoca(2020, 1, 1), epoca(2040, 1, 1));
    QVERIFY(!boa.isEmpty());
    QFile arquivo(tmp.filePath(QStringLiteral("boa.der")));
    QVERIFY(arquivo.open(QIODevice::WriteOnly));
    QVERIFY(arquivo.write(boa) == boa.size());
    arquivo.close();

    Carga carga = carregarRaizesIcp(tmp.path(), tmp.filePath(QStringLiteral("SHA256SUMS")), "geral", agoraDeTeste());
    QVERIFY2(!carga.has_value(), "carga aceita sem manifesto, com a raiz válida no diretório");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::ManifestoAusente, std::string(codigo(carga.error().motivo)).c_str());
}


void TesteCargaRaizesIcp::manifestoMalformadoRecusa()
{
    // Uma linha de hash válida, com metadado, e outra linha de hash sem o "# meta" correspondente: o manifesto inteiro
    // é recusado. A linha boa não é aproveitada sozinha.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QByteArray boa = teste::raizSinteticaDer("Raiz boa", epoca(2020, 1, 1), epoca(2040, 1, 1));
    const QByteArray semMeta = teste::raizSinteticaDer("Raiz sem metadado", epoca(2020, 1, 1), epoca(2040, 1, 1));
    QVERIFY(!boa.isEmpty() && !semMeta.isEmpty());
    QFile arquivoBoa(tmp.filePath(QStringLiteral("boa.der")));
    QVERIFY(arquivoBoa.open(QIODevice::WriteOnly));
    QVERIFY(arquivoBoa.write(boa) == boa.size());
    arquivoBoa.close();
    QFile arquivoSemMeta(tmp.filePath(QStringLiteral("raiz.der")));
    QVERIFY(arquivoSemMeta.open(QIODevice::WriteOnly));
    QVERIFY(arquivoSemMeta.write(semMeta) == semMeta.size());
    arquivoSemMeta.close();
    const QString hashBoa = QString::fromLatin1(QCryptographicHash::hash(boa, QCryptographicHash::Sha256).toHex());
    const QString hashSemMeta = QString::fromLatin1(QCryptographicHash::hash(semMeta, QCryptographicHash::Sha256).toHex());
    QFile manifesto(tmp.filePath(QStringLiteral("SHA256SUMS")));
    QVERIFY(manifesto.open(QIODevice::WriteOnly));
    manifesto.write(QStringLiteral("# meta boa.der proposito=geral fonte=teste\n%1  boa.der\n%2  raiz.der\n")
                        .arg(hashBoa, hashSemMeta).toUtf8());
    manifesto.close();

    Carga carga = carregarRaizesIcp(tmp.path(), manifesto.fileName(), "geral", agoraDeTeste());
    QVERIFY2(!carga.has_value(), "manifesto com linha de hash sem metadado aceito com a raiz boa ao lado");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::ManifestoMalformado, std::string(codigo(carga.error().motivo)).c_str());
}

void TesteCargaRaizesIcp::nenhumaRaizAceitaRecusa()
{
    // Em 2046-01-01 todas as seis raízes de produção estão vencidas: sem âncora, a carga falha (fail-closed).
    Carga carga = carregarRaizesIcp(dirRaizes(), manifestoRaizes(), "geral", instante(2046, 1, 1));
    QVERIFY2(!carga.has_value(), "carga sem nenhuma raiz vigente foi aceita");
    QVERIFY2(carga.error().motivo == ErroRaizIcp::NenhumaRaizAceita, std::string(codigo(carga.error().motivo)).c_str());
}

void TesteCargaRaizesIcp::raizAindaNaoValidaNaoEntraComoAncora()
{
    // Uma raiz cujo início de validade é 2027 (depois de 2026-10-30) fica de fora com o motivo AindaNaoValida; a outra entra.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QByteArray vigente = teste::raizSinteticaDer("Raiz vigente de teste", epoca(2020, 1, 1), epoca(2040, 1, 1));
    const QByteArray futura = teste::raizSinteticaDer("Raiz futura de teste", epoca(2027, 1, 1), epoca(2040, 1, 1));
    QVERIFY(!vigente.isEmpty() && !futura.isEmpty());
    const QString manifesto = teste::escreverRepositorioTeste(
        tmp.path(), {{QStringLiteral("raiz_vigente.der"), vigente, QStringLiteral("geral")},
                     {QStringLiteral("raiz_futura.der"), futura, QStringLiteral("geral")}});
    QVERIFY(!manifesto.isEmpty());

    Carga carga = carregarRaizesIcp(tmp.path(), manifesto, "geral", agoraDeTeste());
    QVERIFY2(carga.has_value(), descreverCarga(carga).c_str());
    QCOMPARE(carga->ancoras.quantidade(), std::size_t{1});
    QCOMPARE(carga->descartes.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(carga->descartes.front().arquivo), QStringLiteral("raiz_futura.der"));
    QVERIFY(carga->descartes.front().motivo == MotivoDescarte::AindaNaoValida);
}

QTEST_APPLESS_MAIN(TesteCargaRaizesIcp)
#include "tst_carga_raizes_icp.moc"
