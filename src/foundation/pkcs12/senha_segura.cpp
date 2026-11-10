// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Senha em buffer dedicado (NL-2.2, ADR-S1 item 4; NL-3.2c). Ver senha_segura.h.
#include <nl/foundation/pkcs12/senha_segura.h>

#include <sodium.h>

#include <cstring>
#include <mutex>
#include <new>
#include <optional>
#include <utility>

namespace nl::foundation::pkcs12 {

namespace {

// Alocador de produção: a região do libsodium (página de guarda, canário e bloqueio de memória por conta dele).
void* alocarSeguro(std::size_t tamanho) noexcept
{
    return sodium_malloc(tamanho);
}

// sodium_init por std::call_once (mesma forma de cripto/argon2id.cpp, sem incluir cripto: DA-2.30). Falha de
// inicialização vale como falha de alocação: a senha não cai em memória comum se a biblioteca não subiu.
bool bibliotecaPronta() noexcept
{
    static std::once_flag umaVez;
    static bool pronta = false;
    std::call_once(umaVez, [] { pronta = sodium_init() >= 0; });
    return pronta;
}

nl::base::Erro erroSemMemoriaSegura()
{
    return nl::base::Erro{"senha.sem_memoria_segura", std::nullopt,
                          "Não foi possível reservar memória protegida para a senha."};
}

} // namespace

SenhaSegura SenhaSegura::de(QByteArrayView bytes)
{
    return criar(bytes, nullptr);
}

std::expected<SenhaSegura, nl::base::Erro> SenhaSegura::tentar(QByteArrayView bytes)
{
    return criarCom(bytes, nullptr, &alocarSeguro, &bibliotecaPronta);
}

SenhaSegura SenhaSegura::criar(QByteArrayView bytes, GanchoLiberacao gancho)
{
    return criarOuLanca(bytes, gancho, &alocarSeguro, &bibliotecaPronta);
}

SenhaSegura SenhaSegura::criarOuLanca(QByteArrayView bytes, GanchoLiberacao gancho, AlocadorSeguro alocar,
                                      ProntidaoBiblioteca pronta)
{
    auto senha = criarCom(bytes, gancho, alocar, pronta);
    if (!senha) {
        throw std::bad_alloc{};
    }
    return std::move(*senha);
}

std::expected<SenhaSegura, nl::base::Erro> SenhaSegura::criarCom(QByteArrayView bytes, GanchoLiberacao gancho,
                                                                 AlocadorSeguro alocar, ProntidaoBiblioteca pronta)
{
    // Invariante: nenhum sodium_malloc antes de sodium_init com sucesso. A ordem é a do contrato, não a de conveniência.
    if (!pronta()) {
        return std::unexpected(erroSemMemoriaSegura());
    }
    const auto tamanho = static_cast<std::size_t>(bytes.size());
    const std::size_t capacidade = tamanho + 1; // NUL final: a API do PKCS#12 lê C-string
    auto* dados = static_cast<char*>(alocar(capacidade));
    if (dados == nullptr) {
        return std::unexpected(erroSemMemoriaSegura());
    }
    // sodium_malloc preenche a região com 0xdb: zera tudo antes de copiar, para o NUL final ser de fato NUL.
    std::memset(dados, 0, capacidade);
    if (tamanho > 0) {
        std::memcpy(dados, bytes.constData(), tamanho);
    }
    // sodium_malloc já tenta o bloqueio; o sodium_mlock aqui torna o resultado observável em bloqueadaEmMemoria().
    const bool bloqueada = sodium_mlock(dados, capacidade) == 0;
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

SenhaSegura& SenhaSegura::operator=(SenhaSegura&& outra) noexcept
{
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

SenhaSegura::~SenhaSegura()
{
    liberar();
}

void SenhaSegura::zerar() noexcept
{
    if (dados_ != nullptr) {
        sodium_memzero(dados_, capacidade_);
    }
    tamanho_ = 0;
}

void SenhaSegura::liberar() noexcept
{
    if (dados_ != nullptr) {
        // Ordem: zerar (sodium_memzero), depois o gancho de observação, depois sodium_free (que desbloqueia e libera).
        zerar();
        if (gancho_ != nullptr) {
            gancho_(dados_, capacidade_);
        }
        sodium_free(dados_);
    }
    dados_ = nullptr;
    capacidade_ = 0;
    tamanho_ = 0;
    bloqueada_ = false;
    gancho_ = nullptr;
}

} // namespace nl::foundation::pkcs12
