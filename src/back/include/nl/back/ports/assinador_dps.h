// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Port AssinadorDps (NL-2.9, DA-2.42): assina a DPS pelo Id da referência. Só a interface. O adaptador do Mid
// (sefin/assinatura) implementa com o xmldsig e os parâmetros da pasta do leiaute; o EmitirNota depende só deste port.
#ifndef NL_BACK_PORTS_ASSINADOR_DPS_H
#define NL_BACK_PORTS_ASSINADOR_DPS_H

#include <expected>

#include <QByteArray>
#include <QByteArrayView>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

class AssinadorDps
{
public:
    virtual ~AssinadorDps();

    /// Devolve o XML com a Signature enveloped no elemento do Id idReferencia. Falha com código estável em nl::base::Erro.
    virtual std::expected<QByteArray, nl::base::Erro> assinar(QByteArrayView xml, QStringView idReferencia) = 0;
};

} // namespace nl::back

#endif // NL_BACK_PORTS_ASSINADOR_DPS_H
