// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras de produção do A1 (NL-2.13, DA-2.44, DA-2.48). Escolhe a finalidade "geral" e delega a conferência à Foundation:
// o motor pkcs12 não sabe o que é ICP, e este adaptador é que decide qual conjunto vale para a custódia do A1.
// As raízes servem só de âncora da cadeia do A1 do próprio usuário (K2 do CLO, 30/10/2026).
#include <nl/mid/cofre/raizes_producao.h>

#include <nl/foundation/pkcs12/carga_raizes_icp.h>

#include <string_view>

namespace nl::mid::cofre {

namespace {

// Finalidade do conjunto de produção: só as raízes que o manifesto declara sem finalidade diferente (CISO, 30/10/2026).
constexpr std::string_view kProposito = "geral";

} // namespace

std::expected<nl::foundation::pkcs12::CargaRaizesIcp, nl::foundation::pkcs12::ErroCarga>
carregarRaizesDeProducao(const QString& diretorio, std::chrono::system_clock::time_point agora) {
    return nl::foundation::pkcs12::carregarRaizesIcp(diretorio, diretorio + QStringLiteral("/SHA256SUMS"), kProposito, agora);
}

} // namespace nl::mid::cofre
