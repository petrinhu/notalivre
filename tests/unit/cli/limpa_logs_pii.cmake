# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Limpeza da pasta de logs da CLI (NL-2.14, adendo 5): sem -E no command, o caminho vem pelo ambiente.
file(REMOVE_RECURSE "$ENV{NL_LOGS_PII_CLI_DIR}")
