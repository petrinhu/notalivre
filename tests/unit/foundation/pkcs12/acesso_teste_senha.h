// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Acesso de teste ao SenhaSegura (seam do M7, rodada 3, decisão do CISO item 1). Mora em tests/, fora de include/ e
// fora do que se instala. O cabeçalho público só declara `friend struct AcessoDeTesteSenhaSegura;`.
#ifndef NL_TESTE_ACESSO_TESTE_SENHA_H
#define NL_TESTE_ACESSO_TESTE_SENHA_H

#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArrayView>

#include <cstddef>

namespace nl::foundation::pkcs12 {

struct AcessoDeTesteSenhaSegura {
    /// Gancho chamado na liberação: depois da zeragem, antes do munlock e do delete[].
    using Gancho = void (*)(const char*, std::size_t) noexcept;

    /// Senha com o gancho instalado na construção. O gancho não muda depois.
    [[nodiscard]] static SenhaSegura comGancho(QByteArrayView bytes, Gancho gancho) {
        return SenhaSegura::criar(bytes, gancho);
    }
};

} // namespace nl::foundation::pkcs12

#endif // NL_TESTE_ACESSO_TESTE_SENHA_H
