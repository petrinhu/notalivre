#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de pulos e do registro Qt Test (NL-2.14, DA-2.61, revisão devops ALTA-1; L-36). Cada caso monta uma
# pasta de build de mentira (junitxml sintético, CTestTestfile.cmake e executaveis_qt.txt) e chama tools/portoes/pulos_testes.sh.
# O caso diz o rc esperado e um trecho da saída. Sem prova de vermelho o portão não conta: rodar este autoteste contra um
# portão que sempre passa tem de reprovar os casos que esperam rc 1 (sabotagem de estreia).
#
# Uso: autoteste_pulos_testes.sh     (usa o portão da árvore onde este arquivo está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PORTAO="$RAIZ/tools/portoes/pulos_testes.sh"
ESTATICO="$RAIZ/tools/portoes/registro_qt_estatico.sh"
LISTA_REAL="$RAIZ/tests/pulos_esperados_por_distro.txt"
# O portão lê a árvore de testes do cwd (checagem estática do registro): o autoteste usa a árvore real.
export PULOS_RAIZ="$RAIZ"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-pulos.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
CASOS=0
FALHAS=0

# Gera um junitxml de mentira: xml ARQUIVO TESTS, com os registros <testcase> lidos da entrada padrão.
xml() {
    {
        printf '%s\n' '<?xml version="1.0" encoding="UTF-8" ?>'
        printf '%s\n' "<testsuite name=\"TstX\" timestamp=\"2026-10-31T00:00:00\" tests=\"$2\" failures=\"0\" errors=\"0\" skipped=\"0\" time=\"0.001\">"
        printf '%s\n' '  <properties>' '  </properties>'
        cat
        printf '%s\n' '</testsuite>'
    } > "$1"
}

# Pasta de build de mentira: pasta_build NOME; cria NOME/junit/ e o manifesto vazio (o caso acrescenta).
pasta_build() {
    mkdir -p "$BASE/$1/junit"
    : > "$BASE/$1/junit/esperados.txt"
    echo "$BASE/$1"
}

# Monta o CTest e a lista de executáveis Qt de mentira. Por padrão: um add_test por nome do manifesto, e os mesmos nomes
# como executáveis Qt. Um caso pode trazer junit/ctest_manual.txt ou junit/executaveis_manual.txt para substituir cada um.
gera_ctest() {
    local d="$1" n
    mkdir -p "$d/tests"
    if [ -r "$d/junit/ctest_manual.txt" ]; then
        cp "$d/junit/ctest_manual.txt" "$d/tests/CTestTestfile.cmake"
    else
        : > "$d/tests/CTestTestfile.cmake"
        while IFS= read -r n; do
            [ -n "$n" ] || continue
            printf 'add_test([=[%s]=] "/x/%s" "-o" "-,txt")\n' "$n" "$n" >> "$d/tests/CTestTestfile.cmake"
        done < "$d/junit/esperados.txt"
    fi
    if [ -r "$d/junit/executaveis_manual.txt" ]; then
        cp "$d/junit/executaveis_manual.txt" "$d/junit/executaveis_qt.txt"
    else
        cp "$d/junit/esperados.txt" "$d/junit/executaveis_qt.txt"
    fi
}

# Registro de um teste de mentira: verdes3 (init, uma função, cleanup) e o registro de pulo.
verdes3() {
    printf '%s\n' \
        '  <testcase name="initTestCase" classname="TstX" time="0.000"/>' \
        '  <testcase name="funcaoA" classname="TstX" time="0.000"/>' \
        '  <testcase name="cleanupTestCase" classname="TstX" time="0.000"/>'
}

# Registro de pulo: pulada_registro NOME MOTIVO
pulada_registro() {
    printf '%s\n' "  <testcase name=\"$1\" classname=\"TstX\" time=\"0.000\">" "    <skipped message=\"$2\"/>" "  </testcase>"
}

OBRIG_VAZIO="$BASE/obrig_vazio.txt"
printf '%s\n' '# obrigatórios vazios' > "$OBRIG_VAZIO"

