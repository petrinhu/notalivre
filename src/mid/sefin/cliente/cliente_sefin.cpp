// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ClienteSefin (NL-2.8). Fluxo do POST /nfse: envelope da NL-2.4, envio pelo TransporteHttps da NL-2.5. Em erro transitório
// o POST nunca repete sozinho: consulta GET /dps/{id} antes e só reenvia com 404 E2404 (INFERÊNCIA). GET repete com
// backoff. 409 consulta e nunca reenvia. Código E nunca repete. O log leva só metodo, status, código E e etapa.
#include <nl/mid/sefin/cliente/cliente_sefin.h>

#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>
#include <nl/mid/sefin/cliente/leitor_erro_sefin.h>
#include <nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h>

#include <nl/foundation/gzip/gzip.h>
#include <nl/foundation/log/log.h>
#include <nl/foundation/transporte/transporte_https.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <cstddef>
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

bk::FalhaSefin falha(bk::MotivoFalhaSefin motivo, std::string codigoE = {})
{
    std::string mensagem = codigoE.empty() ? std::string{} : std::string(CatalogoErrosSefin::mensagem(codigoE));
    return bk::FalhaSefin{motivo, std::move(codigoE), std::move(mensagem)};
}

void registrarRequisicao(fl::Log& registro, const char* metodo, int status)
{
    (void)registro.registrar(fl::Nivel::Info, "sefin.requisicao",
                             {fl::Campo{"metodo", metodo}, fl::Campo{"status", std::to_string(status)}});
}

void registrarErro(fl::Log& registro, const std::string& codigoE)
{
    (void)registro.registrar(fl::Nivel::Erro, "sefin.erro", {fl::Campo{"codigoE", codigoE}});
}

// Chave e XML da NFS-e a partir do corpo de sucesso ({"chaveAcesso", "nfseXmlGZipB64"}; INFERÊNCIA, pesquisa g).
std::expected<bk::NotaSefin, bk::FalhaSefin> notaDoCorpo(const QByteArray& corpo)
{
    const QJsonDocument doc = QJsonDocument::fromJson(corpo);
    if (!doc.isObject()) {
        return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
    }
    const QJsonObject obj = doc.object();
    auto chave = bk::ChaveAcesso::de(obj.value(QStringLiteral("chaveAcesso")).toString());
    if (!chave) {
        return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
    }
    const QByteArray gzip = QByteArray::fromBase64(obj.value(QStringLiteral("nfseXmlGZipB64")).toString().toLatin1());
    return bk::NotaSefin{*chave, gzip};
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
        auto nota = notaDoCorpo(resposta->corpo);
        if (!nota) {
            return std::unexpected(nota.error());
        }
        return nota->chave;
    }
    const auto codigo = codigoDoErro(resposta->corpo).value_or(std::string{});
    registrarErro(registro, codigo);
    if (resposta->status == 404 && codigo == "E2404") {
        return std::unexpected(falha(bk::MotivoFalhaSefin::DpsNaoGerada, codigo));
    }
    return std::unexpected(falha(bk::MotivoFalhaSefin::Rejeitada, codigo));
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
    const auto codigo = codigoDoErro(resposta->corpo).value_or(std::string{});
    registrarErro(registro, codigo);
    return std::unexpected(falha(bk::MotivoFalhaSefin::Rejeitada, codigo));
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

    // Reenvio só depois de consulta com 404 E2404 (DA da NL-2.8). Limite: a mesma contagem de tentativas da política.
    for (int envio = 1; envio <= politica_.tentativas; ++envio) {
        ft::RequisicaoHttp requisicao;
        requisicao.metodo = "POST";
        requisicao.caminho = QStringLiteral("/nfse");
        requisicao.corpo = envelope->corpo;
        requisicao.tipoConteudo = envelope->contentType;
        auto resposta = transporte_.enviar(requisicao);

        const bool transitorio = !resposta ? resposta.error().tipo == ft::TipoErro::Transitorio
                                           : resposta->status >= 500;
        if (!resposta && !transitorio) {
            return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
        }
        if (resposta) {
            registrarRequisicao(registro_, "POST", resposta->status);
        }
        if (resposta && (resposta->status == 200 || resposta->status == 201)) {
            return notaDoCorpo(resposta->corpo);
        }
        if (resposta && resposta->status != 409 && !transitorio) {
            const auto codigo = codigoDoErro(resposta->corpo).value_or(std::string{});
            registrarErro(registro_, codigo);
            if (codigo.empty()) {
                return std::unexpected(falha(bk::MotivoFalhaSefin::Definitiva));
            }
            return std::unexpected(falha(bk::MotivoFalhaSefin::Rejeitada, codigo));  // código E nunca repete
        }

        // 409 e erro transitório: consulta antes de qualquer reenvio. Com nota já gerada, devolve a nota.
        auto consulta = consultarDpsCom(transporte_, registro_, politica_, dps.id);
        if (consulta) {
            return consultarNotaCom(transporte_, registro_, politica_, *consulta);
        }
        if (resposta && resposta->status == 409) {
            return std::unexpected(falha(bk::MotivoFalhaSefin::DpsDuplicada));  // já existe: consultar, nunca reenviar
        }
        if (consulta.error().motivo == bk::MotivoFalhaSefin::DpsNaoGerada && envio < politica_.tentativas) {
            continue;  // E2404: a nota não existe, o reenvio está liberado
        }
        return std::unexpected(consulta.error());
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
