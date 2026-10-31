#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de testes (NL-2.14, DA-2.61 adendo 3 e adendo 4; L-36). Mundo fechado: a lista de testes vem de
# ctest --show-only=json-v1; cada teste tem junitxml por função ou está na lista de exceções, que só aceita interpretador
# da allowlist; todo pulo tem de estar na lista; e a contagem de funções é exata por distro. Cada caso monta um CTest de
# mentira (NL_CTEST_JSON_ARQ) e uma pasta de build com junitxml sintético. O caso diz o rc esperado e um trecho da saída.
# Sem prova de vermelho o portão não conta.
#
# Uso: autoteste_pulos_testes.sh     (usa o portão da árvore onde este arquivo está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PORTAO="$RAIZ/tools/portoes/pulos_testes.sh"
ALLOW="$RAIZ/tests/interpretadores_permitidos.txt"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-pulos.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
CASOS=0
FALHAS=0

# junitxml de mentira: xml ARQUIVO TESTS, com os registros <testcase> da entrada padrão.
xml() {
    {
        printf '%s\n' '<?xml version="1.0" encoding="UTF-8" ?>'
        printf '%s\n' "<testsuite name=\"TstX\" timestamp=\"2026-10-31T00:00:00\" tests=\"$2\" failures=\"0\" errors=\"0\" skipped=\"0\" time=\"0.001\">"
        printf '%s\n' '  <properties>' '  </properties>'
        cat
        printf '%s\n' '</testsuite>'
    } > "$1"
}

verdes3() {
    printf '%s\n' \
        '  <testcase name="initTestCase" classname="TstX" time="0.000"/>' \
        '  <testcase name="funcaoA" classname="TstX" time="0.000"/>' \
        '  <testcase name="cleanupTestCase" classname="TstX" time="0.000"/>'
}

pulada_registro() {
    printf '%s\n' "  <testcase name=\"$1\" classname=\"TstX\" time=\"0.000\">" "    <skipped message=\"$2\"/>" "  </testcase>"
}

# JSON de mentira no formato do ctest --show-only=json-v1 (objeto em indentação 4; command em 6, com o primeiro item em 8;
# name em 6). Cada argumento é NOME ou NOME=CAMINHO_DO_PRIMEIRO_ITEM_DO_COMMAND (padrão /x/NOME, executável compilado).
# json ARQUIVO ITEM...
json() {
    local arq="$1" item nome cmd
    shift
    {
        printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "tests" :' '  ['
        for item in "$@"; do
            nome="${item%%=*}"
            if [ "$nome" = "$item" ]; then cmd="/x/$nome"; else cmd="${item#*=}"; fi
            printf '%s\n' '    {' '      "backtrace" : 2,' '      "command" :' '      [' "        \"$cmd\"," '        "-o"' '      ],' "      \"name\" : \"$nome\"" '    },'
        done
        printf '%s\n' '  ]' '}'
    } > "$arq"
}

pasta() {
    mkdir -p "$BASE/$1/junit"
    echo "$BASE/$1"
}

# Contagens esperadas de funções (tests/funcoes_esperadas_por_distro.txt de mentira): fedora44 N.
func() {
    local arq="$BASE/func_$1.txt"
    printf '%s\n' '# contagem de funções (fixture)' "fedora44 $1" > "$arq"
    echo "$arq"
}
F3="$(func 3)"
F2="$(func 2)"
F4="$(func 4)"
F6="$(func 6)"
FOUTRA="$BASE/func_outra.txt"
printf '%s\n' '# contagem de funções (fixture)' 'mint22.3 3' > "$FOUTRA"

VAZIA="$BASE/vazia.txt"
printf '%s\n' '# sem linhas' > "$VAZIA"

# caso NOME RC_ESPERADO TRECHO DIR DISTRO PULOS EXCECOES JSON FUNCOES [OBRIG]
caso() {
    local nome="$1" esperado="$2" trecho="$3" dir="$4" distro="$5" pulos="$6" excecoes="$7" json="$8" funcs="$9"
    local obrig="${10:-$VAZIA}"
    local saida rc
    CASOS=$((CASOS + 1))
    saida="$(NL_CTEST_JSON_ARQ="$json" sh "$PORTAO" "$dir" "$distro" "$pulos" "$excecoes" "$obrig" "$funcs" "$ALLOW" 2>&1)"
    rc=$?
    echo "--- caso $nome: rc=$rc (esperado $esperado)"
    printf '%s\n' "$saida" | tail -n 3
    if [ "$rc" -eq "$esperado" ] && printf '%s\n' "$saida" | grep -q -F -- "$trecho"; then
        echo "[VERDE] $nome"
    else
        echo "[VERMELHO] $nome: rc=$rc, esperava rc=$esperado com '$trecho'"
        FALHAS=$((FALHAS + 1))
    fi
}

arquivo_com() {
    local arq="$BASE/$1.txt" l
    shift
    printf '%s\n' '# lista de teste' > "$arq"
    for l in "$@"; do printf '%s\n' "$l" >> "$arq"; done
    echo "$arq"
}

# Controles (têm de passar).
D="$(pasta controle_tudo_certo)"
json "$BASE/j1.json" tst_a tst_b=/usr/bin/python3.14
verdes3 | xml "$D/junit/tst_a.xml" 3
caso controle_script_na_excecao_e_contagem_exata_passa 0 "pulos_testes: VERDE" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex1 'fedora44 tst_b script de teste sem junitxml por função')" "$BASE/j1.json" "$F3"

