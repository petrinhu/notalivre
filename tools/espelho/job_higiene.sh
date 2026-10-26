#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do job `higiene` do ci.yml (R2): os mesmos passos, na mesma imagem, sobre cópia com .git.
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
travar
copiar_arvore "$ESPELHO_TRAB/higiene" || { echo "REPROVA: cópia da árvore falhou"; exit 1; }
garantir_imagem_higiene || { echo "REPROVA: imagem de higiene não construída"; exit 1; }
D="$ESPELHO_TRAB/higiene"
CONT=(docker run --rm --network none -v "$D:/src:ro,z" "$IMAGEM_HIGIENE")
passo "segredos-historico" "${CONT[@]}" sh -c 'git config --global --add safe.directory /src && gitleaks git /src --config /src/.gitleaks.toml --redact --no-banner --exit-code 1'
passo "segredos-arvore" "${CONT[@]}" bash /src/tools/portoes/higiene_segredos.sh /src
passo "travessao" "${CONT[@]}" python3 -I /src/tools/portoes/higiene_travessao.py /src
passo "spdx" "${CONT[@]}" sh -c 'git config --global --add safe.directory /src && python3 -I /src/tools/portoes/spdx.py /src && bash /src/tools/portoes/autoteste_spdx.sh'
passo "hex" "${CONT[@]}" python3 -I /src/tools/portoes/nenhum_hex_fora_do_theme.py /src
passo "tema" "${CONT[@]}" python3 -I /src/tools/tema/gerar_theme.py --raiz /src --verificar
passo "fixacao-ci" "${CONT[@]}" sh -c 'python3 -I /src/tools/portoes/fixacao_ci.py /src && python3 -I /src/tools/portoes/fixacao_ci.py --autoteste'
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "JOB higiene: VERDE"; exit 0; }
echo "JOB higiene: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
