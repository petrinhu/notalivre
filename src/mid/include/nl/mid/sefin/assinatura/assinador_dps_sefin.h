// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Adaptador AssinadorDpsSefin (NL-2.9, DA-2.42): implementa o port AssinadorDps com o assinador XMLDSIG e os parâmetros da
// pasta do leiaute prodrest_20261020 (parametrosDaAssinaturaDps, sem segunda cópia). O material vem do cofre (DA-2.36).
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

/// Algoritmo da assinatura da DPS (NL-2.10a, [assinatura] algoritmo). RsaSha1 é o padrão (DA-2.2); RsaSha256 é a alternativa de SHA-256.
enum class AlgoritmoAssinatura { RsaSha1, RsaSha256 };

class AssinadorDpsSefin final : public nl::back::AssinadorDps
{
public:
    /// Empresta a credencial: quem chama a mantém viva enquanto o adaptador existir. O algoritmo escolhe a função de parâmetros
    /// da pasta do leiaute (parametros_infdps.h); o padrão é rsa-sha1, como antes desta fatia.
    explicit AssinadorDpsSefin(const nl::foundation::pkcs12::CredencialX509& credencial,
                               AlgoritmoAssinatura algoritmo = AlgoritmoAssinatura::RsaSha1) noexcept;

    std::expected<QByteArray, nl::base::Erro> assinar(QByteArrayView xml, QStringView idReferencia) override;

private:
    const nl::foundation::pkcs12::CredencialX509& credencial_;
    AlgoritmoAssinatura algoritmo_;
};

} // namespace nl::mid::sefin::assinatura

#endif // NL_MID_SEFIN_ASSINATURA_ASSINADOR_DPS_SEFIN_H
