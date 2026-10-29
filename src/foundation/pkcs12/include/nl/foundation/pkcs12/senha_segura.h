// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Senha em buffer dedicado (NL-2.2, ADR-S1 item 4): mlock de melhor esforço, zeragem por OPENSSL_cleanse e
// liberação só depois da zeragem. Não é libsodium: a dependência nova exige autorização do líder (L-51).
#ifndef NL_FOUNDATION_PKCS12_SENHA_SEGURA_H
#define NL_FOUNDATION_PKCS12_SENHA_SEGURA_H

#include <QByteArrayView>

#include <cstddef>
#include <functional>

namespace nl::foundation::pkcs12 {

/// Zerador de memória da senha. Vazio: OPENSSL_cleanse (produção).
using ZeradorDeMemoria = std::function<void(char*, std::size_t)>;

/// Move-only: uma senha tem um dono, e não existe cópia que escape da zeragem.
class SenhaSegura {
public:
    /// Copia os bytes para o buffer seguro. A origem é do chamador, que responde por ela.
    [[nodiscard]] static SenhaSegura de(QByteArrayView bytes);

    /// Só para teste (seam do revisor, M7): igual a de(), mas a zeragem passa pelo zerador dado. Assim o teste vê
    /// a zeragem acontecer antes da liberação, sem ler memória liberada. Produção usa de().
    [[nodiscard]] static SenhaSegura comZeradorDeTeste(QByteArrayView bytes, ZeradorDeMemoria zerador);

    SenhaSegura(SenhaSegura&& outra) noexcept;
    SenhaSegura& operator=(SenhaSegura&& outra) noexcept;
    SenhaSegura(const SenhaSegura&) = delete;
    SenhaSegura& operator=(const SenhaSegura&) = delete;
    ~SenhaSegura();

    /// Zera o conteúdo agora. O buffer continua alocado até a destruição, então comoC() segue válido.
    void zerar() noexcept;

    /// Ponteiro NUL-terminado para a API do OpenSSL. Válido até zerar() ou a destruição.
    [[nodiscard]] const char* comoC() const noexcept { return dados_; }

    /// Bytes úteis, sem o NUL. Zero depois de zerar().
    [[nodiscard]] std::size_t tamanho() const noexcept { return tamanho_; }

    /// Verdadeiro se o mlock deu certo (melhor esforço: limite de memória travada pode negar).
    [[nodiscard]] bool bloqueadaEmMemoria() const noexcept { return bloqueada_; }

private:
    SenhaSegura(char* dados, std::size_t capacidade, std::size_t tamanho, bool bloqueada) noexcept;
    void liberar() noexcept;

    char* dados_{nullptr};
    ZeradorDeMemoria zerador_{};
    std::size_t capacidade_{0};
    std::size_t tamanho_{0};
    bool bloqueada_{false};
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_SENHA_SEGURA_H
