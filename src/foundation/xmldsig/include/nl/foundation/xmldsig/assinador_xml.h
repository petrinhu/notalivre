// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Assinador XMLDSIG enveloped (NL-2.3, DA-2.1, DA-2.2, DA-2.10, DA-2.30). Motor agnóstico de domínio: não conhece DPS,
// Sefin nem a credencial do NL-2.2. Recebe o material (certificado e chave) emprestado, por ponteiro, e não é dono dele.
// Algoritmos e URIs chegam pelos parâmetros: a pasta do leiaute (Mid) decide, nunca uma constante deste motor.
#ifndef NL_FOUNDATION_XMLDSIG_ASSINADOR_XML_H
#define NL_FOUNDATION_XMLDSIG_ASSINADOR_XML_H

#include <QByteArray>
#include <QByteArrayView>
#include <QStringView>

#include <expected>
#include <string_view>

// Tipos do OpenSSL só por declaração. Nenhum cabeçalho do OpenSSL ou do libxml2 entra no público.
struct x509_st;
struct evp_pkey_st;

namespace nl::foundation::xmldsig {

enum class ErroAssinatura {
    XmlMalformado,
    XmlComDtd,            // DTD ou entidade: recusado antes de qualquer processamento (DA-2.40, item 1)
    ReferenciaNaoEncontrada,
    ReferenciaAmbigua,    // mais de um elemento com o Id (DA-2.40, item 2)
    JaAssinado,           // já há ds:Signature no documento (DA-2.40, item 3)
    AlgoritmoBloqueado,
    MaterialInvalido,
    FalhaInterna,
};

/// Código estável do erro, para o log e para a interface. O algoritmo recusado pelo OpenSSL sai como
/// "assinatura.algoritmo_bloqueado" (DA-2.10), nunca como falha silenciosa.
[[nodiscard]] std::string_view codigo(ErroAssinatura erro) noexcept;

/// URIs do algoritmo, todos vindos da pasta do leiaute (ex.: rsa-sha1, sha1, C14N 20010315, enveloped).
struct ParametrosAssinatura {
    std::string_view uriC14n;
    std::string_view uriEnveloped;
    std::string_view uriDigest;
    std::string_view uriAssinatura;
    // Forma do elemento assinado, declarada pela pasta do leiaute (DA-2.40, item 2): o Id só vale no elemento
    // nomeReferencia, filho de nomePaiReferencia, no namespace namespaceLeiaute.
    std::string_view nomeReferencia;
    std::string_view nomePaiReferencia;
    std::string_view namespaceLeiaute;
};

/// Material emprestado: quem chama mantém o certificado e a chave vivos durante a chamada.
struct MaterialAssinatura {
    const x509_st* certificado = nullptr;
    const evp_pkey_st* chave = nullptr;
};

class AssinadorXml {
public:
    /// Assina o elemento de Id idReferencia com uma Signature irmã dele, enveloped. Devolve o XML assinado.
    [[nodiscard]] static std::expected<QByteArray, ErroAssinatura> assinarEnveloped(
        QByteArrayView xml, QStringView idReferencia, const ParametrosAssinatura& parametros,
        const MaterialAssinatura& material);
};

} // namespace nl::foundation::xmldsig

#endif // NL_FOUNDATION_XMLDSIG_ASSINADOR_XML_H
