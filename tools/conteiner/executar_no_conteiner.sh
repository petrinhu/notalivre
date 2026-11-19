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
# NL-3.1h (v3.11): o build roda com --parallel explícito, vindo do build_local.sh (THREADS, de 1 a 7). Sem a variável, ou fora
# dessa faixa, a execução para antes de qualquer cmake.
if ! [[ "${CMAKE_BUILD_PARALLEL_LEVEL:-}" =~ ^[1-7]$ ]]; then
    echo "PARADA: CMAKE_BUILD_PARALLEL_LEVEL '${CMAKE_BUILD_PARALLEL_LEVEL:-ausente}' fora de 1 a 7 (NL-3.1h)"
    exit 2
fi
# NL-3.1h: a linha NL-PIDS sai sempre, também com build ou ctest vermelho. O trap EXIT não muda o rc do script.
. tools/conteiner/medir_pids_conteiner.sh
trap 'nl_pids_linha' EXIT
# NL-3.1h (v3.17): no preset banco, o Connector/C vem do prefixo declarado da distro (prefixo_mariadb.txt), que o CMake lê de
# NL_MARIADB_PREFIX. Distro fora da tabela é PARADA, antes de qualquer cmake.
if [ "$modo" = "banco" ]; then
    . tools/conteiner/prefixo_mariadb.sh
    NL_MARIADB_PREFIX="$(nl_prefixo_mariadb "${NL_DISTRO:-}")" || { echo "PARADA: distro '${NL_DISTRO:-}' sem prefixo do Connector/C em prefixo_mariadb.txt (NL-3.1h, v3.17)"; exit 2; }
    export NL_MARIADB_PREFIX
fi
cmake --preset "$modo" && cmake --build --preset "$modo" --parallel "$CMAKE_BUILD_PARALLEL_LEVEL" || exit $?
# NL-3.1h (v3.3, seção 1.3, item 3): o preset banco só configura e compila aqui. O ctest do banco roda no harness
# integracao_banco.sh, e pulos e contagem da suíte normal não se aplicam a ele. O portão de funções segue valendo.
if [ "$modo" = "banco" ]; then
    banco_rc=0
    python3 -I tools/portoes/funcoes_no_ctest.py --raiz . --build "$NL_TMP_BASE/notalivre-build/$modo" --excecoes tests/funcoes_fora_do_ctest.txt || banco_rc=$?
    echo "NL-BANCO-BUILD: configure e build; ctest só no harness"
    exit "$banco_rc"
fi
# NL-3.0h (L-36): no preset de sanitizer, o portão prova pelo binário e pelo canário que o sanitizer e o gate estão ativos. O
# ctest roda mesmo com o portão vermelho, para o log trazer as duas coisas.
sanitizer_rc=0
if [ "$modo" = "asan-ubsan" ]; then
    python3 -I tools/portoes/sanitizer_ativo.py "$NL_TMP_BASE/notalivre-build/$modo" CMakePresets.json || sanitizer_rc=$?
fi
ctest_rc=0
ctest --preset "$modo" --output-on-failure --no-tests=error || ctest_rc=$?
# NL-2.14 (DA-2.61): o pulo é lido função a função no junitxml, e o portão sai com as três contagens sempre. NL_DISTRO vazio
# reprova (fechado). O portão roda mesmo com ctest vermelho, para o log mostrar as contagens.
pulos_rc=0
sh tools/portoes/pulos_testes.sh "$NL_TMP_BASE/notalivre-build/$modo" "${NL_DISTRO:-}" tests/pulos_esperados_por_distro.txt || pulos_rc=$?
# NL-2.15 (DA-2.65): testes registrados no nativo e ausentes no build Flatpak (NL_FLATPAK_BUILD=ON) têm de estar na lista
# versionada, por igualdade exata. Roda também com ctest ou pulos vermelhos, para o log trazer as contagens.
fora_rc=0
sh tools/portoes/fora_do_flatpak.sh "$NL_TMP_BASE/notalivre-build/$modo" flatpak/testes_fora_do_flatpak.txt || fora_rc=$?
# NL-2.20 (S2): toda função declarada num programa que o CTest chama por função está registrada, salvo exceção listada
# em tests/funcoes_fora_do_ctest.txt. Roda também com os portões acima vermelhos, para o log trazer as contagens.
funcoes_rc=0
python3 -I tools/portoes/funcoes_no_ctest.py --raiz . --build "$NL_TMP_BASE/notalivre-build/$modo" --excecoes tests/funcoes_fora_do_ctest.txt || funcoes_rc=$?
[ "$ctest_rc" -eq 0 ] || exit "$ctest_rc"
[ "$sanitizer_rc" -eq 0 ] || exit "$sanitizer_rc"
[ "$pulos_rc" -eq 0 ] || exit "$pulos_rc"
[ "$fora_rc" -eq 0 ] || exit "$fora_rc"
exit "$funcoes_rc"
