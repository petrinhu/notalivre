#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão codigo_de_saida (NL-3.A; L-36). Precisa do ctest real: é ele quem prova que o código de saída conta.
exec python3 -I "$(dirname "${BASH_SOURCE[0]}")/codigo_de_saida.py" --autoteste
