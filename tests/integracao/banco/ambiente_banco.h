// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ambiente do teste do banco (NL-3.1h): lê a configuração das variáveis NL_BANCO_* e a senha de um arquivo do par.
// Sem uma variável, nada é devolvido e o teste falha com mensagem; nunca pula. A senha nunca vai a log.
#ifndef NL_TESTE_AMBIENTE_BANCO_H
#define NL_TESTE_AMBIENTE_BANCO_H

#include <QByteArray>
#include <QChar>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtGlobal>

#include <optional>

namespace {

struct DadosBanco {
    QString host;
    unsigned porta = 0;
    QString usuario;
    QString base;
    QString arquivoSenha;
    QString servidorTls;   // "sim" ou "nao": o servidor do par exige TLS (require_secure_transport)
    QString certificado;   // "disco", "memoria" ou "nenhum": de onde vem o certificado do servidor
    QString impressao;     // 64 hexadecimais (SHA-256 do certificado do servidor) ou "nenhuma"
};

// Valor de uma variável NL_BANCO_* obrigatória; vazia ou ausente devolve nada.
std::optional<QString> variavelDoBanco(const char *nome)
{
    const QString valor = qEnvironmentVariable(nome);
    if (valor.isEmpty()) {
        return std::nullopt;
    }
    return valor;
}

// Impressão de 64 hexadecimais, ou a palavra "nenhuma" (sem certificado para fixar).
bool impressaoValida(const QString &valor)
{
    if (valor == QStringLiteral("nenhuma")) {
        return true;
    }
    if (valor.size() != 64) {
        return false;
    }
    for (const QChar c : valor) {
        const QChar minusculo = c.toLower();
        const bool digito = c >= QLatin1Char('0') && c <= QLatin1Char('9');
        const bool hexa = minusculo >= QLatin1Char('a') && minusculo <= QLatin1Char('f');
        if (!digito && !hexa) {
            return false;
        }
    }
    return true;
}

// Dados do servidor do par, pelas variáveis NL_BANCO_HOST, NL_BANCO_PORTA, NL_BANCO_USUARIO, NL_BANCO_BASE,
// NL_BANCO_SENHA_ARQUIVO, NL_BANCO_SERVIDOR_TLS, NL_BANCO_CERTIFICADO e NL_BANCO_IMPRESSAO. Qualquer uma faltando,
// porta que não é número positivo, ou valor fora do conjunto devolve nada.
std::optional<DadosBanco> dadosDoAmbiente()
{
    const auto host = variavelDoBanco("NL_BANCO_HOST");
    const auto porta = variavelDoBanco("NL_BANCO_PORTA");
    const auto usuario = variavelDoBanco("NL_BANCO_USUARIO");
    const auto base = variavelDoBanco("NL_BANCO_BASE");
    const auto arquivo = variavelDoBanco("NL_BANCO_SENHA_ARQUIVO");
    const auto servidorTls = variavelDoBanco("NL_BANCO_SERVIDOR_TLS");
    const auto certificado = variavelDoBanco("NL_BANCO_CERTIFICADO");
    const auto impressao = variavelDoBanco("NL_BANCO_IMPRESSAO");
    if (!host || !porta || !usuario || !base || !arquivo || !servidorTls || !certificado || !impressao) {
        return std::nullopt;
    }
    bool ok = false;
    const unsigned numeroPorta = porta->toUInt(&ok);
    if (!ok || numeroPorta == 0) {
        return std::nullopt;
    }
    if (*servidorTls != QStringLiteral("sim") && *servidorTls != QStringLiteral("nao")) {
        return std::nullopt;
    }
    if (*certificado != QStringLiteral("disco") && *certificado != QStringLiteral("memoria")
        && *certificado != QStringLiteral("nenhum")) {
        return std::nullopt;
    }
    if (!impressaoValida(*impressao)) {
        return std::nullopt;
    }
    return DadosBanco{*host, numeroPorta, *usuario, *base, *arquivo, *servidorTls, *certificado, *impressao};
}

// Conteúdo do arquivo de senha, sem o fim de linha. Arquivo ilegível ou vazio devolve nada.
std::optional<QByteArray> senhaDoArquivo(const QString &caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    QByteArray senha = arquivo.readAll();
    if (senha.endsWith('\n')) {
        senha.chop(1);
    }
    if (senha.isEmpty()) {
        return std::nullopt;
    }
    return senha;
}

} // namespace

#endif // NL_TESTE_AMBIENTE_BANCO_H
