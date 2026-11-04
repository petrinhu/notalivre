#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da cópia do espelho (NL-3.E, L-36): copiar_arvore de comum.sh copia SÓ o commit (o que o CI
# entrega com o actions/checkout), com .git diretório, sem depender do .git de origem, e avisa em voz alta
# quando a worktree tem alteração não commitada, que não entra no espelho.
# Roda no contêiner de higiene (L-50); os repositórios são criados na hora, em TMPDIR.
# Controles:
#   C1 comum: cópia com .git diretório e commit lido;
#   C2 worktree: cópia com .git diretório e commit lido, sem a origem (sem dependência);
#   C2b alteração não commitada da worktree NÃO entra (a cópia tem o conteúdo do commit);
#   C3 arquivo novo não rastreado e pasta build* não rastreada NÃO entram;
#   C4 arquivo rastreado apagado na worktree CONTINUA na cópia (o commit o tem; é o que o CI vê);
#   C5 worktree suja: copiar_arvore imprime AVISO no stderr e sai com rc 0.
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

# Origem com dois arquivos rastreados (a.txt e b.txt), um commit só.
ORIG="$TMP/orig"; git init -q "$ORIG"
echo base > "$ORIG/a.txt"
echo bee > "$ORIG/b.txt"
git -C "$ORIG" add a.txt b.txt
git -C "$ORIG" "${GIT_ID[@]}" commit -q -m "test: fixture" || { echo "ABORTA: commit da origem"; exit 1; }

# C1: cópia de repositório comum.
RAIZ="$ORIG"
copiar_arvore "$TMP/copia-comum"; rc=$?
if [ "$rc" -eq 0 ] && [ -d "$TMP/copia-comum/.git" ] && [ "$(commits_lidos "$TMP/copia-comum")" -ge 1 ]; then
    verde "C1 cópia de repositório comum: .git diretório e commit lido"
else
    vermelho "C1 cópia de repositório comum: rc=$rc, .git diretório ou commit lido falharam"
fi

# Worktree da origem, com: a.txt alterado (não commitado), b.txt apagado (rastreado), novo.txt e build-x/ não rastreados.
WT="$TMP/wt"
git -C "$ORIG" worktree add -q "$WT" -b fixture-wt || { echo "ABORTA: worktree da fixture"; exit 1; }
echo alterado > "$WT/a.txt"
rm -f "$WT/b.txt"
echo novo > "$WT/novo.txt"
mkdir -p "$WT/build-x" && echo saida > "$WT/build-x/o.txt"

# C2, C2b, C3, C4: uma cópia da worktree suja.
RAIZ="$WT"
copiar_arvore "$TMP/copia-wt" 2> "$TMP/aviso.err"; rc=$?
if [ "$rc" -eq 0 ] && [ -d "$TMP/copia-wt/.git" ] && [ "$(commits_lidos "$TMP/copia-wt")" -ge 1 ]; then
    verde "C2 cópia de worktree: .git diretório e commit lido"
else
    vermelho "C2 cópia de worktree: rc=$rc, .git diretório ou commit lido falharam"
fi
if [ -f "$TMP/copia-wt/a.txt" ] && grep -qx base "$TMP/copia-wt/a.txt"; then
    verde "C2b alteração não commitada fora da cópia (a.txt tem o conteúdo do commit)"
else
    vermelho "C2b alteração não commitada entrou na cópia"
fi
if [ ! -e "$TMP/copia-wt/novo.txt" ] && [ ! -e "$TMP/copia-wt/build-x" ]; then
    verde "C3 arquivo novo não rastreado e build* fora da cópia"
else
    vermelho "C3 arquivo novo não rastreado ou build* entrou na cópia"
fi
if [ -f "$TMP/copia-wt/b.txt" ] && grep -qx bee "$TMP/copia-wt/b.txt"; then
    verde "C4 arquivo rastreado apagado na worktree continua na cópia (o commit o tem)"
else
    vermelho "C4 arquivo rastreado apagado na worktree sumiu da cópia"
fi

# C5: aviso visível para a worktree suja.
if grep -q '^AVISO:.*não commitada' "$TMP/aviso.err"; then
    verde "C5 worktree suja: AVISO no stderr"
else
    vermelho "C5 worktree suja: sem AVISO de alteração não commitada no stderr"
    sed 's/^/   | /' "$TMP/aviso.err"
fi

# C6: a cópia não depende do .git de origem. Só depois de todas as cópias da worktree, o .git da origem sai do caminho.
mv "$ORIG/.git" "$TMP/git-origem-fora" || { echo "ABORTA: não tirou o .git da origem"; exit 1; }
if [ "$(commits_lidos "$TMP/copia-wt")" -ge 1 ] && [ ! -e "$TMP/copia-wt/.git/objects/info/alternates" ]; then
    verde "C6 cópia lê commits sem o .git de origem e sem alternates"
else
    vermelho "C6 cópia depende do .git de origem (sem commit lido ou com alternates)"
fi

echo "FALHAS=$FALHAS"
[ "$FALHAS" -eq 0 ] || { echo "AUTOTESTE COPIAR ARVORE: REPROVADO"; exit 1; }
echo "AUTOTESTE COPIAR ARVORE: todos os controles conferem"
exit 0
