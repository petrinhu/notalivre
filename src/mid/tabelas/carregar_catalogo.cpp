// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Carga do catálogo de localidades (NL-3.M, DA-3M.4). Ordem fixa, e cada passo é uma função própria (NF-32):
//   1. o SHA256SUMS existe e o SHA-256 dele é o pino recebido;
//   2. o manifesto tem as três linhas no formato sha256sum (linhas `#` de comentário são ignoradas; o pino do passo 1 já
//      cobre o arquivo inteiro, comentários inclusive);
//   3. o SHA-256 de cada JSON confere com o manifesto (todos os três, antes de qualquer parse);
//   4. só então o parse, com os códigos validados pelo domínio (CodigoMunicipio, CodigoPais).
// As mensagens de erro trazem o rótulo da tabela, nunca caminho de arquivo nem conteúdo do JSON.
#include <nl/mid/tabelas/catalogo_localidades_embutido.h>

#include <nl/back/dps/codigo_pais.h>
#include <nl/back/identificacao/codigo_municipio.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>
#include <QString>
#include <QStringView>

#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace nl::mid::tabelas {

namespace {

using nl::base::Erro;

struct ArquivoTabela
{
    std::string_view nome;
    std::string_view rotulo;
};

constexpr std::size_t kMunicipios = 0;
constexpr std::size_t kPaises = 1;
constexpr std::size_t kLocalidades = 2;

constexpr std::array<ArquivoTabela, 3> kTabelas{{
    {"municipios.json", "a tabela municipios"},
    {"paises.json", "a tabela paises"},
    {"localidades.json", "a tabela localidades"},
}};

constexpr std::string_view kManifesto = "SHA256SUMS";
constexpr std::string_view kRotuloManifesto = "o manifesto SHA256SUMS";
constexpr std::size_t kTamanhoSha256Hex = 64;

Erro falha(std::string_view codigo, std::string mensagem)
{
    return Erro(std::string(codigo), std::nullopt, std::move(mensagem));
}

std::string hashSha256(const QByteArray& bytes)
{
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex().toStdString();
}

bool ehSha256Minusculo(std::string_view texto)
{
    return texto.size() == kTamanhoSha256Hex
           && std::ranges::all_of(texto, [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

std::optional<std::size_t> indiceDaTabela(std::string_view nome)
{
    for (std::size_t i = 0; i < kTabelas.size(); ++i) {
        if (kTabelas[i].nome == nome) {
            return i;
        }
    }
    return std::nullopt;
}

std::expected<QByteArray, Erro> lerArquivo(const QString& diretorio, std::string_view nome, std::string_view rotulo)
{
    QFile arquivo(QDir(diretorio).filePath(QString::fromUtf8(nome.data(), static_cast<qsizetype>(nome.size()))));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(falha("tabelas.ausente", "não foi possível abrir " + std::string(rotulo) + " no diretório configurado"));
    }
    return arquivo.readAll();
}

std::expected<void, Erro> conferirPino(const QByteArray& manifesto, std::string_view pino)
{
    const std::string obtido = hashSha256(manifesto);
    if (std::string_view(obtido) == pino) {
        return {};
    }
    return std::unexpected(falha("configuracao.tabelas_manifesto_nao_fixado",
                                 "o SHA256SUMS das tabelas não é o fixado no build; as tabelas não foram usadas"));
}

Erro manifestoInvalido()
{
    return falha("tabelas.manifesto_invalido", "o SHA256SUMS das tabelas não está no formato sha256sum com as três tabelas");
}

// Formato sha256sum: 64 hex minúsculos, dois espaços e o nome do arquivo. Cada uma das três tabelas aparece uma vez.
// Linha vazia e linha que começa com `#` (comentário, como no SHA256SUMS da árvore) são ignoradas. O pino do arquivo
// inteiro (passo 1) já cobre os comentários: qualquer byte alterado neles muda o SHA-256 e o manifesto não carrega.
std::expected<std::array<std::string, 3>, Erro> lerManifesto(const QByteArray& bytes)
{
    std::array<std::string, 3> hashes{};
    std::array<bool, 3> vistos{};
    for (const QByteArray& linha : bytes.split('\n')) {
        if (linha.isEmpty() || linha.startsWith('#')) {
            continue;
        }
        const std::string_view texto(linha.constData(), static_cast<std::size_t>(linha.size()));
        if (texto.size() <= kTamanhoSha256Hex + 2 || texto[kTamanhoSha256Hex] != ' ' || texto[kTamanhoSha256Hex + 1] != ' '
            || !ehSha256Minusculo(texto.substr(0, kTamanhoSha256Hex))) {
            return std::unexpected(manifestoInvalido());
        }
        const std::optional<std::size_t> indice = indiceDaTabela(texto.substr(kTamanhoSha256Hex + 2));
        if (!indice || vistos[*indice]) {
            return std::unexpected(manifestoInvalido());
        }
        vistos[*indice] = true;
        hashes[*indice] = std::string(texto.substr(0, kTamanhoSha256Hex));
    }
    if (!std::ranges::all_of(vistos, [](bool visto) { return visto; })) {
        return std::unexpected(manifestoInvalido());
    }
    return hashes;
}

std::expected<void, Erro> conferirArquivo(const QByteArray& bytes, const std::string& esperado, std::string_view rotulo)
{
    if (hashSha256(bytes) == esperado) {
        return {};
    }
    return std::unexpected(falha("tabelas.arquivo_divergente", std::string(rotulo) + " não confere com o SHA256SUMS"));
}

Erro jsonInvalido(std::string_view rotulo, std::string_view motivo)
{
    return falha("tabelas.json_invalido",
                 std::string(rotulo) + " não é um JSON de registros válido (" + std::string(motivo) + ")");
}

// Lista "registros" do JSON. Parse quebrado, raiz que não é objeto, ou lista vazia são recusados.
std::expected<QJsonArray, Erro> registrosDe(const QByteArray& bytes, std::string_view rotulo)
{
    QJsonParseError erroJson{};
    const QJsonDocument documento = QJsonDocument::fromJson(bytes, &erroJson);
    if (erroJson.error != QJsonParseError::NoError || !documento.isObject()) {
        return std::unexpected(jsonInvalido(rotulo, "parse"));
    }
    const QJsonArray registros = documento.object().value(QStringLiteral("registros")).toArray();
    if (registros.isEmpty()) {
        return std::unexpected(jsonInvalido(rotulo, "sem registros"));
    }
    return registros;
}

QString campoTexto(const QJsonValue& registro, const char* chave)
{
    const QJsonValue valor = registro.toObject().value(QString::fromLatin1(chave));
    return valor.isString() ? valor.toString() : QString();
}

std::expected<nl::back::CodigoMunicipio, Erro> validaMunicipio(QStringView texto)
{
    return nl::back::CodigoMunicipio::de(texto);
}

std::expected<nl::back::CodigoPais, Erro> validaPais(QStringView texto)
{
    return nl::back::CodigoPais::de(texto);
}

// Textos do campo `chave` de cada registro, validados pelo domínio. Um registro fora do domínio recusa a tabela inteira,
// com o código do domínio na mensagem (nunca o valor lido).
template <typename Validador>
std::expected<QSet<QString>, Erro> textosValidos(const QByteArray& bytes, std::string_view rotulo, const char* chave, Validador validar)
{
    const auto registros = registrosDe(bytes, rotulo);
    if (!registros) {
        return std::unexpected(registros.error());
    }
    QSet<QString> textos;
    for (const QJsonValue registro : *registros) {
        const QString bruto = campoTexto(registro, chave);
        const auto valido = validar(bruto);
        if (!valido) {
            return std::unexpected(jsonInvalido(rotulo, valido.error().codigo()));
        }
        textos.insert(valido->texto());
    }
    return textos;
}

} // namespace

std::expected<CatalogoLocalidadesEmbutido, Erro> CatalogoLocalidadesEmbutido::carregar(const QString& diretorio,
                                                                                       std::string_view pinoManifesto)
{
    // 1. O SHA256SUMS existe e é o do pino.
    const auto manifesto = lerArquivo(diretorio, kManifesto, kRotuloManifesto);
    if (!manifesto) {
        return std::unexpected(manifesto.error());
    }
    if (const auto pino = conferirPino(*manifesto, pinoManifesto); !pino) {
        return std::unexpected(pino.error());
    }

    // 2. O manifesto lista as três tabelas no formato certo.
    const auto hashes = lerManifesto(*manifesto);
    if (!hashes) {
        return std::unexpected(hashes.error());
    }

    // 3. Cada JSON é lido e conferido com o manifesto. Os três passam por esta conferência antes do primeiro parse.
    std::array<QByteArray, 3> conteudos{};
    for (std::size_t i = 0; i < kTabelas.size(); ++i) {
        auto bytes = lerArquivo(diretorio, kTabelas[i].nome, kTabelas[i].rotulo);
        if (!bytes) {
            return std::unexpected(bytes.error());
        }
        if (const auto conferido = conferirArquivo(*bytes, (*hashes)[i], kTabelas[i].rotulo); !conferido) {
            return std::unexpected(conferido.error());
        }
        conteudos[i] = std::move(*bytes);
    }

    // 4. Só agora o parse. Localidades (0000000) entram no conjunto de municípios, como na TAB.LOC.GERAL (port).
    auto municipios = textosValidos(conteudos[kMunicipios], kTabelas[kMunicipios].rotulo, "codigo", validaMunicipio);
    if (!municipios) {
        return std::unexpected(municipios.error());
    }
    auto localidades = textosValidos(conteudos[kLocalidades], kTabelas[kLocalidades].rotulo, "codigo", validaMunicipio);
    if (!localidades) {
        return std::unexpected(localidades.error());
    }
    auto paises = textosValidos(conteudos[kPaises], kTabelas[kPaises].rotulo, "iso2", validaPais);
    if (!paises) {
        return std::unexpected(paises.error());
    }

    QSet<QString> todosMunicipios = std::move(*municipios);
    todosMunicipios.unite(*localidades);
    return CatalogoLocalidadesEmbutido(std::move(todosMunicipios), std::move(*paises));
}

} // namespace nl::mid::tabelas
