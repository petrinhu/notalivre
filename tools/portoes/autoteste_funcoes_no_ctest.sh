#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de funções declaradas e registradas no CTest (NL-2.20 S2, S2c, S2e e S2f; L-35, L-36).
# Vinte e cinco casos, cada um com o veredito esperado e um trecho obrigatório da saída: positivo; falta em Qt e em
# Python; sobra; varredura vazia (2 casos); exceção morta, sem motivo e válida; ctest morto por sinal; fonte sem seção;
# linha não reconhecida; fonte ausente; registro duplicado; comando estranho; o fim da seção (S2c): lambda e classe local
# no corpo de um slot, segunda seção private slots e rótulo de acesso; entrada do registro (S2e): item que não é objeto,
# byte nulo no command, JSON aninhado demais, e exceção que vale só para o próprio programa. Os JSON do CTest, os fontes
# e as listas são gerados aqui, numa pasta temporária, e não são arquivos versionados.
# S2f: o portão impõe um limite próprio de profundidade (64) antes do json.loads; o caso 22 não depende mais do
# interpretador, e os casos 24 e 25 testam a borda e um passo além.
# Sem prova de vermelho o portão não conta.
#
# Uso: autoteste_funcoes_no_ctest.sh     (usa o portão da árvore onde este arquivo está)
set -u

RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
PORTAO="$RAIZ/tools/portoes/funcoes_no_ctest.py"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-funcoes-ctest.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
CASOS=0
FALHAS=0
C=""
RC=0

# preparar N: cria a pasta do caso C, a raiz falsa, a pasta de build, a lista vazia e o fonte Python base.
preparar() {
    C="$BASE/c$1"
    mkdir -p "$C/raiz/tests/q" "$C/raiz/tests/p" "$C/build"
    : > "$C/excecoes.txt"
    escreve_py "$C/raiz/tests/p/test_p.py" ""
}

# escreve_qt ARQ EXTRA: fonte Qt base com a seção private slots; EXTRA, se não vazio, entra antes de cleanup.
escreve_qt() {
    {
        printf '%s\n' '// SPDX-License-Identifier: GPL-3.0-or-later' '#include <QtTest>' 'class TstQ : public QObject' '{' '    Q_OBJECT' 'private slots:' '    void initTestCase() {}' '    void umSlot()' '    {' '    }' '    void outroSlot();' '    void outroSlot_data();'
        if [ -n "$2" ]; then printf '%s\n' "$2"; fi
        printf '%s\n' '    void cleanup() {}' '};' 'QTEST_GUILESS_MAIN(TstQ)'
    } > "$1"
}

# escreve_py ARQ EXTRA: fonte Python base com a classe TestCase; EXTRA, se não vazio, entra no fim da classe.
escreve_py() {
    {
        printf '%s\n' '# SPDX-License-Identifier: GPL-3.0-or-later' 'import unittest' '' 'class TesteP(unittest.TestCase):' '    def setUp(self):' '        pass' '    def _aux(self):' '        pass' '    def casoUm(self):' '        pass' '    def casoDois(self):' '        pass'
        if [ -n "$2" ]; then printf '%s\n' "$2"; fi
    } > "$1"
}

