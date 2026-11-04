#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Conferência do repositório OSTree que o flatpak-builder gera e de onde sai o bundle (NL-3.2a, revisão A2).
# Lê a árvore da ref com `ostree ls -R` (só leitura: nada é instalado nem executado do app) e confere três arquivos:
#   /files/bin/notalivre-gui                       o app (comando do manifesto)
#   /files/lib/libsodium.so.26                     a biblioteca da libsodium
#   /files/share/licenses/<id>/libsodium/LICENSE   a licença da libsodium (license-files do módulo)
# Saída: analisados=N falharam=M. Sai com 1 se falharam for diferente de zero ou se nada for analisado (zero reprova).
# Uso: conferencia_repo_flatpak.sh <repo-ostree> [ref]      (FLATPAK_ID padrão: io.github.petrinhu.notalivre)
set -u

REPO="${1:?uso: conferencia_repo_flatpak.sh <repo-ostree> [ref]}"
ID="${FLATPAK_ID:-io.github.petrinhu.notalivre}"
REF="${2:-app/$ID/x86_64/master}"
LISTA="$(mktemp "${TMPDIR:-/var/tmp}/conferencia-repo.XXXXXX")" || exit 1
trap 'rm -f -- "$LISTA"' EXIT

if ! command -v ostree > /dev/null 2>&1; then
    echo "AUSENTE: ostree (CLI) para ler o repositório $REPO"
    echo "conferencia_repo_flatpak: analisados=0 falharam=1"
    exit 1
fi
if ! ostree --repo="$REPO" ls -R "$REF" > "$LISTA" 2>&1; then
    echo "REPROVA: ostree ls falhou para a ref $REF em $REPO"
    cat "$LISTA"
    echo "conferencia_repo_flatpak: analisados=0 falharam=1"
    exit 1
fi

analisados=0
falharam=0

# resolver <caminho dentro da ref>: a saída de `ostree ls -R` tem a primeira coluna com o tipo e as permissões (- arquivo,
# d diretório, l link) e o caminho na última coluna; num link, o destino vem depois de " -> " e é relativo ao diretório do link.
# Imprime o motivo e sai 0 só se a entrada é arquivo regular ou link cujo alvo existe na mesma ref. Link pendurado reprova.
# O nome fica antes de " -> " (a última coluna de um link é o destino, não o caminho: medido na r2).
resolver() {
    awk -v p="$1" '
    function nome_de(trecho,    partes, n) { n = split(trecho, partes, " "); return partes[n] }
    {
        idx = index($0, " -> ")
        if (idx > 0) { trecho = substr($0, 1, idx - 1); alvo = substr($0, idx + 4) } else { trecho = $0; alvo = "" }
        nome = nome_de(trecho)
        tipo[nome] = substr($1, 1, 1)
        destino[nome] = alvo
        existe[nome] = 1
    }
    END {
        if (!(p in existe)) { print "ausente"; exit 1 }
        if (tipo[p] == "-") { print "arquivo regular"; exit 0 }
        if (tipo[p] == "l") {
            t = destino[p]
            if (substr(t, 1, 1) != "/") { d = p; sub(/\/[^\/]*$/, "", d); t = d "/" t }
            if (t ~ /(^|\/)\.\.(\/|$)/) { print "link pendurado: alvo " destino[p] " com .. não é resolvido por esta conferência"; exit 1 }
            if (t in existe) { print "link para " destino[p]; exit 0 }
            print "link pendurado: alvo " t " não existe na ref"; exit 1
        }
        print "tipo " tipo[p] " inesperado"; exit 1
    }' "$LISTA"
}

# conferir <caminho dentro da ref> <rótulo>
conferir() {
    local caminho="$1" rotulo="$2" motivo rc
    motivo="$(resolver "$caminho")"; rc=$?
    if [ "$rc" -eq 0 ]; then
        analisados=$((analisados + 1))
        echo "ok: $caminho ($motivo)"
    elif [ "$motivo" = ausente ]; then
        falharam=$((falharam + 1))
        echo "AUSENTE: $rotulo ($caminho) na ref $REF"
    else
        falharam=$((falharam + 1))
        echo "AUSENTE: $rotulo ($caminho) na ref $REF: $motivo"
    fi
}

conferir /files/bin/notalivre-gui "app"
conferir /files/lib/libsodium.so.26 "biblioteca libsodium"
conferir "/files/share/licenses/$ID/libsodium/LICENSE" "licença da libsodium"

echo "conferencia_repo_flatpak: analisados=$analisados falharam=$falharam"
if [ "$analisados" -eq 0 ]; then
    echo "REPROVA: varredura vazia (analisados=0); zero não é bundle limpo"
    exit 1
fi
[ "$falharam" -eq 0 ] || exit 1
exit 0
