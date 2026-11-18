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

// Imprime: conexao, resultado (tls, sem_tls ou recusa), cliente, servidor, versao, status e erro.
void registrarTls(const char *conexao, const ResultadoConexao &resultado)
{
    QString cliente = QStringLiteral("nulo");
    QString servidor = QStringLiteral("vazio");
    QString versao = QStringLiteral("vazio");
    QString status = QStringLiteral("nd");
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
    const QByteArray tipoBytes = tipo.toUtf8();
    const QByteArray clienteBytes = cliente.toUtf8();
    const QByteArray servidorBytes = servidor.toUtf8();
    const QByteArray versaoBytes = versao.toUtf8();
    const QByteArray statusBytes = status.toUtf8();
    qInfo("NL-TLS conexao=%s resultado=%s cliente=%s servidor=%s versao=%s status=%s erro=%u", conexao,
          tipoBytes.constData(), clienteBytes.constData(), servidorBytes.constData(), versaoBytes.constData(),
          statusBytes.constData(), erro);
}

} // namespace

#endif // NL_TESTE_REGISTRO_TLS_H
