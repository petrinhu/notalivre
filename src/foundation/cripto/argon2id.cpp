// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Argon2id (NL-3.2b, ADR-0006) por crypto_pwhash do libsodium, com ALG_ARGON2ID13 e lanes 1 (desvio de risco registrado).
// Piso e teto são conferidos ANTES de qualquer derivação (ADR-0006, item 5). O libsodium é privado a este .cpp.

#include <nl/foundation/cripto/argon2id.h>
#include <nl/foundation/cripto/tamanhos.h>

#include "nl/foundation/cripto/erros.h"

#include <sodium.h>

#include <cstddef>
#include <mutex>

namespace nl::foundation::cripto {

namespace {

// sodium_init por std::call_once (NL-3.2b, ciso). Falha de inicialização vale como falha da biblioteca.
bool bibliotecaPronta()
{
    static std::once_flag umaVez;
    static bool pronta = false;
    std::call_once(umaVez, [] { pronta = sodium_init() >= 0; });
    return pronta;
}

} // namespace

std::expected<void, nl::base::Erro> argon2id(QByteArrayView entrada, const ParametrosArgon2id& parametros,
                                             std::span<unsigned char> saida)
{
    if (parametros.salt.size() != kTamanhoSalt) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "o salt do Argon2id precisa ter 16 bytes"));
    }
    if (saida.size() < crypto_pwhash_BYTES_MIN || saida.size() > crypto_pwhash_BYTES_MAX) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "saída do Argon2id fora do que a biblioteca aceita"));
    }
    if (parametros.memlimit > kMemlimitTeto || parametros.opslimit > kOpslimitTeto) {
        return std::unexpected(detalhe::erroCripto("cripto.parametro_acima_do_teto",
                                                   "parâmetro do Argon2id acima do teto de leitura: registro recusado sem derivar"));
    }
    if (parametros.memlimit < kMemlimitPiso || parametros.opslimit < kOpslimitPiso) {
        return std::unexpected(detalhe::erroCripto("cripto.parametro_abaixo_do_piso",
                                                   "parâmetro do Argon2id abaixo do piso: registro recusado sem derivar"));
    }
    if (!bibliotecaPronta()) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o libsodium não iniciou"));
    }

    static const unsigned char vazio = 0;
    const auto* senha = entrada.isEmpty() ? reinterpret_cast<const char*>(&vazio)
                                          : entrada.data();
    const int rc = crypto_pwhash(saida.data(), saida.size(), senha, static_cast<unsigned long long>(entrada.size()),
                                 reinterpret_cast<const unsigned char*>(parametros.salt.constData()),
                                 static_cast<unsigned long long>(parametros.opslimit),
                                 static_cast<std::size_t>(parametros.memlimit), crypto_pwhash_ALG_ARGON2ID13);
    if (rc != 0) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o libsodium não derivou a chave (memória?)"));
    }
    return {};
}

} // namespace nl::foundation::cripto
