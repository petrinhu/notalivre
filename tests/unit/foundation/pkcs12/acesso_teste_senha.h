// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Acesso de teste ao SenhaSegura (seam do M7, rodada 3, decisão do CISO item 1). Mora em tests/, fora de include/ e
// fora do que se instala. O cabeçalho público só declara `friend struct AcessoDeTesteSenhaSegura;`.
#ifndef NL_TESTE_ACESSO_TESTE_SENHA_H
#define NL_TESTE_ACESSO_TESTE_SENHA_H

#include <nl/base/erro.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArrayView>

#include <cstddef>
#include <expected>

namespace nl::foundation::pkcs12 {

struct AcessoDeTesteSenhaSegura {
    /// Gancho chamado na liberação: depois da zeragem, antes do munlock e do delete[].
    using Gancho = void (*)(const char*, std::size_t) noexcept;
    /// Alocador de teste: a falha de alocação entra por ponteiro de função passado na chamada, nunca por global.
    using Alocador = void* (*)(std::size_t) noexcept;
    /// Prontidão de teste: a falha de sodium_init entra por ponteiro de função passado na chamada.
    using Prontidao = bool (*)() noexcept;

    /// Senha com o gancho instalado na construção. O gancho não muda depois.
    [[nodiscard]] static SenhaSegura comGancho(QByteArrayView bytes, Gancho gancho) {
        return SenhaSegura::criar(bytes, gancho);
    }

    /// Caminho de tentar() com alocador e prontidão falsos. Os dois ficam fixos na chamada.
    [[nodiscard]] static std::expected<SenhaSegura, nl::base::Erro> tentarComFalhas(QByteArrayView bytes,
                                                                                    Alocador alocar,
                                                                                    Prontidao pronta) {
        return SenhaSegura::criarCom(bytes, nullptr, alocar, pronta);
    }

    /// Caminho de de() com alocador e prontidão falsos: a falha tem de sair como std::bad_alloc.
    [[nodiscard]] static SenhaSegura deComFalhas(QByteArrayView bytes, Alocador alocar, Prontidao pronta) {
        return SenhaSegura::criarOuLanca(bytes, nullptr, alocar, pronta);
    }
};

} // namespace nl::foundation::pkcs12

#endif // NL_TESTE_ACESSO_TESTE_SENHA_H
