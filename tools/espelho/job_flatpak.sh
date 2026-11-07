#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do job `flatpak` do ci.yml (R2): build do manifesto contra o SDK KDE 6.11 e bundle.
# Local: org.flatpak.Builder (sandbox do flatpak, autorizado pelo líder em NL-0.5), SEM --privileged.
# O CI usa o contêiner flathub com --privileged (ADR 0001); aqui não há privilégio extra.
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
# NL-3.0k: a cota de núcleos do flatpak-builder vem de NL_THREADS (padrão 7, regra no átomo tools/conteiner/cota_nucleos.sh).
# Sem --jobs, o flatpak-builder deixava o build usar os 16 núcleos (medido em 07/11/2026: carga 49, máquina dividida).
# O --jobs chega ao módulo por FLATPAK_BUILDER_N_JOBS (INFERÊNCIA do CTO); a linha jobs-do-build no log prova isso.
. "$(dirname "${BASH_SOURCE[0]}")/../conteiner/cota_nucleos.sh"
JOBS="$(nl_threads_valido "${NL_THREADS:-}" "$(nproc)")" || exit 1
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
# A saída do builder vai para um log e o rc é guardado logo depois do comando (L-45: nada de pipe que esconda o status).
FBLOG="$D/flatpak-builder.log"
"${FB[@]}" --jobs="$JOBS" --disable-rofiles-fuse --force-clean --state-dir="$D/state" --repo="$D/repo" "$D/build" "$MANIFESTO" > "$FBLOG" 2>&1
rc_fb=$?
cat "$FBLOG"
echo "passo=flatpak-builder rc=$rc_fb"
[ "$rc_fb" -eq 0 ] || PASSOS_FALHOS=$((PASSOS_FALHOS + 1))
# NL-3.0k: o manifesto imprime jobs-do-build=<N> no log. Tem de ser exatamente a cota pedida; "ausente" reprova (o --jobs não chegou).
if grep -qx "jobs-do-build=$JOBS" "$FBLOG"; then
    echo "passo=jobs-do-build rc=0 (jobs-do-build=$JOBS)"
else
    echo "passo=jobs-do-build rc=1 (esperado jobs-do-build=$JOBS no log; ausente reprova)"
    PASSOS_FALHOS=$((PASSOS_FALHOS + 1))
fi
passo "bundle" flatpak run --user --filesystem="$D" --filesystem=xdg-data/flatpak:ro \
    --env=FLATPAK_USER_DIR="$HOME/.local/share/flatpak" --command=flatpak org.flatpak.Builder \
    build-bundle "$D/repo" "$D/notalivre.flatpak" io.github.petrinhu.notalivre master
# NL-3.2a (parte 2): a libsodium e a licença dela estão na árvore do build. A conferência roda em contêiner, sem privilégio,
# com o build e a conferência só de leitura (L-50); reprova o job se faltar a biblioteca ou a licença, ou se a varredura for vazia.
passo "conferencia-bundle-libsodium" docker run --pull=never --rm --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp \
    -v "$D/build:/build:ro,z" -v "$D/fonte/tools/conteiner:/conteiner:ro,z" \
    notalivre/build-fedora45:nl-2.0 bash /conteiner/conferencia_bundle_libs.sh /build
# NL-3.2a (revisão A2): o repositório OSTree que gera o bundle tem o app, a biblioteca e a licença. Lido pelo ostree
# da imagem ostree-fedora45 (NL-3.0g), só de leitura; o bundle não é executado.
garantir_imagem_ostree || { echo "REPROVA: imagem ostree-fedora45 não construída"; exit 1; }
passo "conferencia-repo-flatpak" docker run --pull=never --rm --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp \
    -v "$D/repo:/repo:ro,z" -v "$D/fonte/tools/conteiner:/conteiner:ro,z" \
    notalivre/ostree-fedora45:nl-3.0g bash /conteiner/conferencia_repo_flatpak.sh /repo
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "JOB flatpak: VERDE (bundle em $D/notalivre.flatpak)"; exit 0; }
echo "JOB flatpak: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
