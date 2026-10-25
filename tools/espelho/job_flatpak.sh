#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do job `flatpak` do ci.yml (R2): build do manifesto contra o SDK KDE 6.11 e bundle.
# Local: org.flatpak.Builder (sandbox do flatpak, autorizado pelo líder em NL-0.5), SEM --privileged.
# O CI usa o contêiner flathub com --privileged (ADR 0001); aqui não há privilégio extra.
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
travar
D="$ESPELHO_TRAB/flatpak"
# cwd fora da raiz: flatpak-builder grava .flatpak-builder no diretório atual.
mkdir -p "$D" && cd "$D" || exit 1
copiar_arvore "$D/fonte" || { echo "REPROVA: cópia da árvore falhou"; exit 1; }
rm -rf -- "$D/build" "$D/state" "$D/repo" "$D/notalivre.flatpak"
mkdir -p "$D/state" "$D/repo"
MANIFESTO="$D/fonte/flatpak/io.github.petrinhu.notalivre.yml"
FB=(flatpak run --user --filesystem="$D" --filesystem=xdg-data/flatpak:ro
    --env=FLATPAK_USER_DIR="$HOME/.local/share/flatpak" --command=flatpak-builder org.flatpak.Builder)
passo "flatpak-builder" "${FB[@]}" --disable-rofiles-fuse --force-clean --state-dir="$D/state" --repo="$D/repo" "$D/build" "$MANIFESTO"
passo "bundle" flatpak run --user --filesystem="$D" --filesystem=xdg-data/flatpak:ro \
    --env=FLATPAK_USER_DIR="$HOME/.local/share/flatpak" --command=flatpak org.flatpak.Builder \
    build-bundle "$D/repo" "$D/notalivre.flatpak" io.github.petrinhu.notalivre master
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "JOB flatpak: VERDE (bundle em $D/notalivre.flatpak)"; exit 0; }
echo "JOB flatpak: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
