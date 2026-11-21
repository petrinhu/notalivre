#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Corpo do executor do banco (NL-3.1h, v3.19), DENTRO do contêiner que banco/executor.sh sobe: o ctest do preset banco e,
# sempre, a linha NL-PIDS do próprio contêiner, o pico do uid 1000. Ela é lida de dentro porque o --rm apaga o cgroup antes
# de o host conseguir ler. A linha sai também com o ctest vermelho, e o trap EXIT não muda o rc (molde:
# executar_no_conteiner.sh). Roda com bash, nunca sh (no Ubuntu e no Mint, sh é dash).
# Uso: bash /src/tools/conteiner/ctest_banco_no_conteiner.sh     (cwd: /src)
set -u
. "$(dirname "${BASH_SOURCE[0]}")/medir_pids_conteiner.sh"
trap 'nl_pids_linha' EXIT
mkdir -p "$HOME" || exit 2
# NL-3.1a (H-G): o preset vem de NL_PRESET_BANCO (banco, padrão, ou banco-asan), passado pelo executor.
ctest --preset "${NL_PRESET_BANCO:-banco}" --no-tests=error --output-on-failure --verbose
