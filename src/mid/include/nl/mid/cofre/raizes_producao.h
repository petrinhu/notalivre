// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras de produção do A1 (NL-2.13, DA-2.44, DA-2.48). Mora no cofre, junto do adaptador do A1: é política de confiança
// da custódia (ADR-0002, ADR-S1), não da composição. Cabeçalho público do Mid: tipos da Foundation só declarados.
#ifndef NL_MID_COFRE_RAIZES_PRODUCAO_H
#define NL_MID_COFRE_RAIZES_PRODUCAO_H

#include <QString>

#include <chrono>
#include <expected>

namespace nl::foundation::pkcs12 {
struct CargaRaizesIcp;
struct ErroCarga;
}

namespace nl::mid::cofre {

/// Conjunto de produção, fixado no binário (CISO, 30/10/2026, DA-2.48 revista): o SHA256SUMS de `diretorio` tem de ter o
/// SHA-256 embutido no build; divergindo, recusa com ManifestoNaoFixado antes de ler qualquer raiz. Sem a configuração
/// trocar o conjunto: a chave da DA-2.44 não vale para as âncoras do A1 em produção.
[[nodiscard]] std::expected<nl::foundation::pkcs12::CargaRaizesIcp, nl::foundation::pkcs12::ErroCarga>
carregarRaizesDeProducao(const QString& diretorio, std::chrono::system_clock::time_point agora);

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_RAIZES_PRODUCAO_H
