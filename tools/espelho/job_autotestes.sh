#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do job `autotestes` do ci.yml (NL-3.0g, emenda A1 do CTO, §1): um passo por passo do job, na mesma ordem,
# em contêiner notalivre/build-fedora45:nl-2.0, sobre cópia do commit. Nada roda no host (L-50).
# O primeiro passo confere o piso dos portões do próprio commit com a lista do commit: piso errado reprova aqui.
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
travar
copiar_arvore "$ESPELHO_TRAB/autotestes" || { echo "REPROVA: cópia da árvore falhou"; exit 1; }
D="$ESPELHO_TRAB/autotestes"
IMAGEM_BUILD="notalivre/build-fedora45:nl-2.0"
CONT=(docker run --pull=never --rm --network none --user "$(id -u):$(id -g)" -e HOME=/tmp/home -e TMPDIR=/var/tmp \
    -v "$D:/src:ro,z" -w /src "$IMAGEM_BUILD")
passo "piso-dos-portoes-confere" "${CONT[@]}" sh -c \
    'n=$(grep -vc "^#" tools/conteiner/lista_portoes.txt) && p=$(sed -n 1p tools/conteiner/piso_portoes.txt) && echo "piso=$p lista=$n" && [ "$n" = "$p" ]'
passo "autoteste-piso-portoes" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/conteiner/autoteste_piso_portoes.sh'
passo "autoteste-suite-flatpak" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash flatpak/autoteste_suite_flatpak.sh'
passo "autoteste-build-limites" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/conteiner/autoteste_build_limites.sh'
passo "autoteste-pulos-testes" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/portoes/autoteste_pulos_testes.sh'
passo "autoteste-fora-do-flatpak" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/portoes/autoteste_fora_do_flatpak.sh'
passo "autoteste-pontos-de-parse" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/portoes/autoteste_pontos_de_parse.sh'
passo "autoteste-cabecalhos-sodium" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/conteiner/autoteste_cabecalhos_sodium.sh'
passo "autoteste-cabecalhos-openssl" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/conteiner/autoteste_cabecalhos_openssl.sh'
passo "autoteste-build-sha" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/conteiner/autoteste_build_sha.sh'
passo "autoteste-copiar-arvore" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/espelho/autoteste_copiar_arvore.sh'
passo "autoteste-rodar-espelho" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/espelho/autoteste_rodar_espelho.sh'
passo "autoteste-pull-never" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/espelho/autoteste_pull_never.sh /src'
passo "autoteste-sanitizer-ativo" "${CONT[@]}" sh -c 'mkdir -p /tmp/home && bash tools/portoes/autoteste_sanitizer_ativo.sh'
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "JOB autotestes: VERDE"; exit 0; }
echo "JOB autotestes: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
