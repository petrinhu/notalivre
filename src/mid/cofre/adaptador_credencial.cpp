// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// NL-2.2, fase vermelha (DA-2.30): o adaptador ainda recusa tudo, sem chamar o carregador nem o registro.
#include <nl/mid/cofre/adaptador_credencial.h>

#include <utility>

namespace nl::mid::cofre {

AdaptadorCredencial::AdaptadorCredencial(nl::foundation::log::Log& registro) noexcept
    : registro_(registro) {}

std::expected<nl::foundation::pkcs12::CredencialX509, nl::foundation::pkcs12::ErroCredencial>
AdaptadorCredencial::carregar(QByteArrayView, nl::foundation::pkcs12::SenhaSegura&,
                              const nl::foundation::pkcs12::Ancoras&) {
    return std::unexpected(nl::foundation::pkcs12::ErroCredencial::FalhaInterna);
}

} // namespace nl::mid::cofre
