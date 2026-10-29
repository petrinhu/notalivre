// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras de confiança da cadeia (NL-2.2, decisão do CISO: a verificação recebe as âncoras de fora).
#ifndef NL_FOUNDATION_PKCS12_ANCORAS_H
#define NL_FOUNDATION_PKCS12_ANCORAS_H

#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/ponteiros_ossl.h>

#include <QByteArrayView>

#include <cstddef>
#include <expected>
#include <vector>

namespace nl::foundation::pkcs12 {

/// Conjunto de certificados confiáveis. O chamador escolhe; o código de cadeia não embute raiz nenhuma.
class Ancoras {
public:
    /// Lê todos os certificados PEM do texto. Nenhum certificado lido: PemSemCertificado.
    [[nodiscard]] static std::expected<Ancoras, ErroAncora> dePem(QByteArrayView pem);

    Ancoras(Ancoras&& outra) noexcept = default;
    Ancoras& operator=(Ancoras&& outra) noexcept = default;
    Ancoras(const Ancoras&) = delete;
    Ancoras& operator=(const Ancoras&) = delete;
    ~Ancoras() = default;

    [[nodiscard]] std::size_t quantidade() const noexcept;

    /// Ponteiros emprestados, válidos enquanto este objeto existir. Não liberar.
    [[nodiscard]] std::vector<x509_st*> certificados() const;

private:
    explicit Ancoras(std::vector<CertificadoX509> certificados) noexcept;

    std::vector<CertificadoX509> certificados_;
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_ANCORAS_H
