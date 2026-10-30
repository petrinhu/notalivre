// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Raiz de composição da emissão (NL-2.9, DA-2.42): liga o caso de uso EmitirNota ao adaptador real de assinatura. É o único
// lugar que conhece o adaptador; o caso de uso só vê o port AssinadorDps. A CLI usa só esta fachada.
#ifndef NL_MID_COMPOSICAO_EMISSAO_NOTA_H
#define NL_MID_COMPOSICAO_EMISSAO_NOTA_H

#include <nl/back/app/emitir_nota.h>
#include <nl/back/ports/assinador_dps.h>
#include <nl/foundation/pkcs12/credencial_x509.h>

#include <memory>

namespace nl::mid::composicao {

class EmissaoNota
{
public:
    /// Empresta a credencial: quem chama a mantém viva enquanto a emissão existir.
    explicit EmissaoNota(const nl::foundation::pkcs12::CredencialX509& credencial);
    EmissaoNota(const EmissaoNota&) = delete;
    EmissaoNota& operator=(const EmissaoNota&) = delete;
    ~EmissaoNota();

    nl::back::EmitirNota& emitirNota() noexcept;

private:
    std::unique_ptr<nl::back::AssinadorDps> assinador_;
    nl::back::EmitirNota emitir_;
};

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_EMISSAO_NOTA_H
