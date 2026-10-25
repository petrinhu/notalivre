#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Corpo que roda DENTRO do contêiner de uma distro (NL-0.4). Dois modos, chamados em duas etapas:
#   diagnostico   tabela de versões (medir_versoes.sh) e pré-cheque do Qt (precheque_qt.sh). Roda como root,
#                 porque o índice do gerenciador (apt/pacman/dnf) só atualiza com root. Só lê; não grava no host.
#   <preset>      configure, build e ctest do preset. Roda como 1000:1000, para o host não receber arquivo de root.
# O ci.yml chama os dois modos na mesma ordem, em steps separados (a mesma sequência do build local).
# sabotagem-sanidade: sabotagem de estreia do tst_sanidade (A0-05), roda sabotar_sanidade.sh na cópia.
# Uso: executar_no_conteiner.sh diagnostico | sabotagem-sanidade | executar_no_conteiner.sh <preset>     (cwd: /src)
set -u

modo="${1:?uso: executar_no_conteiner.sh diagnostico | <preset>}"
cd /src || exit 90

if [ "$modo" = "sabotagem-sanidade" ]; then
    exec bash tools/conteiner/sabotar_sanidade.sh /src
fi

if [ "$modo" = "diagnostico" ]; then
    bash tools/conteiner/medir_versoes.sh
    exec bash tools/conteiner/precheque_qt.sh
fi

: "${NL_TMP_BASE:?defina NL_TMP_BASE (no contêiner, /var/tmp)}"
cmake --preset "$modo" && cmake --build --preset "$modo" && ctest --preset "$modo" --output-on-failure