# json_arquivo ARQ ITEM...: grava um JSON no layout do ctest json-v1; cada ITEM é um array JSON pronto de command.
json_arquivo() {
    arq="$1"
    shift
    {
        printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "version" : { "major" : 1, "minor" : 0 },' '  "tests" :' '  ['
        n=0
        for cmd in "$@"; do
            n=$((n + 1))
            if [ "$n" -gt 1 ]; then printf '%s\n' ','; fi
            printf '    { "name" : "t%s", "command" : %s }' "$n" "$cmd"
        done
        printf '\n%s\n' '  ]' '}'
    } > "$arq"
}

# json_base [ITEM...]: os seis testes-base do caso C (umSlot, outroSlot, casoUm, casoDois, inteiro e outro) e os ITEM extras.
json_base() {
    json_arquivo "$C/ctest.json" \
        "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.umSlot.xml,junitxml\", \"umSlot\"]" \
        "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.outroSlot.xml,junitxml\", \"outroSlot\"]" \
        "[\"/usr/bin/python3\", \"-I\", \"$C/raiz/tests/p/test_p.py\", \"casoUm\"]" \
        "[\"/usr/bin/python3\", \"-I\", \"$C/raiz/tests/p/test_p.py\", \"casoDois\"]" \
        "[\"$C/build/tests/r/tst_r\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/r.xml,junitxml\"]" \
        "[\"/usr/bin/bash\", \"/x/oraculo.sh\", \"xmllint\"]" \
        "$@"
}

# rodar_com_json: roda o portão sobre o caso C com --json, guarda a saída em saida.txt e o rc em RC.
rodar_com_json() {
    python3 -I "$PORTAO" --raiz "$C/raiz" --build "$C/build" --excecoes "$C/excecoes.txt" --json "$C/ctest.json" > "$C/saida.txt" 2>&1
    RC=$?
}

# avalia N NOME ESPERADO TRECHO: confere o rc e a presença do trecho na saída do caso C; imprime OK ou FALHA com a saída.
avalia() {
    CASOS=$((CASOS + 1))
    achou=0
    while IFS= read -r linha; do
        case "$linha" in
            *"$4"*) achou=1 ;;
        esac
    done < "$C/saida.txt"
    if [ "$RC" -eq "$3" ] && [ "$achou" -eq 1 ]; then
        echo "autoteste_funcoes_no_ctest: caso $1 $2: OK"
    else
        FALHAS=$((FALHAS + 1))
        echo "autoteste_funcoes_no_ctest: caso $1 $2: FALHA (rc=$RC, esperado $3, trecho '$4')"
        while IFS= read -r linha; do
            printf '    | %s\n' "$linha"
        done < "$C/saida.txt"
    fi
}

# Caso 1: base. Ciclo de vida, _data, setUp e _aux não contam.
preparar 1
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
json_base
rodar_com_json
avalia 1 "base" 0 "declaradas=4 registradas=4 excecoes=0 faltam=0 sobram=0 encontrados=2 analisados=2 falharam=0"

# Caso 2: função Qt declarada e sem registro.
preparar 2
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "    void semRegistro() {}"
json_base
rodar_com_json
avalia 2 "falta_qt" 1 "falta: tests/q/tst_q.cpp::semRegistro"

# Caso 3: método Python declarado e sem registro.
preparar 3
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
escreve_py "$C/raiz/tests/p/test_p.py" "    def casoSemRegistro(self):
        pass"
json_base
rodar_com_json
avalia 3 "falta_py" 1 "falta: tests/p/test_p.py::casoSemRegistro"

# Caso 4: registro por função de um nome que o fonte não declara.
preparar 4
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
json_base "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.fantasma.xml,junitxml\", \"fantasma\"]"
rodar_com_json
avalia 4 "sobra" 1 "sobra: tests/q/tst_q.cpp::fantasma"

# Caso 5: JSON com a lista de testes vazia.
preparar 5
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "tests" : []' '}' > "$C/ctest.json"
rodar_com_json
avalia 5 "varredura_vazia_lista" 1 "varredura vazia"

# Caso 6: JSON só com o teste inteiro e o outro, sem registro por função.
preparar 6
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
json_arquivo "$C/ctest.json" "[\"$C/build/tests/r/tst_r\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/r.xml,junitxml\"]" "[\"/usr/bin/bash\", \"/x/oraculo.sh\", \"xmllint\"]"
rodar_com_json
avalia 6 "varredura_vazia_so_inteiro" 1 "varredura vazia"

# Caso 7: exceção para função que já está registrada e declarada; a exceção fica morta.
preparar 7
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
printf '%s\n' 'tests/q/tst_q.cpp::umSlot | motivo de teste' > "$C/excecoes.txt"
json_base
rodar_com_json
avalia 7 "excecao_morta" 1 "excecao morta: tests/q/tst_q.cpp::umSlot"

# Caso 8: exceção sem motivo (linha terminada em barra vazia).
preparar 8
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
printf '%s\n' 'tests/q/tst_q.cpp::umSlot |' > "$C/excecoes.txt"
json_base
rodar_com_json
avalia 8 "excecao_sem_motivo" 2 "linha sem motivo"

