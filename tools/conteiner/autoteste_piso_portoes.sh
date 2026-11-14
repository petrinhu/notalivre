#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste de estreia do piso dos portões estáticos (NL-2.11, achado A1 da revisão QA; L-36, L-27).
# NL-3.0g (CISO, 05/11/2026): o piso e a lista vêm do SHA. Cada caso monta um repositório de brincadeira com um commit
# que traz portoes_estaticos.sh, piso_portoes.txt e lista_portoes.txt (sabotados, quando o caso pede), e roda o portão
# nesse SHA. O docker é um STUB que sempre sai 0, então a única coisa que pode reprovar é o piso ou a lista.
# Uso: autoteste_piso_portoes.sh     (cwd qualquer; usa a árvore onde o script está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ORIGEM="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-piso-portoes.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin"
printf '#!/bin/sh\nexit 0\n' > "$BASE/bin/docker"
chmod +x "$BASE/bin/docker"
export PATH="$BASE/bin:$PATH"

CASOS=0
FALHAS=0

# caso NOME PISO SEDLISTA RC_ESPERADO TEXTO_ESPERADO
# PISO é o conteúdo de piso_portoes.txt do SHA do caso. SEDLISTA vazio = lista original; "__AUSENTE__" = sem lista no SHA.
# Qualquer outro valor é um sed aplicado à lista do SHA do caso (a sabotagem).
caso() {
    local nome="$1" piso="$2" sabotagem="$3" rc_esperado="$4" texto="$5"
    local fora="${6:-}" vazou=0
    local dir="$BASE/f-$nome" saida rc sha
    CASOS=$((CASOS + 1))
    mkdir -p "$dir/tools/conteiner"
    git -C "$dir" init -q
    cp "$ORIGEM/portoes_estaticos.sh" "$dir/tools/conteiner/portoes_estaticos.sh"
    # NL-3.K: o portão carrega os átomos do teto de contêiner da própria pasta; o caso roda o script daqui.
    cp "$ORIGEM/cota_conteiner.sh" "$ORIGEM/cota_nucleos.sh" "$dir/tools/conteiner/"
    printf '%s\n' "$piso" > "$dir/tools/conteiner/piso_portoes.txt"
    if [ "$sabotagem" != "__AUSENTE__" ]; then
        cp "$ORIGEM/lista_portoes.txt" "$dir/tools/conteiner/lista_portoes.txt"
        if [ -n "$sabotagem" ]; then
            sed -i "$sabotagem" "$dir/tools/conteiner/lista_portoes.txt"
        fi
    fi
    git -C "$dir" add -A
    git -C "$dir" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: $nome" \
        || { echo "ABORTA: commit do caso $nome"; exit 1; }
    sha="$(git -C "$dir" rev-parse HEAD)" || { echo "ABORTA: rev-parse do caso $nome"; exit 1; }
    saida="$(timeout 300 bash "$dir/tools/conteiner/portoes_estaticos.sh" "$dir" "$sha" "$BASE/d-$nome" 2>&1)"
    rc=$?
    echo "--- caso $nome: rc=$rc (esperado $rc_esperado)"
    echo "$saida" | tail -n 2
    # Nada pode ter sido criado fora de DESTINO (NL-3.0g, revisão ALTA: nome vindo do SHA usado em caminho no host).
    if [ -n "$fora" ] && [ -e "$fora" ]; then
        vazou=1
    fi
    if [ "$vazou" = 1 ]; then
        echo "[VERMELHO] $nome: arquivo criado fora de DESTINO: $fora"
        FALHAS=$((FALHAS + 1))
    elif [ "$rc" = "$rc_esperado" ] && printf '%s\n' "$saida" | grep -q -F -- "$texto"; then
        echo "[VERDE] $nome"
    else
        echo "[VERMELHO] $nome: rc=$rc, esperava rc=$rc_esperado com '$texto'"
        FALHAS=$((FALHAS + 1))
    fi
}

# Controle positivo: a lista e o piso do SHA batem, o laço roda 15 portões e nenhum reprova.
caso verde 15 "" 0 "portoes=15 falharam=0 (piso 15)"
# Sabotagem 1: um portão sai da lista do SHA (o laço roda 14). Sem piso, passaria verde.
caso lista_sem_um 15 '/^segredos_historico|/d' 1 "a lista de PORTOES tem 14 portões, o piso é 15"
# Sabotagem 2: lista esvaziada no SHA (zero portões, o falso verde de origem).
caso lista_vazia 15 '/^[^#]/d' 1 "a lista de PORTOES tem 0 portões, o piso é 15"
# Sabotagem 3: piso 0 no SHA.
caso piso_zero 0 "" 2 "piso 0 recusado"
# Sabotagem 4: piso maior que a lista do SHA (lista de 15, piso 16).
caso piso_maior 16 "" 1 "a lista de PORTOES tem 15 portões, o piso é 16"
# Sabotagem 5: piso ilegível no SHA.
caso piso_ilegivel "1x" "" 2 "piso ilegível"
# Sabotagem 6: SHA sem lista (a lista não pode vir da árvore de trabalho).
caso lista_ausente 15 "__AUSENTE__" 2 "lista_portoes.txt não lida do SHA"
# Sabotagens 7 a 11 (revisão ALTA, NL-3.0g): nome de portão vindo do SHA não pode sair de DESTINO nem chegar ao host.
# Antes do conserto, cada um destes casos deve ficar vermelho (o nome entra em caminho sem checagem).
caso dotdot 15 's/^camadas|/..\/fuga|/' 2 "nome de portão inválido" "$BASE/fuga.log"
caso barra 15 's/^camadas|/sub\/fuga|/' 2 "nome de portão inválido" "$BASE/d-barra/sub/fuga.log"
caso vazio 15 's/^camadas|/|/' 2 "nome de portão inválido" "$BASE/d-vazio/.log"
caso sem_pipe 15 's/^camadas|.*/camadas sem barra vertical/' 2 "sem '|'" ""
caso duplicado 15 's/^autoteste_camadas|/camadas|/' 2 "nome de portão duplicado" ""

echo "casos=$CASOS falhas=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE PISO PORTOES: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE PISO PORTOES: REPROVADO"
exit 1
