// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do carregador de A1 (NL-2.2, ADR-S1). Cada slot roda por nome no CTest (pkcs12.<slot>).
// Fixtures sintéticas em NL_CERTIFICADOS_DIR, geradas pela NL-2.1 (AC de teste). Nenhum .pfx real é lido.
// Nenhum helper lança exceção: falha de leitura vira QVERIFY no teste que chamou (vermelho por asserção, L-35).
// A senha é passada por referência: o carregador a zera, e o teste confere a zeragem pelo próprio objeto.
// Slots com provedor ausente (OPENSSL_MODULES apontando para pasta vazia) rodam em processo próprio, pelo CMake.

#include <nl/foundation/log/log.h>
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/inscricao.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include <openssl/provider.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <expected>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

using nl::foundation::log::Log;
using namespace nl::foundation::pkcs12;

namespace {

using Resultado = std::expected<CredencialX509, ErroCredencial>;

const char* const CNPJ_OK = "12345678000195";
const char* const CNPJ_ALFANUMERICO = "12ABC34501DE35";
const char* const CPF_OK = "12345678909";
const char* const EVENTO_EKU_AUSENTE = "credencial.eku_ausente";

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

// Registro de auditoria em memória: o teste lê o texto gravado e confere o que pode (e o que não pode) aparecer.
struct RegistroTeste {
    std::ostringstream saida;
    Log log{saida, "notalivre", {}, {EVENTO_EKU_AUSENTE}};
};

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
    void ekuAusenteAceitaComEventoSemPii();
    void registrarFalhaFechaCredencial();
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
};

void TesteCarregadorPkcs12::senhaCorretaCarrega()
{
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = SenhaSegura::de(QByteArrayLiteral("senha-errada"));
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_ok_legado.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SenhaErrada);
}

void TesteCarregadorPkcs12::cifraLegadaCarregaComProviderLegado()
{
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("eku_malformada.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::SemAutenticacaoCliente);
}

void TesteCarregadorPkcs12::ekuAusenteAceitaComEventoSemPii()
{
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("eku_ausente.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QVERIFY(resultado->ekuAusente());
    const std::string evento = registro.saida.str();
    QVERIFY2(evento.find(EVENTO_EKU_AUSENTE) != std::string::npos, "evento de EKU ausente não foi gravado");
    QVERIFY2(evento.find(CNPJ_OK) == std::string::npos, "o evento não pode trazer o CNPJ");
    QVERIFY2(evento.find(CPF_OK) == std::string::npos, "o evento não pode trazer dígitos de identificação");
}

void TesteCarregadorPkcs12::registrarFalhaFechaCredencial()
{
    // Registro de auditoria que não consegue gravar (fluxo em badbit): sem o evento exigido, a credencial sem EKU
    // não é aceita. Falha fechada, com FalhaInterna.
    std::ostringstream saida;
    saida.setstate(std::ios::badbit);
    Log log(saida, "notalivre", {}, {EVENTO_EKU_AUSENTE});
    CarregadorPkcs12 carregador(log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("eku_ausente.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::FalhaInterna);
}

void TesteCarregadorPkcs12::cnpjExtraidoDoCertificado()
{
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_sub_sem_intermediaria.pfx")), senha, *ancoras);
    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error(), ErroCredencial::CadeiaInvalida);
}

void TesteCarregadorPkcs12::cadeiaComSubordinadaCarrega()
{
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
    auto ancoras = ancoraDeTeste();
    QVERIFY2(ancoras.has_value(), "ac.pem nao lido");
    auto senha = senhaDaFixture();
    auto resultado = carregador.carregar(fixture(QStringLiteral("cnpj_sub.pfx")), senha, *ancoras);
    QVERIFY2(resultado.has_value(), descreverErro(resultado).c_str());
    QCOMPARE(resultado->inscricao().valor(), QString::fromLatin1(CNPJ_OK));
}

void TesteCarregadorPkcs12::pfxMalformadoReprova()
{
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
    RegistroTeste registro;
    CarregadorPkcs12 carregador(registro.log);
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