# Caso 9: a função sem registro entra na lista com motivo; o portão passa.
preparar 9
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "    void semRegistro() {}"
printf '%s\n' 'tests/q/tst_q.cpp::semRegistro | motivo de teste' > "$C/excecoes.txt"
json_base
rodar_com_json
avalia 9 "excecao_cobre_falta" 0 "excecoes=1 faltam=0"

# Caso 10: ctest morto por SIGKILL (rc 137), com um ctest falso na frente do PATH; sem --json.
preparar 10
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
mkdir -p "$C/falso"
printf '%s\n' '#!/bin/sh' 'kill -KILL $$' > "$C/falso/ctest"
chmod +x "$C/falso/ctest"
PATH="$C/falso:$PATH" python3 -I "$PORTAO" --raiz "$C/raiz" --build "$C/build" --excecoes "$C/excecoes.txt" > "$C/saida.txt" 2>&1
RC=$?
avalia 10 "ctest_morto_por_sinal" 2 "ctest saiu com rc=137"

# Caso 11: fonte Qt sem a linha private slots.
preparar 11
printf '%s\n' 'class TstQ : public QObject' '{' '};' > "$C/raiz/tests/q/tst_q.cpp"
json_base
rodar_com_json
avalia 11 "fonte_sem_secao" 1 "nao analisado: tests/q/tst_q.cpp"

# Caso 12: linha de recuo 0 (#if) dentro da seção private slots.
preparar 12
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "#if X"
json_base
rodar_com_json
avalia 12 "linha_nao_reconhecida" 1 "linha nao reconhecida"

# Caso 13: fonte Qt ausente.
preparar 13
json_base
rodar_com_json
avalia 13 "fonte_ausente" 1 "nao analisado: tests/q/tst_q.cpp"

# Caso 14: a mesma função registrada duas vezes no mesmo programa.
preparar 14
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
json_base "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.umSlot.xml,junitxml\", \"umSlot\"]"
rodar_com_json
avalia 14 "registro_duplicado" 1 "duplicada: tests/q/tst_q.cpp::umSlot"

# Caso 15: teste Qt com tokens sobrando depois do registro por função.
preparar 15
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
json_base "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.umSlot.xml,junitxml\", \"umSlot\", \"extra\"]"
rodar_com_json
avalia 15 "comando_estranho" 1 "comando nao reconhecido"

# Caso 16: lambda no corpo de um slot; o '};' de recuo 8 fecha a lambda, não a seção.
preparar 16
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "    void comLambda()
    {
        auto f = [] {
        };
    }
    void depoisDaLambda() {}"
json_base "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.comLambda.xml,junitxml\", \"comLambda\"]" \
    "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.depoisDaLambda.xml,junitxml\", \"depoisDaLambda\"]"
rodar_com_json
avalia 16 "lambda_no_corpo" 0 "declaradas=6 registradas=6 excecoes=0 faltam=0 sobram=0"

# Caso 17: classe local com rótulo public no corpo de um slot; o rótulo de recuo 8 não fecha a seção.
preparar 17
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "    void comClasseLocal()
    {
        struct Local {
        public:
            int x = 0;
        };
    }
    void depoisDaClasse() {}"
json_base "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.comClasseLocal.xml,junitxml\", \"comClasseLocal\"]" \
    "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.depoisDaClasse.xml,junitxml\", \"depoisDaClasse\"]"
rodar_com_json
avalia 17 "rotulo_aninhado" 0 "declaradas=6 registradas=6 excecoes=0 faltam=0 sobram=0"

# Caso 18: segunda linha private slots logo depois da primeira seção; a seção reabre.
preparar 18
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "private slots:
    void naSegundaSecao() {}"
json_base "[\"$C/build/tests/q/tst_q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.naSegundaSecao.xml,junitxml\", \"naSegundaSecao\"]"
rodar_com_json
avalia 18 "segunda_secao" 0 "declaradas=5 registradas=5 excecoes=0 faltam=0 sobram=0"

# Caso 19: rótulo public de recuo 0 fecha a seção; a função depois dele não é caso de teste.
preparar 19
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "public:
    void auxiliarPublico() {}"
json_base
rodar_com_json
avalia 19 "rotulo_fecha_secao" 0 "declaradas=4 registradas=4 excecoes=0 faltam=0 sobram=0"

