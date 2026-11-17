// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: a credencial cnpj_ok (NL-2.2), carregada com a cadeia da AC de teste.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_CREDENCIAL_TESTE_H
#define NL_TESTE_CREDENCIAL_TESTE_H

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>

#include "fixture_certificado_teste.h"

namespace fp = nl::foundation::pkcs12;

namespace {

// Credencial cnpj_ok (NL-2.2), com a mesma cadeia que o app usa.
std::expected<fp::CredencialX509, fp::ErroCredencial> credencial()
{
    auto ancoras = fp::Ancoras::dePem(lerFixture(QStringLiteral("ac.pem")));
    if (!ancoras) {
        return std::unexpected(fp::ErroCredencial::CadeiaInvalida);
    }
    QFile senhaArquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    fp::SenhaSegura senha = fp::SenhaSegura::de(senhaArquivo.open(QIODevice::ReadOnly)
                                                    ? QByteArray(senhaArquivo.readLine().trimmed())
                                                    : QByteArray());
    fp::CarregadorPkcs12 carregador;
    return carregador.carregar(lerFixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
}

} // namespace

#endif // NL_TESTE_CREDENCIAL_TESTE_H
