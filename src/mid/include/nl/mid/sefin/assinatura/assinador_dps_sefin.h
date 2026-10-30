// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Adaptador AssinadorDpsSefin (NL-2.9, DA-2.42): implementa o port AssinadorDps com o assinador XMLDSIG e os parâmetros da
// pasta do leiaute prodrest_20261020 (parametrosDoInfDps, sem segunda cópia). O material vem do cofre (DA-2.36).
#ifndef NL_MID_SEFIN_ASSINATURA_ASSINADOR_DPS_SEFIN_H
#define NL_MID_SEFIN_ASSINATURA_ASSINADOR_DPS_SEFIN_H

#include <expected>

#include <QByteArray>
#include <QByteArrayView>
#include <QStringView>

#include <nl/back/ports/assinador_dps.h>
#include <nl/base/erro.h>
#include <nl/foundation/pkcs12/credencial_x509.h>

namespace nl::mid::sefin::assinatura {

class AssinadorDpsSefin final : public nl::back::AssinadorDps
{
public:
    /// Empresta a credencial: quem chama a mantém viva enquanto o adaptador existir.
    explicit AssinadorDpsSefin(const nl::foundation::pkcs12::CredencialX509& credencial) noexcept;

    std::expected<QByteArray, nl::base::Erro> assinar(QByteArrayView xml, QStringView idReferencia) override;

private:
    const nl::foundation::pkcs12::CredencialX509& credencial_;
};

} // namespace nl::mid::sefin::assinatura

#endif // NL_MID_SEFIN_ASSINATURA_ASSINADOR_DPS_SEFIN_H
