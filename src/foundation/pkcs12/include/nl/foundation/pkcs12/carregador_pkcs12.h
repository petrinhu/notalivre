// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Carregador de A1 (PKCS#12) sobre OpenSSL direto, sem QSslCertificate::importPkcs12 (NL-2.2, pesquisa e).
#ifndef NL_FOUNDATION_PKCS12_CARREGADOR_PKCS12_H
#define NL_FOUNDATION_PKCS12_CARREGADOR_PKCS12_H

#include <nl/foundation/log/log.h>
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArrayView>

#include <expected>

namespace nl::foundation::pkcs12 {

/// Abre o .pfx com a senha e valida, nesta ordem: vencimento, cadeia até as âncoras, EKU e inscrição.
/// Não tem estado próprio: o registro de auditoria entra pelo construtor (sem singleton, ADR-S4).
class CarregadorPkcs12 {
public:
    explicit CarregadorPkcs12(nl::foundation::log::Log& registro) noexcept;

    /// A senha é zerada logo após o parse, e a cópia intermediária do PKCS#12 é liberada antes de devolver.
    [[nodiscard]] std::expected<CredencialX509, ErroCredencial> carregar(QByteArrayView pfx, SenhaSegura senha,
                                                                         const Ancoras& ancoras);

private:
    nl::foundation::log::Log& registro_;
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_CARREGADOR_PKCS12_H
