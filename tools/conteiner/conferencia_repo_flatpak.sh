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

# conferir <tipo: - arquivo, l link> <caminho dentro da ref> <rótulo>
# A última coluna de `ostree ls -R` é o caminho completo; a primeira é o tipo e as permissões.
conferir() {
    local tipo="$1" caminho="$2" rotulo="$3"
    if awk -v t="$tipo" -v p="$caminho" 'substr($1, 1, 1) == t && $NF == p { achou = 1 } END { exit !achou }' "$LISTA"; then
        analisados=$((analisados + 1))
        echo "ok: $caminho"
    else
        falharam=$((falharam + 1))
        echo "AUSENTE: $rotulo ($caminho) na ref $REF"
    fi
}

conferir - /files/bin/notalivre-gui "app"
conferir - /files/lib/libsodium.so.26 "biblioteca libsodium"
conferir - "/files/share/licenses/$ID/libsodium/LICENSE" "licença da libsodium"

echo "conferencia_repo_flatpak: analisados=$analisados falharam=$falharam"
if [ "$analisados" -eq 0 ]; then
    echo "REPROVA: varredura vazia (analisados=0); zero não é bundle limpo"
    exit 1
fi
[ "$falharam" -eq 0 ] || exit 1
exit 0
