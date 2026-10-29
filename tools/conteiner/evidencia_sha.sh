#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Evidência por SHA do build local (NL-2.0, achado A1-07). Biblioteca e verificador no mesmo arquivo.
#
# A fonte do build é a lista LISTA_FONTE tirada do commit (git archive). O log do build grava, na linha 1,
# "SHA: <40 hex>", e em NL-MANIFESTO-SHA256 o hash da fonte que o contêiner de fato viu. O verificador recalcula
# o manifesto do commit e compara: se a fonte tinha arquivo fora do commit (trabalho alheio não commitado), os
# manifestos divergem e o log reprova.
#
# Uso:
#   evidencia_sha.sh verificar <log> [repo]   rc 0 se o log prova o SHA; rc 1 se reprova
#   source evidencia_sha.sh                   usa as funções extrair_do_sha, manifesto_de_dir, manifesto_do_sha
#
# Regras do verificador (todas precisam valer):
#   1. a linha 1 do log é "SHA: <40 hex>", seguido de espaço, de "+" (modo cópia) ou fim de linha;
#   2. o SHA é commit do repositório [repo] (padrão: a raiz do repositório deste script);
#   3. há exatamente uma linha NL-MANIFESTO-SHA256 no log;
#   4. o manifesto do log é igual ao manifesto do commit.

# Caminhos que formam a fonte do build. Lista única: mudar aqui muda o archive, a cópia e o verificador juntos.
LISTA_FONTE=(CMakeLists.txt CMakePresets.json src apps tests third_party tools/conteiner tools/tabelas tools/servidor_falso tools/portoes flatpak .github/workflows/ci.yml)

# Extrai do commit $2 do repositório $1 os caminhos de LISTA_FONTE para o diretório $3 (já criado, vazio).
# Usa arquivo tar intermediário ao lado de $3 (fica em $3.tar e é apagado), para que uma falha do git não passe calada.
# -m (--touch): o mtime vira o da extração, não o do commit. Commit antigo com mtime antigo, no mesmo diretório de
# build, faria o ninja pular a recompilação (mesmo defeito que o cp -a corrigido em copiar_fonte).
extrair_do_sha() {
    git -C "$1" archive --format=tar -o "$3.tar" "$2" -- "${LISTA_FONTE[@]}" || return 1
    tar -C "$3" -xmf "$3.tar" || return 1
    rm -f -- "$3.tar"
}

# Manifesto do diretório $1: um único SHA-256 sobre (hash, caminho relativo) de cada arquivo regular, em ordem C.
# xargs -r agrupa os arquivos num número pequeno de processos (um sha256sum por lote, nunca um por arquivo).
manifesto_de_dir() {
    (cd "$1" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum | sha256sum | cut -d' ' -f1)
}

# Manifesto do commit $2 do repositório $1, sem tocar em nenhuma árvore de trabalho.
manifesto_do_sha() {
    local tmp rc=0
    tmp="$(mktemp -d "${NL_TMP_BASE:-/var/tmp}/notalivre-manifesto.XXXXXX")" || return 1
    extrair_do_sha "$1" "$2" "$tmp" && manifesto_de_dir "$tmp" || rc=$?
    rm -rf -- "$tmp"
    return "$rc"
}

# Verificador: $1 = log, $2 = repositório. Imprime o motivo e devolve 0 (prova) ou 1 (reprova).
verificar_evidencia() {
    local log="$1" repo="$2" linha1 re sha n declarado esperado
    if [ ! -f "$log" ]; then
        echo "REPROVA: log ausente: $log"
        return 1
    fi
    linha1="$(head -n 1 -- "$log")"
    re='^SHA: ([0-9a-f]{40})([ +]|$)'
    if ! [[ "$linha1" =~ $re ]]; then
        echo "REPROVA: a linha 1 do log não traz 'SHA: <40 hex>' (linha 1: '${linha1:0:80}')"
        return 1
    fi
    sha="${BASH_REMATCH[1]}"
    if ! git -C "$repo" cat-file -e "$sha^{commit}" 2>/dev/null; then
        echo "REPROVA: $sha não é commit de $repo"
        return 1
    fi
    n="$(grep -c '^NL-MANIFESTO-SHA256=' "$log" || true)"
    if [ "$n" != 1 ]; then
        echo "REPROVA: o log precisa de exatamente uma linha NL-MANIFESTO-SHA256 (achei $n)"
        return 1
    fi
    declarado="$(sed -n 's/^NL-MANIFESTO-SHA256=//p' "$log")"
    if ! esperado="$(manifesto_do_sha "$repo" "$sha")"; then
        echo "REPROVA: não consegui extrair $sha de $repo"
        return 1
    fi
    if [ "$declarado" != "$esperado" ]; then
        echo "REPROVA: manifesto do log ($declarado) diverge do commit $sha ($esperado): a fonte do build tinha arquivo fora do commit"
        return 1
    fi
    echo "VERDE: $sha, manifesto confere ($esperado)"
    return 0
}

# CLI: só roda quando o arquivo é executado, não quando é sourced pelo build_local.sh.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    set -uo pipefail
    RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
    case "${1:-}" in
        verificar)
            verificar_evidencia "${2:?uso: evidencia_sha.sh verificar <log> [repo]}" "${3:-$RAIZ}"
            ;;
        *)
            echo "uso: evidencia_sha.sh verificar <log> [repo]" >&2
            exit 2
            ;;
    esac
fi
