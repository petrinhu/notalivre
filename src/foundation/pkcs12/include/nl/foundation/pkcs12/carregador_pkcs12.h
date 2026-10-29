// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Carregador de A1 (PKCS#12) sobre OpenSSL direto, sem QSslCertificate::importPkcs12 (NL-2.2, pesquisa e).
// Puro: não conhece log nem registro. O evento de EKU ausente é responsabilidade do adaptador no Mid
// (DA-2.30 do CTO): a credencial só indica ekuAusente().
#ifndef NL_FOUNDATION_PKCS12_CARREGADOR_PKCS12_H
#define NL_FOUNDATION_PKCS12_CARREGADOR_PKCS12_H

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArrayView>

#include <expected>

namespace nl::foundation::pkcs12 {

/// Abre o .pfx com a senha e valida, nesta ordem: vencimento da folha, início da validade da folha, cadeia até as
/// âncoras (com validade das intermediárias), EKU e inscrição. Sem estado próprio.
class CarregadorPkcs12 {
public:
    /// A senha entra por referência e sai zerada, com sucesso ou falha. O chamador vê o próprio objeto vazio.
    [[nodiscard]] std::expected<CredencialX509, ErroCredencial> carregar(QByteArrayView pfx, SenhaSegura& senha,
                                                                         const Ancoras& ancoras);
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_CARREGADOR_PKCS12_H
