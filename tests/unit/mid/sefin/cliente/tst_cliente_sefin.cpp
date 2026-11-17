// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do ClienteSefin, da política de retry, do catálogo de erros e do leitor de erro (NL-2.8). Contra o servidor falso
// (tools/servidor_falso, pelo lancador, com oráculo): cada slot sobe o servidor como processo próprio.
// A sequência de requisições sai do Log do próprio cliente (evento sefin.requisicao, com metodo e status), sem ler o
// servidor. Slot por CTest (mid.sefin.cliente.<nome>). Falha é QVERIFY (L-35).
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
#include "comparador_falso.h"
#include "credencial_teste.h"
#include "dps_assinada_teste.h"
#include "montagem_cliente_sefin.h"
#include "posts_no_servidor.h"
#include "registro_observador.h"
#include "sequencia_requisicoes.h"
#include "servidor_de_teste.h"
#include "tls_cliente_teste.h"

namespace {

// NFS-e no formato do XSD (rodada 5): NFSe/infNFSe/DPS/infDPS, com a DPS assinada dentro de infNFSe.
QByteArray nfseDoXsd(const QByteArray& dps)
{
    const QByteArray corpo = dps.mid(dps.indexOf("<DPS"));
    return QByteArrayLiteral("<NFSe xmlns=\"http://www.sped.fazenda.gov.br/nfse\"><infNFSe Id=\"NFS00000000000000000000000000000000000000000000000000000\">") +
           corpo + QByteArrayLiteral("</infNFSe></NFSe>");
}

} // namespace

