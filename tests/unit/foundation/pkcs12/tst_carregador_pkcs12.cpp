// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do carregador de A1 (NL-2.2, ADR-S1). Cada slot roda por nome no CTest (pkcs12.<slot>).
// Fixtures sintéticas em NL_CERTIFICADOS_DIR, geradas pela NL-2.1 (AC de teste). Nenhum .pfx real é lido.
// Nenhum helper lança exceção: falha de leitura vira QVERIFY no teste que chamou (vermelho por asserção, L-35).
// A senha é passada por referência: o carregador a zera, e o teste confere a zeragem pelo próprio objeto.
// Slots com provedor ausente (OPENSSL_MODULES apontando para pasta vazia) rodam em processo próprio, pelo CMake.

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/inscricao.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/pkcs12err.h>
#include <openssl/x509.h>
#include <openssl/provider.h>

#include <nl/foundation/pkcs12/classificacao_erros.h>

#include "acesso_teste_senha.h"

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

using namespace nl::foundation::pkcs12;

namespace {

using Resultado = std::expected<CredencialX509, ErroCredencial>;

const char* const CNPJ_OK = "12345678000195";
const char* const CNPJ_ALFANUMERICO = "12ABC34501DE35";
const char* const CPF_OK = "12345678909";

std::string descreverErro(const Resultado& resultado) {
    return resultado ? std::string("sucesso") : std::string(codigo(resultado.error()));
}

QByteArray fixture(const QString& nome) {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + nome);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

// Senha da fixture. Sem arquivo, a senha fica vazia e o teste falha pela própria asserção (não por exceção).
// Bytes da senha da fixture, sem o fim de linha (para montar senhas de teste a partir da real).
QByteArray senhaBrutaDaFixture() {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readLine().trimmed();
}

SenhaSegura senhaDaFixture() {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return SenhaSegura::de(QByteArray());
    }
    return SenhaSegura::de(arquivo.readLine().trimmed());
}

// Âncoras da fixture (AC de teste). O chamador confere has_value com QVERIFY antes de usar.
std::expected<Ancoras, ErroAncora> ancoraDeTeste() {
    return Ancoras::dePem(fixture(QStringLiteral("ac.pem")));
}


} // namespace

class TesteCarregadorPkcs12 : public QObject {
    Q_OBJECT

private slots:
    void senhaCorretaCarrega();
    void senhaErradaDaErroProprio();
    void senhaErradaNoLegadoDaErroProprio();
    void cifraLegadaCarregaComProviderLegado();
    void cifraLegadaIndisponivelSemProvider();
    void modernoNaoPrecisaDeLegadoSemProvider();
    void legadoNaoDesligaODefault();
    void vencidoSinalizado();
    void folhaComValidadeFuturaReprova();
    void validadeIntermediariaVencidaReprova();
    void semEkuClientAuthSinalizado();
    void ekuMalformadaReprova();
    void credencialSemEkuMarcadaNoIndicador();
    void cnpjExtraidoDoCertificado();
    void cnpjAlfanumericoExtraidoComoTexto();
    void cpfExtraidoDoCertificado();
    void raizComparadaComoTexto();
    void inscricaoFormatoInvalidoRecusado();
    void semInscricaoSinalizada();
    void cnSemOidNaoViraInscricao();
    void otherNameDeOutroOidNaoViraInscricao();
    void cadeiaDeOutraRaizReprova();
    void cadeiaDeOutraSubordinadaReprova();
    void cadeiaIncompletaReprova();
    void cadeiaComSubordinadaCarrega();
    void pfxMalformadoReprova();
    void carregarZeraSenha();
    void ancorasDePemLemUmCertificado();
    void ancorasSemCertificadoReprovam();
    void senhaSeguraZeraConteudo();
    void descartarLiberaChave();
    void classificaMacComoSenhaErrada();
    void classificaUnsupportedDaBibliotecaEvp();
    void classificaMotivoDeOutraBibliotecaComoMalformado();
    void senhaComNulEhRecusada();
    void liberaZeraAntesDeLiberar();
    void senhaErradaSemProvedorNaoTentaLegado();
    void ganchoSobreviveAoMover();
    void pfxSemMacComSenhaErradaFalhaFechado();
    void pfxSemMacComSenhaCertaCarrega();
    void pfxComChaveSemCifraRecusado();
    void chaveNaoConfereRecusada();
};

