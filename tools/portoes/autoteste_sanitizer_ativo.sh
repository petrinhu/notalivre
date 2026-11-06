#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão sanitizer_ativo (NL-3.0h; L-36). Precisa do ctest real: é ele quem decide os casos.
exec python3 -I "$(dirname "${BASH_SOURCE[0]}")/sanitizer_ativo.py" --autoteste
