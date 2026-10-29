// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// NL-2.2, fase vermelha: de() e zerar() ainda não cumprem o contrato (testes da senha falham até a fase verde).
#include <nl/foundation/pkcs12/senha_segura.h>

#include <utility>

namespace nl::foundation::pkcs12 {

SenhaSegura SenhaSegura::de(QByteArrayView) {
    return SenhaSegura(nullptr, 0, 0, false);
}

SenhaSegura::SenhaSegura(char* dados, std::size_t capacidade, std::size_t tamanho, bool bloqueada) noexcept
    : dados_(dados), capacidade_(capacidade), tamanho_(tamanho), bloqueada_(bloqueada) {}

SenhaSegura::SenhaSegura(SenhaSegura&& outra) noexcept
    : dados_(std::exchange(outra.dados_, nullptr)),
      capacidade_(std::exchange(outra.capacidade_, 0)),
      tamanho_(std::exchange(outra.tamanho_, 0)),
      bloqueada_(std::exchange(outra.bloqueada_, false)) {}

SenhaSegura& SenhaSegura::operator=(SenhaSegura&& outra) noexcept {
    if (this != &outra) {
        liberar();
        dados_ = std::exchange(outra.dados_, nullptr);
        capacidade_ = std::exchange(outra.capacidade_, 0);
        tamanho_ = std::exchange(outra.tamanho_, 0);
        bloqueada_ = std::exchange(outra.bloqueada_, false);
    }
    return *this;
}

SenhaSegura::~SenhaSegura() {
    liberar();
}

void SenhaSegura::zerar() noexcept {}

void SenhaSegura::liberar() noexcept {
    delete[] dados_;
    dados_ = nullptr;
    capacidade_ = 0;
    tamanho_ = 0;
    bloqueada_ = false;
}

} // namespace nl::foundation::pkcs12