// Testes novos da revisão 3 (A7, M7, M4). Ver a descrição de cada um.
void TesteCarregadorPkcs12::classificaMacComoSenhaErrada()
{
    using nl::foundation::pkcs12::interno::Falha;
    using nl::foundation::pkcs12::interno::classificarCodigos;
    QCOMPARE(classificarCodigos({ERR_PACK(ERR_LIB_PKCS12, 0, PKCS12_R_MAC_VERIFY_FAILURE)}), Falha::SenhaErrada);
}

void TesteCarregadorPkcs12::classificaUnsupportedDaBibliotecaEvp()
{
    // Cifra que o provider default não abre: erro "unsupported" da biblioteca EVP (é o caso que dispara o legacy).
    using nl::foundation::pkcs12::interno::Falha;
    using nl::foundation::pkcs12::interno::classificarCodigos;
    QCOMPARE(classificarCodigos({ERR_PACK(ERR_LIB_EVP, 0, ERR_R_UNSUPPORTED)}), Falha::Unsupported);
}

void TesteCarregadorPkcs12::classificaMotivoDeOutraBibliotecaComoMalformado()
{
    // Mesmo motivo (ERR_R_UNSUPPORTED) vindo de outra biblioteca (ASN1): não pode disparar a retentativa com legado.
    using nl::foundation::pkcs12::interno::Falha;
    using nl::foundation::pkcs12::interno::classificarCodigos;
    QCOMPARE(classificarCodigos({ERR_PACK(ERR_LIB_ASN1, 0, ERR_R_UNSUPPORTED)}), Falha::Malformado);
}

void TesteCarregadorPkcs12::senhaComNulEhRecusada()
{
    // Senha real da fixture, NUL e lixo depois: a C-string truncaria no NUL e aceitaria a senha real. Tem de ser recusada.
    const QByteArray real = senhaBrutaDaFixture();
    QVERIFY(!real.isEmpty());
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    CarregadorPkcs12 carregador;
    auto senha = SenhaSegura::de(real + QByteArray(1, '\0') + QByteArrayLiteral("x"));
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SenhaErrada);
}


// Gancho de observação da liberação (M7, decisão do CISO): registra se todos os bytes estavam zerados no instante em
// que o gancho foi chamado. Estado de teste (static), nunca de produção.
namespace {
int gChamadasGancho = 0;
bool gTodosZeroNoGancho = false;
std::size_t gCapacidadeNoGancho = 0;

void observaLiberacao(const char* dados, std::size_t capacidade) noexcept {
    ++gChamadasGancho;
    gCapacidadeNoGancho = capacidade;
    gTodosZeroNoGancho = true;
    for (std::size_t i = 0; i < capacidade; ++i) {
        if (dados[i] != '\0') {
            gTodosZeroNoGancho = false;
        }
    }
}

void zeraContadoresDoGancho() {
    gChamadasGancho = 0;
    gTodosZeroNoGancho = false;
    gCapacidadeNoGancho = 0;
}
} // namespace

