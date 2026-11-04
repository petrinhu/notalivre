#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Conferência do bundle Flatpak (NL-3.2a, parte 2): a libsodium e a licença dela têm de estar na árvore do build.
# Olha o diretório do build do flatpak-builder (o que vira /app): files/lib/libsodium.so* e
# files/share/licenses/$FLATPAK_ID/ com ao menos um arquivo. Só confere nomes e presença, nunca o conteúdo.
# Reprova se faltar qualquer um, e reprova se a varredura não achar nenhum arquivo (L-36: zero é varredura quebrada).
# Saída: analisados=N falharam=M. Sai com 1 se falharam for diferente de zero ou analisados for zero.
# Uso: conferencia_bundle_libs.sh <diretorio-do-build>      (FLATPAK_ID padrão: io.github.petrinhu.notalivre)
set -u
shopt -s nullglob

ARVORE="${1:?uso: conferencia_bundle_libs.sh <diretorio-do-build>}"
ID="${FLATPAK_ID:-io.github.petrinhu.notalivre}"
LICENCAS="$ARVORE/files/share/licenses/$ID"
analisados=0
falharam=0

libs=("$ARVORE"/files/lib/libsodium.so*)
if [ "${#libs[@]}" -eq 0 ]; then
    echo "AUSENTE: libsodium.so* em $ARVORE/files/lib"
    falharam=$((falharam + 1))
fi
for f in "${libs[@]}"; do
    analisados=$((analisados + 1))
    echo "ok: $f"
done

licencas=()
if [ -d "$LICENCAS" ]; then
    licencas=("$LICENCAS"/*)
fi
if [ "${#licencas[@]}" -eq 0 ]; then
    echo "AUSENTE: licença em $LICENCAS (share/licenses)"
    falharam=$((falharam + 1))
fi
for f in "${licencas[@]}"; do
    [ -f "$f" ] || continue
    analisados=$((analisados + 1))
    echo "ok: $f"
done

echo "conferencia_bundle_libs: analisados=$analisados falharam=$falharam"
if [ "$analisados" -eq 0 ]; then
    echo "REPROVA: varredura vazia (analisados=0); zero não é bundle limpo"
    exit 1
fi
[ "$falharam" -eq 0 ] || exit 1
exit 0
