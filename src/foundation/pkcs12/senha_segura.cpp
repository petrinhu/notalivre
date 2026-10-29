// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Senha em buffer dedicado (NL-2.2, ADR-S1 item 4). Zeragem por OPENSSL_cleanse antes de cada liberação;
// mlock de melhor esforço, em sistemas POSIX (o limite de memória travada pode negar, e a senha segue protegida só pela zeragem).
#include <nl/foundation/pkcs12/senha_segura.h>

#include <openssl/crypto.h>

#include <cstring>
#include <utility>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#define NL_TEM_MLOCK 1
#endif

namespace nl::foundation::pkcs12 {

SenhaSegura SenhaSegura::de(QByteArrayView bytes) {
    const auto tamanho = static_cast<std::size_t>(bytes.size());
    const std::size_t capacidade = tamanho + 1; // NUL final: a API do PKCS#12 lê C-string
    char* dados = new char[capacidade]{};
    if (tamanho > 0) {
        std::memcpy(dados, bytes.constData(), tamanho);
    }
    bool bloqueada = false;
#ifdef NL_TEM_MLOCK
    bloqueada = mlock(dados, capacidade) == 0;
#endif
    return SenhaSegura(dados, capacidade, tamanho, bloqueada);
}

// Seam de teste (revisão 3, M7): a senha fica com o zerador injetado, que zerar() passa a usar.
SenhaSegura SenhaSegura::comZeradorDeTeste(QByteArrayView bytes, ZeradorDeMemoria zerador) {
    SenhaSegura senha = de(bytes);
    senha.zerador_ = std::move(zerador);
    return senha;
}

SenhaSegura::SenhaSegura(char* dados, std::size_t capacidade, std::size_t tamanho, bool bloqueada) noexcept
    : dados_(dados), capacidade_(capacidade), tamanho_(tamanho), bloqueada_(bloqueada) {}

SenhaSegura::SenhaSegura(SenhaSegura&& outra) noexcept
    : dados_(std::exchange(outra.dados_, nullptr)),
      zerador_(std::move(outra.zerador_)),
      capacidade_(std::exchange(outra.capacidade_, 0)),
      tamanho_(std::exchange(outra.tamanho_, 0)),
      bloqueada_(std::exchange(outra.bloqueada_, false)) {}

SenhaSegura& SenhaSegura::operator=(SenhaSegura&& outra) noexcept {
    if (this != &outra) {
        liberar();
        dados_ = std::exchange(outra.dados_, nullptr);
        zerador_ = std::move(outra.zerador_);
        capacidade_ = std::exchange(outra.capacidade_, 0);
        tamanho_ = std::exchange(outra.tamanho_, 0);
        bloqueada_ = std::exchange(outra.bloqueada_, false);
    }
    return *this;
}

SenhaSegura::~SenhaSegura() {
    liberar();
}

void SenhaSegura::zerar() noexcept {
    if (dados_ != nullptr) {
        if (zerador_) {
            zerador_(dados_, capacidade_);
        } else {
            OPENSSL_cleanse(dados_, capacidade_);
        }
    }
    tamanho_ = 0;
}

void SenhaSegura::liberar() noexcept {
    if (dados_ != nullptr) {
        // Uma única zeragem: liberar() passa por zerar(), então não há caminho de liberação sem o OPENSSL_cleanse.
        zerar();
#ifdef NL_TEM_MLOCK
        if (bloqueada_) {
            munlock(dados_, capacidade_);
        }
#endif
        delete[] dados_;
    }
    dados_ = nullptr;
    capacidade_ = 0;
    tamanho_ = 0;
    bloqueada_ = false;
}

} // namespace nl::foundation::pkcs12
