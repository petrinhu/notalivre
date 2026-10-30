// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Port GatewaySefin (NL-2.8, L-33): o que o caso de uso enxerga da Sefin. A implementação é o ClienteSefin do Mid, com a
// política de retry. Este cabeçalho não conhece HTTP, JSON nem TLS: só o que o domínio precisa saber.
// Falha vem como FalhaSefin: o motivo decide o caminho, e o texto é do catálogo, nunca o texto livre da API (ADR-0005 c).
#ifndef NL_BACK_PORTS_GATEWAY_SEFIN_H
#define NL_BACK_PORTS_GATEWAY_SEFIN_H

#include <nl/back/dps/chave_acesso.h>
#include <nl/back/dps/dps_assinada.h>
#include <nl/back/dps/id_dps.h>

#include <QByteArray>

#include <expected>
#include <string>

namespace nl::back {

enum class MotivoFalhaSefin {
    Transitoria,   ///< Falha de rede ou 5xx que não se resolveu depois da consulta. Vale tentar de novo, pelo caso de uso.
    DpsNaoGerada,  ///< GET /dps/{id} respondeu 404 com E2404 (INFERÊNCIA até a NL-2.10): a nota não existe.
    DpsDuplicada,  ///< 409: a DPS já existe na Sefin. Consultar, nunca reenviar.
    Rejeitada,     ///< Código E de regra de negócio (ou de certificado). Nunca repete.
    Definitiva,    ///< Resposta que o cliente não consegue ler, ou configuração inválida.
    Indeterminado, ///< Os bytes do POST podem ter chegado e a consulta GET /dps não disse o que aconteceu. Nunca vira
                   ///< Definitiva nem Rejeitada: o chamador não sabe se a nota existe e não pode reenviar (DA-2.41).
};

struct FalhaSefin {
    MotivoFalhaSefin motivo;
    std::string codigoE;   ///< Código E da Sefin, quando houver (ex.: "E1207"). Vazio nas falhas de rede.
    std::string mensagem;  ///< Texto do catálogo do Mid para o código E. Nunca o texto livre da API.
};

struct NotaSefin {
    ChaveAcesso chave;
    QByteArray xmlNfse;               ///< XML da NFS-e já descompactado (DA-2.39). Vazio quando xmlPendente.
    bool xmlPendente = false;         ///< Sucesso sem XML legível: a chave vale, o reenvio fica bloqueado e o XML vem por GET /nfse/{chave}.
};

class GatewaySefin {
public:
    virtual ~GatewaySefin() = default;

    /// Envia a DPS assinada. Repete só o que a política do Mid permite (nunca reenvia depois de um aceite).
    [[nodiscard]] virtual std::expected<NotaSefin, FalhaSefin> emitir(const DpsAssinada& dps) = 0;

    /// Consulta a NFS-e pela chave de acesso.
    [[nodiscard]] virtual std::expected<NotaSefin, FalhaSefin> consultarNota(const ChaveAcesso& chave) = 0;

    /// Consulta se a DPS gerou nota e devolve a chave de acesso dela.
    [[nodiscard]] virtual std::expected<ChaveAcesso, FalhaSefin> consultarDps(const IdDps& id) = 0;
};

} // namespace nl::back

#endif // NL_BACK_PORTS_GATEWAY_SEFIN_H