class TesteClienteSefin : public QObject {
    Q_OBJECT

private slots:
    // (E4 complemento, DA-2.75) falha de transporte no POST vai ao log com a etapa POST (porta sem servidor: recusa antes de qualquer byte).
    void falhaDePostVaiAoLogComEtapaPost();
    // (E4 complemento, DA-2.75) falha de transporte no GET vai ao log com a etapa GET.
    void falhaDeGetVaiAoLogComEtapaGet();
    // NL-2.10e (item 6, DA-2.82): âncoras TLS vazias são recusadas antes de qualquer byte, com causa de configuração no evento.
    void ancorasTlsVaziasSaiComCausaDeConfiguracao();
    void sucessoExtraiChaveDe50Posicoes()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "a Sefin de teste tem de aceitar a DPS assinada");
        QCOMPARE(nota->chave.texto().size(), qsizetype{50});
        QVERIFY(!nota->xmlNfse.isEmpty());
    }

    void postTransitorioConsultaDpsAntesDeReenviar()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "depois do 5xx, o cliente consulta e reenvia até aceitar");
        const auto itens = sequencia(m->destino.str());
        const std::vector<std::string> esperado{"POST:503", "GET:404", "POST:201"};
        QVERIFY2(itens == esperado, "o POST nunca repete sozinho: consulta GET /dps antes do reenvio");
    }

    void dps404E2404LiberaReenvio()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        QVERIFY2(m->cliente->emitir(*dps).has_value(), "com E2404 na consulta, o reenvio libera a emissão");
        const std::string saida = m->destino.str();
        QVERIFY2(saida.find("\"codigoE\":\"E2404\"") != std::string::npos,
                 "a consulta tem de registrar o código E2404 (só o código, nada mais)");
    }

    void conflito409ConsultaENaoReenvia()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse conflito409")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "409 não pode virar nota");
        // Rodada 5 (DA-2.41): DpsDuplicada só vale para conteúdo diferente. Com a consulta sem resolução, é Indeterminado.
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "409 com a consulta sem resolução é Indeterminado, nunca DpsDuplicada");
        const std::vector<std::string> esperado{"POST:409", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "409 consulta a DPS e nunca reenvia o POST");
    }

    // Rodada 5 (revisão, A): 409 e a consulta respondendo outro código (não 404 E2404, não chave): Indeterminado, nunca
    // DpsDuplicada. DpsDuplicada só vale para conteúdo diferente, e aqui não há como comparar.
    // Rodada 5d (revisão, B): o 409 cuja consulta devolve a chave compara o infDPS antes de devolver a nota. O mesmo Id
    // não garante o mesmo conteúdo (o princípio do adendo E0014). Igual: a nota recuperada.
    void conflito409ComMesmoInfDpsRecuperaANota()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse conflito409_gravado")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto igual = std::make_shared<ComparadorFalso>(mc::ResultadoComparacaoInfDps::Igual);
        auto m = montar(srv.porta(), *cred, igual);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "409 com o mesmo infDPS recupera a nota");
        QVERIFY2(nota->xmlNfse == dps->xml, "a nota recuperada é a DPS enviada");
        QVERIFY2(igual->chamadas_ == 1, "o 409 com chave compara o infDPS antes de devolver a nota");
        const std::vector<std::string> esperado{"POST:409", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "o 409 consulta a chave e busca a NFS-e");
    }

    // Rodada 5d (revisão, B): o 409 com infDPS diferente é colisão de numeração: DpsDuplicada, nunca a nota.
    void conflito409ComInfDpsDiferenteViraDpsDuplicada()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse conflito409_gravado")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto diferente = std::make_shared<ComparadorFalso>(mc::ResultadoComparacaoInfDps::Diferente);
        auto m = montar(srv.porta(), *cred, diferente);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "409 com infDPS diferente não pode virar nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::DpsDuplicada,
                 "colisão de numeração no 409 é DpsDuplicada, para o usuário decidir");
        const std::vector<std::string> esperado{"POST:409", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "a colisão não reenvia nem renumera a DPS");
    }

    void conflito409ComConsultaSemResolverViraIndeterminado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse conflito409"),
                             QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_404_outro")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "409 sem consulta resolvida não é nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "409 com a consulta sem resolução é Indeterminado, nunca DpsDuplicada");
        QVERIFY2(sequencia(m->destino.str()) == std::vector<std::string>({"POST:409", "GET:404"}),
                 "a consulta foi feita e o POST não repetiu");
    }

    void rejeicaoE0014NaoRepete()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        // Adendo da DA-2.41: E0014 reconcilia em qualquer tentativa. Aqui a DPS não foi gerada, e a consulta diz 404: não
        // há nota para comparar, então é Indeterminado. Nunca Rejeitada e nunca reenvio.
        QVERIFY2(!resultado.has_value(), "E0014 nunca vira nota sem reconciliação");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "E0014 sem nota para comparar é Indeterminado, nunca Rejeitada");
        const std::vector<std::string> esperado{"POST:400", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "E0014 consulta e não repete o POST");
    }

    void observadorRecebeCadaRespostaDaEmissao()
    {
        // NL-2.10c (DA-2.66, passo 1): o observador vê cada resposta HTTP da emissão, com método, modelo da rota (sem o Id),
        // status e Content-Type. A recusa E0014 sem nota faz POST /nfse 400 e GET /dps 404: as duas chegam ao observador.
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        RegistroObservador observador;
        auto m = montar(srv.porta(), *cred, nullptr, &observador);
        const auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "E0014 sem nota continua sendo recusa, não sucesso");
        QVERIFY2(observador.respostas.size() == 2,
                 qPrintable(QStringLiteral("o observador tinha de ver 2 respostas (POST e GET /dps); viu %1")
                                .arg(observador.respostas.size())));
        const RegistroObservador::Resposta& post = observador.respostas[0];
        QVERIFY2(post.metodo == "POST" && post.rota == "/nfse" && post.status == 400,
                 "a primeira resposta tem de ser o POST /nfse com status 400, com a rota sem Id");
        QVERIFY2(post.tipoConteudo.startsWith("application/json"), "o Content-Type do POST tem de chegar ao observador");
        const RegistroObservador::Resposta& consulta = observador.respostas[1];
        QVERIFY2(consulta.metodo == "GET" && consulta.rota == "/dps/{id}" && consulta.status == 404,
                 "a segunda resposta tem de ser o GET /dps com status 404, com a rota modelo /dps/{id}");
    }

    void certificadoRevogadoMapeadoPelaRejeicaoDaSefin()
    {
        // Revogação fora da v1 (decisão do líder, 29/10/2026): a recusa da Sefin com E1207 vira a mensagem do catálogo.
        // FATO (Anexo I de 20/10/2026, sharedStrings: "E1207" seguido de "Certificado de Transmissão revogado"). A linha
        // exata na aba RN é INFERÊNCIA, a conferir na NL-2.10.
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao:E1207")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "certificado revogado não pode emitir");
        QCOMPARE(QString::fromStdString(resultado.error().codigoE), QStringLiteral("E1207"));
        QCOMPARE(QString::fromStdString(resultado.error().mensagem),
                 QStringLiteral("Certificado de Transmissão revogado"));
    }

    void textoLivreDaApiComCpfSoLogaCodigoE()
    {
        // ADR-0005 (c): o texto livre da API nunca entra no log nem na mensagem. O servidor põe um CPF no texto livre.
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY(!resultado.has_value());
        const std::string saida = m->destino.str();
        QVERIFY2(saida.find("12345678909") == std::string::npos, "o CPF do texto livre não pode aparecer no log");
        QVERIFY2(saida.find("\"codigoE\":\"E0014\"") != std::string::npos, "o log tem de trazer o código E");
        QVERIFY2(resultado.error().mensagem.find("12345678909") == std::string::npos,
                 "a mensagem do erro não pode trazer o texto livre");
    }

    void getTransitorioRepeteComBackoff()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto consulta = m->cliente->consultarDps(dps->id);
        QVERIFY2(!consulta.has_value(), "sem DPS gerada, a consulta devolve erro, não chave");
        QVERIFY2(consulta.error().motivo == bk::MotivoFalhaSefin::DpsNaoGerada, "E2404 tem de ser DpsNaoGerada");
        QVERIFY2(consulta.error().causa == bk::CausaFalhaSefin::Nenhuma, "E2404 não tem causa de rede nem de resposta: a nota não existe");
        QVERIFY2(consulta.error().codigoE == "E2404", "o código E2404 vai no erro");
        const std::vector<std::string> esperado{"GET:503", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "GET repete com backoff depois do 5xx");
    }

    void leitorAceitaErroEErrosECodigoECodigo()
    {
        QVERIFY(mc::codigoDoErro(QByteArrayLiteral("{\"erro\":{\"Codigo\":\"E0014\",\"Descricao\":\"x\"}}")) ==
                std::optional<std::string>{"E0014"});
        QVERIFY(mc::codigoDoErro(QByteArrayLiteral("{\"erros\":[{\"codigo\":\"E1207\"}]}")) ==
                std::optional<std::string>{"E1207"});
        QVERIFY(mc::codigoDoErro(QByteArrayLiteral("{\"Codigo\":\"E2404\"}")) == std::optional<std::string>{"E2404"});
        QVERIFY(!mc::codigoDoErro(QByteArrayLiteral("{\"sem\":\"codigo\"}")).has_value());
        QVERIFY(!mc::codigoDoErro(QByteArrayLiteral("não é json")).has_value());
    }

    void catalogoDaMensagemDoCodigoE()
    {
        QVERIFY(mc::CatalogoErrosSefin::conhece("E1207"));
        QCOMPARE(QString::fromUtf8(mc::CatalogoErrosSefin::mensagem("E1207").data()),
                 QStringLiteral("Certificado de Transmissão revogado"));
        QVERIFY2(!mc::CatalogoErrosSefin::conhece("E9999"), "código fora do catálogo não é conhecido");
    }

    void xmlNfseGuardaXmlDescomprimido()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "a Sefin de teste tem de aceitar a DPS assinada");
        QVERIFY2(!nota->xmlPendente, "o XML de um sucesso legível não fica pendente");
        QVERIFY2(nota->xmlNfse == dps->xml, "xmlNfse guarda o XML descompactado, não o GZip nem o Base64");
    }

    void base64InvalidoRecusado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse nfse_base64_invalido")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "sucesso com caractere fora do Base64 devolve a chave, sem falha");
        QVERIFY2(nota->xmlPendente, "caractere fora do Base64 marca o XML como pendente");
        QVERIFY2(nota->xmlNfse.isEmpty(), "XML pendente não traz bytes");
        QCOMPARE(nota->chave.texto().size(), qsizetype{50});
    }

    // Rodada 5 (revisão, Base64): '=' só é aceito no fim (um ou dois caracteres). '=' no meio é recusado.
    void base64ComIgualNoMeioRecusado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse nfse_base64_igual_no_meio")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "sucesso com '=' no meio devolve a chave, sem falha");
        QVERIFY2(nota->xmlPendente, "'=' fora do fim do Base64 marca o XML como pendente");
        QVERIFY2(nota->xmlNfse.isEmpty(), "XML pendente não traz bytes");
    }

    void xmlIlegivelNumSucessoNaoLiberaReenvio()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        const QStringList roteiros{QStringLiteral("POST /nfse nfse_gzip_invalido"),
                                   QStringLiteral("POST /nfse nfse_acima_do_teto")};
        for (const QString& roteiro : roteiros) {
            ServidorDeTeste srv({roteiro});
            QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
            auto m = montar(srv.porta(), *cred);
            auto nota = m->cliente->emitir(*dps);
            QVERIFY2(nota.has_value(), "sucesso com XML ilegível devolve a chave, sem falha");
            QVERIFY2(nota->xmlPendente, "XML que não descompacta fica pendente");
            QVERIFY2(sequencia(m->destino.str()) == std::vector<std::string>{"POST:201"},
                     "XML ilegível num sucesso nunca libera reenvio: um POST e nenhuma consulta");
        }
    }

    void backoffDobraAteAPrimeiraRepeticao()
    {
        mc::PoliticaRetry politica;
        politica.base = 500ms;
        QCOMPARE(static_cast<qint64>(politica.esperaAntesDaTentativa(1).count()), qint64{0});
        QCOMPARE(static_cast<qint64>(politica.esperaAntesDaTentativa(2).count()), qint64{500});
        QCOMPARE(static_cast<qint64>(politica.esperaAntesDaTentativa(3).count()), qint64{1000});
    }

    // Adendo da DA-2.41: E0014 reconcilia já na primeira tentativa (a nota pode ter nascido numa execução anterior).
    void e0014NaPrimeiraTentativaReconcilia()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_gerada:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto igual = std::make_shared<ComparadorFalso>(mc::ResultadoComparacaoInfDps::Igual);
        auto m = montar(srv.porta(), *cred, igual);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "E0014 na primeira tentativa reconcilia com a nota, sem rejeitar");
        const std::vector<std::string> esperado{"POST:400", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "E0014 na primeira tentativa vai para a consulta");
    }

    // Adendo: com o mesmo infDPS, a nota recuperada é a da DPS enviada, e o comparador recebe as duas.
    void e0014ComMesmoInfDpsRecuperaANota()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_gerada:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto igual = std::make_shared<ComparadorFalso>(mc::ResultadoComparacaoInfDps::Igual);
        auto m = montar(srv.porta(), *cred, igual);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "mesmo infDPS é sucesso com a nota recuperada");
        QVERIFY2(!nota->xmlPendente, "a nota recuperada traz o XML, não fica pendente");
        QVERIFY2(nota->xmlNfse == dps->xml, "a nota recuperada é a DPS enviada");
        QVERIFY2(igual->chamadas_ == 1, "a reconciliação compara uma vez");
        QVERIFY2(igual->dpsRecebido_ == dps->xml, "o comparador recebe a DPS enviada");
        QVERIFY2(!igual->nfseRecebido_.isEmpty(), "o comparador recebe o XML devolvido pela NFS-e");
    }

    // Adendo: com infDPS diferente (colisão de numeração), é DpsDuplicada para o usuário: nunca renumera, nunca reenvia.
    void e0014ComInfDpsDiferenteViraDpsDuplicada()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_gerada:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto diferente = std::make_shared<ComparadorFalso>(mc::ResultadoComparacaoInfDps::Diferente);
        auto m = montar(srv.porta(), *cred, diferente);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "infDPS diferente não pode virar nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::DpsDuplicada,
                 "colisão de numeração é DpsDuplicada, para o usuário decidir");
        const std::vector<std::string> esperado{"POST:400", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "a colisão não reenvia nem renumera a DPS");
    }

    // Adendo: sem conseguir comparar (comparador ausente), não há sucesso silencioso: é Indeterminado.
    void e0014SemConseguirCompararViraIndeterminado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_gerada:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);  // sem comparador real: a comparação não tem como acontecer
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem comparação não há sucesso silencioso");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "sem conseguir comparar, o estado é Indeterminado");
        const std::vector<std::string> esperado{"POST:400", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "a reconciliação buscou a nota antes de desistir");
    }

    // Comparador real (motor xmldsig, digest canônico do infDPS): a mesma DPS dá Igual.
    void comparadorRealIgualParaMesmaDps()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        mc::ComparadorInfDpsC14n real;
        QVERIFY2(real.comparar(dps->id, dps->xml, nfseDoXsd(dps->xml)) == mc::ResultadoComparacaoInfDps::Igual,
                 "a mesma DPS dá Igual pelo digest canônico do infDPS");
    }

    // Comparador real: um campo alterado no infDPS dá Diferente, nunca Igual.
    void comparadorRealDiferenteParaDpsAlterada()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        QByteArray alterada = dps->xml;
        alterada.replace("</infDPS>", "<xAlterado>1</xAlterado></infDPS>");
        mc::ComparadorInfDpsC14n real;
        QVERIFY2(real.comparar(dps->id, dps->xml, nfseDoXsd(alterada)) == mc::ResultadoComparacaoInfDps::Diferente,
                 "um campo alterado no infDPS dá Diferente, não Igual");
    }

    // Ponta a ponta: E0014 com a nota já gerada e o comparador real recupera a nota (Igual pelo digest do motor).
    void e0014ComComparadorRealRecuperaANota()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_gerada:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        mc::ComparadorInfDpsC14n real;
        auto m = montar(srv.porta(), *cred, std::make_shared<mc::ComparadorInfDpsC14n>());
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "com o comparador real, E0014 da mesma DPS recupera a nota");
        QVERIFY2(nota->xmlNfse == dps->xml, "a nota recuperada é a DPS enviada");
    }

    void backoffNaoEstouraComExpoenteAlto()
    {
        // DA-2.41 (d): 1 << (n - 2) com n de 64 ou 1000 é comportamento indefinido. O expoente tem teto explícito.
        mc::PoliticaRetry politica;
        politica.base = 500ms;
        const auto saturado = politica.esperaAntesDaTentativa(12).count();
        QVERIFY2(saturado > 0, "a espera saturada tem de ser positiva");
        QCOMPARE(politica.esperaAntesDaTentativa(64).count(), saturado);
        QCOMPARE(politica.esperaAntesDaTentativa(1000).count(), saturado);
        for (int n = 2; n < 70; ++n) {
            QVERIFY2(politica.esperaAntesDaTentativa(n).count() <= politica.esperaAntesDaTentativa(n + 1).count(),
                     "a espera não pode diminuir com o número da tentativa");
        }
    }

    // DA-2.41 (a): POST com 2xx sem chave consulta GET /dps e devolve a chave da consulta, nunca Definitiva.
    void posPost2xxSemChaveConsultaDps()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse post_sem_chave")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "201 sem chave consulta GET /dps e devolve a chave da consulta");
        const std::vector<std::string> esperado{"POST:201", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "2xx sem chave consulta GET /dps antes de responder");
    }

    void posPost2xxForaDe200e201ConsultaDps()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse status202")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "202 consulta GET /dps e devolve a chave da consulta");
        const std::vector<std::string> esperado{"POST:202", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "2xx fora de 200 e 201 consulta GET /dps");
    }

    // DA-2.41 (a): 3xx no POST é estado indeterminado. Consulta antes de qualquer reenvio.
    void post3xxConsultaDpsAntesDeReenviar()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse redireciona_post")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "depois do 3xx, a consulta mostra que a DPS não gerou nota e o reenvio emite");
        const std::vector<std::string> esperado{"GET:404", "POST:201"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "3xx no POST consulta GET /dps antes de reenviar");
    }

    // DA-2.41 (a): erro definitivo depois do envio é indeterminado, e a consulta que não diz o estado também.
    void erroDefinitivoDepoisDoEnvioViraIndeterminado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse post_ce_gzip"),
                             QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_404_outro")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem resposta definitiva, não há nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "erro depois do envio, com consulta sem resposta, é Indeterminado");
        QVERIFY2(sequencia(m->destino.str()) == std::vector<std::string>{"GET:404"}, "a consulta é feita");
    }

    // DA-2.41 (a): qualquer resultado da consulta diferente de chave ou E2404 é Indeterminado, nunca Rejeitada.
    void consultaComCodigoQueNaoE2404ViraIndeterminado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx"),
                             QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_404_outro")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem chave e sem E2404, não há nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "consulta com código diferente de E2404 é Indeterminado, nunca Rejeitada");
        const std::vector<std::string> esperado{"POST:503", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "a consulta vem antes de qualquer decisão");
    }

    // DA-2.41 (a): E2404 libera o reenvio da mesma DPS (mesmo Id, mesmo número, por construção do envelope).
    void naoGeradaReenviaMesmoIdNuncaNumeroNovo()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        // Dupla emissão (o pior defeito fiscal): o servidor registra o Id e o nDPS de cada POST que chegou. Os dois
        // envios têm de ser a mesma DPS. Este teste fica vermelho se o cliente remontar o envelope com outro número.
        auto registro = m->transporte->enviar(
            ft::RequisicaoHttp{QByteArrayLiteral("GET"), QStringLiteral("/_teste/posts"), {}, {}});
        QVERIFY2(registro.has_value(), "o registro de POSTs do servidor falso tem de responder");
        const QJsonArray envios = QJsonDocument::fromJson(registro->corpo).array();
        QCOMPARE(envios.size(), qsizetype{2});
        const QJsonObject primeiro = envios.at(0).toObject();
        const QJsonObject segundo = envios.at(1).toObject();
        QVERIFY2(!primeiro.value(QStringLiteral("nDPS")).toString().isEmpty(), "o primeiro POST tem de trazer o nDPS");
        QVERIFY2(primeiro.value(QStringLiteral("id")).toString() == segundo.value(QStringLiteral("id")).toString(),
                 "o reenvio tem de ter o mesmo Id da DPS");
        QVERIFY2(primeiro.value(QStringLiteral("nDPS")).toString() == segundo.value(QStringLiteral("nDPS")).toString(),
                 "o reenvio tem de ter o mesmo número (nDPS): remontar com outro número é dupla emissão");
        QVERIFY2(nota.has_value(), "E2404 na consulta libera o reenvio da mesma DPS");
        // O reenvio reaproveita o envelope e o Id da DPS montada uma vez: a sequência tem um POST a mais, e nenhum GET de nota.
        const std::vector<std::string> esperado{"POST:503", "GET:404", "POST:201"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "o reenvio segue a consulta, sem outra tentativa de número");
    }

    // DA-2.41 (a): E0014 num reenvio leva à consulta, e a consulta devolve a chave (a nota já existia).
    void e0014NoReenvioConsultaEDevolveAChave()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx"), QStringLiteral("POST /nfse rejeicao_gerada:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto igual = std::make_shared<ComparadorFalso>(mc::ResultadoComparacaoInfDps::Igual);
        auto m = montar(srv.porta(), *cred, igual);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "E0014 num reenvio consulta GET /dps e devolve a chave, nunca rejeição");
        const std::vector<std::string> esperado{"POST:503", "GET:404", "POST:400", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "E0014 no reenvio vai para a consulta");
    }


    // Adendo de 30/10/2026: recusa no handshake TLS 1.2 (o servidor falso limita a versão). Zero bytes: Transitoria, sem GET.
    void recusaNoHandshakeTls12NaoEscreveCorpoESemConsulta()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({}, QStringLiteral("1.2"));
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montarSemCertificado(srv.porta());
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem certificado, o TLS 1.2 recusa o handshake");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva,
                 "recusa do certificado no handshake TLS 1.2, com zero bytes escritos, é Definitiva sem consulta (emenda do CISO à DA-2.41)");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::CertificadoDoEmissorProvavel,
                 "sem erro de verificação do servidor, a causa é o certificado do emissor");
        QVERIFY2(sequencia(m->destino.str()).empty(), "sem bytes escritos não há GET");
        QVERIFY2(postsNoServidor(srv.porta(), *cred) == 0, "o servidor não recebeu POST nenhum");
    }

    // Adendo de 30/10/2026: corpo escrito e conexão fechada sem resposta. A consulta GET /dps decide, sem reenvio.
    void corpoEscritoEConexaoFechadaConsultaAntesDeResponder()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse conexao_cortada_apos_aceite")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(resultado.has_value(), "com corpo escrito, a consulta GET /dps devolve a chave");
        const std::vector<std::string> esperado{"GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "POST com bytes escritos consulta antes de responder");
        QVERIFY2(postsNoServidor(srv.porta(), *cred) == 1, "o servidor recebeu o POST uma vez");
    }

    // NL-3.0j (E4, CISO parecer): gêmeo do slot acima com RST em vez de fechamento limpo. O servidor grava a DPS e corta com RST
    // antes de responder; a consulta GET /dps decide, e a nota volta com um POST só. Um POST a mais, ou outro Id, é parada.
    void corpoEscritoComRstConsultaAntesDeResponder()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse corte_rst_apos_gravacao")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        // Medição por execução (NL-3.0j, CTO, 11/11/2026): o -V do ctest leva a linha ao log do ensaio, e uma
        // repetição vermelha se explica sozinha. Só o resultado, a sequência METODO:status e a contagem de POST.
        const std::vector<std::string> obtida = sequencia(m->destino.str());
        const int posts = postsNoServidor(srv.porta(), *cred);
        QStringList passos;
        for (const std::string& passo : obtida) {
            passos << QString::fromStdString(passo);
        }
        const QString medida =
            QStringLiteral("corpoEscritoComRstConsultaAntesDeResponder;resultado=%1;sequencia=%2;posts=%3")
                .arg(resultado.has_value() ? 1 : 0)
                .arg(passos.join(QLatin1Char(',')))
                .arg(posts);
        qInfo().noquote() << "medicao" << medida;
        QVERIFY2(resultado.has_value(), "com corpo gravado e RST, a consulta GET /dps devolve a chave");
        const std::vector<std::string> esperado{"GET:200", "GET:200"};
        QVERIFY2(obtida == esperado, "POST com bytes escritos consulta antes de responder");
        QVERIFY2(posts == 1, "o servidor recebeu o POST uma vez");
    }

    // Adendo de 30/10/2026: conexão fechada antes do TLS, sem nenhum byte escrito. Transitoria, sem consulta. Servidor
    // próprio em QTcpServer: fecha a conexão assim que ela chega, antes de qualquer TLS.
    void conexaoFechadaAntesDoTlsSemBytesEhTransitoria()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        QTcpServer fechador;
        QVERIFY2(fechador.listen(QHostAddress::LocalHost), "o fechador de teste tem de escutar");
        QObject::connect(&fechador, &QTcpServer::newConnection, &fechador, [&fechador] {
            if (auto conexao = fechador.nextPendingConnection()) {
                conexao->close();
                conexao->deleteLater();
            }
        });
        auto m = montar(fechador.serverPort(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "conexão fechada antes do TLS não é nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Transitoria,
                 "zero bytes escritos: Transitoria, sem consulta");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::Rede, "conexão fechada antes do TLS é causa de rede");
        QVERIFY2(sequencia(m->destino.str()).empty(), "zero bytes escritos não consulta a DPS");
    }

    // Adendo de 30/10/2026: TLS 1.3 sem certificado. A recusa pode chegar antes ou depois dos bytes do POST: os dois
    // desfechos são aceitos, mas o POST nunca chega à aplicação, e nenhum POST tem resposta.
    void recusaDoCertificadoNoTls13NuncaReenviaAsCegas()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montarSemCertificado(srv.porta());
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem certificado não há nota");
        // NL-3.C (F1): a causa tipada decide; a mensagem do erro não traz mais o marcador "certificado recusado na consulta".
        const auto motivo = resultado.error().motivo;
        const auto causa = resultado.error().causa;
        const bool zeroBytesCertificado = motivo == bk::MotivoFalhaSefin::Definitiva && causa == bk::CausaFalhaSefin::CertificadoDoEmissorProvavel;
        const bool zeroBytesRede = motivo == bk::MotivoFalhaSefin::Transitoria && causa == bk::CausaFalhaSefin::Rede;
        const bool consultaCertificado = motivo == bk::MotivoFalhaSefin::Indeterminado && causa == bk::CausaFalhaSefin::CertificadoDoEmissorProvavel;
        const bool consultaRede = motivo == bk::MotivoFalhaSefin::Indeterminado && causa == bk::CausaFalhaSefin::Rede;
        QVERIFY2(zeroBytesCertificado || zeroBytesRede || consultaCertificado || consultaRede,
                 "a recusa no TLS 1.3 vira C1 ou C3 (zero bytes) ou C5 ou C4 (com bytes), sempre pela causa tipada");
        QVERIFY2(resultado.error().mensagem.empty(), "falha sem código E não leva texto no FalhaSefin: o texto vem do catálogo da fachada");
        QVERIFY2(postsNoServidor(srv.porta(), *cred) == 0, "a recusa do TLS não entrega o POST à aplicação");
        for (const std::string& item : sequencia(m->destino.str())) {
            QVERIFY2(item.rfind("POST", 0) != 0, "nenhum POST teve resposta do servidor");
        }
    }

    // DA-2.41 (b): código E fora de ^E\d{3,4}$ fica vazio, e o log recebe só o marcador codigo_invalido.
    void codigoEComCpfNaoChegaAoLog()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_bruta:E1203_CPF_12345678909")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY(!resultado.has_value());
        const std::string saida = m->destino.str();
        QVERIFY2(saida.find("sefin.requisicao") != std::string::npos, "piso: a varredura tem de ler um log não vazio");
        QVERIFY2(saida.find("12345678909") == std::string::npos, "o CPF do campo Codigo não pode aparecer no log");
        QVERIFY2(saida.find("codigo_invalido") != std::string::npos, "o log recebe só o marcador codigo_invalido");
        QVERIFY2(resultado.error().codigoE.empty(), "código fora do formato fica vazio no erro");
        QVERIFY2(resultado.error().mensagem.find("12345678909") == std::string::npos,
                 "a mensagem do erro não pode trazer o CPF");
    }

    void codigoEForaDoFormatoFicaVazio()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_bruta:E12345")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY(!resultado.has_value());
        QVERIFY2(resultado.error().codigoE.empty(), "E12345 tem cinco dígitos: fora de ^E\\d{3,4}$, fica vazio");
        QVERIFY2(m->destino.str().find("codigo_invalido") != std::string::npos, "o log marca codigo_invalido");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "código fora do formato, sem consulta, é Definitiva");
        QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::RespostaSemCodigoValido, "código fora do formato é resposta sem código válido");
    }

    // DA-2.41 (c): Base64 com caractere fora do alfabeto e tamanho válido. Só a checagem de alfabeto recusa.
    void base64ComCaractereInvalidoETamanhoValidoRecusado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse nfse_base64_caractere_invalido")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "sucesso com caractere fora do Base64 devolve a chave");
        QVERIFY2(nota->xmlPendente, "caractere fora do alfabeto marca o XML como pendente, mesmo com tamanho válido");
        QVERIFY2(nota->xmlNfse.isEmpty(), "XML pendente não traz bytes");
    }

    // DA-2.41 (d): o segundo membro do gzip é erro explícito e leva ao XML pendente, sem liberar reenvio.
    void membroExtraDoGzipViraXmlPendente()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse nfse_gzip_dois_membros")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "sucesso com segundo membro gzip devolve a chave");
        QVERIFY2(nota->xmlPendente, "o segundo membro gzip faz o XML ficar pendente, sem XML falso");
        QVERIFY2(sequencia(m->destino.str()) == std::vector<std::string>{"POST:201"}, "sem reenvio");
    }
};

