// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ClienteSefin (NL-2.8, DA-2.41). Fluxo do POST /nfse: envelope da NL-2.4, envio pelo TransporteHttps da NL-2.5.
// Depois de qualquer envio que pode ter chegado à Sefin, a consulta GET /dps/{id} decide o estado antes de qualquer
// resposta: chave é sucesso; 404 com E2404 libera o reenvio da mesma DPS; qualquer outro resultado é Indeterminado,
// nunca Definitiva nem Rejeitada. Falha antes do envio é Transitoria, sem consulta. O código E só entra se casar com
// ^E\d{3,4}$. O log leva metodo, status, código E (ou codigo_invalido) e etapa, nunca o texto livre da API.
#include <nl/mid/sefin/cliente/cliente_sefin.h>

#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>
#include <nl/mid/sefin/cliente/leitor_erro_sefin.h>
#include <nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h>
// Teto do XML (kLimiteXmlNfse): o número mora só na pasta do leiaute (include relativo, como codec_dps.cpp).
#include "../leiaute/prodrest_20261020/limites_leiaute.h"

#include <nl/foundation/gzip/gzip.h>
#include <nl/foundation/log/log.h>
#include <nl/foundation/transporte/transporte_https.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace nl::mid::sefin::cliente {
namespace {

namespace bk = nl::back;
namespace fl = nl::foundation::log;
namespace ft = nl::foundation::transporte;
namespace lx = nl::mid::sefin::prodrest_20261020;

using Desfecho = std::expected<bk::NotaSefin, bk::FalhaSefin>;

bk::FalhaSefin falha(bk::MotivoFalhaSefin motivo, std::string codigoE = {})
{
    std::string mensagem = codigoE.empty() ? std::string{} : std::string(CatalogoErrosSefin::mensagem(codigoE));
    return bk::FalhaSefin{motivo, std::move(codigoE), std::move(mensagem)};
}

// Código E só entra se casar com ^E\d{3,4}$ (DA-2.41, b). Fora disso, o valor vem vazio e nem o log nem o erro o veem.
bool codigoEValido(const std::string& codigo)
{
    if (codigo.size() != 4 && codigo.size() != 5) {
        return false;
    }
    if (codigo[0] != 'E') {
        return false;
    }
    for (std::size_t i = 1; i < codigo.size(); ++i) {
        if (codigo[i] < '0' || codigo[i] > '9') {
            return false;
        }
    }
    return true;
}

std::string codigoSeValido(const std::string& bruto)
{
    return codigoEValido(bruto) ? bruto : std::string{};
}

// O que o log recebe do código de um erro: o código válido, o marcador codigo_invalido, ou nada se não havia código.
std::string codigoParaLog(const std::string& bruto)
{
    if (bruto.empty()) {
        return {};
    }
    return codigoEValido(bruto) ? bruto : std::string("codigo_invalido");
}

void registrarRequisicao(fl::Log& registro, const char* metodo, int status)
{
    (void)registro.registrar(fl::Nivel::Info, "sefin.requisicao",
                             {fl::Campo{"metodo", metodo}, fl::Campo{"status", std::to_string(status)}});
}

void registrarErro(fl::Log& registro, const std::string& codigoBruto)
{
    (void)registro.registrar(fl::Nivel::Erro, "sefin.erro", {fl::Campo{"codigoE", codigoParaLog(codigoBruto)}});
}

// Chave da NFS-e do objeto de sucesso (INFERÊNCIA, pesquisa g).
std::expected<bk::ChaveAcesso, bk::FalhaSefin> chaveDoObjeto(const QJsonObject& obj)
{
    auto chave = bk::ChaveAcesso::de(obj.value(QStringLiteral("chaveAcesso")).toString());
    if (!chave) {
        return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
    }
    return *chave;
}

// Base64 estrito (DA-2.39): caractere fora do alfabeto ou tamanho que não fecha blocos de 4 é recusado, sem decodificar
// lixo em silêncio.
std::optional<QByteArray> base64Estrito(const QString& texto)
{
    const QByteArray bytes = texto.toLatin1();
    if (bytes.size() % 4 != 0) {
        return std::nullopt;
    }
    for (const char c : bytes) {
        const bool alfabeto = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+'
                              || c == '/' || c == '=';
        if (!alfabeto) {
            return std::nullopt;
        }
    }
    return QByteArray::fromBase64(bytes);
}

// XML da NFS-e: Base64 estrito, depois GZip com o teto do leiaute. Qualquer falha devolve vazio, nunca um falso XML.
std::optional<QByteArray> xmlDoCampo(const QString& campo)
{
    auto gzip = base64Estrito(campo);
    if (!gzip) {
        return std::nullopt;
    }
    auto xml = nl::foundation::gzip::descomprimir(
        std::span<const std::byte>(reinterpret_cast<const std::byte*>(gzip->constData()),
                                   static_cast<std::size_t>(gzip->size())),
        lx::kLimiteXmlNfse);
    if (!xml) {
        return std::nullopt;
    }
    return QByteArray(reinterpret_cast<const char*>(xml->data()), static_cast<qsizetype>(xml->size()));
}

// Nota do corpo de sucesso. Se o XML não lê (Base64 ou GZip ruins, ou acima do teto), a chave vale e o XML fica
// pendente: o reenvio continua bloqueado e o XML vem depois por GET /nfse/{chave} (DA-2.39).
std::expected<bk::NotaSefin, bk::FalhaSefin> notaDoCorpo(const QByteArray& corpo)
{
    const QJsonDocument doc = QJsonDocument::fromJson(corpo);
    if (!doc.isObject()) {
        return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
    }
    auto chave = chaveDoObjeto(doc.object());
    if (!chave) {
        return std::unexpected(chave.error());
    }
    auto xml = xmlDoCampo(doc.object().value(QStringLiteral("nfseXmlGZipB64")).toString());
    if (!xml) {
        return bk::NotaSefin{*chave, {}, true};
    }
    return bk::NotaSefin{*chave, *xml, false};
}

// Chave do corpo de GET /dps/{id}: não toca no XML, então um XML ruim nunca libera nem bloqueia a consulta.
std::expected<bk::ChaveAcesso, bk::FalhaSefin> chaveDoCorpo(const QByteArray& corpo)
{
    const QJsonDocument doc = QJsonDocument::fromJson(corpo);
    if (!doc.isObject()) {
        return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
    }
    return chaveDoObjeto(doc.object());
}

// GET com repetição em erro transitório e em 5xx, com a espera da política. Devolve a resposta de sucesso ou a falha.
std::expected<ft::RespostaHttp, bk::FalhaSefin> getComRetry(ft::TransporteHttps& transporte, fl::Log& registro,
                                                            const PoliticaRetry& politica, const QString& caminho)
{
    for (int n = 1; n <= politica.tentativas; ++n) {
        if (n > 1) {
            std::this_thread::sleep_for(politica.esperaAntesDaTentativa(n));
        }
        ft::RequisicaoHttp requisicao;
        requisicao.metodo = "GET";
        requisicao.caminho = caminho;
        auto resposta = transporte.enviar(requisicao);
        const bool ultima = n == politica.tentativas;
        if (!resposta) {
            if (resposta.error().tipo == ft::TipoErro::Transitorio && !ultima) {
                continue;
            }
            return std::unexpected(falha(resposta.error().tipo == ft::TipoErro::Transitorio
                                             ? bk::MotivoFalhaSefin::Transitoria
                                             : bk::MotivoFalhaSefin::Definitiva));
        }
        registrarRequisicao(registro, "GET", resposta->status);
        if (resposta->status >= 500 && !ultima) {
            continue;
        }
        if (resposta->status >= 500) {
            return std::unexpected(falha(bk::MotivoFalhaSefin::Transitoria));
        }
        return *resposta;
    }
    return std::unexpected(falha(bk::MotivoFalhaSefin::Transitoria));
}

std::expected<bk::ChaveAcesso, bk::FalhaSefin> consultarDpsCom(ft::TransporteHttps& transporte, fl::Log& registro,
                                                               const PoliticaRetry& politica, const bk::IdDps& id)
{
    auto resposta = getComRetry(transporte, registro, politica, QStringLiteral("/dps/") + id.texto());
    if (!resposta) {
        return std::unexpected(resposta.error());
    }
    if (resposta->status == 200) {
        return chaveDoCorpo(resposta->corpo);
    }
    const std::string bruto = codigoDoErro(resposta->corpo).value_or(std::string{});
    registrarErro(registro, bruto);
    if (resposta->status == 404 && bruto == "E2404") {
        return std::unexpected(falha(bk::MotivoFalhaSefin::DpsNaoGerada, "E2404"));
    }
    return std::unexpected(falha(bk::MotivoFalhaSefin::Rejeitada, codigoSeValido(bruto)));
}

std::expected<bk::NotaSefin, bk::FalhaSefin> consultarNotaCom(ft::TransporteHttps& transporte, fl::Log& registro,
                                                              const PoliticaRetry& politica,
                                                              const bk::ChaveAcesso& chave)
{
    auto resposta = getComRetry(transporte, registro, politica, QStringLiteral("/nfse/") + chave.texto());
    if (!resposta) {
        return std::unexpected(resposta.error());
    }
    if (resposta->status == 200) {
        return notaDoCorpo(resposta->corpo);
    }
    const std::string bruto = codigoDoErro(resposta->corpo).value_or(std::string{});
    registrarErro(registro, bruto);
    return std::unexpected(falha(bk::MotivoFalhaSefin::Rejeitada, codigoSeValido(bruto)));
}

// Nota da chave da consulta. Se o GET /nfse falha, a chave vale: a nota fica pendente e o XML vem depois (DA-2.39).
Desfecho notaDaChave(ft::TransporteHttps& transporte, fl::Log& registro, const PoliticaRetry& politica,
                     const bk::ChaveAcesso& chave)
{
    auto nota = consultarNotaCom(transporte, registro, politica, chave);
    if (nota) {
        return *nota;
    }
    return bk::NotaSefin{chave, {}, true};
}

// Estado depois de um envio que pode ter chegado à Sefin. A consulta GET /dps/{id} decide (DA-2.41, a). Devolve o
// desfecho, ou nullopt quando a DPS não gerou nota e o reenvio, com o mesmo Id, está liberado.
std::optional<Desfecho> resolverPosEnvio(ft::TransporteHttps& transporte, fl::Log& registro,
                                         const PoliticaRetry& politica, const bk::DpsAssinada& dps, bool podeReenviar)
{
    auto consulta = consultarDpsCom(transporte, registro, politica, dps.id);
    if (consulta) {
        return Desfecho{notaDaChave(transporte, registro, politica, *consulta)};
    }
    if (consulta.error().motivo == bk::MotivoFalhaSefin::DpsNaoGerada) {
        if (podeReenviar) {
            return std::nullopt;  // E2404: a nota não existe, o reenvio da mesma DPS está liberado
        }
        return Desfecho{std::unexpected(falha(bk::MotivoFalhaSefin::Transitoria))};  // sem tentativa restante
    }
    // Qualquer outro resultado da consulta (código que não é E2404, 5xx, falha de rede): o estado é desconhecido.
    return Desfecho{std::unexpected(falha(bk::MotivoFalhaSefin::Indeterminado))};
}

// 409: a DPS já existe. Consulta; com chave, devolve a nota; sem chave, é duplicada e nunca se reenvia.
Desfecho resolverDuplicada(ft::TransporteHttps& transporte, fl::Log& registro, const PoliticaRetry& politica,
                           const bk::DpsAssinada& dps)
{
    auto consulta = consultarDpsCom(transporte, registro, politica, dps.id);
    if (consulta) {
        return notaDaChave(transporte, registro, politica, *consulta);
    }
    return std::unexpected(falha(bk::MotivoFalhaSefin::DpsDuplicada));
}

} // namespace

