// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do validador XSD da Foundation (NL-1.6a). Fixtures escritas à mão em tests/fixtures/xsd, régua do xmllint
// no oráculo CTest (oraculo_xmllint.sh). Os XSD vêm do build (third_party/nfse-xsd), nunca da árvore.

#include <nl/foundation/xml/erros_xsd.h>
#include <nl/foundation/xml/esquema.h>
#include <nl/foundation/xml/validador_xsd.h>
#include <nl/teste/tabela_por_distro.h>

#include "parse_seguro.h"

#include <libxml/SAX2.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <expected>
#include <string>
#include <thread>

namespace {

using nl::teste::valorDaDistroAtual;

const QString kDiretorioXsd = QStringLiteral(NL_XSD_PRODREST_DIR);
const QString kManifesto = QStringLiteral(NL_XSD_PRODREST_MANIFESTO);
const QString kRaiz = QStringLiteral(NL_XSD_PRODREST_RAIZ);
const QString kFixtures = QStringLiteral(NL_FIXTURES_XSD);

using nl::foundation::xml::ErrosXsd;
using nl::foundation::xml::Esquema;
using nl::foundation::xml::ValidadorXsd;

// Lê uma fixture; arquivo ilegível vira conteúdo vazio, que as asserções dos testes reprovam.
QByteArray lerFixture(const QString& nome)
{
    QFile arquivo{QDir{kFixtures}.filePath(nome)};
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

// Mensagens de todos os erros concatenadas, para asserções de conteúdo.
QString mensagensDe(const ErrosXsd& erros)
{
    QStringList partes;
    for (const auto& erro : erros.erros()) {
        partes << QString::fromStdString(erro.mensagem());
    }
    return partes.join(QStringLiteral(" | "));
}

// Descrição para QVERIFY2: o macro avalia a descrição nos dois ramos, então só lê o erro quando houve falha.
template <typename T>
QString descricaoDe(const std::expected<T, ErrosXsd>& resultado)
{
    return resultado.has_value() ? QString{} : mensagensDe(resultado.error());
}

// Tabela de cliente HTTP por distro (NL-2.21): a mesma do slot do xmldsig, na pasta de fixtures do xmldsig.
QString caminhoTabelaHttp()
{
    return QStringLiteral(NL_FIXTURES_XMLDSIG) + QStringLiteral("/tabela_http_por_distro.txt");
}

// DOCTYPE SYSTEM apontando para o ouvinte local: o DTD externo é o único pedido de rede do parse.
QByteArray dtdApontandoPara(int porta)
{
    const QByteArray url = "http://127.0.0.1:" + QByteArray::number(porta) + "/dtd.dtd";
    return "<!DOCTYPE DPS SYSTEM \"" + url + "\">\n<DPS/>";
}

// Soma XML_PARSE_DTDLOAD às opções do contexto (NL-2.21). xmlCtxtSetOptions substitui o conjunto (parser.c da 2.14.6), então
// somar sem ler as opções atuais apagaria o NONET. 2.14 em diante (>= 21400): xmlCtxtGetOptions. Faixa 2.13 (21300 a 21399):
// INFERÊNCIA, não compilada em nenhuma imagem; as opções saem montadas à mão, NONET | DTDLOAD quando comNonet (o contexto
// do parse seguro), só DTDLOAD no controle cru. Antes da 2.13, xmlCtxtUseOptions soma sozinho e mantém o NONET.
void somaDtdLoad(xmlParserCtxtPtr contexto, bool comNonet)
{
#if LIBXML_VERSION >= 21400
    (void)comNonet;
    xmlCtxtSetOptions(contexto, xmlCtxtGetOptions(contexto) | XML_PARSE_DTDLOAD);
#elif LIBXML_VERSION >= 21300
    xmlCtxtSetOptions(contexto, (comNonet ? XML_PARSE_NONET : 0) | XML_PARSE_DTDLOAD);
#else
    (void)comNonet;
    xmlCtxtUseOptions(contexto, XML_PARSE_DTDLOAD);
#endif
}

// Libera um contexto cru do controle positivo. O documento parseado continua dono do contexto até ser tirado dele, e
// xmlFreeParserCtxt não o libera: sem xmlFreeDoc, o LeakSanitizer acusa o xmlDoc (NL-3.0h, causa A).
struct LiberaContextoCru
{
    void operator()(xmlParserCtxtPtr contexto) const noexcept
    {
        if (contexto->myDoc != nullptr) {
            xmlFreeDoc(contexto->myDoc);
        }
        xmlFreeParserCtxt(contexto);
    }
};
using ContextoCru = std::unique_ptr<xmlParserCtxt, LiberaContextoCru>;

// Ouvinte TCP em 127.0.0.1 (NL-2.21): conta cada conexão aceita e a fecha. Mesma régua do ouvinte do xmldsig: as conexões
// que já estão na fila do listen quando o laço para são aceitas e contadas no parar, sem corrida.
class OuvinteTcp
{
public:
    OuvinteTcp()
    {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in endereco{};
        endereco.sin_family = AF_INET;
        endereco.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t tamanho = sizeof(endereco);
        if (fd_ < 0 || ::bind(fd_, reinterpret_cast<sockaddr*>(&endereco), sizeof(endereco)) != 0 ||
            ::listen(fd_, 16) != 0 || ::getsockname(fd_, reinterpret_cast<sockaddr*>(&endereco), &tamanho) != 0) {
            return;
        }
        porta_ = ntohs(endereco.sin_port);
        ativo_ = true;
        thread_ = std::thread([this] { laco(); });
    }
    ~OuvinteTcp() { parar(); }
    OuvinteTcp(const OuvinteTcp&) = delete;
    OuvinteTcp& operator=(const OuvinteTcp&) = delete;

    int porta() const { return porta_; }
    int conexoes() const { return conexoes_.load(); }

    void parar()
    {
        ativo_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
        if (fd_ >= 0) {
            drenar();
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    void laco()
    {
        while (ativo_) {
            pollfd espera{fd_, POLLIN, 0};
            if (::poll(&espera, 1, 20) > 0) {
                const int conexao = ::accept(fd_, nullptr, nullptr);
                if (conexao >= 0) {
                    conexoes_++;
                    ::close(conexao);
                }
            }
        }
    }

    void drenar()
    {
        const int flags = ::fcntl(fd_, F_GETFL, 0);
        if (flags < 0 || ::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
            return;
        }
        for (;;) {
            const int conexao = ::accept(fd_, nullptr, nullptr);
            if (conexao < 0) {
                return;
            }
            conexoes_++;
            ::close(conexao);
        }
    }

    int fd_ = -1;
    int porta_ = 0;
    std::atomic<bool> ativo_{false};
    std::atomic<int> conexoes_{0};
    std::thread thread_;
};

} // namespace

class TstValidadorXsd : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void dpsSemAssinaturaValidaNoXsd();
    void campoObrigatorioAusenteReprova();
    void xmlMalformadoReprova();
    void idForaDoPatternReprova();
    void shaErradoDoEsquemaRecusado();
    void fixturesConcordamComORegraDoNome();
    void xmlComDoctypeNoValidadorParaSemProcessar();
    void validadorNaoAcessaARede();
};

void TstValidadorXsd::initTestCase()
{
    QVERIFY2(QFileInfo::exists(kManifesto), qPrintable(kManifesto));
    QVERIFY2(QFileInfo::exists(QDir{kDiretorioXsd}.filePath(kRaiz)), qPrintable(kRaiz));
}

void TstValidadorXsd::dpsSemAssinaturaValidaNoXsd()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("valida_dps_minima.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY2(resultado.has_value(), qPrintable(descricaoDe(resultado)));
}

void TstValidadorXsd::campoObrigatorioAusenteReprova()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("invalida_campo_obrigatorio_ausente.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error().motivo(), ErrosXsd::Motivo::invalido);
    // A mensagem precisa apontar o campo que falta (dhEmi), não só dizer "inválido".
    QVERIFY2(mensagensDe(resultado.error()).contains(QStringLiteral("dhEmi")), qPrintable(mensagensDe(resultado.error())));
    // Formato "not expected ... Expected is ( dhEmi )": o campo é o esperado (o que falta); o elemento que apareceu
    // no lugar (verAplic) fica na mensagem. Decisão documentada em libxml_util.h (campoDaMensagem).
    QVERIFY(resultado.error().erros().front().campo().has_value());
    QCOMPARE(QString::fromStdString(resultado.error().erros().front().campo().value()), QStringLiteral("dhEmi"));
    QVERIFY2(mensagensDe(resultado.error()).contains(QStringLiteral("verAplic")), qPrintable(mensagensDe(resultado.error())));
}

void TstValidadorXsd::xmlMalformadoReprova()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("invalida_xml_malformado.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error().motivo(), ErrosXsd::Motivo::xmlMalformado);
    QVERIFY(!resultado.error().erros().empty());
}

void TstValidadorXsd::idForaDoPatternReprova()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("invalida_id_fora_do_pattern.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error().motivo(), ErrosXsd::Motivo::invalido);
    const QString msg = mensagensDe(resultado.error());
    QVERIFY2(msg.contains(QStringLiteral("pattern")), qPrintable(msg));
    QVERIFY2(msg.contains(QStringLiteral("Id")), qPrintable(msg));
    // Formato "Element '{ns}infDPS', attribute 'Id': [facet 'pattern']...": sem "Expected is", o campo é o elemento citado.
    QVERIFY(resultado.error().erros().front().campo().has_value());
    QCOMPARE(QString::fromStdString(resultado.error().erros().front().campo().value()), QStringLiteral("infDPS"));
}

void TstValidadorXsd::shaErradoDoEsquemaRecusado()
{
    // Cópia da pasta do XSD com um byte alterado num arquivo da cadeia (tiposSimples, incluído pelo DPS).
    QTemporaryDir temporario;
    QVERIFY(temporario.isValid());
    const QDir origem{kDiretorioXsd};
    for (const QString& nome : origem.entryList(QDir::Files)) {
        QVERIFY(QFile::copy(origem.filePath(nome), QDir{temporario.path()}.filePath(nome)));
    }
    QFile adulterado{QDir{temporario.path()}.filePath(QStringLiteral("tiposSimples_v1.01.xsd"))};
    QVERIFY(adulterado.open(QIODevice::Append));
    QCOMPARE(adulterado.write(" "), qint64{1});
    adulterado.close();

    const auto esquema = Esquema::abrir(temporario.path(), kManifesto, kRaiz);

    QVERIFY(!esquema.has_value());
    QCOMPARE(esquema.error().motivo(), ErrosXsd::Motivo::esquemaRecusado);
    QVERIFY2(mensagensDe(esquema.error()).contains(QStringLiteral("tiposSimples_v1.01.xsd")),
             qPrintable(mensagensDe(esquema.error())));
}

void TstValidadorXsd::fixturesConcordamComORegraDoNome()
{
    // Mesma régua do oráculo xmllint: valida_* deve validar, invalida_* deve falhar. Zero fixtures reprova.
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QStringList nomes = QDir{kFixtures}.entryList({QStringLiteral("*.xml")}, QDir::Files);
    QVERIFY2(!nomes.isEmpty(), "varredura vazia: nenhuma fixture em tests/fixtures/xsd");

    int analisados = 0;
    for (const QString& nome : nomes) {
        const bool deveValidar = nome.startsWith(QStringLiteral("valida_"));
        const bool deveFalhar = nome.startsWith(QStringLiteral("invalida_"));
        QVERIFY2(deveValidar || deveFalhar, qPrintable(QStringLiteral("nome sem veredito: %1").arg(nome)));

        const auto resultado = ValidadorXsd::validar(lerFixture(nome), esquema.value());
        QCOMPARE(resultado.has_value(), deveValidar);
        ++analisados;
    }
    QCOMPARE(analisados, static_cast<int>(nomes.size()));
}

void TstValidadorXsd::xmlComDoctypeNoValidadorParaSemProcessar()
{
    // NL-2.21: DOCTYPE com subconjunto interno antes do corpo, e o corpo é o mesmo que valida sem ele. O validador para no
    // DOCTYPE e recusa com código próprio: o DTD nunca é lido. Sem a parada, o corpo válido passaria.
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    QByteArray comDtd = lerFixture(QStringLiteral("valida_dps_minima.xml"));
    const int inicioDaRaiz = comDtd.indexOf("<DPS");
    QVERIFY2(inicioDaRaiz > 0, "a fixture valida_dps_minima.xml tem de ter a raiz DPS depois da declaração XML");
    comDtd.insert(inicioDaRaiz, "<!DOCTYPE DPS [<!ENTITY e \"x\">]>\n");

    const auto resultado = ValidadorXsd::validar(comDtd, esquema.value());

    QVERIFY2(!resultado.has_value(), "DOCTYPE tem de ser recusado antes da leitura do corpo");
    QCOMPARE(resultado.error().motivo(), ErrosXsd::Motivo::xmlMalformado);
    QVERIFY(!resultado.error().erros().empty());
    QCOMPARE(QString::fromStdString(resultado.error().erros().front().codigo()), QStringLiteral("xml.com_dtd"));
}

void TstValidadorXsd::validadorNaoAcessaARede()
{
    // NL-2.21 (gêmeo do slot contextoDoParserBloqueiaRede): a expectativa sai da tabela de HTTP por distro, nunca de nome
    // de distro, e o teste nunca pula. Onde a tabela diz sem_http, prova a ausência do cliente HTTP; onde diz com_http,
    // prova o bloqueio com o ouvinte local e o controle positivo.
    const QByteArray expectativa = valorDaDistroAtual(caminhoTabelaHttp()).toUtf8();
    QVERIFY2(expectativa == "com_http" || expectativa == "sem_http",
             "a distro atual não tem linha com_http ou sem_http em tests/fixtures/xmldsig/tabela_http_por_distro.txt");
    const bool temHttp = xmlHasFeature(XML_WITH_HTTP) != 0;
    if (expectativa == "sem_http") {
        QVERIFY2(!temHttp, "a tabela diz sem_http, mas a libxml2 desta imagem tem cliente HTTP: a tabela está errada");
        return;
    }
    QVERIFY2(temHttp, "a tabela diz com_http, mas a libxml2 desta imagem não tem cliente HTTP: a tabela está errada");

    // Controle positivo (L-36): contexto cru, sem NONET e sem a parada, com DTDLOAD. Tem de conectar.
    {
        OuvinteTcp controle;
        QVERIFY2(controle.porta() != 0, "o ouvinte TCP local tem de subir");
        const QByteArray xmlControle = dtdApontandoPara(controle.porta());
        ContextoCru cru(xmlCreateMemoryParserCtxt(xmlControle.constData(), static_cast<int>(xmlControle.size())));
        QVERIFY2(cru != nullptr, "a libxml2 tem de criar o contexto cru do controle");
        somaDtdLoad(cru.get(), false);
        xmlParseDocument(cru.get());
        controle.parar();
        QVERIFY2(controle.conexoes() >= 1, "o controle sem NONET tem de conectar: sem isso o zero do validador não prova nada");
    }

    // Validador público: o DOCTYPE SYSTEM para o ouvinte é recusado antes de qualquer pedido de rede.
    {
        const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
        QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
        OuvinteTcp ouvinte;
        QVERIFY2(ouvinte.porta() != 0, "o ouvinte TCP local tem de subir");
        const auto resultado = ValidadorXsd::validar(dtdApontandoPara(ouvinte.porta()), esquema.value());
        ouvinte.parar();
        QVERIFY2(!resultado.has_value(), "DOCTYPE SYSTEM tem de ser recusado pelo validador");
        QCOMPARE(QString::fromStdString(resultado.error().erros().front().codigo()), QStringLiteral("xml.com_dtd"));
        QCOMPARE(ouvinte.conexoes(), 0);
    }

    // Parse seguro sem a parada (só NONET fica), com DTDLOAD aditivo: zero conexões.
    {
        OuvinteTcp ouvinte;
        QVERIFY2(ouvinte.porta() != 0, "o ouvinte TCP local tem de subir");
        const QByteArray xml = dtdApontandoPara(ouvinte.porta());
        auto contexto = nl::foundation::xml::detail::criarContextoParseSeguro(xml);
        QVERIFY2(contexto != nullptr, "o contexto do parse seguro tem de existir");
        contexto->sax->internalSubset = xmlSAX2InternalSubset;
        somaDtdLoad(contexto.get(), true);
        xmlParseDocument(contexto.get());
        ouvinte.parar();
        // O contexto do parse seguro não é o do produto que tira o documento: quem o parseou libera (NL-3.0h, causa A).
        if (contexto->myDoc != nullptr) {
            xmlFreeDoc(contexto->myDoc);
            contexto->myDoc = nullptr;
        }
        QCOMPARE(ouvinte.conexoes(), 0);
    }
}

QTEST_GUILESS_MAIN(TstValidadorXsd)
#include "tst_validador_xsd.moc"
