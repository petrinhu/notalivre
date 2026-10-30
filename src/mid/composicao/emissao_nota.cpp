// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Raiz de composição da emissão (NL-2.9, DA-2.42). Ver emissao_nota.h.
#include <nl/mid/composicao/emissao_nota.h>

#include <nl/mid/sefin/assinatura/assinador_dps_sefin.h>

#include <memory>

namespace nl::mid::composicao {

// A raiz é o único lugar que escolhe o adaptador real. O caso de uso recebe só o port.
EmissaoNota::EmissaoNota(const nl::foundation::pkcs12::CredencialX509& credencial)
    : assinador_(std::make_unique<nl::mid::sefin::assinatura::AssinadorDpsSefin>(credencial)), emitir_(*assinador_)
{
}

EmissaoNota::~EmissaoNota() = default;

nl::back::EmitirNota& EmissaoNota::emitirNota() noexcept
{
    return emitir_;
}

} // namespace nl::mid::composicao
