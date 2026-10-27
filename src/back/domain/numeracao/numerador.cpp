// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do Numerador (NL-1.3). Invariante: reservar usa o maior número conhecido pelo port mais um.
// Transições conferidas contra o estado lido no port antes de gravar. Erros do port propagam com o código original.

#include <nl/back/numeracao/numerador.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace nl::back {
namespace {

constexpr std::int64_t kMaiorNumero = 999999999999999;

nl::base::Erro comContexto(const nl::base::Erro& causa, const std::string& contexto)
{
    return nl::base::Erro{causa.codigo(), causa.campo(), contexto + ": " + causa.mensagem()};
}

nl::base::Erro estadoInvalido(const std::string& mensagem)
{
    return nl::base::Erro{"numeracao.estado_invalido", std::nullopt, mensagem};
}

nl::base::Erro serieDivergente()
{
    return nl::base::Erro{"numeracao.serie_divergente", std::nullopt, "reserva pertence a outra série"};
}

} // namespace

Numerador::Numerador(Serie serie, RepositorioNumeracao& repositorio)
    : serie_{std::move(serie)}
    , repositorio_{repositorio}
{
}

std::expected<Reserva, nl::base::Erro> Numerador::reservar()
{
    const std::string contexto = "não foi possível reservar o número da DPS";
    const auto ultimo = repositorio_.ultimoReservado(serie_);
    if (!ultimo) {
        return std::unexpected(comContexto(ultimo.error(), contexto));
    }

    std::int64_t proximo = 1;
    if (ultimo->has_value()) {
        const std::int64_t maior = ultimo->value().valor();
        if (maior >= kMaiorNumero) {
            return std::unexpected(nl::base::Erro{"numeracao.esgotada", std::nullopt, "faixa de números da série esgotada"});
        }
        proximo = maior + 1;
    }

    const auto numero = NumeroDps::de(proximo);
    if (!numero) {
        return std::unexpected(comContexto(numero.error(), contexto));
    }
    Reserva reserva{serie_, *numero};
    const auto gravada = repositorio_.gravarReserva(reserva);
    if (!gravada) {
        return std::unexpected(comContexto(gravada.error(), contexto));
    }
    return reserva;
}

std::expected<void, nl::base::Erro> Numerador::confirmar(const Reserva& reserva, const ChaveAcesso& chave)
{
    if (!(reserva.serie() == serie_)) {
        return std::unexpected(serieDivergente());
    }
    const std::string contexto = "não foi possível confirmar a reserva";
    const auto estado = repositorio_.estadoDe(reserva);
    if (!estado) {
        return std::unexpected(comContexto(estado.error(), contexto));
    }
    if (!estado->has_value() || estado->value() != EstadoNumero::Reservado) {
        return std::unexpected(estadoInvalido("só uma reserva em estado Reservado pode ser confirmada"));
    }
    const auto gravada = repositorio_.gravarConfirmacao(reserva, chave);
    if (!gravada) {
        return std::unexpected(comContexto(gravada.error(), contexto));
    }
    return {};
}

std::expected<void, nl::base::Erro> Numerador::pular(const Reserva& reserva, const MotivoPulo& motivo)
{
    if (!(reserva.serie() == serie_)) {
        return std::unexpected(serieDivergente());
    }
    const std::string contexto = "não foi possível pular a reserva";
    const auto estado = repositorio_.estadoDe(reserva);
    if (!estado) {
        return std::unexpected(comContexto(estado.error(), contexto));
    }
    if (!estado->has_value() || estado->value() != EstadoNumero::Reservado) {
        return std::unexpected(estadoInvalido("só uma reserva em estado Reservado pode ser pulada"));
    }
    const auto gravada = repositorio_.gravarPulo(reserva, motivo);
    if (!gravada) {
        return std::unexpected(comContexto(gravada.error(), contexto));
    }
    return {};
}

} // namespace nl::back
