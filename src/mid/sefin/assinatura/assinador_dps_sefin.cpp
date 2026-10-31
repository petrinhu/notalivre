// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Adaptador AssinadorDpsSefin (NL-2.9, DA-2.42). Ver assinador_dps_sefin.h.
#include <nl/mid/sefin/assinatura/assinador_dps_sefin.h>

#include <nl/foundation/xmldsig/assinador_xml.h>
#include <nl/mid/cofre/material_assinatura.h>
#include <nl/mid/sefin/leiaute/prodrest_20261020/parametros_infdps.h>

#include <optional>
#include <string>
#include <utility>

namespace nl::mid::sefin::assinatura {

AssinadorDpsSefin::AssinadorDpsSefin(const nl::foundation::pkcs12::CredencialX509& credencial) noexcept
    : credencial_(credencial) {}

// Os parâmetros vêm da pasta do leiaute (parametrosDaAssinaturaDps), sem cópia aqui (DA-2.42, L-34). O material é emprestado
// do cofre na chamada, e o xmldsig não guarda nada entre chamadas.
std::expected<QByteArray, nl::base::Erro> AssinadorDpsSefin::assinar(QByteArrayView xml, QStringView idReferencia) {
    auto material = nl::mid::cofre::materialParaAssinatura(credencial_);
    if (!material) {
        return std::unexpected(nl::base::Erro("cofre.material_invalido", std::nullopt,
                                              "a credencial está descartada: o material de assinatura não está disponível"));
    }
    auto assinado = nl::foundation::xmldsig::AssinadorXml::assinarEnveloped(
        xml, idReferencia, nl::mid::sefin::prodrest_20261020::parametrosDaAssinaturaDps(), *material);
    if (!assinado) {
        const auto codigo = nl::foundation::xmldsig::codigo(assinado.error());
        return std::unexpected(nl::base::Erro(std::string(codigo), std::nullopt, "falha ao assinar a DPS"));
    }
    return std::move(*assinado);
}

} // namespace nl::mid::sefin::assinatura
