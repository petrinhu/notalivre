#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Resumo de revisão (NL-2.14, adendo 5 da DA-2.61, item 2): imprime o diff das quatro listas de teste e dos dois arquivos de
# contagem contra o merge-base com a onda-2 (NL_BASE_REF, padrão onda-2), no commit NL_SHA (padrão HEAD). No CI, grava o
# mesmo texto no resumo do job (GITHUB_STEP_SUMMARY). Sem repositório git, diz que o diff está indisponível e por quê.
# Uso: resumo_listas.sh <raiz>
RAIZ="${1:-.}"
REF="${NL_BASE_REF:-onda-2}"
SHA="${NL_SHA:-HEAD}"
ARQS="tests/pulos_esperados_por_distro.txt tests/testes_sem_saida_por_funcao.txt tests/interpretadores_permitidos.txt tests/testes_obrigatorios_por_distro.txt tests/funcoes_esperadas_por_distro.txt flatpak/piso_suite.txt"
emitir() {
    echo "$*"
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        echo "$*" >> "$GITHUB_STEP_SUMMARY"
    fi
}
emitir "## Diff das listas de teste e das contagens (contra o merge-base com $REF)"
if ! git -C "$RAIZ" rev-parse --git-dir >/dev/null 2>&1; then
    emitir "diff indisponível: sem repositório git em $RAIZ (contêiner sem GIT_DIR montado). Conferir no diff do PR."
    exit 0
fi
base="$(git -C "$RAIZ" merge-base "$SHA" "$REF" 2>/dev/null)" || base=""
if [ -z "$base" ]; then
    emitir "merge-base de $SHA com $REF indisponível: diff não impresso. Conferir no diff do PR."
    exit 0
fi
emitir "merge-base: $base"
emitir "arquivos: $ARQS"
# shellcheck disable=SC2086
d="$(git -C "$RAIZ" diff "$base" "$SHA" -- $ARQS 2>&1)"
if [ -z "$d" ]; then
    emitir "sem alteração nas listas e contagens desde o merge-base."
else
    emitir "$d"
fi
exit 0
