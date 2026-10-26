// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Numerador da DPS (NL-1.3): reserva, confirma e pula números de uma série. Domínio puro: a persistência entra pelo port.
//
// Invariante nomeado: reservar devolve sempre número maior que qualquer número reservado, confirmado ou pulado
// conhecido pelo port. O Numerador não trava nada; a exclusão mútua entre processos é do adaptador (NL-3.5/3.7),
// que recusa a gravação de (série, número) já existente com numeracao.conflito.
//
// Erros: "numeracao.esgotada" (faixa de quinze posições acabou), "numeracao.estado_invalido" (transição não
// permitida para o estado atual), "numeracao.serie_divergente" (reserva de outra série). Erros do repositório
// propagam com o código original e mensagem com o contexto da operação.

#ifndef NL_BACK_NUMERACAO_NUMERADOR_H
#define NL_BACK_NUMERACAO_NUMERADOR_H

#include <expected>

#include <nl/base/erro.h>
#include <nl/back/dps/chave_acesso.h>
#include <nl/back/dps/serie.h>
#include <nl/back/numeracao/motivo_pulo.h>
#include <nl/back/numeracao/reserva.h>
#include <nl/back/ports/repositorio_numeracao.h>

namespace nl::back {

/// Numerador de uma série. Série e repositório entram por construtor; não há estado global.
class Numerador
{
public:
    Numerador(Serie serie, RepositorioNumeracao& repositorio);

    /// Reserva o próximo número livre e grava a reserva antes de devolvê-la.
    std::expected<Reserva, nl::base::Erro> reservar();

    /// Confirma uma reserva em estado Reservado, gravando a chave de acesso.
    std::expected<void, nl::base::Erro> confirmar(const Reserva& reserva, const ChaveAcesso& chave);

    /// Pula uma reserva em estado Reservado. O motivo é obrigatório pelo tipo.
    std::expected<void, nl::base::Erro> pular(const Reserva& reserva, const MotivoPulo& motivo);

private:
    Serie serie_;
    RepositorioNumeracao& repositorio_;
};

} // namespace nl::back

#endif // NL_BACK_NUMERACAO_NUMERADOR_H
