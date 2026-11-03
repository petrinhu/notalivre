// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Adaptador de cofre do A1 (NL-2.2, DA-2.30 do CTO): chama o carregador puro da Foundation e registra o evento de
// EKU ausente pelo Log da NL-2.7. Sem registro, a credencial sem EKU é recusada (falha fechada, decisão 3 do CISO).
// Cabeçalho público do Mid: declarações antecipadas dos tipos da Foundation (sem includes da Foundation).
#ifndef NL_MID_COFRE_ADAPTADOR_CREDENCIAL_H
#define NL_MID_COFRE_ADAPTADOR_CREDENCIAL_H

#include <QByteArrayView>

#include <expected>

namespace nl::foundation::log {
class Log;
}

namespace nl::foundation::pkcs12 {
class Ancoras;
class CredencialX509;
class SenhaSegura;
enum class ErroCredencial : int;
}

namespace nl::mid::cofre {

class AdaptadorCredencial {
public:
    /// registro: destino do evento credencial.eku_ausente (Log da NL-2.7, com a política de PII dela).
    explicit AdaptadorCredencial(nl::foundation::log::Log& registro) noexcept;

    [[nodiscard]] std::expected<nl::foundation::pkcs12::CredencialX509, nl::foundation::pkcs12::ErroCredencial>
    carregar(QByteArrayView pfx, nl::foundation::pkcs12::SenhaSegura& senha,
             const nl::foundation::pkcs12::Ancoras& ancoras);

private:
    nl::foundation::log::Log& registro_;
};

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_ADAPTADOR_CREDENCIAL_H
