// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Senha em buffer dedicado (NL-2.2, ADR-S1 item 4). Ver senha_segura.h.
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
    return criar(bytes, nullptr);
}

SenhaSegura SenhaSegura::criar(QByteArrayView bytes, GanchoLiberacao gancho) {
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
    return SenhaSegura(dados, capacidade, tamanho, bloqueada, gancho);
}

SenhaSegura::SenhaSegura(char* dados, std::size_t capacidade, std::size_t tamanho, bool bloqueada,
                         GanchoLiberacao gancho) noexcept
    : dados_(dados), capacidade_(capacidade), tamanho_(tamanho), bloqueada_(bloqueada), gancho_(gancho) {}

SenhaSegura::SenhaSegura(SenhaSegura&& outra) noexcept
    : dados_(std::exchange(outra.dados_, nullptr)),
      capacidade_(std::exchange(outra.capacidade_, 0)),
      tamanho_(std::exchange(outra.tamanho_, 0)),
      bloqueada_(std::exchange(outra.bloqueada_, false)),
      gancho_(std::exchange(outra.gancho_, nullptr)) {}

SenhaSegura& SenhaSegura::operator=(SenhaSegura&& outra) noexcept {
    if (this != &outra) {
        liberar();
        dados_ = std::exchange(outra.dados_, nullptr);
        capacidade_ = std::exchange(outra.capacidade_, 0);
        tamanho_ = std::exchange(outra.tamanho_, 0);
        bloqueada_ = std::exchange(outra.bloqueada_, false);
        gancho_ = std::exchange(outra.gancho_, nullptr);
    }
    return *this;
}

SenhaSegura::~SenhaSegura() {
    liberar();
}

void SenhaSegura::zerar() noexcept {
    if (dados_ != nullptr) {
        OPENSSL_cleanse(dados_, capacidade_);
    }
    tamanho_ = 0;
}

void SenhaSegura::liberar() noexcept {
    if (dados_ != nullptr) {
        // Ordem: zerar (OPENSSL_cleanse), depois o gancho de observação, depois munlock e delete[].
        zerar();
        if (gancho_ != nullptr) {
            gancho_(dados_, capacidade_);
        }
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
    gancho_ = nullptr;
}

} // namespace nl::foundation::pkcs12
