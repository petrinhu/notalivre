#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portão do teto de processos em todo docker run (NL-3.1h, emenda v3.11; L-36): primeiro os casos de fixture, depois a
# varredura da árvore RAIZ. Sem caso verde não há varredura.
# Uso: autoteste_pids_em_docker_run.sh RAIZ   (RAIZ precisa de .git: a varredura usa git ls-files)
set -u
RAIZ="${1:?uso: autoteste_pids_em_docker_run.sh RAIZ}"
PORTAO="$(dirname "${BASH_SOURCE[0]}")/pids_em_docker_run.py"
python3 -I "$PORTAO" --autoteste || { echo "REPROVA: casos do portão de pids reprovados; a varredura não roda"; exit 1; }
python3 -I "$PORTAO" "$RAIZ"
