#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Trava de trabalho pesado no CI (NL-3.1v, seção 1.5 do plano do CTO). Mesmo contrato do pesado local (~/.claude/bin/pesado):
# "--teto N -- comando...", com N em minutos. O job banco exporta NL_PESADO apontando para cá, e o build_local.sh e o
# integracao_banco.sh rodam no runner sem mudar uma linha deles; fora do CI o padrão deles segue o pesado local, teto 15.
# Só num runner hospedado do GitHub Actions: fora dele recusa com rc 2. Trava por flock num arquivo de NL_TMP_BASE com o uid no nome, porque o
# usuário de uid 1000 que roda o build não escreve em RUNNER_TEMP, e o /var/tmp com sticky bit não deixa um usuário abrir
# com O_CREAT o arquivo de outro. nice -n 5 (NF-07). Teto por timeout; devolve o rc do comando (124 quando bate no teto).
# Uso: pesado_ci.sh --teto N -- comando [argumentos...]
set -u

# nl_pesado_ci_no_ci: rc 0 só num runner hospedado do GitHub Actions (GITHUB_ACTIONS=true, RUNNER_ENVIRONMENT=github-hosted
# e RUNNER_TEMP como pasta existente).
nl_pesado_ci_no_ci() {
    [ "${GITHUB_ACTIONS:-}" = true ] && [ "${RUNNER_ENVIRONMENT:-}" = github-hosted ] \
        && [ -n "${RUNNER_TEMP:-}" ] && [ -d "$RUNNER_TEMP" ]
}

# nl_pesado_ci_teto_valido N: rc 0 só para inteiro de 1 a 999 minutos, sem zero à esquerda.
nl_pesado_ci_teto_valido() {
    [[ "${1:-}" =~ ^[1-9][0-9]{0,2}$ ]]
}

# nl_pesado_ci_trava: imprime o caminho do arquivo da trava deste usuário.
nl_pesado_ci_trava() {
    printf '%s/notalivre-pesado-ci-%s.lock\n' "${NL_TMP_BASE:-/var/tmp}" "$(id -u)"
}

if ! nl_pesado_ci_no_ci; then
    echo "PARADA: pesado_ci só no CI"
    exit 2
fi
if [ "${1:-}" != --teto ] || ! nl_pesado_ci_teto_valido "${2:-}" || [ "${3:-}" != -- ] || [ "$#" -lt 4 ]; then
    echo "PARADA: uso: pesado_ci.sh --teto N -- comando [argumentos...] (N de 1 a 999 minutos)"
    exit 2
fi
teto="$2"
shift 3
echo "NL-PESADO-CI teto=$teto"
exec flock "$(nl_pesado_ci_trava)" nice -n 5 timeout --kill-after=30s "${teto}m" "$@"
