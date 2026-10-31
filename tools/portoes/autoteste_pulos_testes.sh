#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de pulos (NL-2.14, DA-2.61; L-36). Cada caso monta uma pasta de build de mentira (junitxml
# sintético, no formato que o QTest grava) e chama tools/portoes/pulos_testes.sh. O caso diz o rc esperado e um trecho
# da saída. Sem prova de vermelho o portão não conta: rodar este autoteste contra um portão que sempre passa tem de
# reprovar os casos que esperam rc 1 (sabotagem de estreia).
#
# Uso: autoteste_pulos_testes.sh     (usa o portão da árvore onde este arquivo está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PORTAO="$RAIZ/tools/portoes/pulos_testes.sh"
LISTA_REAL="$RAIZ/tests/pulos_esperados_por_distro.txt"
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

# Três registros verdes (init, uma função, cleanup), usados por vários casos.
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

# caso NOME RC_ESPERADO TRECHO DIR DISTRO LISTA: roda o portão e confere rc e trecho da saída.
caso() {
    local nome="$1" esperado="$2" trecho="$3" dir="$4" distro="$5" lista="$6" saida rc
    CASOS=$((CASOS + 1))
    saida="$(sh "$PORTAO" "$dir" "$distro" "$lista" 2>&1)"
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

echo "CASOS=$CASOS FALHAS=$FALHAS"
if [ "$FALHAS" -ne 0 ]; then
    echo "AUTOTESTE: FALHOU"
    exit 1
fi
echo "AUTOTESTE: todos os controles conferem"
exit 0
