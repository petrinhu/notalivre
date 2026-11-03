// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do SHA256SUMS das raízes da ICP-Brasil (NL-2.13, DA-2.47). Puro: recebe o texto, não abre arquivo.
#ifndef NL_FOUNDATION_PKCS12_MANIFESTO_RAIZES_H
#define NL_FOUNDATION_PKCS12_MANIFESTO_RAIZES_H

#include <nl/foundation/pkcs12/erro_raiz_icp.h>

#include <QByteArrayView>

#include <expected>
#include <string>
#include <vector>

namespace nl::foundation::pkcs12 {

/// Uma raiz listada no manifesto: hash do DER, nome do arquivo e metadado declarado (proposito e fonte).
struct EntradaRaiz {
    std::string arquivo;
    std::string sha256;
    std::string proposito;
    std::string fonte;
};

/// Lê o manifesto. Cada linha "<sha256>  <arquivo>" exige exatamente uma linha "# meta <arquivo> proposito=... fonte=...".
/// Sem nenhuma linha de hash: ManifestoVazio. Sem metadado, com metadado repetido ou com nome repetido: ManifestoMalformado.
[[nodiscard]] std::expected<std::vector<EntradaRaiz>, ErroRaizIcp> lerManifesto(QByteArrayView texto);

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_MANIFESTO_RAIZES_H
