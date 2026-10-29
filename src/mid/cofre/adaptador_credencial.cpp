// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Adaptador de cofre do A1 (NL-2.2, DA-2.30). Carrega pela Foundation; se a credencial não tem EKU, registra o evento
// e recusa a credencial quando o registro falha (a credencial é descartada, a chave sai da memória).
#include <nl/mid/cofre/adaptador_credencial.h>

#include <utility>

namespace nl::mid::cofre {

namespace {

constexpr const char* kEventoEkuAusente = "credencial.eku_ausente";

} // namespace

AdaptadorCredencial::AdaptadorCredencial(nl::foundation::log::Log& registro) noexcept
    : registro_(registro) {}

std::expected<nl::foundation::pkcs12::CredencialX509, nl::foundation::pkcs12::ErroCredencial>
AdaptadorCredencial::carregar(QByteArrayView pfx, nl::foundation::pkcs12::SenhaSegura& senha,
                              const nl::foundation::pkcs12::Ancoras& ancoras) {
    auto credencial = carregador_.carregar(pfx, senha, ancoras);
    if (!credencial) {
        return std::unexpected(credencial.error());
    }
    if (credencial->ekuAusente()) {
        auto gravado = registro_.registrar(nl::foundation::log::Nivel::Aviso, kEventoEkuAusente, {});
        if (!gravado) {
            // Falha fechada (decisão 3 do CISO): sem o registro exigido, a credencial sem EKU não é aceita.
            credencial->descartar();
            return std::unexpected(nl::foundation::pkcs12::ErroCredencial::FalhaInterna);
        }
    }
    return std::move(*credencial);
}

} // namespace nl::mid::cofre
