#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de testes (NL-2.14, DA-2.61 adendo 3; L-36). Desenho de mundo fechado: a lista de testes vem de
# ctest --show-only=json-v1, cada teste tem de ter junitxml por função (ou estar na lista de exceções, por igualdade exata),
# e todo pulo tem de estar na lista de pulos. Cada caso monta um CTest de mentira (NL_CTEST_JSON_ARQ) e uma pasta de build
# com junitxml sintético. O caso diz o rc esperado e um trecho da saída. Sem prova de vermelho o portão não conta.
#
# Uso: autoteste_pulos_testes.sh     (usa o portão da árvore onde este arquivo está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PORTAO="$RAIZ/tools/portoes/pulos_testes.sh"
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

# JSON de mentira no formato do ctest --show-only=json-v1 (objeto por teste em indentação 4, nome e comando em 6):
# json ARQUIVO NOME...
json() {
    local arq="$1" n
    shift
    {
        printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "tests" :' '  ['
        for n in "$@"; do
            printf '%s\n' '    {' '      "command" :' '      [' "        \"/x/$n\"" '      ],' "      \"name\" : \"$n\"" '    },'
        done
        printf '%s\n' '  ]' '}'
    } > "$arq"
}

# Pasta de build de mentira: pasta NOME (cria junit/).
pasta() {
    mkdir -p "$BASE/$1/junit"
    echo "$BASE/$1"
}

VAZIA="$BASE/vazia.txt"
printf '%s\n' '# sem linhas' > "$VAZIA"

# caso NOME RC_ESPERADO TRECHO DIR DISTRO PULOS EXCECOES JSON [OBRIG]
caso() {
    local nome="$1" esperado="$2" trecho="$3" dir="$4" distro="$5" pulos="$6" excecoes="$7" json="$8" obrig="${9:-$VAZIA}"
    local saida rc
    CASOS=$((CASOS + 1))
    saida="$(NL_CTEST_JSON_ARQ="$json" sh "$PORTAO" "$dir" "$distro" "$pulos" "$excecoes" "$obrig" 2>&1)"
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

# Arquivo de lista com as linhas dadas: arquivo_com NOME LINHA...
arquivo_com() {
    local arq="$BASE/$1.txt" l
    shift
    printf '%s\n' '# lista de teste' > "$arq"
    for l in "$@"; do printf '%s\n' "$l" >> "$arq"; done
    echo "$arq"
}

# Controles (têm de passar).
D="$(pasta controle_tudo_certo)"
json "$BASE/j_controle_tudo.json" tst_a tst_b
verdes3 | xml "$D/junit/tst_a.xml" 3
caso controle_script_na_excecao_e_junit_passa 0 "pulos_testes: VERDE" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex_controle 'fedora44 tst_b script de teste sem junitxml por função')" "$BASE/j_controle_tudo.json"

D="$(pasta controle_pulo_listado)"
json "$BASE/j_pulo_listado.json" tst_a
{ verdes3 | head -n 2; pulada_registro pulada "sem cliente HTTP"; } | xml "$D/junit/tst_a.xml" 3
caso controle_pulo_listado_que_ocorre_passa 0 "pulou=1" "$D" fedora44 \
    "$(arquivo_com pu_listado 'fedora44 TstX.pulada libxml2 sem cliente HTTP')" "$VAZIA" "$BASE/j_pulo_listado.json"

# 1. Teste registrado sem a função de registro: está no CTest, não tem junitxml, e não está na lista de exceções.
D="$(pasta teste_sem_funcao_de_registro)"
json "$BASE/j_sem_registro.json" tst_a tst_raw
verdes3 | xml "$D/junit/tst_a.xml" 3
caso teste_registrado_sem_funcao_reprova 1 "teste sem junitxml fora da lista de exceções: tst_raw" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j_sem_registro.json"

# 2. junitxml vazio (0 bytes) reprova.
D="$(pasta junitxml_vazio)"
json "$BASE/j_vazio_xml.json" tst_a
: > "$D/junit/tst_a.xml"
caso junitxml_vazio_reprova 1 "junitxml vazio ou incompleto" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j_vazio_xml.json"

# 3. QSKIP fora da lista reprova.
D="$(pasta qskip_fora_da_lista)"
json "$BASE/j_qskip.json" tst_a
{ verdes3 | head -n 2; pulada_registro pulada "sem cliente HTTP"; } | xml "$D/junit/tst_a.xml" 3
caso qskip_fora_da_lista_reprova 1 "pulo fora da lista" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j_qskip.json"

# 4. Linha velha na lista de exceções (não é teste do CTest) reprova.
D="$(pasta linha_velha_excecao)"
json "$BASE/j_velha.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso linha_velha_na_excecao_reprova 1 "linha velha na lista de exceções" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex_velha 'fedora44 tst_fantasma script que não existe mais')" "$BASE/j_velha.json"

# 5. JSON do ctest vazio reprova (zero testes não é varredura).
D="$(pasta json_vazio)"
printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "tests" :' '  [' '  ]' '}' > "$BASE/j_vazio.json"
caso json_do_ctest_vazio_reprova 1 "JSON do ctest vazio" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j_vazio.json"

