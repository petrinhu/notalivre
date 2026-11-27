#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Resumo do job banco no $GITHUB_STEP_SUMMARY (NL-3.1v, seção 1.7 do plano do CTO), lido do log do harness da distro.
# Só copia quatro formas de linha, e cada campo só com caracteres de uma lista fechada (o log do CI é público):
#   NL-BANCO par=...  (uma linha da tabela por par)   NL-BANCO-RESUMO ...   funcoes_no_ctest: ...   NL-RC=N
# Reprova (rc 1) se o log falta, se falta a linha NL-BANCO-RESUMO, ou se pares_esperados ou executados não são o número de
# pares da distro em matriz_banco.txt: o job não fica verde sem ter rodado os pares.
# Uso: resumo.sh LOG DISTRO
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
NL_BANCO_DIR="$RAIZ/tools/conteiner"
. "$NL_BANCO_DIR/banco/listas.sh"
SAIDA="${GITHUB_STEP_SUMMARY:-/dev/stdout}"

# nl_resumo_limpo TEXTO: imprime TEXTO só com [A-Za-z0-9._/+:=-] e espaço; o resto vira '?'.
nl_resumo_limpo() {
    printf '%s\n' "$1" | sed 's/[^A-Za-z0-9._/+:= -]/?/g'
}

# nl_resumo_campo LINHA CHAVE: imprime o valor de CHAVE=valor na LINHA (campos separados por espaço); vazio se não há.
nl_resumo_campo() {
    printf '%s\n' "$1" | awk -v k="$2" '{ for (i = 1; i <= NF; i++) if (index($i, k "=") == 1) { print substr($i, length(k) + 2); exit } }'
}

# nl_resumo_tabela LOG: imprime a tabela de pares, uma linha por "NL-BANCO par=".
nl_resumo_tabela() {
    local linha c
    echo "| par | versão | método | encontrados | analisados | pulados | falharam | rc |"
    echo "| --- | --- | --- | --- | --- | --- | --- | --- |"
    while IFS= read -r linha; do
        linha="$(nl_resumo_limpo "$linha")"
        printf '|'
        for c in par versao metodo encontrados analisados pulados falharam rc; do
            printf ' %s |' "$(nl_resumo_campo "$linha" "$c")"
        done
        printf '\n'
    done < <(grep '^NL-BANCO par=' "$1")
}

# nl_resumo_linhas LOG: imprime as linhas NL-BANCO-RESUMO, funcoes_no_ctest e NL-RC do LOG, limpas.
nl_resumo_linhas() {
    local linha
    while IFS= read -r linha; do
        nl_resumo_limpo "$linha"
    done < <(grep -E '^(NL-BANCO-RESUMO |funcoes_no_ctest: |NL-RC=[0-9]+$)' "$1")
}

# nl_resumo_confere LOG ESPERADOS: rc 0 só com a linha NL-BANCO-RESUMO e pares_esperados e executados iguais a ESPERADOS.
nl_resumo_confere() {
    local linha esperados executados
    linha="$(grep '^NL-BANCO-RESUMO ' "$1" | tail -n 1)"
    if [ -z "$linha" ]; then
        echo "REPROVA: linha NL-BANCO-RESUMO ausente"
        return 1
    fi
    esperados="$(nl_resumo_campo "$linha" pares_esperados)"
    executados="$(nl_resumo_campo "$linha" executados)"
    if [ "$esperados" != "$2" ] || [ "$executados" != "$2" ]; then
        echo "REPROVA: pares_esperados=$esperados executados=$executados, matriz_banco.txt tem $2 pares"
        return 1
    fi
}

LOG="${1:-}"
DISTRO="${2:-}"
[ -n "$LOG" ] && [ -n "$DISTRO" ] || { echo "PARADA: uso: resumo.sh LOG DISTRO"; exit 2; }
PARES="$(nl_banco_pares_de "$DISTRO" | grep -c . || true)"
if [ ! -f "$LOG" ]; then
    echo "REPROVA: log do harness ausente ($LOG)" | tee -a "$SAIDA"
    exit 1
fi
{
    echo "## Banco: $(nl_resumo_limpo "$DISTRO") ($PARES pares em matriz_banco.txt)"
    nl_resumo_tabela "$LOG"
    echo
    echo '```'
    nl_resumo_linhas "$LOG"
    echo '```'
} >> "$SAIDA"
veredito="$(nl_resumo_confere "$LOG" "$PARES")"
rc=$?
if [ "$rc" -ne 0 ]; then
    printf '%s\n' "$veredito" | tee -a "$SAIDA"
    exit 1
fi
echo "PASSA: resumo do banco com $PARES pares ($DISTRO)"
