#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Stub vermelho da NL-2.1: cria o diretório e não gera nenhum certificado (os testes têm de falhar nos asserts)."""
import sys
from pathlib import Path

Path(sys.argv[1]).mkdir(parents=True, exist_ok=True)
sys.exit(0)
