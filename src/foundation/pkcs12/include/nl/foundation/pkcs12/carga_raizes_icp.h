// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Carga das raízes da ICP-Brasil como âncoras (NL-2.13, DA-2.47). O motor não sabe o que é ICP: recebe o diretório, o
// manifesto, a finalidade aceita e o instante. Quem escolhe o conjunto de produção é o Mid (DA-2.44).
// Política: todo arquivo do manifesto é conferido por hash (qualquer divergência recusa a carga inteira). Depois, cada
// raiz é descartada, sem recusar a carga, se estiver vencida, ainda não válida ou com finalidade diferente da pedida.
// Arquivo do diretório que não está no manifesto não entra nunca.
#ifndef NL_FOUNDATION_PKCS12_CARGA_RAIZES_ICP_H
#define NL_FOUNDATION_PKCS12_CARGA_RAIZES_ICP_H

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/erro_raiz_icp.h>

#include <QString>

#include <chrono>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace nl::foundation::pkcs12 {

/// Motivo do descarte de uma raiz que passou na conferência de hash.
enum class MotivoDescarte {
    Vencida,              ///< notAfter anterior ao instante pedido.
    AindaNaoValida,       ///< notBefore posterior ao instante pedido.
    PropositoDiferente,   ///< O metadado do manifesto declara outra finalidade.
};

struct DescarteRaiz {
    std::string arquivo;  ///< Nome do arquivo no manifesto, sem caminho.
    MotivoDescarte motivo;
};

/// Erro da carga com o arquivo que o causou (nome, nunca conteúdo). Vazio quando o erro é do manifesto.
struct ErroCarga {
    ErroRaizIcp motivo;
    std::string arquivo;
};

/// Âncoras aceitas e a lista do que foi descartado e por quê, para o log do Mid.
struct CargaRaizesIcp {
    Ancoras ancoras;
    std::vector<DescarteRaiz> descartes;
};

/// Confere e carrega as raízes listadas em `manifesto` (caminho do SHA256SUMS), lidas de `diretorio`.
/// `proposito` é a finalidade que o chamador aceita; `agora` é o instante usado na validade.
[[nodiscard]] std::expected<CargaRaizesIcp, ErroCarga> carregarRaizesIcp(const QString& diretorio, const QString& manifesto,
                                                                         std::string_view proposito,
                                                                         std::chrono::system_clock::time_point agora);

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_CARGA_RAIZES_ICP_H
