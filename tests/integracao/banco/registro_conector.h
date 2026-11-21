// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Registro NL-CONECTOR do teste de banco (NL-3.1h, G-R do CISO; extraído de tst_conexao_banco.cpp no H-G do NL-3.1a).
// A linha sai antes das asserções, uma por processo; o harness a conta como a NL-TLS (banco/linhas.sh). O formato não muda.
#ifndef NL_TESTE_BANCO_REGISTRO_CONECTOR_H
#define NL_TESTE_BANCO_REGISTRO_CONECTOR_H

#include <QByteArray>
#include <QDebug>
#include <QString>

inline void registrarNlConector(const QString &cabecalho, const QString &execucao, const QString &biblioteca,
                                const QString &prefixo)
{
    const QByteArray cabecalhoBytes = cabecalho.toUtf8();
    const QByteArray execucaoBytes = execucao.toUtf8();
    const QByteArray bibliotecaBytes = biblioteca.toUtf8();
    const QByteArray prefixoBytes = prefixo.isEmpty() ? QByteArrayLiteral("nd") : prefixo.toUtf8();
    qInfo("NL-CONECTOR cabecalho=%s execucao=%s biblioteca=%s prefixo=%s", cabecalhoBytes.constData(),
          execucaoBytes.constData(), bibliotecaBytes.constData(), prefixoBytes.constData());
}

#endif // NL_TESTE_BANCO_REGISTRO_CONECTOR_H
