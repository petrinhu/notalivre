// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: lê um arquivo da pasta de certificados de teste (NL_CERTIFICADOS_DIR).
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_FIXTURE_CERTIFICADO_TESTE_H
#define NL_TESTE_FIXTURE_CERTIFICADO_TESTE_H

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>

namespace {

QByteArray lerFixture(const QString& nome)
{
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + nome);
    return arquivo.open(QIODevice::ReadOnly) ? arquivo.readAll() : QByteArray();
}

} // namespace

#endif // NL_TESTE_FIXTURE_CERTIFICADO_TESTE_H
