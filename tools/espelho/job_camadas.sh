#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do job `camadas` do ci.yml (R2): portão A2 e autoteste, em contêiner, sobre cópia da árvore.
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
travar
copiar_arvore "$ESPELHO_TRAB/camadas" || { echo "REPROVA: cópia da árvore falhou"; exit 1; }
garantir_imagem_higiene || { echo "REPROVA: imagem de higiene não construída"; exit 1; }
D="$ESPELHO_TRAB/camadas"
# Como no CI (runner comum): UID do host. Root no contêiner ignora o chmod 000 do autoteste (L-49).
CONT=(docker run --rm --network none --user "$(id -u):$(id -g)" -e HOME=/var/tmp -v "$D:/src:ro,z" -e TMPDIR=/var/tmp "$IMAGEM_HIGIENE")
passo "portao-a2" "${CONT[@]}" python3 -I /src/tools/portoes/camadas.py /src
passo "autoteste-a2" "${CONT[@]}" bash /src/tools/portoes/autoteste_camadas.sh /src
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "JOB camadas: VERDE"; exit 0; }
echo "JOB camadas: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
