// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ambiente do teste do banco (NL-3.1h): lê a configuração das variáveis NL_BANCO_* e a senha de um arquivo do par.
// Sem uma variável, nada é devolvido e o teste falha com mensagem; nunca pula. A senha nunca vai a log.
#ifndef NL_TESTE_AMBIENTE_BANCO_H
#define NL_TESTE_AMBIENTE_BANCO_H

#include <QByteArray>
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

// Dados do servidor do par, pelas variáveis NL_BANCO_HOST, NL_BANCO_PORTA, NL_BANCO_USUARIO, NL_BANCO_BASE e
// NL_BANCO_SENHA_ARQUIVO. Qualquer uma faltando, ou porta que não é número positivo, devolve nada.
std::optional<DadosBanco> dadosDoAmbiente()
{
    const auto host = variavelDoBanco("NL_BANCO_HOST");
    const auto porta = variavelDoBanco("NL_BANCO_PORTA");
    const auto usuario = variavelDoBanco("NL_BANCO_USUARIO");
    const auto base = variavelDoBanco("NL_BANCO_BASE");
    const auto arquivo = variavelDoBanco("NL_BANCO_SENHA_ARQUIVO");
    if (!host || !porta || !usuario || !base || !arquivo) {
        return std::nullopt;
    }
    bool ok = false;
    const unsigned numeroPorta = porta->toUInt(&ok);
    if (!ok || numeroPorta == 0) {
        return std::nullopt;
    }
    return DadosBanco{*host, numeroPorta, *usuario, *base, *arquivo};
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
