#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Resumo do job banco no $GITHUB_STEP_SUMMARY (NL-3.1v, seção 1.7 do plano do CTO), lido do log do harness da distro.
# Só copia quatro formas de linha, e cada campo só com caracteres de uma lista fechada (o log do CI é público):
#   NL-BANCO par=...  (uma linha da tabela por par)   NL-BANCO-RESUMO ...   funcoes_no_ctest: ...   NL-RC=N
# O piso de pares e o número de slots por par vêm do ci.yml (matrix.include do job banco), fora de matriz_banco.txt e do
# CMake: um diff que tire um par da matriz, ou um teste e a linha dele no CMake, muda a contagem do harness, não o piso.
# Reprova (rc 1) se o log falta; se matriz_banco.txt não tem PISO pares da distro; se falta a linha NL-BANCO-RESUMO; se
# pares_esperados, executados ou verdes não são PISO; ou se n_por_par não é SLOTS. O job não fica verde com menos.
# Uso: resumo.sh LOG DISTRO PISO SLOTS
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

# nl_resumo_inteiro VALOR: rc 0 só para inteiro positivo sem zero à esquerda, de até 4 dígitos.
nl_resumo_inteiro() {
    [[ "${1:-}" =~ ^[1-9][0-9]{0,3}$ ]]
}

# nl_resumo_linha_resumo LOG: imprime a última linha NL-BANCO-RESUMO do LOG; sem ela, imprime a REPROVA e sai 1.
nl_resumo_linha_resumo() {
    local linha
    linha="$(grep '^NL-BANCO-RESUMO ' "$1" | tail -n 1)"
    if [ -z "$linha" ]; then
        echo "REPROVA: linha NL-BANCO-RESUMO ausente"
        return 1
    fi
    printf '%s\n' "$linha"
}

# nl_resumo_confere_matriz MATRIZ PISO: rc 0 só se matriz_banco.txt tem PISO pares da distro (a lista não andou sem o piso).
nl_resumo_confere_matriz() {
    if [ "$1" != "$2" ]; then
        echo "REPROVA: matriz_banco.txt tem $1 pares da distro, o piso do ci.yml é $2"
        return 1
    fi
}

# nl_resumo_confere_pares LINHA PISO: rc 0 só com pares_esperados, executados e verdes iguais a PISO.
nl_resumo_confere_pares() {
    local esperados executados verdes
    esperados="$(nl_resumo_campo "$1" pares_esperados)"
    executados="$(nl_resumo_campo "$1" executados)"
    verdes="$(nl_resumo_campo "$1" verdes)"
    if [ "$esperados" != "$2" ] || [ "$executados" != "$2" ] || [ "$verdes" != "$2" ]; then
        echo "REPROVA: pares_esperados=$esperados executados=$executados verdes=$verdes, piso do ci.yml $2"
        return 1
    fi
}

# nl_resumo_confere_slots LINHA SLOTS: rc 0 só com n_por_par igual a SLOTS (cada par rodou os slots do ci.yml).
nl_resumo_confere_slots() {
    local n
    n="$(nl_resumo_campo "$1" n_por_par)"
    if [ "$n" != "$2" ]; then
        echo "REPROVA: n_por_par=$n, slots por par no ci.yml $2"
        return 1
    fi
}

# nl_resumo_veredito LOG MATRIZ PISO SLOTS: as quatro conferências, na ordem; imprime a primeira REPROVA e sai 1.
nl_resumo_veredito() {
    local linha
    nl_resumo_confere_matriz "$2" "$3" || return 1
    linha="$(nl_resumo_linha_resumo "$1")" || { printf '%s\n' "$linha"; return 1; }
    nl_resumo_confere_pares "$linha" "$3" || return 1
    nl_resumo_confere_slots "$linha" "$4"
}

LOG="${1:-}"
DISTRO="${2:-}"
PISO="${3:-}"
SLOTS="${4:-}"
[ -n "$LOG" ] && [ -n "$DISTRO" ] || { echo "PARADA: uso: resumo.sh LOG DISTRO PISO SLOTS"; exit 2; }
nl_resumo_inteiro "$PISO" && nl_resumo_inteiro "$SLOTS" || { echo "PARADA: PISO e SLOTS têm de ser inteiros positivos (recebi '$PISO' e '$SLOTS')"; exit 2; }
PARES="$(nl_banco_pares_de "$DISTRO" | grep -c . || true)"
if [ ! -f "$LOG" ]; then
    echo "REPROVA: log do harness ausente ($LOG)" | tee -a "$SAIDA"
    exit 1
fi
{
    echo "## Banco: $(nl_resumo_limpo "$DISTRO") (piso $PISO pares e $SLOTS slots por par no ci.yml; $PARES pares em matriz_banco.txt)"
    nl_resumo_tabela "$LOG"
    echo
    echo '```'
    nl_resumo_linhas "$LOG"
    echo '```'
} >> "$SAIDA"
veredito="$(nl_resumo_veredito "$LOG" "$PARES" "$PISO" "$SLOTS")"
rc=$?
if [ "$rc" -ne 0 ]; then
    printf '%s\n' "$veredito" | tee -a "$SAIDA"
    exit 1
fi
echo "PASSA: resumo do banco com $PISO pares e $SLOTS slots por par ($DISTRO)"
