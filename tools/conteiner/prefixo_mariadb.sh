#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# nl_prefixo_mariadb DISTRO: imprime o prefixo do Connector/C da DISTRO, lido de prefixo_mariadb.txt (mesma pasta). Distro
# vazia, fora da tabela ou repetida nela: rc 1 (NL-3.1h, v3.17). Único leitor da tabela. Uso: . prefixo_mariadb.sh
NL_PREFIXO_MARIADB_TABELA="$(dirname "${BASH_SOURCE[0]}")/prefixo_mariadb.txt"
nl_prefixo_mariadb() {
    [ -n "${1:-}" ] || return 1
    awk -v d="$1" '$1 == d { print $2; n++ } END { exit (n == 1 ? 0 : 1) }' "$NL_PREFIXO_MARIADB_TABELA"
}
