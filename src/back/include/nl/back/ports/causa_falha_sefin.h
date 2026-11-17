// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Causa de uma falha da Sefin (NL-3.C; requisito 1 do CISO, N2): o que se sabe da causa, em tipo do domínio, sem Qt (L-33).
// O Mid preenche; a fachada escolhe a classe e o texto do catálogo por motivo e causa. Nenhum fluxo decide por texto.
#ifndef NL_BACK_PORTS_CAUSA_FALHA_SEFIN_H
#define NL_BACK_PORTS_CAUSA_FALHA_SEFIN_H

namespace nl::back {

enum class CausaFalhaSefin {
    Nenhuma,                      ///< Sem causa a dizer: falha com código E, ou resultado já decidido pela consulta.
    CertificadoDoEmissorProvavel, ///< Conexão segura recusada sem erro na verificação do servidor: provável certificado do emissor.
    ServidorNaoConfirmado,        ///< Conexão segura recusada com erro na verificação do servidor: o certificado do emissor não é a causa.
    Rede,                         ///< Conexão, tempo limite ou resposta de rede que o transporte não lê.
    ConfiguracaoDaConexao,        ///< Configuração do transporte inválida: nada sai da máquina.
    PreparacaoDoEnvio,            ///< Compressão ou envelope falharam: nada saiu da máquina.
    RespostaSemCodigoValido,      ///< A Sefin respondeu sem código E válido, ou com corpo que o cliente não lê.
    SefinIndisponivel,            ///< 5xx até a última tentativa da consulta.
    NaoGerada,                    ///< A consulta respondeu E2404 e não há tentativa restante.
};

} // namespace nl::back

#endif // NL_BACK_PORTS_CAUSA_FALHA_SEFIN_H
