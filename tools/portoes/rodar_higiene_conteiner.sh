#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Gêmeo local (L-56) dos portões de higiene do NotaLivre (NL-0.6).
# Copia a árvore do repositório (sem .git, sem build*) para /var/tmp, constrói a imagem
# de higiene se ainda não existir e roda o autoteste DENTRO do contêiner, com a árvore
# montada só para leitura e sem rede. Tudo sob flock (um trabalho pesado por vez, L-11).
# Os portões em si (higiene_segredos.sh, higiene_travessao.py) não dependem deste script.
#
# Uso: rodar_higiene_conteiner.sh RAIZ_DO_REPO LOG [DIR_PORTOES_ALTERNATIVOS]
#   RAIZ_DO_REPO: raiz do repositório a copiar
#   LOG: arquivo de log (use /var/tmp/notalivre-logs/NL-0.6-*.log)
#   DIR_PORTOES_ALTERNATIVOS: opcional; portões substitutos (usado só no controle com stub)
set -u

RAIZ_REPO="${1:?uso: rodar_higiene_conteiner.sh RAIZ_DO_REPO LOG [DIR_PORTOES]}"
LOG="${2:?uso: rodar_higiene_conteiner.sh RAIZ_DO_REPO LOG [DIR_PORTOES]}"
PORTOES_ALT="${3:-}"
IMAGEM="notalivre/higiene-fedora44:nl-0.6"
DEST="/var/tmp/notalivre-higiene-src"
export TMPDIR=/var/tmp

mkdir -p "$(dirname "$LOG")"
exec 9> /var/tmp/notalivre-heavy.lock
flock -w 1200 9 || { echo "sem trava de trabalho pesado em 1200 s" | tee -a "$LOG"; exit 1; }

{
    echo "== disco (btrfs, nunca df)"
    btrfs filesystem usage / | /usr/bin/grep -E 'Device unallocated|Free \(estimated\)'

    echo "== copia da arvore para $DEST"
    if [ -d "$DEST" ]; then
        echo "removendo copia anterior, conteudo listado:"
        ls -la "$DEST"
        rm -rf -- "$DEST"
    fi
    mkdir -p "$DEST"
    set -o pipefail
    tar --exclude=./.git --exclude='./build*' -C "$RAIZ_REPO" -cf - . | tar -C "$DEST" -xf -
    echo "copia_rc=$?"
    set +o pipefail
    echo "arquivos copiados: $(find "$DEST" -type f | wc -l)"

    if ! docker image inspect "$IMAGEM" > /dev/null 2>&1; then
        echo "== construindo imagem $IMAGEM"
        nice -n 5 docker build -t "$IMAGEM" -f "$DEST/tools/portoes/higiene.Dockerfile" "$DEST/tools/portoes"
        echo "build_rc=$?"
    else
        echo "== imagem $IMAGEM ja existe, reutilizada"
    fi
    echo "imagem: $(docker image inspect --format '{{.Id}} tamanho={{.Size}} bytes' "$IMAGEM" 2>&1)"

    extra=()
    if [ -n "$PORTOES_ALT" ]; then
        extra=(-v "$PORTOES_ALT:/portoes:ro,z" -e HIGIENE_PORTOES=/portoes)
        echo "== portoes alternativos montados: $PORTOES_ALT"
    fi

    echo "== autoteste no conteiner (sem rede, arvore so leitura)"
    nice -n 5 docker run --rm --network none \
        -v "$DEST:/src:ro,z" -e TMPDIR=/var/tmp "${extra[@]}" \
        "$IMAGEM" bash /src/tools/portoes/autoteste_higiene.sh
    RC=$?
    echo "rc_conteiner=$RC"
} > "$LOG" 2>&1
RC=$(/usr/bin/grep -o 'rc_conteiner=[0-9]*' "$LOG" | cut -d= -f2)
echo "log: $LOG"
echo "rc_conteiner=${RC:-ausente}"
[ "${RC:-1}" = "0" ] || exit 1
exit 0
