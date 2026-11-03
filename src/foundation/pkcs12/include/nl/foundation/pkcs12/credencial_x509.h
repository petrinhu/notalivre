// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Credencial A1 aberta (NL-2.2, decisão do CISO, item 4): guarda a chave privada enquanto a credencial vive.
#ifndef NL_FOUNDATION_PKCS12_CREDENCIAL_X509_H
#define NL_FOUNDATION_PKCS12_CREDENCIAL_X509_H

#include <nl/foundation/pkcs12/inscricao.h>
#include <nl/foundation/pkcs12/ponteiros_ossl.h>

namespace nl::foundation::pkcs12 {

class CarregadorPkcs12;

/// Move-only: a chave privada tem um dono e não é copiada. descartar() libera chave e certificado na hora.
class CredencialX509 {
public:
    CredencialX509(CredencialX509&& outra) noexcept = default;
    CredencialX509& operator=(CredencialX509&& outra) noexcept = default;
    CredencialX509(const CredencialX509&) = delete;
    CredencialX509& operator=(const CredencialX509&) = delete;
    ~CredencialX509() = default;

    [[nodiscard]] Inscricao inscricao() const { return inscricao_; }

    /// Verdadeiro quando o certificado não traz extensão EKU (aceito, com evento de auditoria sem PII).
    [[nodiscard]] bool ekuAusente() const noexcept { return ekuAusente_; }

    /// Certificado folha, emprestado. nullptr depois de descartar().
    [[nodiscard]] x509_st* certificado() const noexcept { return certificado_.get(); }

    /// Chave privada, emprestada. Válida só enquanto a credencial existe e não foi descartada.
    [[nodiscard]] evp_pkey_st* chavePrivada() const noexcept { return chave_.get(); }

    /// Libera chave e certificado agora. Depois disto a credencial não assina nem conecta.
    void descartar() noexcept;

    [[nodiscard]] bool descartada() const noexcept { return chave_ == nullptr; }

private:
    friend class CarregadorPkcs12;

    CredencialX509(CertificadoX509 certificado, ChavePrivada chave, Inscricao inscricao, bool ekuAusente) noexcept;

    CertificadoX509 certificado_;
    ChavePrivada chave_;
    Inscricao inscricao_;
    bool ekuAusente_;
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_CREDENCIAL_X509_H