// M7 (seam): a liberação zera antes de chamar o gancho, e o gancho vê todos os bytes em zero. Mutantes que a quebram:
// "liberar sem zerar" (sem a chamada a zerar em liberar) e "sem cleanse" (sem OPENSSL_cleanse em zerar).
void TesteCarregadorPkcs12::liberaZeraAntesDeLiberar()
{
    zeraContadoresDoGancho();
    {
        auto senha = AcessoDeTesteSenhaSegura::comGancho(QByteArrayLiteral("segredo"), &observaLiberacao);
    }
    QCOMPARE(gChamadasGancho, 1);
    QVERIFY2(gTodosZeroNoGancho, "o gancho viu bytes não zerados: a liberação não zerou antes");
    QVERIFY(gCapacidadeNoGancho >= 7);
}

// O gancho é transferido no movimento: a origem vazia não chama; só o destino chama, uma vez.
void TesteCarregadorPkcs12::ganchoSobreviveAoMover()
{
    zeraContadoresDoGancho();
    {
        auto origem = AcessoDeTesteSenhaSegura::comGancho(QByteArrayLiteral("segredo"), &observaLiberacao);
        SenhaSegura destino = std::move(origem);
        QCOMPARE(destino.tamanho(), std::size_t{7});
    }
    QCOMPARE(gChamadasGancho, 1);
    QVERIFY(gTodosZeroNoGancho);
}

// Sem MAC não há verificação de senha pelo MAC: a senha errada falha fechado, nunca carrega.
void TesteCarregadorPkcs12::pfxSemMacComSenhaErradaFalhaFechado()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = SenhaSegura::de(QByteArrayLiteral("senha-errada"));
    auto resultado = carregador.carregar(fixture(QStringLiteral("sem_mac.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QVERIFY2(resultado.error() == ErroCredencial::PfxMalformado || resultado.error() == ErroCredencial::SenhaErrada,
             descreverErro(resultado).c_str());
}

void TesteCarregadorPkcs12::pfxSemMacComSenhaCertaCarrega()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("sem_mac.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
}

// Chave privada fora de bag cifrado (ADR-0002): a senha está certa, e mesmo assim o carregador recusa.
void TesteCarregadorPkcs12::pfxComChaveSemCifraRecusado()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("chave_sem_cifra.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::PfxMalformado);
}

// PKCS#12 com a chave de OUTRO par, montado em memória: o openssl pkcs12 -export recusa esse par, e a API PKCS12_create
// não confere. A chave é gerada nesta execução e nada é gravado em disco. Devolve vazio se a montagem falhar.
QByteArray pfxComChaveDeOutroPar(const QByteArray& senha) {
    const QByteArray certPem = fixture(QStringLiteral("cnpj_ok.pem"));
    BIO* bioCert = BIO_new_mem_buf(certPem.constData(), static_cast<int>(certPem.size()));
    X509* cert = bioCert != nullptr ? PEM_read_bio_X509(bioCert, nullptr, nullptr, nullptr) : nullptr;
    BIO_free(bioCert);
    EVP_PKEY* chave = EVP_RSA_gen(2048);
    QByteArray saida;
    if (cert != nullptr && chave != nullptr) {
        PKCS12* p12 = PKCS12_create(senha.constData(), "teste", chave, cert, nullptr, 0, 0, 0, 0, 0);
        if (p12 != nullptr) {
            BIO* bioSaida = BIO_new(BIO_s_mem());
            if (bioSaida != nullptr && i2d_PKCS12_bio(bioSaida, p12) == 1) {
                char* dados = nullptr;
                const long tamanho = BIO_get_mem_data(bioSaida, &dados);
                saida = QByteArray(dados, static_cast<qsizetype>(tamanho));
            }
            BIO_free(bioSaida);
            PKCS12_free(p12);
        }
    }
    X509_free(cert);
    EVP_PKEY_free(chave);
    return saida;
}

// PKCS#12 íntegro e senha certa, mas a chave não pertence ao certificado: recusada.
void TesteCarregadorPkcs12::chaveNaoConfereRecusada()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    const QByteArray senhaBruta = senhaBrutaDaFixture();
    const QByteArray pfx = pfxComChaveDeOutroPar(senhaBruta);
    QVERIFY2(!pfx.isEmpty(), "não foi possível montar o PKCS#12 de teste com a chave de outro par");
    auto senha = SenhaSegura::de(senhaBruta);
    auto resultado = carregador.carregar(pfx, senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::PfxMalformado);
}

