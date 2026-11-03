// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras de confiança lidas de PEM (NL-2.2). O conjunto é fornecido pelo chamador; nenhuma raiz é embutida.
#include <nl/foundation/pkcs12/ancoras.h>

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <climits>
#include <utility>

namespace nl::foundation::pkcs12 {

std::expected<Ancoras, ErroAncora> Ancoras::dePem(QByteArrayView pem) {
    if (pem.size() <= 0 || pem.size() > INT_MAX) {
        return std::unexpected(ErroAncora::PemSemCertificado);
    }
    BIO* bio = BIO_new_mem_buf(pem.constData(), static_cast<int>(pem.size()));
    if (bio == nullptr) {
        ERR_clear_error();
        return std::unexpected(ErroAncora::FalhaInterna);
    }
    std::vector<CertificadoX509> certificados;
    while (X509* lido = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) {
        // Posse antes do push: se o vetor não crescer (bad_alloc), o certificado já é liberado pelo unique_ptr.
        CertificadoX509 posse(lido);
        certificados.push_back(std::move(posse));
    }
    BIO_free(bio);
    // A leitura termina com "no start line" no fim do texto: não é erro do chamador.
    ERR_clear_error();
    if (certificados.empty()) {
        return std::unexpected(ErroAncora::PemSemCertificado);
    }
    return Ancoras(std::move(certificados));
}

std::expected<Ancoras, ErroAncora> Ancoras::deCertificados(std::vector<CertificadoX509> certificados) {
    if (certificados.empty()) {
        return std::unexpected(ErroAncora::SemCertificado);
    }
    return Ancoras(std::move(certificados));
}

Ancoras::Ancoras(std::vector<CertificadoX509> certificados) noexcept
    : certificados_(std::move(certificados)) {}

std::size_t Ancoras::quantidade() const noexcept {
    return certificados_.size();
}

std::vector<x509_st*> Ancoras::certificados() const {
    std::vector<x509_st*> emprestados;
    emprestados.reserve(certificados_.size());
    for (const CertificadoX509& certificado : certificados_) {
        emprestados.push_back(certificado.get());
    }
    return emprestados;
}

} // namespace nl::foundation::pkcs12
