// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Falhas de envio do ClienteSefin (NL-3.C, S2 a S4; S5b, DA-3C.22): os 16 slots da causa tipada, movidos byte a byte de
// tst_cliente_sefin.cpp, e o 403 sem código da NL-3.L (passo 15). Montagem e apoio nos átomos de teste da S5c. Slot por CTest (mid.sefin.cliente.<nome>). Falha é QVERIFY.
#include <nl/back/dps/dps_assinada.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/inscricao.h>
#include <nl/back/dps/id_dps.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/back/ports/gateway_sefin.h>

#include <nl/foundation/log/log.h>
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/foundation/transporte/transporte_https.h>

#include <nl/mid/sefin/cliente/allowlist_log_sefin.h>
#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>
#include <nl/mid/sefin/cliente/comparador_infdps_c14n.h>
#include <nl/mid/sefin/cliente/cliente_sefin.h>
#include <nl/mid/sefin/cliente/leitor_erro_sefin.h>
#include <nl/mid/sefin/cliente/politica_retry.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIODevice>
#include <QProcess>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QTcpServer>
#include <QHostAddress>
#include <QTemporaryDir>
#include <QTcpSocket>
#include <QtTest>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "credencial_teste.h"
#include "dps_assinada_teste.h"
#include "fixture_certificado_teste.h"
#include "montagem_cliente_sefin.h"
#include "posts_no_servidor.h"
#include "sequencia_requisicoes.h"
#include "servidor_de_teste.h"
#include "tls_cliente_teste.h"
#include "repassador_tcp.h"

using nl::teste::RepassadorTcp;