// Mesmo processo com OPENSSL_MODULES vazio (entrada do CMake): senha errada no .pfx moderno é SenhaErrada.
// Mutante M4 (tentar o legado sempre) daria CifraLegadaIndisponivel aqui, porque o provider não carrega.
void TesteCarregadorPkcs12::senhaErradaSemProvedorNaoTentaLegado()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = SenhaSegura::de(QByteArrayLiteral("senha-errada"));
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SenhaErrada);
}

void TesteCarregadorPkcs12::senhaCorretaCarrega()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QVERIFY(resultado->certificado() != nullptr);
    QVERIFY(resultado->chavePrivada() != nullptr);
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
}

void TesteCarregadorPkcs12::senhaErradaDaErroProprio()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = SenhaSegura::de(QByteArrayLiteral("senha-errada"));
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SenhaErrada);
}

void TesteCarregadorPkcs12::senhaErradaNoLegadoDaErroProprio()
{
    // A senha errada falha no MAC, antes de qualquer cifra: não pode virar CifraLegadaIndisponivel.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = SenhaSegura::de(QByteArrayLiteral("senha-errada"));
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok_legado.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SenhaErrada);
}

void TesteCarregadorPkcs12::cifraLegadaCarregaComProviderLegado()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok_legado.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
}

// Roda em processo próprio, com OPENSSL_MODULES apontando para uma pasta vazia (CMake): o provider legacy não carrega.
// O .pfx legado tem de falhar com CifraLegadaIndisponivel, e não com um erro genérico.
void TesteCarregadorPkcs12::cifraLegadaIndisponivelSemProvider()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok_legado.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::CifraLegadaIndisponivel);
}

// Mesmo processo sem provider legacy: o .pfx moderno tem de abrir. Se o carregador tentasse o legacy sem
// motivo "unsupported", este teste reprovaria (retentativa indevida).
void TesteCarregadorPkcs12::modernoNaoPrecisaDeLegadoSemProvider()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
}

void TesteCarregadorPkcs12::legadoNaoDesligaODefault()
{
    // Depois de abrir o legado, o processo não pode ter o provider legacy no contexto global, e o caminho
    // moderno tem de continuar funcionando na mesma execução.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senhaLegado = senhaDaFixture();
    auto legado = carregador.carregar(fixture(QStringLiteral("cnpj_ok_legado.pfx")), senhaLegado, *ancoras);
    QVERIFY2(legado.has_value(), descreverErro(legado).c_str());
    QCOMPARE(OSSL_PROVIDER_available(nullptr, "legacy"), 0);
    auto senhaModerno = senhaDaFixture();
    auto moderno = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senhaModerno, *ancoras);
    QVERIFY2(moderno.has_value(), descreverErro(moderno).c_str());
}

void TesteCarregadorPkcs12::vencidoSinalizado()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("vencido.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::Vencido);
}

void TesteCarregadorPkcs12::folhaComValidadeFuturaReprova()
{
    // notBefore em 2099: a folha ainda não vale. Reprova com erro próprio, não com Vencido nem CadeiaInvalida.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("folha_futura.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::AindaNaoValido);
}