# Caso 20: item da lista tests que não é objeto; a entrada sai com rc 2 e o resumo na primeira linha (S2e).
preparar 20
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "tests" : [ 1 ]' '}' > "$C/ctest.json"
rodar_com_json
avalia 20 "item_nao_objeto" 2 "teste nao e objeto na posicao 1"

# Caso 21: command com byte nulo, escrito no JSON como \u0000; a entrada sai com rc 2 antes de qualquer contagem (S2e).
preparar 21
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
item_nulo="[\"$C/build/tests/q/tst\\u0000q\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/q.xml,junitxml\", \"umSlot\"]"
json_base "$item_nulo"
rodar_com_json
avalia 21 "byte_nulo_no_command" 2 "command com byte nulo: t7"

# Caso 22: JSON com 100000 colchetes aninhados; o limite próprio de profundidade do portão recusa antes do json.loads
# (S2f). Dois processos tr, número fixo (L-11, item 6).
preparar 22
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
{
    printf '%s\n' '{"kind": "ctestInfo", "tests": '
    printf '%100000s' '' | tr ' ' '['
    printf '%100000s' '' | tr ' ' ']'
    printf '%s\n' '}'
} > "$C/ctest.json"
rodar_com_json
avalia 22 "json_aninhado_demais" 2 "JSON ilegivel: profundidade acima de 64"

# Caso 23: a exceção vale só para o próprio programa; o mesmo nome em outro programa continua faltando (S2e).
preparar 23
escreve_qt "$C/raiz/tests/q/tst_q.cpp" "    void semRegistro() {}"
mkdir -p "$C/raiz/tests/r2"
escreve_qt "$C/raiz/tests/r2/tst_r2.cpp" "    void semRegistro() {}"
printf '%s\n' 'tests/q/tst_q.cpp::semRegistro | motivo de teste' > "$C/excecoes.txt"
R2="$C/build/tests/r2/tst_r2"
item_r2_um="[\"$R2\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/r2.u.xml,junitxml\", \"umSlot\"]"
item_r2_outro="[\"$R2\", \"-o\", \"-,txt\", \"-o\", \"$C/build/junit/r2.o.xml,junitxml\", \"outroSlot\"]"
json_base "$item_r2_um" "$item_r2_outro"
rodar_com_json
avalia 23 "excecao_so_do_programa" 1 "falta: tests/r2/tst_r2.cpp::semRegistro"

# Casos 24 e 25 (S2f): a base com a profundidade do fundo em N listas, com os colchetes e um texto entre aspas dentro.
# Profundidade = 1 (objeto do topo) + N. Com N = 63 dá 64, aceito; com N = 64 dá 65, recusado. Número fixo de processos.
# Caso 24: a borda, aceita.
preparar 24
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
json_base
mv "$C/ctest.json" "$C/base.json"
{
    printf '%s' '{ "fundo" : '
    printf '%63s' '' | tr ' ' '['
    printf '%s' '"a[{\"b"'
    printf '%63s' '' | tr ' ' ']'
    printf ',\n'
    tail -n +2 "$C/base.json"
} > "$C/ctest.json"
rodar_com_json
avalia 24 "profundidade_no_limite" 0 "declaradas=4 registradas=4 excecoes=0 faltam=0 sobram=0"

# Caso 25: um passo além da borda, recusado.
preparar 25
escreve_qt "$C/raiz/tests/q/tst_q.cpp" ""
json_base
mv "$C/ctest.json" "$C/base.json"
{
    printf '%s' '{ "fundo" : '
    printf '%64s' '' | tr ' ' '['
    printf '%s' '"a[{\"b"'
    printf '%64s' '' | tr ' ' ']'
    printf ',\n'
    tail -n +2 "$C/base.json"
} > "$C/ctest.json"
rodar_com_json
avalia 25 "profundidade_um_acima" 2 "JSON ilegivel: profundidade acima de 64"

echo "autoteste_funcoes_no_ctest: casos=$CASOS falhas=$FALHAS"
if [ "$CASOS" -ne 25 ] || [ "$FALHAS" -ne 0 ]; then
    exit 1
fi
exit 0