namespace {

// NL-3.C (S2, 4.6b): mesmo PKCS12 que o teste do transporte usa (cadeia_outra_raiz), só o certificado: é a âncora que não
// é a do servidor de teste, e a verificação do servidor tem de recusá-la.
QSslCertificate certificadoDeOutraRaiz()
{
    const QByteArray pfx = lerFixture(QStringLiteral("cadeia_outra_raiz.pfx"));
    QFile senhaArquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    const QByteArray senha = senhaArquivo.open(QIODevice::ReadOnly) ? QByteArray(senhaArquivo.readLine().trimmed()) : QByteArray();
    BIO* bio = BIO_new_mem_buf(pfx.constData(), static_cast<int>(pfx.size()));
    PKCS12* p12 = bio != nullptr ? d2i_PKCS12_bio(bio, nullptr) : nullptr;
    BIO_free(bio);
    if (p12 == nullptr) {
        ERR_clear_error();
        return QSslCertificate();
    }
    EVP_PKEY* chave = nullptr;
    X509* cert = nullptr;
    STACK_OF(X509)* ca = nullptr;
    const int ok = PKCS12_parse(p12, senha.constData(), &chave, &cert, &ca);
    PKCS12_free(p12);
    sk_X509_pop_free(ca, X509_free);
    EVP_PKEY_free(chave);
    QSslCertificate resultado;
    if (ok == 1 && cert != nullptr) {
        unsigned char* der = nullptr;
        const int tamanho = i2d_X509(cert, &der);
        if (tamanho > 0 && der != nullptr) {
            resultado = QSslCertificate(QByteArray(reinterpret_cast<const char*>(der), tamanho), QSsl::Der);
        }
        OPENSSL_free(der);
    }
    X509_free(cert);
    return resultado;
}

// Montagem com a política e a configuração TLS dadas. logCompleto: o log leva bytesEscritos e etapa (allowlist do app).
std::unique_ptr<Montagem> montarComTls(const ft::ConfiguracaoTls& tls, int porta, const mc::PoliticaRetry& politica, bool logCompleto)
{
    auto m = std::make_unique<Montagem>();
    if (logCompleto) {
        m->log = std::make_unique<fl::Log>(m->destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    } else {
        m->log = std::make_unique<fl::Log>(
            m->destino, "sefin",
            std::vector<fl::DeclaracaoCampo>{{"metodo", fl::Regra::Cru}, {"status", fl::Regra::Cru},
                                             {"codigoE", fl::Regra::Cru}, {"etapa", fl::Regra::Cru}},
            std::vector<std::string>{"sefin.requisicao", "sefin.consulta", "sefin.emissao", "sefin.erro", "sefin.transporte_falhou"});
    }
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    m->transporte = std::make_unique<ft::TransporteHttps>(base, tls);
    m->cliente = std::make_unique<mc::ClienteSefin>(*m->transporte, *m->log, politica, nullptr, nullptr);
    return m;
}

} // namespace

class TesteClienteSefinFalhasEnvio : public QObject {
    Q_OBJECT

private slots:
    // NL-3.C (S2, tabela 1.5): a consulta GET /dps que responde 200 sem chave ou com corpo que não é objeto não é Definitiva
    // sem causa: é a resposta que o cliente não lê (RespostaSemCodigoValido, linhas 99 e 168 do cliente).
    void consultaDps200SemChaveDaRespostaSemCodigo()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_200_sem_chave")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->consultarDps(dps->id);
        QVERIFY2(!resultado.has_value(), "200 sem chave não é chave de acesso");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "resposta sem chave é Definitiva, sem consulta");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::RespostaSemCodigoValido, "a causa é a resposta que o cliente não lê");
    }

    void consultaDps200NaoObjetoDaRespostaSemCodigo()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_200_nao_objeto")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->consultarDps(dps->id);
        QVERIFY2(!resultado.has_value(), "200 com corpo que não é objeto não é chave");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "corpo que não é objeto é Definitiva");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::RespostaSemCodigoValido, "a causa é a resposta que o cliente não lê");
    }

    void consultaNota200NaoObjetoDaRespostaSemCodigo()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        const auto chave = bk::ChaveAcesso::de(QStringLiteral("12345678901234567890123456789012345678901234567890"));
        QVERIFY2(chave.has_value(), "a chave de teste tem de ser uma chave de acesso válida");
        ServidorDeTeste srv({QStringLiteral("GET /nfse/12345678901234567890123456789012345678901234567890 nfse_200_nao_objeto")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->consultarNota(*chave);
        QVERIFY2(!resultado.has_value(), "200 que não é objeto não é nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "corpo que não é objeto é Definitiva");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::RespostaSemCodigoValido, "a causa é a resposta que o cliente não lê");
    }

    void consultaDps404SemCodigoValidoDaRespostaSemCodigo()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_404_sem_codigo")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->consultarDps(dps->id);
        QVERIFY2(!resultado.has_value(), "404 sem código válido não é chave");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Rejeitada, "404 com código fora do formato segue Rejeitada, sem código E");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::RespostaSemCodigoValido, "a causa é a resposta sem código válido");
    }

    void consultaNotaInexistenteDaRespostaSemCodigo()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        const auto chave = bk::ChaveAcesso::de(QStringLiteral("12345678901234567890123456789012345678901234567890"));
        QVERIFY2(chave.has_value(), "a chave de teste tem de ser uma chave de acesso válida");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->consultarNota(*chave);
        QVERIFY2(!resultado.has_value(), "chave que o servidor não conhece não é nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Rejeitada, "404 NFSE_NAO_ENCONTRADA segue Rejeitada, sem código E");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::RespostaSemCodigoValido, "o código do 404 não é válido: resposta sem código");
    }

    // Rede: porta sem servidor. A consulta GET /dps esgota as tentativas e a causa é a rede, sem código E e sem texto.
    void consultaDpsSemConexaoDaRede()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        mc::PoliticaRetry politica;
        politica.base = 1ms;
        auto m = montarComTls(configuracaoTls(*cred), 1, politica, false);
        auto resultado = m->cliente->consultarDps(dps->id);
        QVERIFY2(!resultado.has_value(), "sem servidor não há chave");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Transitoria, "rede na consulta é Transitoria");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::Rede, "a causa é a rede");
        QVERIFY2(resultado.error().codigoE.empty() && resultado.error().mensagem.empty(), "falha de rede não leva código E nem texto");
    }

    // TLS 1.2 sem certificado de cliente: a recusa no handshake, sem erro de verificação do servidor, é a causa do certificado do emissor.
    void consultaDpsRecusadaNoTls12DaCertificadoProvavel()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({}, QStringLiteral("1.2"));
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montarSemCertificado(srv.porta());
        auto resultado = m->cliente->consultarDps(dps->id);
        QVERIFY2(!resultado.has_value(), "sem certificado não há chave");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "recusa do TLS na consulta é Definitiva (erro do Qt de cliente, não de rede)");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::CertificadoDoEmissorProvavel, "sem erro de verificação do servidor, a causa é o certificado do emissor");
    }

    void consultaDps5xxPersistenteDaSefinIndisponivel()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 erro5xx"),
                             QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 erro5xx"),
                             QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->consultarDps(dps->id);
        QVERIFY2(!resultado.has_value(), "5xx em todas as tentativas não é chave");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Transitoria, "5xx até a última tentativa é Transitoria");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::SefinIndisponivel, "a causa é o sistema nacional indisponível");
        const std::vector<std::string> esperado{"GET:503", "GET:503", "GET:503"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "as três tentativas da consulta foram feitas");
    }

    // Caracterização (verde em V e em G): com zero tentativas, a consulta não sai da máquina.
    void consultaSemTentativaNaoEnviaNada()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        mc::PoliticaRetry politica;
        politica.base = 1ms;
        politica.tentativas = 0;
        auto m = montarComTls(configuracaoTls(*cred), srv.porta(), politica, false);
        auto resultado = m->cliente->consultarDps(dps->id);
        QVERIFY2(!resultado.has_value(), "sem tentativa, não há chave");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Transitoria, "sem tentativa, o resultado é Transitoria");
        QVERIFY2(sequencia(m->destino.str()).empty(), "sem tentativa, nenhuma requisição sai");
    }

    // E2404 em todas as tentativas, com POST 5xx a cada envio: na última, sem tentativa restante, a nota não existe (C6).
    void naoGeradaSemTentativaDaNaoConcluido()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx"), QStringLiteral("POST /nfse erro5xx"),
                             QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem nota, não há desfecho de sucesso");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Transitoria, "sem tentativa restante, com E2404, é Transitoria");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::NaoGerada, "a causa é a nota que não foi gerada");
        const std::vector<std::string> esperado{"POST:503", "GET:404", "POST:503", "GET:404", "POST:503", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "cada envio consulta antes do próximo");
    }

    // Consulta com 5xx em todas as tentativas, depois de um POST 5xx: o estado é Indeterminado, com a causa da consulta.
    void consulta5xxDepoisDoEnvioPropagaACausa()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        const QString consulta = QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 erro5xx");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx"), consulta, consulta, consulta});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem consulta resolvida, não há nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado, "POST 5xx com consulta sem resposta é Indeterminado");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::SefinIndisponivel, "a causa da consulta chega ao erro");
        const std::vector<std::string> esperado{"POST:503", "GET:503", "GET:503", "GET:503"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "a consulta esgota as tentativas e o POST não repete");
    }

    void conflito409ComConsulta5xxPropagaACausa()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        const QString consulta = QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 erro5xx");
        ServidorDeTeste srv({QStringLiteral("POST /nfse conflito409"), consulta, consulta, consulta});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "409 sem consulta resolvida não é nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado, "409 com consulta 5xx é Indeterminado, nunca DpsDuplicada");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::SefinIndisponivel, "a causa da consulta chega ao erro");
        const std::vector<std::string> esperado{"POST:409", "GET:503", "GET:503", "GET:503"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "a consulta esgota as tentativas e o POST não repete");
    }

    // Caracterização (verde em V e em G): com zero tentativas, o envio não sai da máquina.
    void emitirSemTentativaNaoEnviaNada()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        mc::PoliticaRetry politica;
        politica.base = 1ms;
        politica.tentativas = 0;
        auto m = montarComTls(configuracaoTls(*cred), srv.porta(), politica, false);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem tentativa, não há nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Transitoria, "sem tentativa, o resultado é Transitoria");
        QVERIFY2(sequencia(m->destino.str()).empty(), "sem tentativa, nenhum POST sai");
        QVERIFY2(postsNoServidor(srv.porta(), *cred) == 0, "o servidor não recebeu POST nenhum");
    }

    // Adendo da S2 (tabela 1.5, linha C2): servidor de outra raiz, sem bytes escritos: o app não confirmou a identidade do servidor.
    void servidorDeOutraRaizSemBytesDaServidorNaoConfirmado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        ft::ConfiguracaoTls tls = configuracaoTls(*cred);
        tls.autoridades = QList<QSslCertificate>{certificadoDeOutraRaiz()};
        QVERIFY2(!tls.autoridades.first().isNull(), "o certificado de outra AC tem de ler");
        mc::PoliticaRetry politica;
        politica.base = 1ms;
        auto m = montarComTls(tls, srv.porta(), politica, false);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "servidor não confirmado não recebe a DPS");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "recusa de identidade do servidor, sem bytes, é Definitiva sem consulta");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::ServidorNaoConfirmado, "a causa é o servidor não confirmado, não o certificado do emissor");
        QVERIFY2(sequencia(m->destino.str()).empty(), "sem bytes escritos não há GET");
        QVERIFY2(postsNoServidor(srv.porta(), *cred) == 0, "o servidor não recebeu POST nenhum");
    }

    // NL-3.C (S2, 4.6b; R-10 b do CISO): primeiro envio chega ao servidor sem gravar (503), a consulta responde E2404, o
    // reenvio falha com zero bytes. Nunca causa de C1, C2, C3 ou C8: é NaoGerada (C6), sem reenvio às cegas.
    void zeroBytesNoReenvioDepoisDeE2404NaoDizNadaFoiEnviado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        RepassadorTcp repassador(srv.porta(), 2);
        QVERIFY2(repassador.escutar(), "o repassador de teste tem de escutar em LocalHost");
        mc::PoliticaRetry politica;
        politica.base = 1ms;
        auto m = montarComTls(configuracaoTls(*cred), repassador.porta(), politica, true);
        auto resultado = m->cliente->emitir(*dps);
        const int posts = postsNoServidor(srv.porta(), *cred);
        qInfo().noquote() << QStringLiteral("medicao zeroBytesNoReenvio;conexoes=%1;repassadas=%2;posts=%3")
                                 .arg(repassador.conexoes()).arg(repassador.repassadas()).arg(posts);
        QVERIFY2(!resultado.has_value(), "o reenvio sem bytes não entrega nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Transitoria, "zero bytes num reenvio, depois de E2404, é Transitoria");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::NaoGerada, "zero bytes num reenvio é NaoGerada (C6), nunca causa de recusa");
        const std::vector<std::string> esperado{"POST:503", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "o reenvio nunca tem resposta, e a consulta vem antes");
        QVERIFY2(posts == 1, "o servidor recebeu um POST só: o reenvio não chegou à aplicação");
        QVERIFY2(repassador.repassadas() == 2, "só as duas primeiras conexões chegaram ao servidor");
        QVERIFY2(repassador.conexoes() >= 3, "o reenvio abriu uma terceira conexão, fechada na chegada");
        // Último evento de falha de transporte: POST com zero bytes (ramo de cliente_sefin.cpp, com envio > 1).
        std::string ultimo;
        std::istringstream linhas(m->destino.str());
        for (std::string linha; std::getline(linhas, linha);) {
            if (linha.find("\"evento\":\"sefin.transporte_falhou\"") != std::string::npos) {
                ultimo = linha;
            }
        }
        QVERIFY2(!ultimo.empty(), "o reenvio que falhou tem de estar no log como sefin.transporte_falhou");
        QVERIFY2(ultimo.find("\"etapa\":\"POST\"") != std::string::npos, "a falha do reenvio é de etapa POST");
        QVERIFY2(ultimo.find("\"bytesEscritos\":\"0\"") != std::string::npos, "o reenvio falhou com zero bytes escritos");
    }

    // NL-3.C (S2, passo 4.6d, v5): 409 com a DPS gravada; a consulta da chave responde 200 e a busca da NFS-e cai por 5xx em
    // todas as tentativas. Não se sabe se a nota é a desta DPS: Indeterminado, com a causa da busca (linha 315 do cliente).
    void conflito409ComNotaIndisponivelPropagaACausa()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste primeiro({});
        QVERIFY2(primeiro.porta() > 0, qPrintable(primeiro.diagnostico()));
        auto m1 = montar(primeiro.porta(), *cred);
        auto nota = m1->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "o primeiro servidor tem de gerar a nota, para ler a chave");
        const QString busca = QStringLiteral("GET /nfse/") + nota->chave.texto() + QStringLiteral(" erro5xx");
        QStringList falhas{QStringLiteral("POST /nfse conflito409_gravado")};
        falhas << busca << busca << busca;
        ServidorDeTeste segundo(falhas);
        QVERIFY2(segundo.porta() > 0, qPrintable(segundo.diagnostico()));
        auto m2 = montar(segundo.porta(), *cred);
        auto resultado = m2->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem a nota buscada, o 409 não vira nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado, "409 com a busca da nota sem resposta é Indeterminado, nunca DpsDuplicada");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::SefinIndisponivel, "a causa da busca da nota chega ao erro");
        const std::vector<std::string> esperado{"POST:409", "GET:200", "GET:503", "GET:503", "GET:503"};
        QVERIFY2(sequencia(m2->destino.str()) == esperado, "a busca da nota tentou três vezes e o POST não repetiu");
        QVERIFY2(postsNoServidor(segundo.porta(), *cred) == 1, "o servidor recebeu o POST uma vez");
    }

    // NL-3.L (passo 15): 403 da porta de entrada, em HTML e sem código E. É Definitiva, sem nova tentativa e sem consulta, e o
    // log leva só o tipo de conteúdo e o tamanho do corpo (139 bytes, PAGINA_403 do servidor falso), nunca o texto dele.
    void status403HtmlSemCodigoDefinitivoSemNovaTentativa()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse status403_html")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        mc::PoliticaRetry politica;
        politica.base = 1ms;
        auto m = montarComTls(configuracaoTls(*cred), srv.porta(), politica, true);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "403 não pode emitir");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "403 sem código é Definitiva");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::RespostaSemCodigoValido, "a causa é a resposta sem código válido");
        QVERIFY2(resultado.error().codigoE.empty(), "403 sem código não ganha código E");
        const std::vector<std::string> esperado{"POST:403"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "403 não repete o POST nem consulta");
        QVERIFY2(postsNoServidor(srv.porta(), *cred) == 1, "o servidor recebeu o POST uma vez");
        const std::string saida = m->destino.str();
        QVERIFY2(saida.find("sefin.resposta_sem_codigo") != std::string::npos, "o 403 sem código tem de ir ao log");
        QVERIFY2(saida.find("\"tipoMidia\":\"html\"") != std::string::npos, "o log tem de dizer que o corpo era HTML");
        QVERIFY2(saida.find("\"bytesResposta\":\"139\"") != std::string::npos, "o log tem de trazer o tamanho do corpo");
        QVERIFY2(saida.find("12345678909") == std::string::npos, "o CPF do corpo não pode aparecer no log");
        QVERIFY2(saida.find("Forbidden") == std::string::npos, "o texto do corpo não pode aparecer no log");
    }
};

// QTEST_GUILESS_MAIN cria o QCoreApplication: o transporte espera pelo QEventLoop e o seu tempo limite é um QTimer, e
// nenhum dos dois funciona sem laço de eventos (com APPLESS, o envio ficava sem prazo até o timeout de 300 s do QTest).
QTEST_GUILESS_MAIN(TesteClienteSefinFalhasEnvio)
#include "tst_cliente_sefin_falhas_envio.moc"
