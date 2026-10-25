#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Funções comuns dos espelhos locais (R2, A0-02). Carregado com `. comum.sh` pelos job_*.sh.
# NL_TMP_BASE: base de temporários e de trava no host (padrão /var/tmp, A0-08).
NL_TMP_BASE="${NL_TMP_BASE:-/var/tmp}"
export NL_TMP_BASE TMPDIR="$NL_TMP_BASE"
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TRAVA="$NL_TMP_BASE/notalivre-heavy.lock"
ESPELHO_TRAB="$NL_TMP_BASE/notalivre-espelho"
IMAGEM_HIGIENE="notalivre-higiene:ci"

# Trava única de trabalho pesado (L-11). Cada job_*.sh chama travar antes de qualquer contêiner.
travar() {
    exec 9> "$TRAVA"
    flock -w 3600 9 || { echo "REPROVA: sem trava de trabalho pesado em 3600 s ($TRAVA)"; exit 1; }
    # Marca para os filhos: nenhum comando filho pega a mesma trava de novo (flock aninhado trava o processo).
    NL_TRAVA_DETIDA=1
    export NL_TRAVA_DETIDA
}

# Cópia da árvore de trabalho COM .git (o modo git do gitleaks precisa do histórico), sem build* nem worktrees.
copiar_arvore() {
    local destino="$1"
    rm -rf -- "$destino" && mkdir -p "$destino" || return 1
    (cd "$RAIZ" && tar --exclude='./build*' --exclude='./worktrees' -cf - .) | tar -C "$destino" -xf - || return 1
}

# Espelho de higiene precisa da imagem de higiene (mesma do CI). Constrói só se faltar.
garantir_imagem_higiene() {
    if [ -z "$(docker image ls -q "$IMAGEM_HIGIENE")" ]; then
        docker build -t "$IMAGEM_HIGIENE" -f "$RAIZ/tools/portoes/higiene.Dockerfile" "$RAIZ/tools/portoes" || return 1
    fi
}

# Um passo do job: roda, imprime o rc e acumula a falha. Uso: passo NOME COMANDO...
PASSOS_FALHOS=0
passo() {
    local nome="$1"; shift
    "$@"
    local rc=$?
    echo "passo=$nome rc=$rc"
    [ "$rc" -eq 0 ] || PASSOS_FALHOS=$((PASSOS_FALHOS + 1))
    return 0
}
