// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Códigos estáveis da falha de envio (NL-3.C; S5, DA-3C.21): o contrato de identificadores que o script, o log e o código de
// saída usam, e que a NL-3.8 congela. Átomo próprio (L-34): muda com o contrato, nunca com o texto aprovado do catálogo.
#ifndef NL_MID_COMPOSICAO_CODIGOS_FALHA_ENVIO_H
#define NL_MID_COMPOSICAO_CODIGOS_FALHA_ENVIO_H

#include <string_view>

namespace nl::mid::composicao {

// Códigos estáveis: o script, o log e o código de saída usam estas constantes; nenhuma string repetida.
inline constexpr std::string_view kCodigoCertificadoProvavel = "envio.certificado_provavel";
inline constexpr std::string_view kCodigoServidorNaoConfirmado = "envio.servidor_nao_confirmado";
inline constexpr std::string_view kCodigoRedeAntesDoEnvio = "envio.rede_antes_do_envio";
inline constexpr std::string_view kCodigoEstadoIncerto = "envio.estado_incerto";
inline constexpr std::string_view kCodigoEstadoIncertoCertificadoProvavel = "envio.estado_incerto_certificado_provavel";
inline constexpr std::string_view kCodigoNaoConcluido = "envio.nao_concluido";
inline constexpr std::string_view kCodigoFalhaAntesDoEnvio = "envio.falha_antes_do_envio";
inline constexpr std::string_view kCodigoRespostaSemCodigo = "envio.resposta_sem_codigo";
inline constexpr std::string_view kCodigoConfiguracaoDaConexao = "envio.configuracao_da_conexao";
inline constexpr std::string_view kCodigoComparacaoIndisponivel = "sefin.comparacao_indisponivel";

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_CODIGOS_FALHA_ENVIO_H
