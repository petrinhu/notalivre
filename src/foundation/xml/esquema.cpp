// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Esquema XSD verificado (NL-1.6a). Ver esquema.h.
#include <nl/foundation/xml/esquema.h>

#include "esquema_recurso.h"
#include "libxml_util.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QIODevice>

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nl::foundation::xml {
namespace {

using Motivo = ErrosXsd::Motivo;

struct EntradaManifesto
{
    QString nome;
    QString sha256; // hex minúsculo, 64 caracteres
};

ErrosXsd recusado(std::string mensagem)
{
    return ErrosXsd{Motivo::esquemaRecusado, {nl::base::Erro{"xsd.esquema", std::nullopt, std::move(mensagem)}}};
}

bool hashValido(const QByteArray& hash)
{
    return hash.size() == 64 && std::all_of(hash.cbegin(), hash.cend(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

// Nome puro de arquivo: nada de separador de caminho, nem "." ou "..". O manifesto não atravessa diretórios.
bool nomeValido(const QString& nome)
{
    return !nome.isEmpty() && nome != QLatin1String(".") && nome != QLatin1String("..") && !nome.contains(QLatin1Char('/'))
           && !nome.contains(QLatin1Char('\\'));
}

std::expected<std::vector<EntradaManifesto>, ErrosXsd> lerManifesto(const QString& caminho)
{
    QFile arquivo{caminho};
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(recusado("manifesto ilegível: " + caminho.toStdString() + " (" +
                                        arquivo.errorString().toStdString() + ")"));
    }

    std::vector<EntradaManifesto> entradas;
    for (const QByteArray& linha : arquivo.readAll().split('\n')) {
        const QByteArray limpa = linha.trimmed();
        if (limpa.isEmpty() || limpa.startsWith('#')) {
            continue;
        }
        // Formato do sha256sum: 64 hex, dois espaços e o nome do arquivo.
        const QByteArray hash = limpa.first(std::min<qsizetype>(64, limpa.size()));
        const bool formato = limpa.size() > 66 && limpa.at(64) == ' ' && limpa.at(65) == ' ';
        const QString nome = formato ? QString::fromUtf8(limpa.mid(66)) : QString{};
        if (!formato || !hashValido(hash) || !nomeValido(nome)) {
            return std::unexpected(recusado("linha de manifesto inválida em " + caminho.toStdString() + ": " +
                                            limpa.toStdString()));
        }
        entradas.push_back(EntradaManifesto{nome, QString::fromLatin1(hash)});
    }
    if (entradas.empty()) {
        return std::unexpected(recusado("manifesto sem nenhuma entrada: " + caminho.toStdString()));
    }
    return entradas;
}

std::expected<void, ErrosXsd> conferirHash(const QString& diretorio, const EntradaManifesto& entrada)
{
    QFile arquivo{QDir{diretorio}.filePath(entrada.nome)};
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(recusado("arquivo do esquema ausente ou ilegível: " + entrada.nome.toStdString()));
    }
    QCryptographicHash hash{QCryptographicHash::Sha256};
    hash.addData(&arquivo);
    if (arquivo.error() != QFileDevice::NoError) {
        return std::unexpected(recusado("falha ao ler o esquema: " + entrada.nome.toStdString()));
    }
    const QString obtido = QString::fromLatin1(hash.result().toHex());
    if (obtido != entrada.sha256) {
        return std::unexpected(recusado("SHA-256 divergente em " + entrada.nome.toStdString() + ": esperado " +
                                        entrada.sha256.toStdString() + ", obtido " + obtido.toStdString()));
    }
    return {};
}

} // namespace

Esquema::Esquema(std::shared_ptr<const Recurso> recurso) noexcept : recurso_{std::move(recurso)} {}

const Esquema::Recurso& Esquema::recurso() const noexcept
{
    return *recurso_;
}

std::expected<Esquema, ErrosXsd> Esquema::abrir(const QString& diretorio, const QString& manifesto,
                                                const QString& arquivoRaiz)
{
    auto entradas = lerManifesto(manifesto);
    if (!entradas) {
        return std::unexpected(entradas.error());
    }

    const bool raizListada = std::any_of(entradas->cbegin(), entradas->cend(),
                                         [&](const EntradaManifesto& e) { return e.nome == arquivoRaiz; });
    if (!raizListada) {
        return std::unexpected(recusado("arquivo raiz " + arquivoRaiz.toStdString() + " não consta no manifesto"));
    }

    // Cada arquivo da cadeia é conferido, não só o raiz: include adulterado também muda o esquema.
    for (const EntradaManifesto& entrada : *entradas) {
        if (auto conferido = conferirHash(diretorio, entrada); !conferido) {
            return std::unexpected(conferido.error());
        }
    }

    xmlInitParser();
    detail::ColetorErros coletor{"xsd.esquema"};
    const QByteArray caminhoRaiz = QDir{diretorio}.filePath(arquivoRaiz).toUtf8();
    std::unique_ptr<xmlSchemaParserCtxt, detail::ParserCtxtLiberador> contexto{
        xmlSchemaNewParserCtxt(caminhoRaiz.constData())};
    if (!contexto) {
        return std::unexpected(ErrosXsd{Motivo::falhaInterna,
                                        {nl::base::Erro{"xsd.interno", std::nullopt,
                                                        "libxml2 não criou o contexto de leitura do esquema"}}});
    }
    xmlSchemaSetParserStructuredErrors(contexto.get(), detail::coletarErro, &coletor);

    std::unique_ptr<xmlSchema, detail::SchemaLiberador> compilado{xmlSchemaParse(contexto.get())};
    if (!compilado) {
        return std::unexpected(ErrosXsd{Motivo::esquemaRecusado, std::move(coletor.erros)});
    }
    return Esquema{std::make_shared<Recurso>(compilado.release())};
}

} // namespace nl::foundation::xml