# 6. junitxml sem nenhum testcase reprova.
D="$(pasta xml_sem_testcase)"
json "$BASE/j_sem_case.json" tst_a
xml "$D/junit/tst_a.xml" 0 < /dev/null
caso junitxml_sem_testcase_reprova 1 "junitxml sem testcase" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j_sem_case.json"

# 7. Exceção que tem junitxml (lista desatualizada) reprova.
D="$(pasta excecao_com_xml)"
json "$BASE/j_excxml.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso excecao_com_junitxml_reprova 1 "exceção com junitxml" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex_com_xml 'fedora44 tst_a script que já gera junitxml')" "$BASE/j_excxml.json"

# 8. Linha de exceção sem motivo reprova.
D="$(pasta excecao_sem_motivo)"
json "$BASE/j_semmot.json" tst_a tst_b
verdes3 | xml "$D/junit/tst_a.xml" 3
caso excecao_sem_motivo_reprova 1 "sem motivo" "$D" fedora44 "$VAZIA" \
    "$(arquivo_com ex_sem_motivo 'fedora44 tst_b')" "$BASE/j_semmot.json"

# 9. Teste obrigatório ausente do CTest reprova.
D="$(pasta obrigatorio_ausente)"
json "$BASE/j_obrig.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso obrigatorio_ausente_reprova 1 "teste obrigatório ausente" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j_obrig.json" \
    "$(arquivo_com ob_ausente 'fedora44 tst_b prova obrigatória')"

# 10. Pulo listado que não ocorre reprova.
D="$(pasta pulo_listado_nao_ocorre)"
json "$BASE/j_naoocorre.json" tst_a
verdes3 | xml "$D/junit/tst_a.xml" 3
caso pulo_listado_que_nao_ocorre_reprova 1 "pulo listado que não ocorreu" "$D" fedora44 \
    "$(arquivo_com pu_morto 'fedora44 TstX.pulada libxml2 sem cliente HTTP')" "$VAZIA" "$BASE/j_naoocorre.json"

# 11. Formato do JSON que o portão não reconhece reprova (dois objetos, um nome).
D="$(pasta json_formato)"
printf '%s\n' '{' '  "tests" :' '  [' '    {' '      "command" :' '      [' '        "/x/tst_a"' '      ],' '      "name" : "tst_a"' '    },' '    {' '      "command" :' '      [' '        "/x/tst_b"' '      ]' '    }' '  ]' '}' > "$BASE/j_formato.json"
verdes3 | xml "$D/junit/tst_a.xml" 3
caso formato_json_desconhecido_reprova 1 "formato do JSON do ctest" "$D" fedora44 "$VAZIA" "$VAZIA" "$BASE/j_formato.json"

echo "CASOS=$CASOS FALHAS=$FALHAS"
if [ "$FALHAS" -ne 0 ]; then
    echo "AUTOTESTE: FALHOU"
    exit 1
fi
echo "AUTOTESTE: todos os controles conferem"
exit 0
