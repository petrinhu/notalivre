// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Linhas de diagnóstico da validação na CLI (NL-3.M): "erro <código> (<campo>): <mensagem>" e "aviso <código> (<campo>): <mensagem>".
// Usadas pelo `dps validar` e pelo `emitir`; só formatam e escrevem no stderr, sem decidir código de saída.
#ifndef NL_CLI_SAIDA_VALIDACAO_H
#define NL_CLI_SAIDA_VALIDACAO_H

#include <nl/base/erro.h>
#include <nl/mid/composicao/dps_validar.h>

#include <QString>

#include <vector>

namespace nl::cli {

/// Uma linha de erro: "erro <código> (<campo>): <mensagem>". Sem campo, sai "erro <código>: <mensagem>".
QString linhaDeErro(const nl::base::Erro& erro);

/// Uma linha de aviso: "aviso <código> (<campo>): <mensagem>". Aviso não muda o código de saída (DA-3M.3).
QString linhaDeAviso(const nl::mid::composicao::AvisoDps& aviso);

/// Escreve no stderr uma linha por erro, na ordem recebida.
void mostrarErros(const std::vector<nl::base::Erro>& erros);

/// Escreve no stderr uma linha por aviso, na ordem recebida.
void mostrarAvisos(const std::vector<nl::mid::composicao::AvisoDps>& avisos);

} // namespace nl::cli

#endif // NL_CLI_SAIDA_VALIDACAO_H
