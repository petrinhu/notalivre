// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Registro de uma conexão do critério 4 (NL-3.1h): uma linha NL-TLS por slot, impressa antes de qualquer asserção.
// O harness transcreve essa linha; o teste é quem afirma o esperado (L-17). Nenhum valor tem espaço, e a mensagem de
// erro não vai à linha.
#ifndef NL_TESTE_REGISTRO_TLS_H
#define NL_TESTE_REGISTRO_TLS_H

#include "conexao_banco.h"
#include "tls_da_sessao.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

namespace {

// Marca da mensagem de recusa, sem espaço, para a linha NL-TLS (C1 do CISO, emenda 1). O texto inteiro vai só à NL-TLS-MSG.
QString marcaDaMensagem(const QString &mensagem)
{
    if (mensagem.isEmpty()) {
        return QStringLiteral("nenhuma");
    }
    if (mensagem.contains(QStringLiteral("SSL is required"))) {
        return QStringLiteral("ssl_requerido");
    }
    if (mensagem.contains(QStringLiteral("Fingerprint validation of peer certificate failed"))) {
        return QStringLiteral("impressao");
    }
    if (mensagem.contains(QStringLiteral("TLS/SSL error"))) {
        return QStringLiteral("cadeia_openssl");
    }
    return QStringLiteral("outra");
}

// Imprime: conexao, resultado (tls, sem_tls ou recusa), cliente, servidor, versao, status e erro.
void registrarTls(const char *conexao, const ResultadoConexao &resultado)
{
    QString cliente = QStringLiteral("nulo");
    QString servidor = QStringLiteral("vazio");
    QString versao = QStringLiteral("vazio");
    QString status = resultado.status;
    QString tipo = QStringLiteral("recusa");
    unsigned erro = resultado.erro;
    if (resultado.conexao) {
        MYSQL *sessao = resultado.conexao.get();
        cliente = tlsDoCliente(sessao);
        servidor = tlsDoServidor(sessao);
        versao = versaoTlsDoServidor(sessao);
        status = statusDaVerificacao(sessao);
        const bool comTls = cliente != QStringLiteral("nulo") && servidor != QStringLiteral("vazio");
        tipo = comTls ? QStringLiteral("tls") : QStringLiteral("sem_tls");
        erro = 0;
    }
    const QString marca = resultado.conexao ? QStringLiteral("nenhuma") : marcaDaMensagem(resultado.mensagem);
    const QByteArray marcaBytes = marca.toUtf8();
    if (!resultado.conexao) {
        const QByteArray mensagemBytes = resultado.mensagem.toUtf8();
        qInfo("NL-TLS-MSG conexao=%s mensagem=%s", conexao, mensagemBytes.constData());
    }
    const QByteArray tipoBytes = tipo.toUtf8();
    const QByteArray clienteBytes = cliente.toUtf8();
    const QByteArray servidorBytes = servidor.toUtf8();
    const QByteArray versaoBytes = versao.toUtf8();
    const QByteArray statusBytes = status.toUtf8();
    qInfo("NL-TLS conexao=%s resultado=%s cliente=%s servidor=%s versao=%s status=%s msg=%s erro=%u", conexao,
          tipoBytes.constData(), clienteBytes.constData(), servidorBytes.constData(), versaoBytes.constData(),
          statusBytes.constData(), marcaBytes.constData(), erro);
}

} // namespace

#endif // NL_TESTE_REGISTRO_TLS_H