D="$(pasta controle_pulo_listado)"
json "$BASE/j2.json" tst_a
{ verdes3 | head -n 2; pulada_registro pulada "sem cliente HTTP"; } | xml "$D/junit/tst_a.xml" 3
caso controle_pulo_listado_que_ocorre_passa 0 "pulou=1" "$D" fedora44 \
    "$(arquivo_com pu1 'fedora44 TstX.pulada libxml2 sem cliente HTTP')" "$VAZIA" "$BASE/j2.json" "$F3"

# Estreia do adendo 3 (mantida): cada um reprova.
D="$(pasta teste_sem_funcao_de_registro)"
json "$BASE/j3.json" tst_a tst_raw
verdes3 | xml "$D/junit/tst_a.xml" 3
caso teste_registrado_sem_funcao_reprova 1 "teste sem junitxml fora da lista de exceções: tst_raw" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j3.json" "$F3"

D="$(pasta junitxml_vazio)"
json "$BASE/j4.json" tst_a
: > "$D/junit/tst_a.xml"
caso junitxml_vazio_reprova 1 "junitxml vazio ou incompleto" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j4.json" "$F3"

D="$(pasta qskip_fora_da_lista)"
json "$BASE/j5.json" tst_a
{ verdes3 | head -n 2; pulada_registro pulada "sem cliente HTTP"; } | xml "$D/junit/tst_a.xml" 3
caso qskip_fora_da_lista_reprova 1 "pulo fora da lista" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j5.json" "$F3"

D="$(pasta linha_velha_excecao)"
json "$BASE/j6.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso linha_velha_na_excecao_reprova 1 "linha velha na lista de exceções" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex2 'fedora44 tst_fantasma script que não existe mais')" "$BASE/j6.json" "$F3"

D="$(pasta json_vazio)"
printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "tests" :' '  [' '  ]' '}' > "$BASE/j7.json"
caso json_do_ctest_vazio_reprova 1 "JSON do ctest vazio" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j7.json" "$F3"

D="$(pasta xml_sem_testcase)"
json "$BASE/j8.json" tst_a
xml "$D/junit/tst_a.xml" 0 < /dev/null
caso junitxml_sem_testcase_reprova 1 "junitxml sem testcase" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j8.json" "$F3"

D="$(pasta excecao_com_xml)"
json "$BASE/j9.json" tst_a=/usr/bin/python3.14
verdes3 | xml "$D/junit/tst_a.xml" 3
caso excecao_com_junitxml_reprova 1 "exceção com junitxml" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex3 'fedora44 tst_a script que já gera junitxml')" "$BASE/j9.json" "$F3"