// QTEST_GUILESS_MAIN cria o QCoreApplication: o transporte espera pelo QEventLoop e o seu tempo limite é um QTimer, e
// nenhum dos dois funciona sem laço de eventos (com APPLESS, o envio ficava sem prazo até o timeout de 300 s do QTest).
void TesteClienteSefin::falhaDePostVaiAoLogComEtapaPost()
{
    auto cred = credencial();
    QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
    auto dps = dpsAssinada(*cred);
    QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
    // Porta 1 sem servidor: a conexão é recusada antes de qualquer byte do POST (bytesEscritos 0).
    auto m = montar(1, *cred);
    QVERIFY(!m->cliente->emitir(*dps).has_value());
    const std::string saida = m->destino.str();
    QVERIFY2(saida.find("sefin.transporte_falhou") != std::string::npos, "a falha do POST tem de ir ao log (E4, DA-2.75)");
    QVERIFY2(saida.find("\"etapa\":\"POST\"") != std::string::npos, "a etapa da falha do POST tem de ser POST");
}

void TesteClienteSefin::falhaDeGetVaiAoLogComEtapaGet()
{
    auto cred = credencial();
    QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
    auto dps = dpsAssinada(*cred);
    QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
    auto m = montar(1, *cred);
    QVERIFY(!m->cliente->consultarDps(dps->id).has_value());
    const std::string saida = m->destino.str();
    QVERIFY2(saida.find("sefin.transporte_falhou") != std::string::npos, "a falha do GET tem de ir ao log (E4, DA-2.75)");
    QVERIFY2(saida.find("\"etapa\":\"GET\"") != std::string::npos, "a etapa da falha do GET tem de ser GET");
}

