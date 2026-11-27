#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do passo "Ensaio do emitir_teste.sh no XSD de 24/11/2026 (NL-3.L, passo 11)" do job nativo (ubuntu26.04) do ci.yml.
# Roda depois do build de tools/conteiner/build_local.sh (linha `nativo (ubuntu26.04)` do espelho_local.txt), no contêiner
# da imagem ubuntu26.04, com o build do host montado em /var/tmp/notalivre-build (o caminho que o CMakeCache do build grava).
# Como o CI: uid 1000, HOME descartável, CLI e XSD saem do build (sem rede). Nada roda no host (L-50).
# Uso: NL_FATIA=<a mesma da linha do build> bash tools/espelho/ensaio_xsd_nativo.sh
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
travar
[ -n "${NL_FATIA:-}" ] || { echo "REPROVA: NL_FATIA vazia (use a mesma do build de ubuntu26.04)"; exit 1; }
BUILD_HOST="${NL_BUILD_ROOT:-$NL_TMP_BASE/notalivre-build-ubuntu26.04-$NL_FATIA}"
[ -d "$BUILD_HOST/debug" ] || { echo "REPROVA: build ausente em $BUILD_HOST/debug (rode antes o build de ubuntu26.04)"; exit 1; }
copiar_arvore "$ESPELHO_TRAB/ensaio-xsd" || { echo "REPROVA: cópia da árvore falhou"; exit 1; }
D="$ESPELHO_TRAB/ensaio-xsd"
IMAGEM_BUILD="notalivre/build-ubuntu26.04:nl-2.0"
CONT=(docker run --pull=never "${NL_COTAS_DOCKER[@]}" --rm --network none --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp \
    -v "$D:/src:ro,z" -v "$BUILD_HOST:/var/tmp/notalivre-build:ro,z" -v notalivre-qt-6120:/opt/qt:ro -w /src "$IMAGEM_BUILD")
passo "ensaio-xsd-nativo" "${CONT[@]}" sh -c \
    'mkdir -p /tmp/home && b=/var/tmp/notalivre-build/debug && cli="$b/apps/cli/notalivre-cli" && test -x "$cli" && xsd="$(sed -n "s/^NL_XSD_PRODREST_DIR:INTERNAL=//p" "$b/CMakeCache.txt")" && test -d "$xsd" && man=/src/third_party/nfse-xsd/prodrest-20261124/SHA256SUMS && test -f "$man" && NL_ENSAIO_CLI="$cli" NL_ENSAIO_XSD_DIR="$xsd" NL_ENSAIO_XSD_MANIFESTO="$man" bash tools/producao_restrita/emitir_teste.sh --ensaio'
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "ESPELHO ensaio-xsd-nativo: VERDE"; exit 0; }
echo "ESPELHO ensaio-xsd-nativo: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
