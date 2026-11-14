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
# Teto de contêiner (NL-3.K): todo docker run dos jobs recebe "${NL_COTAS_DOCKER[@]}", do átomo único.
. "$RAIZ/tools/conteiner/cota_conteiner.sh"
nl_cotas_docker || { echo "REPROVA: teto de contêiner inválido"; exit 2; }

# Trava única de trabalho pesado (L-11). Cada job_*.sh chama travar antes de qualquer contêiner.
travar() {
    exec 9> "$TRAVA"
    flock -w 3600 9 || { echo "REPROVA: sem trava de trabalho pesado em 3600 s ($TRAVA)"; exit 1; }
    # Marca para os filhos: nenhum comando filho pega a mesma trava de novo (flock aninhado trava o processo).
    NL_TRAVA_DETIDA=1
    export NL_TRAVA_DETIDA
}

# Cópia do COMMIT de $RAIZ, com .git DIRETÓRIO, como o actions/checkout do CI entrega (NL-3.E).
# O .git de uma worktree é arquivo que aponta para fora da cópia; o gitleaks lia 0 commits e saía com rc 0 (falso
# verde). Por isso a cópia é um `git clone` local: resolve o .git-arquivo, dá um .git próprio (sem alternates) e
# traz só o que está commitado. Não se sobrepõe a árvore de trabalho: o espelho testa o que o CI vai testar.
# Alteração não commitada não entra; isso é avisado no stderr, sem mudar o rc (o que não é commitado não chega ao CI).
# Por que clone e não recusa: recusar tornaria o espelho inutilizável nas worktrees, que são o fluxo padrão da fatia.
copiar_arvore() {
    local destino="$1" sujos area modo
    # Área de trabalho do espelho: pasta do usuário, privada (NL-3.0g, revisão de segurança). /var/tmp é gravável por todos
    # (sticky), e o nome é previsível: sem esta checagem, outro usuário pré-cria o caminho. Nunca clone numa área alheia.
    area="$(dirname -- "$destino")"
    [ -d "$area" ] || mkdir -m 700 -p -- "$area" || { echo "REPROVA: não criei a área $area; nada foi clonado" >&2; return 1; }
    if [ ! -O "$area" ]; then
        echo "REPROVA: a área $area não é do usuário atual; nada foi clonado" >&2
        return 1
    fi
    modo="$(stat -c %a -- "$area")"
    if [ $(( 0$modo & 022 )) -ne 0 ]; then
        echo "REPROVA: a área $area tem escrita para grupo ou outros (modo $modo); nada foi clonado" >&2
        return 1
    fi
    if [ $(( 0$modo & 077 )) -ne 0 ]; then
        chmod 700 -- "$area" || { echo "REPROVA: não ajustei o modo de $area; nada foi clonado" >&2; return 1; }
    fi
    rm -rf -- "$destino" && mkdir -p "$destino" || return 1
    git clone --quiet --no-hardlinks -- "$RAIZ" "$destino" || return 1
    [ -d "$destino/.git" ] || return 1
    sujos="$(git -C "$RAIZ" status --porcelain | wc -l | tr -d ' ')"
    if [ "${sujos:-0}" -gt 0 ]; then
        echo "AVISO: $sujos alteração(ões) não commitada(s) em $RAIZ NÃO entram no espelho (o CI só vê o commit). Commite antes para testar essas mudanças." >&2
    fi
}

# Imagem de higiene (mesma do CI). Reconstruída a cada espelho: o digest fixado no Dockerfile é o que vale, e uma imagem
# antiga com digest diferente não pode ficar em uso (NL-3.0g). Com cache de camadas o build é rápido.
garantir_imagem_higiene() {
    docker build -q -t "$IMAGEM_HIGIENE" -f "$RAIZ/tools/portoes/higiene.Dockerfile" "$RAIZ/tools/portoes" >/dev/null || return 1
}

# Imagem de ostree da conferência do repositório Flatpak (NL-3.0g). Mesma regra: reconstruída a cada espelho, com cache.
garantir_imagem_ostree() {
    docker build -q -t notalivre/ostree-fedora45:nl-3.0g -f "$RAIZ/tools/conteiner/ostree-fedora45.Dockerfile" "$RAIZ/tools/conteiner" >/dev/null || return 1
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
