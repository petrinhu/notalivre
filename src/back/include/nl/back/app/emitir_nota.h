// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de uso EmitirNota (NL-2.9, DA-2.42): recebe os ports por interface. Em teste, o port é um fake em memória; o
// adaptador real (AssinadorDpsSefin) é ligado pela raiz de composição, depois da pausa.
#ifndef NL_BACK_APP_EMITIR_NOTA_H
#define NL_BACK_APP_EMITIR_NOTA_H

#include <expected>

#include <QByteArray>
#include <QByteArrayView>
#include <QStringView>

#include <nl/back/ports/assinador_dps.h>
#include <nl/base/erro.h>

namespace nl::back {

class EmitirNota
{
public:
    explicit EmitirNota(AssinadorDps& assinador) noexcept;

    /// Assina a DPS pelo Id dela. Falha com o código do port, sem traduzir.
    std::expected<QByteArray, nl::base::Erro> executar(QByteArrayView dpsXml, QStringView idDps);

private:
    AssinadorDps& assinador_;
};

} // namespace nl::back

#endif // NL_BACK_APP_EMITIR_NOTA_H
