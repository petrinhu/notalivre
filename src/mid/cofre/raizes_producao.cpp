// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras de produção do A1 (NL-2.13, DA-2.48). STUB do passo vermelho do TDD: reprova até a implementação.
#include <nl/mid/cofre/raizes_producao.h>

#include <nl/foundation/pkcs12/carga_raizes_icp.h>

namespace nl::mid::cofre {

std::expected<nl::foundation::pkcs12::CargaRaizesIcp, nl::foundation::pkcs12::ErroCarga>
carregarRaizesDeProducao(const QString&, const QString&, std::chrono::system_clock::time_point) {
    return std::unexpected(nl::foundation::pkcs12::ErroCarga{nl::foundation::pkcs12::ErroRaizIcp::FalhaInterna, {}});
}

} // namespace nl::mid::cofre
