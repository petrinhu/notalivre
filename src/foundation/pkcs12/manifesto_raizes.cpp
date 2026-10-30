// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do SHA256SUMS das raízes (NL-2.13). STUB do passo vermelho do TDD: reprova tudo até a implementação.
#include <nl/foundation/pkcs12/manifesto_raizes.h>

namespace nl::foundation::pkcs12 {

std::expected<std::vector<EntradaRaiz>, ErroRaizIcp> lerManifesto(QByteArrayView) {
    return std::unexpected(ErroRaizIcp::FalhaInterna);
}

} // namespace nl::foundation::pkcs12
