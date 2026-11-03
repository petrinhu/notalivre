// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Carga das raízes da ICP-Brasil como âncoras (NL-2.13, DA-2.47). Ordem por arquivo do manifesto: existe, hash do DER
// confere, DER decodifica, validade no instante pedido, finalidade. Hash, existência e decodificação são fatais (a carga
// inteira falha); validade e finalidade só descartam a raiz. Sem nenhuma raiz aceita, a carga falha (fail-closed).
#include <nl/foundation/pkcs12/carga_raizes_icp.h>

#include <nl/foundation/pkcs12/manifesto_raizes.h>
#include <nl/foundation/pkcs12/ponteiros_ossl.h>

#include <openssl/err.h>
#include <openssl/x509.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QIODevice>

#include <ctime>
#include <optional>
#include <utility>

namespace nl::foundation::pkcs12 {

namespace {

ErroCarga falha(ErroRaizIcp motivo, std::string arquivo = {}) {
    return ErroCarga{motivo, std::move(arquivo)};
}

// Primeiro .der do diretório que o manifesto não declara. A extensão é comparada sem distinguir caixa ("raiz.DER" conta).
// Só .der conta: CMakeLists, NOTICE e SHA256SUMS não são raízes. O filtro por padrão de nome não serve aqui, porque no
// Linux ele diferencia maiúscula de minúscula.
std::optional<std::string> primeiroNaoDeclarado(const QString& diretorio, const std::vector<EntradaRaiz>& entradas) {
    const QDir dir(diretorio);
    for (const QString& nome : dir.entryList(QDir::Files)) {
        if (!nome.endsWith(QStringLiteral(".der"), Qt::CaseInsensitive)) {
            continue;
        }
        const std::string texto = nome.toStdString();
        bool declarado = false;
        for (const EntradaRaiz& entrada : entradas) {
            if (entrada.arquivo == texto) {
                declarado = true;
                break;
            }
        }
        if (!declarado) {
            return texto;
        }
    }
    return std::nullopt;
}

// Lê o arquivo e confere o SHA-256 do DER com o do manifesto. Ausente ou divergente: erro (fatal para a carga).
std::expected<QByteArray, ErroRaizIcp> lerConferido(const QString& caminho, const EntradaRaiz& entrada) {
    QFile arquivo(caminho);
    if (!arquivo.exists() || !arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(ErroRaizIcp::ArquivoAusente);
    }
    QByteArray der = arquivo.readAll();
    if (arquivo.error() != QFileDevice::NoError) {
        return std::unexpected(ErroRaizIcp::FalhaInterna);
    }
    const QByteArray hash = QCryptographicHash::hash(der, QCryptographicHash::Sha256).toHex();
    if (hash.toStdString() != entrada.sha256) {
        return std::unexpected(ErroRaizIcp::HashDivergente);
    }
    return der;
}

// DER para certificado X.509. Falha do OpenSSL: CertificadoIlegivel, com a fila de erros limpa.
std::expected<CertificadoX509, ErroRaizIcp> decodificar(const QByteArray& der) {
    const unsigned char* posicao = reinterpret_cast<const unsigned char*>(der.constData());
    X509* lido = d2i_X509(nullptr, &posicao, static_cast<long>(der.size()));
    if (lido == nullptr) {
        ERR_clear_error();
        return std::unexpected(ErroRaizIcp::CertificadoIlegivel);
    }
    return CertificadoX509(lido);
}

// Validade no instante pedido: nullopt se vigente; Vencida ou AindaNaoValida se fora dela.
// Datas que o OpenSSL não consegue comparar são CertificadoIlegivel.
std::expected<std::optional<MotivoDescarte>, ErroRaizIcp> validadeNoInstante(x509_st* certificado, std::time_t instante) {
    const int aindaVigente = X509_cmp_time(X509_get0_notAfter(certificado), const_cast<std::time_t*>(&instante));
    const int antesDoInicio = X509_cmp_time(X509_get0_notBefore(certificado), const_cast<std::time_t*>(&instante));
    ERR_clear_error();
    if (aindaVigente == 0 || antesDoInicio == 0) {
        return std::unexpected(ErroRaizIcp::CertificadoIlegivel);
    }
    if (aindaVigente < 0) {
        return std::optional<MotivoDescarte>{MotivoDescarte::Vencida};
    }
    if (antesDoInicio > 0) {
        return std::optional<MotivoDescarte>{MotivoDescarte::AindaNaoValida};
    }
    return std::optional<MotivoDescarte>{};
}

std::expected<QByteArray, ErroRaizIcp> lerManifestoEmDisco(const QString& manifesto) {
    QFile arquivo(manifesto);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(ErroRaizIcp::ManifestoAusente);
    }
    QByteArray conteudo = arquivo.readAll();
    if (arquivo.error() != QFileDevice::NoError) {
        return std::unexpected(ErroRaizIcp::FalhaInterna);
    }
    return conteudo;
}

} // namespace

std::expected<CargaRaizesIcp, ErroCarga> carregarRaizesIcp(const QString& diretorio, const QString& manifesto,
                                                           std::string_view proposito,
                                                           std::chrono::system_clock::time_point agora) {
    auto conteudo = lerManifestoEmDisco(manifesto);
    if (!conteudo) {
        return std::unexpected(falha(conteudo.error()));
    }
    auto entradas = lerManifesto(*conteudo);
    if (!entradas) {
        return std::unexpected(falha(entradas.error()));
    }

    // Todo .der do diretório tem de estar no manifesto (CISO, 30/10/2026, item 4). O conteúdo de produção é conhecido
    // por inteiro, então um arquivo a mais é recusado antes de qualquer certificado ser lido.
    auto nomeNaoDeclarado = primeiroNaoDeclarado(diretorio, *entradas);
    if (nomeNaoDeclarado) {
        return std::unexpected(falha(ErroRaizIcp::ArquivoNaoDeclarado, *nomeNaoDeclarado));
    }

    const std::time_t instante = std::chrono::system_clock::to_time_t(agora);
    std::vector<CertificadoX509> aceitas;
    std::vector<DescarteRaiz> descartes;
    for (const EntradaRaiz& entrada : *entradas) {
        const QString caminho = diretorio + QLatin1Char('/') + QString::fromStdString(entrada.arquivo);
        auto der = lerConferido(caminho, entrada);
        if (!der) {
            return std::unexpected(falha(der.error(), entrada.arquivo));
        }
        auto certificado = decodificar(*der);
        if (!certificado) {
            return std::unexpected(falha(certificado.error(), entrada.arquivo));
        }
        auto motivo = validadeNoInstante(certificado->get(), instante);
        if (!motivo) {
            return std::unexpected(falha(motivo.error(), entrada.arquivo));
        }
        if (std::string_view(entrada.proposito) != proposito) {
            descartes.push_back(DescarteRaiz{entrada.arquivo, MotivoDescarte::PropositoDiferente});
            continue;
        }
        if (motivo->has_value()) {
            descartes.push_back(DescarteRaiz{entrada.arquivo, **motivo});
            continue;
        }
        aceitas.push_back(std::move(*certificado));
    }

    if (aceitas.empty()) {
        return std::unexpected(falha(ErroRaizIcp::NenhumaRaizAceita));
    }
    auto ancoras = Ancoras::deCertificados(std::move(aceitas));
    if (!ancoras) {
        return std::unexpected(falha(ErroRaizIcp::FalhaInterna));
    }
    return CargaRaizesIcp{std::move(*ancoras), std::move(descartes)};
}

} // namespace nl::foundation::pkcs12
