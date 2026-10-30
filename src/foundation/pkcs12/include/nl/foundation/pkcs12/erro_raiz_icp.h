// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Erros da carga das raízes da ICP-Brasil (NL-2.13, DA-2.47). O código é estável para log; o nome do arquivo sai junto,
// nunca o conteúdo nem o caminho absoluto.
#ifndef NL_FOUNDATION_PKCS12_ERRO_RAIZ_ICP_H
#define NL_FOUNDATION_PKCS12_ERRO_RAIZ_ICP_H

#include <string_view>

namespace nl::foundation::pkcs12 {

/// Motivo pelo qual a carga das raízes não entrega âncoras. Qualquer um deles aborta a carga inteira (fail-closed).
enum class ErroRaizIcp {
    ManifestoAusente,     ///< O SHA256SUMS não existe ou não abre.
    ManifestoVazio,       ///< Nenhuma linha de hash no manifesto (piso não-vazio, L-36).
    ManifestoMalformado,  ///< Linha de hash sem metadado, metadado repetido ou sem par, ou nome de arquivo repetido.
    ArquivoAusente,       ///< Arquivo listado no manifesto não existe no diretório.
    ArquivoNaoDeclarado,  ///< Há um .der no diretório que o manifesto não declara (CISO, item 4).
    HashDivergente,       ///< SHA-256 do DER diferente do manifesto. Não usar este arquivo.
    CertificadoIlegivel,  ///< O DER não decodifica como certificado X.509.
    NenhumaRaizAceita,    ///< Todas as raízes foram descartadas: sem âncora, a carga falha.
    FalhaInterna,         ///< Alocação ou falha do OpenSSL ou do Qt fora dos casos acima.
};

/// Código estável para log e para o chamador. Não muda com o texto da mensagem.
[[nodiscard]] constexpr std::string_view codigo(ErroRaizIcp erro) noexcept {
    switch (erro) {
    case ErroRaizIcp::ManifestoAusente: return "raiz_icp.manifesto_ausente";
    case ErroRaizIcp::ManifestoVazio: return "raiz_icp.manifesto_vazio";
    case ErroRaizIcp::ManifestoMalformado: return "raiz_icp.manifesto_malformado";
    case ErroRaizIcp::ArquivoAusente: return "raiz_icp.arquivo_ausente";
    case ErroRaizIcp::ArquivoNaoDeclarado: return "raiz_icp.arquivo_nao_declarado";
    case ErroRaizIcp::HashDivergente: return "raiz_icp.hash_divergente";
    case ErroRaizIcp::CertificadoIlegivel: return "raiz_icp.certificado_ilegivel";
    case ErroRaizIcp::NenhumaRaizAceita: return "raiz_icp.nenhuma_raiz_aceita";
    case ErroRaizIcp::FalhaInterna: return "raiz_icp.falha_interna";
    }
    return "raiz_icp.falha_interna";
}

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_ERRO_RAIZ_ICP_H
