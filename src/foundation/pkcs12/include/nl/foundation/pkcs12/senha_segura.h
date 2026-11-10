// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Senha em buffer dedicado (NL-2.2, ADR-S1 item 4; NL-3.2c, ADR-0002): a região vem do sodium_malloc (página de
// guarda, canário e bloqueio de memória por conta do libsodium), zerada por sodium_memzero antes da liberação.
// A biblioteca é privada ao .cpp (L-51, autorizada pelo líder em 29/10/2026): o cabeçalho não traz nenhum tipo dela.
#ifndef NL_FOUNDATION_PKCS12_SENHA_SEGURA_H
#define NL_FOUNDATION_PKCS12_SENHA_SEGURA_H

#include <nl/base/erro.h>

#include <QByteArrayView>

#include <cstddef>
#include <expected>

namespace nl::foundation::pkcs12 {

/// Move-only: uma senha tem um dono, e não existe cópia que escape da zeragem.
class SenhaSegura {
public:
    /// Copia os bytes para o buffer seguro. A origem é do chamador, que responde por ela.
    /// Lança std::bad_alloc se não houver memória segura para a senha.
    [[nodiscard]] static SenhaSegura de(QByteArrayView bytes);

    /// Como de(), mas devolve erro em vez de lançar. Chamador novo de segredo usa este (NL-3.3).
    [[nodiscard]] static std::expected<SenhaSegura, nl::base::Erro> tentar(QByteArrayView bytes);

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

    // Acesso só de teste (o seam do M7): a struct mora em tests/, fora de include/ e fora do que se instala.
    friend struct AcessoDeTesteSenhaSegura;

private:
    using GanchoLiberacao = void (*)(const char*, std::size_t) noexcept;
    /// Alocador da região segura. Em produção é o sodium_malloc; o teste passa um que devolve nulo.
    using AlocadorSeguro = void* (*)(std::size_t) noexcept;
    /// Prontidão da biblioteca. Em produção é o sodium_init por std::call_once; o teste passa uma que falha.
    using ProntidaoBiblioteca = bool (*)() noexcept;

    /// Cria a senha com o gancho e os recursos fixados na chamada (nenhuma variável global). Devolve erro se a
    /// biblioteca não estiver pronta ou se o alocador devolver nulo.
    [[nodiscard]] static std::expected<SenhaSegura, nl::base::Erro> criarCom(QByteArrayView bytes,
                                                                             GanchoLiberacao gancho,
                                                                             AlocadorSeguro alocar,
                                                                             ProntidaoBiblioteca pronta);
    /// Como criarCom(), mas lança std::bad_alloc na falha. Usado por de() e pelo acesso de teste.
    [[nodiscard]] static SenhaSegura criarOuLanca(QByteArrayView bytes, GanchoLiberacao gancho,
                                                  AlocadorSeguro alocar, ProntidaoBiblioteca pronta);
    /// Cria a senha com o gancho já instalado. O gancho não muda depois (só o movimento o transfere).
    [[nodiscard]] static SenhaSegura criar(QByteArrayView bytes, GanchoLiberacao gancho);
    SenhaSegura(char* dados, std::size_t capacidade, std::size_t tamanho, bool bloqueada,
                GanchoLiberacao gancho) noexcept;
    void liberar() noexcept;

    char* dados_{nullptr};
    std::size_t capacidade_{0};
    std::size_t tamanho_{0};
    bool bloqueada_{false};
    GanchoLiberacao gancho_{nullptr};
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_SENHA_SEGURA_H
