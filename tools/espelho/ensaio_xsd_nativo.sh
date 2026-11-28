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
# Os test -x/-d/-f da cadeia saem com 1, cedo, se o build não tiver a CLI, o XSD ou o manifesto; o script reprova com 11 se faltar
# algum dos quatro caminhos (NL_ENSAIO_CLI, NL_ENSAIO_XSD_DIR, NL_ENSAIO_XSD_MANIFESTO, NL_ENSAIO_TABELAS_DIR).
passo "ensaio-xsd-nativo" "${CONT[@]}" sh -c \
    'mkdir -p /tmp/home && b=/var/tmp/notalivre-build/debug && cli="$b/apps/cli/notalivre-cli" && test -x "$cli" && xsd="$(sed -n "s/^NL_XSD_PRODREST_DIR:INTERNAL=//p" "$b/CMakeCache.txt")" && test -d "$xsd" && tab="$(sed -n "s/^NL_TABELAS_DIR:INTERNAL=//p" "$b/CMakeCache.txt")" && test -d "$tab" && man=/src/third_party/nfse-xsd/prodrest-20261124/SHA256SUMS && test -f "$man" && NL_ENSAIO_CLI="$cli" NL_ENSAIO_XSD_DIR="$xsd" NL_ENSAIO_XSD_MANIFESTO="$man" NL_ENSAIO_TABELAS_DIR="$tab" bash tools/producao_restrita/emitir_teste.sh --ensaio'
# NL-3.L (revisão de QA de 27/11/2026, B1; NF-33 e L-36): o controle negativo do mesmo passo do ci.yml, depois do caso real.
# O controle roda dentro do mesmo contêiner, com o mesmo uid e o mesmo HOME. Ele é gravado na cópia da árvore ($D), montada ro em /src.
cat > "$D/controle_ensaio_xsd.sh" <<'FIM_CONTROLE'
#!/bin/sh
# Controle negativo do ensaio xsd (espelho do passo "Controle negativo do ensaio xsd" do ci.yml). Sai 0 só se os dois casos mordem.
set -u
b=/var/tmp/notalivre-build/debug
cli="$b/apps/cli/notalivre-cli"
xsd="$(sed -n 's/^NL_XSD_PRODREST_DIR:INTERNAL=//p' "$b/CMakeCache.txt")"
tab="$(sed -n 's/^NL_TABELAS_DIR:INTERNAL=//p' "$b/CMakeCache.txt")"
man=/src/third_party/nfse-xsd/prodrest-20261124/SHA256SUMS
test -x "$cli" && test -d "$xsd" && test -d "$tab" && test -f "$man" || { echo "::error::controle negativo sem CLI, XSD, tabelas ou manifesto no build"; exit 1; }
ctl=/tmp/controle-xsd
mkdir -p "$ctl" /tmp/home || exit 1
printf '%s\n' '#!/bin/sh' 'echo "erro XSD_CONTROLE: cli falsa do controle negativo" >&2; exit 1' > "$ctl/cli-falsa"
chmod 755 "$ctl/cli-falsa"
falhas=""
rc1=0
env -u NL_ENSAIO_CLI NL_ENSAIO_XSD_DIR="$xsd" NL_ENSAIO_XSD_MANIFESTO="$man" NL_ENSAIO_TABELAS_DIR="$tab" bash tools/producao_restrita/emitir_teste.sh --ensaio > "$ctl/saida-sem-cli.txt" 2>&1 || rc1=$?
echo "controle 1 (sem NL_ENSAIO_CLI): rc=$rc1 (esperado 11)"
grep -F 'xsd_24_11' "$ctl/saida-sem-cli.txt" || true
if [ "$rc1" -eq 11 ] && grep -qF 'FALHA xsd_24_11: não conferi a DPS de teste no modelo de 24/11/2026. Faltam caminhos' "$ctl/saida-sem-cli.txt"; then
    echo "controle 1 mordeu"
else
    falhas="$falhas controle 1 (sem NL_ENSAIO_CLI: esperado rc 11 e a linha FALHA xsd_24_11 dos caminhos, veio rc $rc1);"
fi
rc2=0
NL_ENSAIO_CLI="$ctl/cli-falsa" NL_ENSAIO_XSD_DIR="$xsd" NL_ENSAIO_XSD_MANIFESTO="$man" NL_ENSAIO_TABELAS_DIR="$tab" bash tools/producao_restrita/emitir_teste.sh --ensaio > "$ctl/saida-cli-falsa.txt" 2>&1 || rc2=$?
echo "controle 2 (CLI falsa que reprova): rc=$rc2 (esperado 9)"
grep -F 'xsd_24_11' "$ctl/saida-cli-falsa.txt" || true
if [ "$rc2" -eq 9 ] && grep -qF 'FALHA xsd_24_11: o programa reprovou a DPS de teste no modelo de 24/11/2026 (códigos: XSD_CONTROLE)' "$ctl/saida-cli-falsa.txt"; then
    echo "controle 2 mordeu"
else
    falhas="$falhas controle 2 (CLI falsa que reprova: esperado rc 9 e a linha de reprovação com o código XSD_CONTROLE, veio rc $rc2);"
fi
if [ -n "$falhas" ]; then
    echo "::error::controle negativo do ensaio xsd NÃO mordeu:$falhas"
    exit 1
fi
exit 0
FIM_CONTROLE
chmod 755 "$D/controle_ensaio_xsd.sh"
passo "ensaio-xsd-controle-nativo" "${CONT[@]}" sh /src/controle_ensaio_xsd.sh
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "ESPELHO ensaio-xsd-nativo: VERDE"; exit 0; }
echo "ESPELHO ensaio-xsd-nativo: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
