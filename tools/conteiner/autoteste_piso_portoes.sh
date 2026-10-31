#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste de estreia do piso dos portões estáticos (NL-2.11, achado A1 da revisão QA; L-36, L-27).
# O piso é o número em tools/conteiner/piso_portoes.txt. Cada sabotagem roda numa CÓPIA temporária de
# portoes_estaticos.sh e do piso, e tem de dar PARADA pelo motivo esperado. O docker é um STUB que sempre sai 0,
# então a única coisa que pode reprovar é o piso.
# Uso: autoteste_piso_portoes.sh     (cwd qualquer; usa a árvore onde o script está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ORIGEM="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-piso-portoes.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin" "$BASE/fixture"
printf '#!/bin/sh\nexit 0\n' > "$BASE/bin/docker"
chmod +x "$BASE/bin/docker"
export PATH="$BASE/bin:$PATH"

# Repositório de brincadeira com um commit (o portão clona um SHA dele).
git -C "$BASE/fixture" init -q
printf 'x\n' > "$BASE/fixture/arquivo.txt"
git -C "$BASE/fixture" add -A
git -C "$BASE/fixture" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: fixture" || { echo "ABORTA: commit da fixture"; exit 1; }
SHA="$(git -C "$BASE/fixture" rev-parse HEAD)" || { echo "ABORTA: rev-parse"; exit 1; }

CASOS=0
FALHAS=0

# caso NOME ORIGEM_DO_SCRIPT PISO SEDSCRIPT RC_ESPERADO TEXTO_ESPERADO
# ORIGEM_DO_SCRIPT: "atual" usa tools/conteiner do repositório; qualquer outro valor é caminho de um script alternativo.
# SEDSCRIPT vazio = sem sabotagem da lista.
caso() {
    local nome="$1" script="$2" piso="$3" sabotagem="$4" rc_esperado="$5" texto="$6"
    local dir="$BASE/c-$nome" saida rc
    CASOS=$((CASOS + 1))
    mkdir -p "$dir/tools/conteiner"
    if [ "$script" = atual ]; then
        cp "$ORIGEM/portoes_estaticos.sh" "$dir/tools/conteiner/portoes_estaticos.sh"
    else
        cp "$script" "$dir/tools/conteiner/portoes_estaticos.sh"
    fi
    printf '%s\n' "$piso" > "$dir/tools/conteiner/piso_portoes.txt"
    if [ -n "$sabotagem" ]; then
        sed -i "$sabotagem" "$dir/tools/conteiner/portoes_estaticos.sh"
    fi
    saida="$(timeout 300 bash "$dir/tools/conteiner/portoes_estaticos.sh" "$BASE/fixture" "$SHA" "$BASE/d-$nome" 2>&1)"
    rc=$?
    echo "--- caso $nome: rc=$rc (esperado $rc_esperado)"
    echo "$saida" | tail -n 2
    if [ "$rc" = "$rc_esperado" ] && printf '%s\n' "$saida" | grep -q -F -- "$texto"; then
        echo "[VERDE] $nome"
    else
        echo "[VERMELHO] $nome: rc=$rc, esperava rc=$rc_esperado com '$texto'"
        FALHAS=$((FALHAS + 1))
    fi
}

# Controle positivo: a lista e o piso batem, o laço roda 11 portões e nenhum reprova.
caso verde atual 11 "" 0 "portoes=11 falharam=0 (piso 11)"
# Sabotagem 1: um portão sai da lista (o laço roda 10). Sem piso, passaria verde.
caso lista_sem_um atual 11 '/segredos_historico|/d' 1 "a lista de PORTOES tem 10 portões, o piso é 11"
# Sabotagem 2: lista esvaziada (zero portões, o falso verde de origem).
caso lista_vazia atual 11 '/^PORTOES=(/,/^)/{/^PORTOES=(/!{/^)/!d}}' 1 "a lista de PORTOES tem 0 portões, o piso é 11"
# Sabotagem 3: piso 0 no arquivo versionado.
caso piso_zero atual 0 "" 2 "piso 0 recusado"
# Sabotagem 4: piso maior que a lista (lista de 11, piso 12).
caso piso_maior atual 12 "" 1 "a lista de PORTOES tem 11 portões, o piso é 12"
# Sabotagem 5: piso ilegível.
caso piso_ilegivel atual "1x" "" 2 "piso ilegível"

echo "casos=$CASOS falhas=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE PISO PORTOES: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE PISO PORTOES: REPROVADO"
exit 1