D="$(pasta excecao_sem_motivo)"
json "$BASE/j10.json" tst_a tst_b=/usr/bin/python3.14
verdes3 | xml "$D/junit/tst_a.xml" 3
caso excecao_sem_motivo_reprova 1 "sem motivo" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex4 'fedora44 tst_b')" "$BASE/j10.json" "$F3"

D="$(pasta obrigatorio_ausente)"
json "$BASE/j11.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso obrigatorio_ausente_reprova 1 "teste obrigatório ausente" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j11.json" "$F3" \
    "$(arquivo_com ob1 'fedora44 tst_b prova obrigatória')"

D="$(pasta pulo_listado_nao_ocorre)"
json "$BASE/j12.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso pulo_listado_que_nao_ocorre_reprova 1 "pulo listado que não ocorreu" "$D" fedora44 \
    "$(arquivo_com pu2 'fedora44 TstX.pulada libxml2 sem cliente HTTP')" "$VAZIA" "$BASE/j12.json" "$F3"

D="$(pasta json_formato)"
printf '%s\n' '{' '  "tests" :' '  [' '    {' '      "command" :' '      [' '        "/x/tst_a"' '      ],' '      "name" : "tst_a"' '    },' '    {' '      "command" :' '      [' '        "/x/tst_b"' '      ]' '    }' '  ]' '}' > "$BASE/j13.json"
verdes3 | xml "$D/junit/tst_a.xml" 3
caso formato_json_desconhecido_reprova 1 "formato do JSON do ctest" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j13.json" "$F3"

# Adendo 4 (estreia): V1 de revisão e os casos pedidos. Cada um tem de reprovar.
# N1. Qt Test (executável compilado) na lista de exceções, sem junitxml: reprova pelo nome.
D="$(pasta n1_qt_na_excecao)"
json "$BASE/n1.json" tst_a tst_qt=/x/tst_qt
verdes3 | xml "$D/junit/tst_a.xml" 3
caso qt_test_na_lista_de_excecoes_reprova 1 "executável sem interpretador permitido" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com n1ex 'fedora44 tst_qt script de teste sem junitxml')" "$BASE/n1.json" "$F3"

# N2. Qt Test embrulhado em bash: o teste saiu do junitxml para um bash, e a contagem cai. Reprova pela contagem exata.
D="$(pasta n2_qt_embrulhado_bash)"
json "$BASE/n2.json" tst_a tst_wrap=/usr/bin/bash
verdes3 | xml "$D/junit/tst_a.xml" 3
caso qt_test_embrulhado_em_bash_reprova 1 "contagem de funções: esperadas 6, obtidas 3" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com n2ex 'fedora44 tst_wrap script bash que roda o teste Qt')" "$BASE/n2.json" "$F6"

# N3. junitxml perdido de um teste listado (executável compilado): reprova pela lista (interpretador) antes da contagem.
D="$(pasta n3_xml_perdido_listado)"
json "$BASE/n3.json" tst_a tst_x=/x/tst_x
verdes3 | xml "$D/junit/tst_a.xml" 3
caso junitxml_perdido_de_listado_reprova 1 "executável sem interpretador permitido: tst_x" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com n3ex 'fedora44 tst_x teste que perdeu o junitxml')" "$BASE/n3.json" "$F3"

# N4. contagem com um a mais.
D="$(pasta n4_contagem_a_mais)"
json "$BASE/n4.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso contagem_com_um_a_mais_reprova 1 "contagem de funções: esperadas 2, obtidas 3" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/n4.json" "$F2"

# N5. contagem com um a menos.
D="$(pasta n5_contagem_a_menos)"
json "$BASE/n5.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso contagem_com_um_a_menos_reprova 1 "contagem de funções: esperadas 4, obtidas 3" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/n5.json" "$F4"

# N6. distro sem contagem versionada reprova (fechado).
D="$(pasta n6_sem_contagem)"
json "$BASE/n6.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso distro_sem_contagem_versionada_reprova 1 "sem contagem de funções versionada" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/n6.json" "$FOUTRA"

echo "CASOS=$CASOS FALHAS=$FALHAS"
if [ "$FALHAS" -ne 0 ]; then
    echo "AUTOTESTE: FALHOU"
    exit 1
fi
echo "AUTOTESTE: todos os controles conferem"
exit 0
