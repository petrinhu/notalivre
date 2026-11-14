#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portão do teto em todo contêiner (NL-3.K; L-36): primeiro os casos de fixture, depois a varredura da árvore RAIZ.
# Uso: autoteste_cota_em_todo_conteiner.sh RAIZ   (RAIZ precisa de .git: a varredura usa git ls-files)
set -u
RAIZ="${1:?uso: autoteste_cota_em_todo_conteiner.sh RAIZ}"
PORTAO="$(dirname "${BASH_SOURCE[0]}")/cota_em_todo_conteiner.py"
python3 -I "$PORTAO" --autoteste || { echo "REPROVA: casos do portão de teto reprovados; a varredura não roda"; exit 1; }
python3 -I "$PORTAO" "$RAIZ"
