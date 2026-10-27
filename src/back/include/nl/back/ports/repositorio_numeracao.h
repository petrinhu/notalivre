// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Port RepositorioNumeracao (NL-1.3, P3): só a interface. O adaptador de banco é NL-3.5; o protocolo durável
// de reservar, assinar, enviar e registrar fecha em NL-3.7.
//
// Contrato para quem implementa o adaptador:
//  - gravarReserva é inserção que FALHA se (série, número) já existe, com "numeracao.conflito". Essa é a barreira
//    entre processos: duas leituras iguais de ultimoReservado não podem produzir duas reservas do mesmo número.
//  - As transições (gravarConfirmacao, gravarPulo) só são chamadas pelo Numerador depois de conferir o estado.
//  - Erros de infraestrutura voltam como nl::base::Erro com código estável; o adaptador não engole nem traduz para genérico.

#ifndef NL_BACK_PORTS_REPOSITORIO_NUMERACAO_H
#define NL_BACK_PORTS_REPOSITORIO_NUMERACAO_H

#include <expected>
#include <optional>

#include <nl/base/erro.h>
#include <nl/back/dps/chave_acesso.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/back/numeracao/estado_numero.h>
#include <nl/back/numeracao/motivo_pulo.h>
#include <nl/back/numeracao/reserva.h>

namespace nl::back {

class RepositorioNumeracao
{
public:
    virtual ~RepositorioNumeracao();

    /// Maior número já reservado nesta série, em qualquer estado. Vazio se a série ainda não tem reserva.
    virtual std::expected<std::optional<NumeroDps>, nl::base::Erro> ultimoReservado(const Serie& serie) = 0;

    /// Estado de uma reserva. Vazio se o (série, número) nunca foi reservado.
    virtual std::expected<std::optional<EstadoNumero>, nl::base::Erro> estadoDe(const Reserva& reserva) = 0;

    /// Grava a reserva em estado Reservado. Falha com "numeracao.conflito" se (série, número) já existe.
    virtual std::expected<void, nl::base::Erro> gravarReserva(const Reserva& reserva) = 0;

    /// Grava a confirmação com a chave de acesso.
    virtual std::expected<void, nl::base::Erro> gravarConfirmacao(const Reserva& reserva, const ChaveAcesso& chave) = 0;

    /// Grava o pulo com o motivo.
    virtual std::expected<void, nl::base::Erro> gravarPulo(const Reserva& reserva, const MotivoPulo& motivo) = 0;
};

} // namespace nl::back

#endif // NL_BACK_PORTS_REPOSITORIO_NUMERACAO_H
