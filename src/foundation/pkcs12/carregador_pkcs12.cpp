// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// NL-2.2, fase vermelha (revisão 2): carregar() recusa todo arquivo sem tocar na senha. Cada teste de
// carregamento reprova pela própria expectativa (sucesso ou o código que ele testa).
#include <nl/foundation/pkcs12/carregador_pkcs12.h>

#include <utility>

namespace nl::foundation::pkcs12 {

CarregadorPkcs12::CarregadorPkcs12(nl::foundation::log::Log& registro) noexcept
    : registro_(registro) {}

std::expected<CredencialX509, ErroCredencial> CarregadorPkcs12::carregar(QByteArrayView, SenhaSegura&,
                                                                         const Ancoras&) {
    return std::unexpected(ErroCredencial::FalhaInterna);
}

} // namespace nl::foundation::pkcs12
