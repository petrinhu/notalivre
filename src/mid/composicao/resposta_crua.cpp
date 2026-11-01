// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta crua da Sefin para análise local (NL-2.9, DA-2.59 item 4; NL-2.10c, DA-2.66: recusas incluídas). Ver resposta_crua.h.
#include <nl/mid/composicao/resposta_crua.h>
#include <nl/mid/sefin/cliente/campos_lidos_sefin.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QFileInfo>
#include <QRegularExpression>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>

namespace nl::mid::composicao {
namespace {

// Nome igual a um campo lido pelo cliente: a comparação é com a mesma constante que o leitor usa (campos_lidos_sefin.h).
bool nomeIgual(const QString& nome, std::string_view campo)
{
    return nome == QString::fromLatin1(campo.data(), static_cast<qsizetype>(campo.size()));
}

// Limites da descida (condição do CISO, 31/10/2026): oito níveis de objeto ou array; cem elementos por array.
constexpr int kProfundidadeMaxima = 8;
constexpr qsizetype kElementosMaximos = 100;
// Teto do arquivo de análise por emissão (condição do CISO): 4 MiB. A linha de corte fica fora do teto.
constexpr qsizetype kTetoArquivoBytes = 4 * 1024 * 1024;

QJsonObject mascararObjeto(const QJsonObject& objeto, int nivel);
QJsonValue mascararValor(const QString& nome, const QJsonValue& valor, int nivel);

// Nome de chave que sai em claro (CISO, MÉDIA-1, Emenda 3): só um dos nomes que o cliente lê (campos_lidos_sefin.h). A regra
// antiga (regex e limite de dígitos) saiu: um nome como ABCDEFGHIJKL12 passava nela.
bool lidoPeloCliente(const QString& nome)
{
    for (const std::string_view campo : nl::mid::sefin::cliente::kCamposLidosPeloCliente) {
        if (nomeIgual(nome, campo)) {
            return true;
        }
    }
    return false;
}

// Classe dos caracteres de um nome: letras, digitos ou alfanumerico (só ASCII); outro, se houver qualquer outro caractere.
QString classeDoNome(const QString& nome)
{
    bool letras = false;
    bool digitos = false;
    bool outro = false;
    for (const QChar c : nome) {
        const char16_t u = c.unicode();
        if ((u >= u'a' && u <= u'z') || (u >= u'A' && u <= u'Z')) {
            letras = true;
        } else if (u >= u'0' && u <= u'9') {
            digitos = true;
        } else {
            outro = true;
        }
    }
    if (outro || nome.isEmpty()) {
        return QStringLiteral("outro");
    }
    if (letras && digitos) {
        return QStringLiteral("alfanumerico");
    }
    return letras ? QStringLiteral("letras") : QStringLiteral("digitos");
}

// Nome que não é lido pelo cliente, na forma <chave:N:classe:k>: N o tamanho, classe a forma dos caracteres e k a ordem no objeto.
// A ordem torna únicos os nomes de mesmo tamanho, então nenhum campo colide (INFO-9).
QString formaDoNome(const QString& nome, qsizetype ordem)
{
    return QStringLiteral("<chave:%1:%2:%3>").arg(nome.size()).arg(classeDoNome(nome)).arg(ordem);
}

// Content-Type que pode sair em claro (CISO, BAIXA-2', Emenda 3): application/json ou application/problem+json, com ou sem charset
// da lista fechada utf-8 ou utf8, sem diferenciar maiúsculas. O resto sai como <string:N>.
bool tipoDeConteudoSeguro(const QString& tipo)
{
    static const QRegularExpression padrao(
        QStringLiteral("\\A(application/json|application/problem\\+json)(; charset=(?i:utf-8|utf8))?\\z"));
    return padrao.match(tipo).hasMatch();
}

// O Content-Type como vai ao arquivo: o tipo permitido em claro, ou <string:N> com N o tamanho do valor recebido.
QJsonValue tipoDeConteudoEscrito(const QByteArray& tipoConteudo)
{
    const QString tipo = QString::fromUtf8(tipoConteudo);
    if (tipoDeConteudoSeguro(tipo)) {
        return tipo;
    }
    return QStringLiteral("<string:%1>").arg(tipoConteudo.size());
}

// Código E válido: ^E\d{3,4}$ (DA-2.41, b). A mesma regra existe em cliente_sefin.cpp, que é privada dele (segunda ocorrência).
bool codigoEValido(const QString& texto)
{
    if (texto.size() != 4 && texto.size() != 5) {
        return false;
    }
    if (texto.at(0) != QLatin1Char('E')) {
        return false;
    }
    for (qsizetype i = 1; i < texto.size(); ++i) {
        if (texto.at(i) < QLatin1Char('0') || texto.at(i) > QLatin1Char('9')) {
            return false;
        }
    }
    return true;
}

// Um texto sai inteiro só se o campo é Codigo ou codigo e o valor inteiro casa com ^E\d{3,4}$.
bool campoDeCodigoE(const QString& nome, const QString& texto)
{
    return (nomeIgual(nome, nl::mid::sefin::cliente::kCampoCodigo) || nomeIgual(nome, nl::mid::sefin::cliente::kCampoCodigoMinusculo))
           && codigoEValido(texto);
}

// Array: cada um dos cem primeiros elementos passa pela mesma regra; o resto vira um marcador <mais:N>.
QJsonArray mascararArray(const QJsonArray& array, int nivel)
{
    QJsonArray mascarado;
    const qsizetype visiveis = std::min(array.size(), kElementosMaximos);
    for (qsizetype i = 0; i < visiveis; ++i) {
        mascarado.append(mascararValor(QString(), array.at(i), nivel + 1));
    }
    if (array.size() > visiveis) {
        mascarado.append(QStringLiteral("<mais:%1>").arg(array.size() - visiveis));
    }
    return mascarado;
}

// Allowlist de valor (CISO N-03, ADR-0005): só a chave e o código E válido saem; qualquer outro valor sai como <tipo:tamanho>. Os
// nomes dos campos e a forma do objeto ficam, para o analista ver o que a Sefin mandou sem ver o conteúdo. O nível é o do valor:
// a raiz é 1, e um objeto ou array no nível 9 ou abaixo vira <profundidade>.
QJsonValue mascararValor(const QString& nome, const QJsonValue& valor, int nivel)
{
    if (valor.isObject()) {
        if (nivel > kProfundidadeMaxima) {
            return QStringLiteral("<profundidade>");
        }
        return mascararObjeto(valor.toObject(), nivel);
    }
    if (valor.isArray()) {
        if (nivel > kProfundidadeMaxima) {
            return QStringLiteral("<profundidade>");
        }
        return mascararArray(valor.toArray(), nivel);
    }
    if (valor.isString()) {
        const QString texto = valor.toString();
        if (nomeIgual(nome, nl::mid::sefin::cliente::kCampoChaveAcesso)) {
            QString mascara;
            mascara.fill(QLatin1Char('#'), texto.size());
            return mascara;
        }
        if (campoDeCodigoE(nome, texto)) {
            return texto;
        }
        return QStringLiteral("<string:%1>").arg(texto.size());
    }
    if (valor.isBool()) {
        return QStringLiteral("<bool:%1>").arg(valor.toBool() ? 4 : 5);
    }
    if (valor.isDouble()) {
        return QStringLiteral("<number:%1>").arg(QString::number(valor.toDouble()).size());
    }
    return QStringLiteral("<null:0>");
}

QJsonObject mascararObjeto(const QJsonObject& objeto, int nivel)
{
    QJsonObject mascarado;
    qsizetype ordem = 0;
    for (auto it = objeto.constBegin(); it != objeto.constEnd(); ++it) {
        ++ordem;
        const QString chave = lidoPeloCliente(it.key()) ? it.key() : formaDoNome(it.key(), ordem);
        mascarado.insert(chave, mascararValor(it.key(), it.value(), nivel + 1));
    }
    return mascarado;
}

// A pasta respostas nasce 0700 (CISO, BAIXA-4): mkdir com o modo fechado. Pasta que já existe não tem o modo mudado.
// A pasta respostas sai 0700 (CISO, BAIXA-4 e BAIXA-4b, Emenda 3): criada com mkdir 0700, e se já existe com outro modo, fixada em 0700.
bool criarPasta0700(const QString& pasta)
{
    if (!QDir(pasta).exists() && ::mkdir(QFile::encodeName(pasta).constData(), 0700) != 0) {
        return false;
    }
    return QFile::setPermissions(pasta, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

// O arquivo sai 0600 (CISO, BAIXA-3 e INFO-3, Emenda 3): criado com modo 0600 (o umask só tira bits) e, se já existia com modo largo,
// fixado em 0600 antes de qualquer linha. Quem escreve abre em seguida com QFile.
bool criarArquivo0600(const QString& caminho)
{
    const int descritor = ::open(QFile::encodeName(caminho).constData(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if (descritor < 0) {
        return false;
    }
    const bool ajustado = ::fchmod(descritor, 0600) == 0;
    ::close(descritor);
    return ajustado;
}

// Nome do arquivo: 16 caracteres do hash do Id. Nem o Id nem a chave aparecem no nome.
QByteArray hashDoId(std::string_view idDps)
{
    return QCryptographicHash::hash(QByteArray(idDps.data(), static_cast<qsizetype>(idDps.size())), QCryptographicHash::Sha256)
        .toHex()
        .left(16);
}

// Uma linha do arquivo: o envelope da resposta (método, rota modelo, status, Content-Type) e o corpo pela allowlist.
QByteArray linhaDaResposta(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                           const QByteArray& corpo)
{
    const QJsonDocument mascarado = QJsonDocument::fromJson(mascararRespostaCrua(corpo));
    const QJsonObject linha{
        {QStringLiteral("metodo"), QString::fromUtf8(metodo.data(), static_cast<qsizetype>(metodo.size()))},
        {QStringLiteral("rota"), QString::fromUtf8(rota.data(), static_cast<qsizetype>(rota.size()))},
        {QStringLiteral("status"), status},
        {QStringLiteral("contentType"), tipoDeConteudoEscrito(tipoConteudo)},
        {QStringLiteral("corpo"), mascarado.object()},
    };
    return QJsonDocument(linha).toJson(QJsonDocument::Compact) + '\n';
}

} // namespace

QByteArray mascararRespostaCrua(const QByteArray& corpo)
{
    QJsonParseError erro;
    const QJsonDocument doc = QJsonDocument::fromJson(corpo, &erro);
    if (!doc.isObject()) {
        return QByteArrayLiteral("{\"aviso\":\"corpo que nao e JSON, omitido\"}");
    }
    return QJsonDocument(mascararObjeto(doc.object(), 1)).toJson(QJsonDocument::Compact);
}

GravadorRespostaCrua::GravadorRespostaCrua(QString pastaBase, std::string idDps)
    : pastaBase_(std::move(pastaBase)), idDps_(std::move(idDps))
{
}

void GravadorRespostaCrua::registrar(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                                     const QByteArray& corpo)
{
    if (erro_) {
        return;  // Depois do primeiro erro o arquivo não recebe mais linhas, para não sair uma linha pela metade.
    }
    const QString pasta = pastaBase_ + QStringLiteral("/respostas");
    if (!QDir().mkpath(pastaBase_) || !criarPasta0700(pasta)) {
        erro_ = nl::base::Erro("analise.pasta", std::nullopt, "não foi possível criar a pasta da resposta para análise");
        return;
    }
    if (caminho_.isEmpty()) {
        caminho_ = pasta + QStringLiteral("/") + QString::fromLatin1(hashDoId(idDps_)) + QStringLiteral(".jsonl");
    }
    if (!retomado_) {
        retomarArquivo();
        if (erro_) {
            return;
        }
    }
    if (cortado_) {
        ++omitidas_;
        escreverCorte();
        return;
    }
    const QByteArray linha = linhaDaResposta(metodo, rota, status, tipoConteudo, corpo);
    if (tamanhoRespostas_ + linha.size() > kTetoArquivoBytes) {
        cortado_ = true;  // a resposta inteira não cabe: nenhuma linha é cortada pela metade
        ++omitidas_;
        escreverCorte();
        return;
    }
    if (!criarArquivo0600(caminho_)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível criar o arquivo da resposta");
        return;
    }
    QFile arquivo(caminho_);
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Append)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível abrir o arquivo da resposta");
        return;
    }
    if (arquivo.write(linha) != linha.size() || !arquivo.flush()) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível gravar a resposta");
        return;
    }
    arquivo.close();
    tamanhoRespostas_ += linha.size();
}

void GravadorRespostaCrua::escreverCorte()
{
    // Corta o arquivo no fim das respostas e escreve a linha de corte com o total atual: uma linha só, nunca duas com N defasado.
    if (!criarArquivo0600(caminho_)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível criar o arquivo da resposta");
        return;
    }
    QFile arquivo(caminho_);
    if (!arquivo.open(QIODevice::ReadWrite)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível abrir o arquivo da resposta");
        return;
    }
    const QByteArray corte = QByteArrayLiteral("{\"corte\":\"teto de ") + QByteArray::number(kTetoArquivoBytes)
                             + QByteArrayLiteral(" bytes atingido\",\"respostas_omitidas\":") + QByteArray::number(omitidas_)
                             + QByteArrayLiteral("}\n");
    if (!arquivo.resize(tamanhoRespostas_) || !arquivo.seek(tamanhoRespostas_) || arquivo.write(corte) != corte.size()
        || !arquivo.flush()) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível gravar a linha de corte");
    }
}

// Primeira gravação do gravador (CISO, BAIXA-5, Emenda 2): o arquivo pode já existir, do mesmo Id. O gravador segue do tamanho que
// o arquivo tem. Se a última linha já é a de corte, ele continua antes dela, com o total de omitidas que ela traz: a linha de
// corte nunca fica no meio nem se duplica, e as respostas anteriores não são apagadas.
void GravadorRespostaCrua::retomarArquivo()
{
    retomado_ = true;
    if (!QFileInfo(caminho_).exists()) {
        return;
    }
    QFile arquivo(caminho_);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível ler o arquivo da resposta já gravado");
        return;
    }
    const QByteArray conteudo = arquivo.readAll();
    const qsizetype fimDaUltima = conteudo.endsWith('\n') ? conteudo.size() - 1 : conteudo.size();
    const qsizetype inicioDaUltima = fimDaUltima > 0 ? conteudo.lastIndexOf('\n', fimDaUltima - 1) + 1 : 0;
    const QByteArray ultima = conteudo.mid(inicioDaUltima, fimDaUltima - inicioDaUltima);
    if (!ultima.startsWith(QByteArrayLiteral("{\"corte\":"))) {
        tamanhoRespostas_ = conteudo.size();
        return;
    }
    const QByteArray marcaOmitidas = QByteArrayLiteral("\"respostas_omitidas\":");
    const qsizetype posicaoOmitidas = ultima.indexOf(marcaOmitidas);
    if (posicaoOmitidas < 0) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "a linha de corte do arquivo já gravado não confere");
        return;
    }
    const QByteArray depois = ultima.mid(posicaoOmitidas + marcaOmitidas.size());
    bool convertido = false;
    omitidas_ = depois.left(depois.indexOf('}')).toLongLong(&convertido);
    if (!convertido) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "a linha de corte do arquivo já gravado não confere");
        return;
    }
    tamanhoRespostas_ = inicioDaUltima;
    cortado_ = true;
}

} // namespace nl::mid::composicao
