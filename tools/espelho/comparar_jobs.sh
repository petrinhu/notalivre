#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Roda o comparador do espelho (comparar_jobs.py) dentro do contêiner de higiene (L-50), com rede para a API.
# Uso: comparar_jobs.sh [RUN_ID]    (sem RUN_ID: último run concluído de onda-0)
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
docker run --rm -v "$RAIZ:/src:ro,z" -e TMPDIR=/var/tmp notalivre-higiene:ci python3 -I /src/tools/espelho/comparar_jobs.py /src "${1:-}"