# caso NOME RC_ESPERADO TRECHO DIR DISTRO LISTA [OBRIG]: monta o CTest de mentira, roda o portão e confere rc e trecho.
caso() {
    local nome="$1" esperado="$2" trecho="$3" dir="$4" distro="$5" lista="$6" obrig="${7:-$OBRIG_VAZIO}"
    local saida rc
    CASOS=$((CASOS + 1))
    gera_ctest "$dir"
    saida="$(sh "$PORTAO" "$dir" "$distro" "$lista" "$obrig" 2>&1)"
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

# Lista de teste: uma linha de pulo esperado (distro teste motivo).
lista_com() {
    local arq="$BASE/$1.txt"
    printf '%s\n' '# lista de teste' > "$arq"
    printf '%s\n' "$2" >> "$arq"
    echo "$arq"
}
LISTA_VAZIA="$BASE/lista_vazia.txt"
printf '%s\n' '# lista vazia' > "$LISTA_VAZIA"

# 1. Controle verde: zero pulos, lista vazia, três funções verdes. O resumo com as três contagens sai sempre.
D="$(pasta_build verde)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
caso lista_vazia_zero_pulos_passa 0 "passou=3 falhou=0 pulou=0" "$D" fedora44 "$LISTA_VAZIA"

# 2. Pulo fora da lista reprova (o caso que o CTest contava como passou).
D="$(pasta_build pulo_fora)"; echo tst_a >> "$D/junit/esperados.txt"
{ verdes3 | head -n 2; pulada_registro pulada "sem cliente HTTP"; } | xml "$D/junit/tst_a.xml" 3
caso pulo_fora_da_lista_reprova 1 "pulo fora da lista" "$D" fedora44 "$LISTA_VAZIA"

# 3. Pulo listado para a distro, que de fato ocorre, passa.
D="$(pasta_build pulo_listado)"; echo tst_a >> "$D/junit/esperados.txt"
{ verdes3 | head -n 2; pulada_registro pulada "sem cliente HTTP"; } | xml "$D/junit/tst_a.xml" 3
L="$(lista_com lista_pulo_listado "fedora44 TstX.pulada libxml2 sem cliente HTTP")"
caso pulo_listado_que_ocorre_passa 0 "pulou=1" "$D" fedora44 "$L"

# 4. Pulo listado que não ocorre reprova (a lista não pode ter linha morta).
D="$(pasta_build listado_nao_ocorre)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
L="$(lista_com lista_morta "fedora44 TstX.pulada libxml2 sem cliente HTTP")"
caso pulo_listado_que_nao_ocorre_reprova 1 "pulo listado que não ocorreu" "$D" fedora44 "$L"

# 5. Arquivo junitxml ausente reprova (teste registrado que não gravou resultado).
D="$(pasta_build xml_ausente)"; printf '%s\n' tst_a tst_b >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
caso junitxml_ausente_reprova 1 "xml ausente para o teste tst_b" "$D" fedora44 "$LISTA_VAZIA"

# 6. Arquivo junitxml incompleto (processo morreu no meio) reprova.
D="$(pasta_build xml_incompleto)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 > "$BASE/incompleto_corpo.txt"
{ printf '%s\n' '<?xml version="1.0" encoding="UTF-8" ?>' '<testsuite name="TstX" tests="3">'; cat "$BASE/incompleto_corpo.txt"; } > "$D/junit/tst_a.xml"
caso junitxml_incompleto_reprova 1 "xml incompleto" "$D" fedora44 "$LISTA_VAZIA"

# 7. Contagem do testsuite diferente dos registros lidos reprova (xml adulterado ou truncado).
D="$(pasta_build contagem_errada)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 5
caso contagem_inconsistente_reprova 1 "xml inconsistente" "$D" fedora44 "$LISTA_VAZIA"

# 8. Função que falhou reprova, e a contagem sai com falhou=1.
D="$(pasta_build falha)"; echo tst_a >> "$D/junit/esperados.txt"
{ verdes3 | head -n 2; printf '%s\n' '  <testcase name="quebrada" classname="TstX" time="0.000">' '    <failure message="esperado 1, obtido 2" type="fail"/>' '  </testcase>' '  <testcase name="cleanupTestCase" classname="TstX" time="0.000"/>'; } | xml "$D/junit/tst_a.xml" 4
caso funcao_que_falhou_reprova 1 "falhou=1" "$D" fedora44 "$LISTA_VAZIA"

# 9. Sem distro não há lista de pulos esperados: reprova (fechado), nunca usa uma lista por omissão.
D="$(pasta_build sem_distro)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
caso distro_ausente_reprova 1 "distro ausente" "$D" "" "$LISTA_VAZIA"

# 10. Pulo listado para OUTRA distro não vale aqui: na distro atual é pulo fora da lista.
D="$(pasta_build lista_de_outra)"; echo tst_a >> "$D/junit/esperados.txt"
{ verdes3 | head -n 2; pulada_registro pulada "sem cliente HTTP"; } | xml "$D/junit/tst_a.xml" 3
L="$(lista_com lista_outra_distro "ubuntu26.04 TstX.pulada libxml2 sem cliente HTTP")"
caso pulo_listado_de_outra_distro_nao_vale 1 "pulo fora da lista" "$D" fedora44 "$L"

# 11. Linha da lista sem motivo reprova: o motivo é documentação obrigatória do pulo.
D="$(pasta_build sem_motivo)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
L="$(lista_com lista_sem_motivo "fedora44 TstX.pulada")"
caso linha_sem_motivo_reprova 1 "sem motivo" "$D" fedora44 "$L"

# 12. Manifesto vazio reprova: zero funções registradas não é varredura.
D="$(pasta_build manifesto_vazio)"
caso manifesto_vazio_reprova 1 "manifesto vazio" "$D" fedora44 "$LISTA_VAZIA"

# 13. Lista real do repositório: com zero pulos e lista vazia, o portão passa. Prova que o arquivo versionado parseia.
D="$(pasta_build lista_real)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
caso lista_real_do_repo_zero_pulos_passa 0 "pulou=0" "$D" fedora44 "$LISTA_REAL"

# 13b. Formato de CTestTestfile.cmake com aspas (o CMake do manjaro grava assim; o das outras imagens grava [=[NOME]=]).
# Os dois têm de ser lidos: o portão de registro não pode depender da versão do CMake (rodada de 31/10 06:57, manjaro).
D="$(pasta_build formato_aspas)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
printf '%s\n' 'add_test("tst_a" "/x/tst_a" "-o" "-,txt")' > "$D/junit/ctest_manual.txt"
printf '%s\n' tst_a > "$D/junit/executaveis_manual.txt"
caso formato_ctest_com_aspas_passa 0 "registro_qt: VERDE" "$D" fedora44 "$LISTA_VAZIA"

# 13c. Formato de CTestTestfile.cmake que o portão não reconhece reprova (fechado: linha de add_test sem leitura é varredura
# falsa), com o motivo de formato, e não com "zero testes".
D="$(pasta_build formato_desconhecido)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
printf '%s\n' 'add_test(tst_a)' > "$D/junit/ctest_manual.txt"
printf '%s\n' tst_a > "$D/junit/executaveis_manual.txt"
caso formato_ctest_desconhecido_reprova 1 "formato do CTestTestfile.cmake não reconhecido" "$D" fedora44 "$LISTA_VAZIA"

# 14. (revisão devops ALTA-1, M1c) Qt Test que sai do registro único e entra por add_test direto: está no CTest, não está
# no manifesto. O CTest conta o pulo como passou; só a conferência CTest × manifesto pega. Reprova com o nome.
D="$(pasta_build qt_fora_do_registro)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
printf '%s\n' 'add_test([=[tst_a]=] "/x/tst_a" "-o" "-,txt")' 'add_test([=[tst_raw]=] "/x/tst_raw")' > "$D/junit/ctest_manual.txt"
printf '%s\n' tst_a tst_raw > "$D/junit/executaveis_manual.txt"
caso qt_fora_do_registro_reprova 1 "fora do registro" "$D" fedora44 "$LISTA_VAZIA"

# 15. Registrado no manifesto e ausente do CTest reprova (teste removido do CTest com xml antigo).
D="$(pasta_build registrado_fora_do_ctest)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
printf '%s\n' 'add_test([=[outro]=] "/x/outro")' > "$D/junit/ctest_manual.txt"
printf '%s\n' tst_a > "$D/junit/executaveis_manual.txt"
caso registrado_fora_do_ctest_reprova 1 "fora do CTest" "$D" fedora44 "$LISTA_VAZIA"

# 16. Lista de executáveis Qt vazia reprova (zero executáveis conferidos não é varredura).
D="$(pasta_build exec_vazio)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
: > "$D/junit/executaveis_manual.txt"
caso executaveis_qt_vazio_reprova 1 "nenhum executável Qt Test" "$D" fedora44 "$LISTA_VAZIA"

# 17. Teste obrigatório da distro ausente do CTest reprova (a prova de rede não pode sumir em silêncio).
D="$(pasta_build obrigatorio_ausente)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
O="$BASE/obrig_ausente.txt"; printf '%s\n' '# obrigatórios' 'fedora44 tst_b prova obrigatória de uma distro' > "$O"
caso obrigatorio_ausente_reprova 1 "teste obrigatório ausente" "$D" fedora44 "$LISTA_VAZIA" "$O"

# 18. Teste obrigatório presente e passando passa.
D="$(pasta_build obrigatorio_presente)"; echo tst_a >> "$D/junit/esperados.txt"
verdes3 | xml "$D/junit/tst_a.xml" 3
O="$BASE/obrig_presente.txt"; printf '%s\n' '# obrigatórios' 'fedora44 tst_a prova obrigatória de uma distro' > "$O"
caso obrigatorio_presente_passa 0 "VERDE" "$D" fedora44 "$LISTA_VAZIA" "$O"

# 19. Checagem estática: add_test de Qt Test fora da função reprova (reforço da conferência, revisão devops ALTA-1).
F="$BASE/fonte_ruim"; mkdir -p "$F/tests/x"
printf '%s\n' 'add_test(NAME tst_raw' '         COMMAND tst_raw)' > "$F/tests/x/CMakeLists.txt"
saida="$(sh "$ESTATICO" "$F" 2>&1)"; rc=$?
CASOS=$((CASOS + 1))
echo "--- caso estatico_add_test_qt_reprova: rc=$rc (esperado 1)"
if [ "$rc" -eq 1 ] && printf '%s\n' "$saida" | grep -q -F -- "add_test de Qt Test fora da função"; then
    echo "[VERDE] estatico_add_test_qt_reprova"
else
    echo "[VERMELHO] estatico_add_test_qt_reprova: rc=$rc, esperava rc=1 com 'add_test de Qt Test fora da função'"
    FALHAS=$((FALHAS + 1))
fi

# 20. Checagem estática sobre a árvore limpa passa (nenhum add_test de tst_ fora da função).
F="$BASE/fonte_limpa"; mkdir -p "$F/tests/x"
printf '%s\n' 'nl_registra_teste_qt(NAME tst_ok COMMAND tst_ok)' 'add_test(NAME python.ok COMMAND python3 -I x.py)' > "$F/tests/x/CMakeLists.txt"
saida="$(sh "$ESTATICO" "$F" 2>&1)"; rc=$?
CASOS=$((CASOS + 1))
echo "--- caso estatico_fonte_limpa_passa: rc=$rc (esperado 0)"
if [ "$rc" -eq 0 ]; then
    echo "[VERDE] estatico_fonte_limpa_passa"
else
    echo "[VERMELHO] estatico_fonte_limpa_passa: rc=$rc, esperava rc=0"
    FALHAS=$((FALHAS + 1))
fi

echo "CASOS=$CASOS FALHAS=$FALHAS"
if [ "$FALHAS" -ne 0 ]; then
    echo "AUTOTESTE: FALHOU"
    exit 1
fi
echo "AUTOTESTE: todos os controles conferem"
exit 0