void TesteClienteSefin::ancorasTlsVaziasSaiComCausaDeConfiguracao()
{
    auto cred = credencial();
    QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
    auto dps = dpsAssinada(*cred);
    QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
    ft::ConfiguracaoTls tls = configuracaoTls(*cred);
    tls.autoridades.clear();  // âncoras TLS vazias: a configuração é inválida, e nada sai da máquina
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:1")), QString(), 5000ms};
    ft::TransporteHttps transporte(base, tls);
    std::ostringstream destino;
    fl::Log log(destino, "sefin", mc::allowlistLogSefin(), mc::eventosLogSefin());
    mc::PoliticaRetry politica;
    politica.base = 1ms;
    mc::ClienteSefin cliente(transporte, log, politica, nullptr, nullptr);
    const auto resultado = cliente.emitir(*dps);
    QVERIFY(!resultado.has_value());
    QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Definitiva, "âncoras vazias são configuração inválida: Definitiva, sem consulta");
    QVERIFY2(resultado.error().causa == bk::CausaFalhaSefin::ConfiguracaoDaConexao, "âncoras vazias têm causa de configuração da conexão");
    const std::string saida = destino.str();
    QVERIFY2(saida.find("\"causa\":\"configuracao\"") != std::string::npos, "âncoras vazias têm de sair com causa de configuração no evento");
    QVERIFY2(saida.find("\"bytesEscritos\":\"0\"") != std::string::npos, "âncoras vazias não escrevem nenhum byte");
}

QTEST_GUILESS_MAIN(TesteClienteSefin)
#include "tst_cliente_sefin.moc"
