#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da cópia do espelho (NL-3.E, L-36): copiar_arvore de comum.sh tem de dar uma cópia com .git
# DIRETÓRIO e histórico legível sem o .git de origem, também quando a origem é uma worktree (.git-arquivo).
# Roda no contêiner de higiene (L-50); os repositórios são criados na hora, em TMPDIR.
# Controles:
#   C1 comum: cópia com .git diretório, com commit lido e com a árvore de trabalho (alteração não commitada);
#   C2 worktree: idem, e a cópia continua lendo commits depois que o .git da origem some (sem dependência);
#   C3 build* não entra na cópia.
# Uso: autoteste_copiar_arvore.sh   (sem argumentos)
set -u

DIR="$(cd "$(dirname "$0")" && pwd)"
. "$DIR/comum.sh"
TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/copia-autoteste.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT
FALHAS=0
GIT_ID=(-c user.name=autoteste -c user.email=autoteste@invalid -c core.hooksPath=/dev/null)

verde() { echo "[VERDE] $1"; }
vermelho() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# commits_lidos DIR: quantos commits o git da cópia enxerga a partir de HEAD (0 se o git falha).
commits_lidos() {
    local n
    n="$(git -C "$1" rev-list --count HEAD 2>/dev/null)" || n=0
    echo "${n:-0}"
}

# Origem comum, com 1 commit, alteração não commitada e uma pasta build* que não deve entrar.
ORIG="$TMP/orig"; git init -q "$ORIG"
echo base > "$ORIG/a.txt"
git -C "$ORIG" add a.txt
git -C "$ORIG" "${GIT_ID[@]}" commit -q -m "test: fixture" || { echo "ABORTA: commit da origem"; exit 1; }

# C1: cópia de repositório comum.
RAIZ="$ORIG"
copiar_arvore "$TMP/copia-comum"; rc=$?
if [ "$rc" -eq 0 ] && [ -d "$TMP/copia-comum/.git" ] && [ "$(commits_lidos "$TMP/copia-comum")" -ge 1 ]; then
    verde "C1 cópia de repositório comum: .git diretório e commit lido"
else
    vermelho "C1 cópia de repositório comum: rc=$rc, .git diretório ou commit lido falharam"
fi

# Worktree da origem, com alteração não commitada e uma pasta build*.
WT="$TMP/wt"
git -C "$ORIG" worktree add -q "$WT" -b fixture-wt || { echo "ABORTA: worktree da fixture"; exit 1; }
echo alterado > "$WT/a.txt"
mkdir -p "$WT/build-x" && echo saida > "$WT/build-x/o.txt"

# C2: cópia da worktree. Depois da cópia, o .git da origem é tirado do caminho: a cópia não pode depender dele.
RAIZ="$WT"
copiar_arvore "$TMP/copia-wt"; rc=$?
mv "$ORIG/.git" "$TMP/git-origem-fora" || { echo "ABORTA: não tirou o .git da origem"; exit 1; }
if [ "$rc" -eq 0 ] && [ -d "$TMP/copia-wt/.git" ] && [ "$(commits_lidos "$TMP/copia-wt")" -ge 1 ]; then
    verde "C2 cópia de worktree: .git diretório e commit lido sem a origem"
else
    vermelho "C2 cópia de worktree: rc=$rc, .git diretório ou commit lido sem a origem falharam"
fi

# C2b: a alteração não commitada da worktree tem de chegar à cópia (árvore de trabalho, como antes).
if [ -f "$TMP/copia-wt/a.txt" ] && grep -qx alterado "$TMP/copia-wt/a.txt"; then
    verde "C2b cópia de worktree: alteração não commitada presente"
else
    vermelho "C2b cópia de worktree: alteração não commitada ausente"
fi

# C3: build* não entra na cópia.
if [ ! -e "$TMP/copia-wt/build-x" ]; then
    verde "C3 build* fora da cópia"
else
    vermelho "C3 build* entrou na cópia"
fi

echo "FALHAS=$FALHAS"
[ "$FALHAS" -eq 0 ] || { echo "AUTOTESTE COPIAR ARVORE: REPROVADO"; exit 1; }
echo "AUTOTESTE COPIAR ARVORE: todos os controles conferem"
exit 0
