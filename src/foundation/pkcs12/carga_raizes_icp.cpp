// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Carga das raízes da ICP-Brasil como âncoras (NL-2.13). STUB do passo vermelho do TDD: reprova tudo até a implementação.
#include <nl/foundation/pkcs12/carga_raizes_icp.h>

namespace nl::foundation::pkcs12 {

std::expected<CargaRaizesIcp, ErroCarga> carregarRaizesIcp(const QString&, const QString&, std::string_view,
                                                           std::chrono::system_clock::time_point) {
    return std::unexpected(ErroCarga{ErroRaizIcp::FalhaInterna, {}});
}

} // namespace nl::foundation::pkcs12