void TesteCarregadorPkcs12::validadeIntermediariaVencidaReprova()
{
    // Folha válida sob subordinada vencida: a cadeia reprova pela validade da intermediária (sem NO_CHECK_TIME).
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("intermediaria_vencida.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::CadeiaInvalida);
}

void TesteCarregadorPkcs12::semEkuClientAuthSinalizado()
{
    // EKU presente (emailProtection) e sem clientAuth: recusado. A ausência total de EKU é outro caso (aceito).
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("sem_clientauth.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SemAutenticacaoCliente);
}

void TesteCarregadorPkcs12::ekuMalformadaReprova()
{
    // A extensão EKU existe e não decodifica: não pode ser tratada como ausente (aceita) só porque o parse falhou.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("eku_malformada.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SemAutenticacaoCliente);
}

void TesteCarregadorPkcs12::credencialSemEkuMarcadaNoIndicador()
{
    // O carregador puro só marca a ausência de EKU no indicador. O evento de auditoria é do adaptador no Mid (DA-2.30).
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("eku_ausente.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QVERIFY(resultado->ekuAusente());
    // Controle: uma credencial com EKU clientAuth não marca a ausência.
    auto senhaControle = senhaDaFixture();
    auto comEku = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senhaControle, *ancoras);
    QVERIFY2(comEku.has_value(), descreverErro(comEku).c_str());
    QVERIFY(!comEku->ekuAusente());
}

void TesteCarregadorPkcs12::cnpjExtraidoDoCertificado()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().tipo(), TipoInscricao::Cnpj);
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
    QVERIFY(!resultado->ekuAusente());
}

void TesteCarregadorPkcs12::cnpjAlfanumericoExtraidoComoTexto()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_alfanumerico.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().tipo(), TipoInscricao::Cnpj);
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_ALFANUMERICO));
}

void TesteCarregadorPkcs12::cpfExtraidoDoCertificado()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cpf_ok.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().tipo(), TipoInscricao::Cpf);
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CPF_OK));
}

void TesteCarregadorPkcs12::raizComparadaComoTexto()
{
    auto primeiro = Inscricao::cnpj(QStringLiteral("12ABC34501DE35"));
    auto mesmaRaiz = Inscricao::cnpj(QStringLiteral("12ABC34599ZZ99"));
    auto outraRaiz = Inscricao::cnpj(QStringLiteral("12ABC34601DE35"));
    auto numerico = Inscricao::cnpj(QStringLiteral("12345678000195"));
    auto numericoComLetraNaCauda = Inscricao::cnpj(QStringLiteral("12345678ZZ0195"));
    auto cpf = Inscricao::cpf(QStringLiteral("12345678909"));
    QVERIFY(primeiro && mesmaRaiz && outraRaiz && numerico && numericoComLetraNaCauda && cpf);
    // Mesma raiz como texto, mesmo com letras: a comparação não converte para número.
    QVERIFY(mesmaRaizCnpj(*primeiro, *mesmaRaiz));
    QVERIFY(!mesmaRaizCnpj(*primeiro, *outraRaiz));
    QVERIFY(mesmaRaizCnpj(*numerico, *numericoComLetraNaCauda));
    // CPF não tem raiz de CNPJ: nunca é mesma raiz.
    QVERIFY(!mesmaRaizCnpj(*numerico, *cpf));
}

void TesteCarregadorPkcs12::inscricaoFormatoInvalidoRecusado()
{
    QVERIFY(!Inscricao::cnpj(QStringLiteral("1234567800019")).has_value());   // 13 caracteres
    QVERIFY(!Inscricao::cnpj(QStringLiteral("1234567800019X")).has_value()); // DV com letra
    QVERIFY(!Inscricao::cpf(QStringLiteral("1234567890A")).has_value());      // letra no CPF
    QVERIFY(!Inscricao::cpf(QStringLiteral("123456789012")).has_value());     // 12 dígitos
}

void TesteCarregadorPkcs12::semInscricaoSinalizada()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("sem_identificacao.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SemInscricao);
}

void TesteCarregadorPkcs12::cnSemOidNaoViraInscricao()
{
    // O CN traz um CNPJ válido, mas não há otherName: o CN nunca é usado como inscrição.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cn_com_cnpj_sem_san.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SemInscricao);
}