ClienteSefin::ClienteSefin(ft::TransporteHttps& transporte, fl::Log& registro, PoliticaRetry politica)
    : transporte_(transporte), registro_(registro), politica_(politica)
{
}

std::expected<bk::NotaSefin, bk::FalhaSefin> ClienteSefin::emitir(const bk::DpsAssinada& dps)
{
    const QByteArray xml = dps.xml;
    auto gzip = nl::foundation::gzip::comprimir(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(xml.constData()), static_cast<std::size_t>(xml.size())));
    if (!gzip) {
        return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
    }
    const QByteArray gzipBytes(reinterpret_cast<const char*>(gzip->data()), static_cast<qsizetype>(gzip->size()));
    auto envelope = lx::montarEnvelopeDps(gzipBytes);
    if (!envelope) {
        return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
    }

    // O reenvio só acontece depois de consulta com E2404, com o mesmo envelope e o mesmo Id: nunca um número novo.
    for (int envio = 1; envio <= politica_.tentativas; ++envio) {
        const bool podeReenviar = envio < politica_.tentativas;
        ft::RequisicaoHttp requisicao;
        requisicao.metodo = "POST";
        requisicao.caminho = QStringLiteral("/nfse");
        requisicao.corpo = envelope->corpo;
        requisicao.tipoConteudo = envelope->contentType;
        auto resposta = transporte_.enviar(requisicao);

        if (!resposta) {
            if (resposta.error().antesDoEnvio) {
                // Nada chegou à Sefin: sem consulta. Configuração inválida é definitiva; o resto vale tentar de novo.
                return std::unexpected(falha(resposta.error().causa == ft::CausaErro::Configuracao
                                                 ? bk::MotivoFalhaSefin::Definitiva
                                                 : bk::MotivoFalhaSefin::Transitoria));
            }
            // Conexão aberta e depois caiu: os bytes podem ter chegado. Consulta antes de qualquer decisão.
            if (auto desfecho = resolverPosEnvio(transporte_, registro_, politica_, dps, podeReenviar)) {
                return *desfecho;
            }
            continue;
        }

        const int status = resposta->status;
        registrarRequisicao(registro_, "POST", status);
        if ((status == 200 || status == 201) && chaveDoCorpo(resposta->corpo).has_value()) {
            return notaDoCorpo(resposta->corpo);
        }
        if (status == 409) {
            return resolverDuplicada(transporte_, registro_, politica_, dps);
        }
        if (status >= 400 && status < 500) {
            // Recusa com resposta definitiva. Código E válido nunca repete, exceto E0014 num reenvio: a DPS já existe,
            // então a consulta é a fonte da verdade (DA-2.41, a).
            const std::string bruto = codigoDoErro(resposta->corpo).value_or(std::string{});
            registrarErro(registro_, bruto);
            const std::string codigo = codigoSeValido(bruto);
            if (codigo == "E0014" && envio > 1) {
                if (auto desfecho = resolverPosEnvio(transporte_, registro_, politica_, dps, false)) {
                    return *desfecho;
                }
            }
            if (!codigo.empty()) {
                return std::unexpected(falha(bk::MotivoFalhaSefin::Rejeitada, codigo));
            }
            return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
        }

        // 2xx sem chave, 2xx fora de 200 e 201, 3xx e 5xx: a DPS pode ter sido gravada. A consulta decide.
        if (auto desfecho = resolverPosEnvio(transporte_, registro_, politica_, dps, podeReenviar)) {
            return *desfecho;
        }
    }
    return std::unexpected(falha(bk::MotivoFalhaSefin::Transitoria));
}

std::expected<bk::NotaSefin, bk::FalhaSefin> ClienteSefin::consultarNota(const bk::ChaveAcesso& chave)
{
    return consultarNotaCom(transporte_, registro_, politica_, chave);
}

std::expected<bk::ChaveAcesso, bk::FalhaSefin> ClienteSefin::consultarDps(const bk::IdDps& id)
{
    return consultarDpsCom(transporte_, registro_, politica_, id);
}

} // namespace nl::mid::sefin::cliente