void TesteCarregadorPkcs12::otherNameDeOutroOidNaoViraInscricao()
{
    // otherName de OID 2.16.76.1.3.4 com um CNPJ válido: não é o OID de CNPJ nem o de CPF, então não vira inscrição.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("outro_oid_sem_inscricao.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SemInscricao);
}

void TesteCarregadorPkcs12::cadeiaDeOutraRaizReprova()
{
    // Autoassinado com CNPJ, EKU clientAuth e validade: só a cadeia o recusa.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cadeia_outra_raiz.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::CadeiaInvalida);
}

void TesteCarregadorPkcs12::cadeiaDeOutraSubordinadaReprova()
{
    // Folha de uma subordinada que pertence a outra raiz: a cadeia está completa no .pfx, mas não chega à âncora.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cadeia_outra_sub.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::CadeiaInvalida);
}

void TesteCarregadorPkcs12::cadeiaIncompletaReprova()
{
    // Folha da subordinada sem a subordinada no .pfx: a âncora (AC raiz) não alcança a folha.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_sub_sem_intermediaria.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::CadeiaInvalida);
}

void TesteCarregadorPkcs12::cadeiaComSubordinadaCarrega()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_sub.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
}

void TesteCarregadorPkcs12::pfxMalformadoReprova()
{
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(QByteArrayLiteral("isto nao e um pkcs12"), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::PfxMalformado);
}

void TesteCarregadorPkcs12::carregarZeraSenha()
{
    // A senha entra por referência e sai zerada, com sucesso ou falha: o chamador vê o próprio objeto vazio.
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    QVERIFY(senha.tamanho() > 0);
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(senha.tamanho(), std::size_t{0});
}

void TesteCarregadorPkcs12::ancorasDePemLemUmCertificado()
{
    auto ancoras = Ancoras::dePem(fixture(QStringLiteral("ac.pem")));
    QVERIFY(ancoras.has_value());
    QCOMPARE(ancoras->quantidade(), std::size_t{1});
    QCOMPARE(ancoras->certificados().size(), std::size_t{1});
}

void TesteCarregadorPkcs12::ancorasSemCertificadoReprovam()
{
    auto ancoras = Ancoras::dePem(QByteArrayLiteral("lixo que nao e pem"));
    QVERIFY(!ancoras.has_value());
    QCOMPARE(ancoras.error(), ErroAncora::PemSemCertificado);
}

void TesteCarregadorPkcs12::senhaSeguraZeraConteudo()
{
    // Zerar não libera o buffer: o ponteiro segue válido e os bytes do segredo viram zero.
    SenhaSegura senha = SenhaSegura::de(QByteArrayLiteral("segredo"));
    QCOMPARE(senha.tamanho(), std::size_t{7});
    const char* bytes = senha.comoC();
    QVERIFY(bytes != nullptr);
    senha.zerar();
    QCOMPARE(senha.tamanho(), std::size_t{0});
    for (std::size_t i = 0; i < 7; ++i) {
        QCOMPARE(bytes[i], '\0');
    }
}

void TesteCarregadorPkcs12::descartarLiberaChave()
{
    // CredencialX509 não é copiável (a chave tem um dono). O descarte solta chave e certificado agora.
    // A zeragem dos componentes privados do RSA é do OpenSSL (BN_clear_free); ler memória liberada seria UB,
    // então esta suíte prova o descarte, não a zeragem.
    static_assert(!std::is_copy_constructible_v<CredencialX509>);
    static_assert(!std::is_copy_assignable_v<CredencialX509>);
    CarregadorPkcs12 carregador;
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QVERIFY(!resultado->descartada());
    resultado->descartar();
    QVERIFY(resultado->descartada());
    QVERIFY(resultado->chavePrivada() == nullptr);
    QVERIFY(resultado->certificado() == nullptr);
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
}

QTEST_APPLESS_MAIN(TesteCarregadorPkcs12)
#include "tst_carregador_pkcs12.moc"
