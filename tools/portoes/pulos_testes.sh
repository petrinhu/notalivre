#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portão de testes (NL-2.14, DA-2.61 adendo 3; L-36): mundo fechado em tempo de execução.
# 1. A lista de testes é a do próprio CTest: ctest --show-only=json-v1 (JSON 3.14+, sem introspecção do CMake).
# 2. Cada teste da lista tem junitxml por função em <build>/junit/<NOME>.xml, gravado pela função de registro
#    nl_registra_teste_qt; ou está em tests/testes_sem_saida_por_funcao.txt (scripts, python, e2e), por igualdade exata.
# 3. Cada junitxml é completo e tem ao menos um testcase. Todo pulo tem de estar na lista de pulos da distro, e toda linha
#    da lista de pulos tem de ocorrer.
# 4. Os testes obrigatórios da distro têm de estar no CTest.
# Uso: pulos_testes.sh <dir_build> <distro> <lista_pulos> [sem_saida] [obrigatorios]
# Só o autoteste define NL_CTEST_JSON_ARQ (JSON de mentira); em produção o portão roda o ctest.
# Sem processo por teste ou por registro: laços com read e case, e comparações por grep -f numa passagem.
set -u

BUILD="${1:-}"
DISTRO="${2:-}"
PULOS="${3:-}"
SEM_SAIDA="${4:-tests/testes_sem_saida_por_funcao.txt}"
OBRIG="${5:-tests/testes_obrigatorios_por_distro.txt}"
if [ -z "$BUILD" ] || [ -z "$PULOS" ]; then
    echo "uso: pulos_testes.sh <dir_build> <distro> <lista_pulos> [sem_saida] [obrigatorios]"
    exit 2
fi
J="$BUILD/junit"
mkdir -p "$J"
ruins=0
parada() {
    echo "PARADA: $*"
    ruins=$((ruins + 1))
}

if [ -z "$DISTRO" ]; then
    echo "PARADA: distro ausente (NL_DISTRO vazio): sem distro não há lista de pulos esperados"
    exit 1
fi
for arq in "$PULOS" "$SEM_SAIDA" "$OBRIG"; do
    if [ ! -r "$arq" ]; then
        echo "PARADA: lista ilegível: $arq"
        exit 1
    fi
done

# 1. Lista de testes do CTest (a autoridade).
JSON="${NL_CTEST_JSON_ARQ:-$J/.ctest.json}"
if [ -z "${NL_CTEST_JSON_ARQ:-}" ]; then
    if ! ctest --test-dir "$BUILD" --show-only=json-v1 > "$JSON" 2> "$J/.ctest.err"; then
        parada "ctest --show-only=json-v1 falhou (ver $J/.ctest.err)"
    fi
fi
TESTES="$J/.testes.txt"
sed -n 's/^      "name" : "\(.*\)",\{0,1\}$/\1/p' "$JSON" 2>/dev/null > "$TESTES"
n_nomes=$(wc -l < "$TESTES" | tr -d ' ')
n_objetos=$(grep -c '^    {$' "$JSON" 2>/dev/null)
n_comandos=$(grep -c '^      "command" :' "$JSON" 2>/dev/null)
n_objetos=${n_objetos:-0}
n_comandos=${n_comandos:-0}
if [ "$n_nomes" -eq 0 ]; then
    parada "JSON do ctest vazio: nenhum teste listado (zero testes não é varredura)"
elif [ "$n_objetos" -ne "$n_nomes" ] || [ "$n_nomes" -ne "$n_comandos" ]; then
    parada "formato do JSON do ctest não reconhecido: $n_objetos objetos, $n_nomes nomes, $n_comandos comandos"
fi

# Particiona: com junitxml, e sem junitxml (candidatos à lista de exceções).
COM="$J/.com_junit.txt"
SEM="$J/.sem_junit.txt"
: > "$COM"
: > "$SEM"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    if [ -e "$J/$nome.xml" ]; then
        echo "$nome" >> "$COM"
    else
        echo "$nome" >> "$SEM"
    fi
done < "$TESTES"
n_com=$(wc -l < "$COM" | tr -d ' ')
n_sem=$(wc -l < "$SEM" | tr -d ' ')

# 2. Lista de exceções da distro (sem junitxml). Linha sem motivo reprova, em qualquer distro.
EXC="$J/.excecoes.txt"
: > "$EXC"
n_linha=0
while IFS= read -r linha; do
    n_linha=$((n_linha + 1))
    case "$linha" in
        ''|'#'*) continue ;;
    esac
    set -f
    set -- $linha
    set +f
    dist="${1:-}"
    teste="${2:-}"
    resto="${linha#* }"
    motivo="${resto#* }"
    if [ -z "$dist" ] || [ -z "$teste" ] || [ "$motivo" = "$resto" ] || [ -z "$motivo" ]; then
        parada "linha sem motivo na lista de exceções (linha $n_linha): $linha"
        continue
    fi
    if [ "$dist" = "$DISTRO" ]; then
        echo "$teste" >> "$EXC"
    fi
done < "$SEM_SAIDA"
n_exc=$(wc -l < "$EXC" | tr -d ' ')

grep -vxF -f "$TESTES" "$EXC" > "$J/.velhas.txt"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    parada "linha velha na lista de exceções (não é teste do CTest): $nome"
done < "$J/.velhas.txt"
grep -xF -f "$COM" "$EXC" > "$J/.excecao_com_xml.txt"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    parada "exceção com junitxml (lista desatualizada): $nome"
done < "$J/.excecao_com_xml.txt"
grep -vxF -f "$EXC" "$SEM" > "$J/.sem_sem_excecao.txt"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    parada "teste sem junitxml fora da lista de exceções: $nome"
done < "$J/.sem_sem_excecao.txt"

# 3. Junitxml de cada teste com junitxml: completo, com testcase, contagem igual, e os pulos e falhas por função.
OBT="$J/.pulos_obtidos.txt"
: > "$OBT"
total=0
passou=0
falhou=0
pulou=0
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    xml="$J/$nome.xml"
    fechado=0
    declarados=""
    lidos=0
    rec=""
    tem_rec=0
    while IFS= read -r linha; do
        case "$linha" in
            *'<testsuite '*)
                declarados="${linha#*tests=\"}"
                declarados="${declarados%%\"*}"
                ;;
            *'<testcase '*)
                if [ "$tem_rec" -eq 1 ]; then
                    rec_fecha="$rec"
                    # classificação do registro anterior (ver abaixo)
                    case "$rec_fecha" in
                        *'<failure'*|*'<error'*) falhou=$((falhou + 1)) ;;
                        *' result="pass"'*) passou=$((passou + 1)) ;;
                        *'<skipped'*|*' result="skip"'*)
                            pulou=$((pulou + 1))
                            cls="${rec_fecha#*classname=\"}"
                            cls="${cls%%\"*}"
                            nt="${rec_fecha#*name=\"}"
                            nt="${nt%%\"*}"
                            echo "$cls.$nt" >> "$OBT"
                            ;;
                        *' result="'*) falhou=$((falhou + 1)) ;;
                        *) passou=$((passou + 1)) ;;
                    esac
                fi
                rec="$linha"
                tem_rec=1
                lidos=$((lidos + 1))
                ;;
            *'</testsuite>'*)
                fechado=1
                ;;
            *)
                if [ "$tem_rec" -eq 1 ]; then
                    rec="$rec $linha"
                fi
                ;;
        esac
    done < "$xml" 2>/dev/null
    if [ "$tem_rec" -eq 1 ] && [ "$fechado" -eq 1 ]; then
        case "$rec" in
            *'<failure'*|*'<error'*) falhou=$((falhou + 1)) ;;
            *' result="pass"'*) passou=$((passou + 1)) ;;
            *'<skipped'*|*' result="skip"'*)
                pulou=$((pulou + 1))
                cls="${rec#*classname=\"}"
                cls="${cls%%\"*}"
                nt="${rec#*name=\"}"
                nt="${nt%%\"*}"
                echo "$cls.$nt" >> "$OBT"
                ;;
            *' result="'*) falhou=$((falhou + 1)) ;;
            *) passou=$((passou + 1)) ;;
        esac
    fi
    if [ "$fechado" -ne 1 ]; then
        parada "junitxml vazio ou incompleto (sem </testsuite>): $nome"
        continue
    fi
    if [ "$lidos" -eq 0 ]; then
        parada "junitxml sem testcase: $nome"
        continue
    fi
    total=$((total + lidos))
    if [ -z "$declarados" ] || [ "$lidos" -ne "$declarados" ]; then
        parada "xml inconsistente: $nome declara ${declarados:-nenhum} testes e tem $lidos registros"
    fi
done < "$COM"

# 4. Pulos: igualdade exata, por distro, nas duas direções. O motivo é documentação e não é comparado.
ESP="$J/.pulos_esperados.txt"
: > "$ESP"
n_linha=0
while IFS= read -r linha; do
    n_linha=$((n_linha + 1))
    case "$linha" in
        ''|'#'*) continue ;;
    esac
    set -f
    set -- $linha
    set +f
    dist="${1:-}"
    teste="${2:-}"
    resto="${linha#* }"
    motivo="${resto#* }"
    if [ -z "$dist" ] || [ -z "$teste" ] || [ "$motivo" = "$resto" ] || [ -z "$motivo" ]; then
        parada "linha sem motivo na lista de pulos (linha $n_linha): $linha"
        continue
    fi
    if [ "$dist" = "$DISTRO" ]; then
        echo "$teste" >> "$ESP"
    fi
done < "$PULOS"
nesp=$(wc -l < "$ESP" | tr -d ' ')
nobt=$(wc -l < "$OBT" | tr -d ' ')
grep -vxF -f "$ESP" "$OBT" > "$J/.pulo_fora_da_lista.txt"
while IFS= read -r chave; do
    [ -n "$chave" ] || continue
    parada "pulo fora da lista ($DISTRO): $chave"
done < "$J/.pulo_fora_da_lista.txt"
grep -vxF -f "$OBT" "$ESP" > "$J/.pulo_listado_nao_ocorreu.txt"
while IFS= read -r chave; do
    [ -n "$chave" ] || continue
    parada "pulo listado que não ocorreu ($DISTRO): $chave"
done < "$J/.pulo_listado_nao_ocorreu.txt"
if [ "$falhou" -gt 0 ]; then
    parada "$falhou funcao(oes) falharam"
fi

# 5. Testes obrigatórios da distro: têm de estar no CTest.
OBRIG_DIST="$J/.obrigatorios.txt"
: > "$OBRIG_DIST"
while IFS= read -r linha; do
    case "$linha" in
        ''|'#'*) continue ;;
    esac
    set -f
    set -- $linha
    set +f
    if [ "${1:-}" = "$DISTRO" ] && [ -n "${2:-}" ]; then
        echo "$2" >> "$OBRIG_DIST"
    fi
done < "$OBRIG"
n_obrig=$(wc -l < "$OBRIG_DIST" | tr -d ' ')
grep -vxF -f "$TESTES" "$OBRIG_DIST" > "$J/.obrig_ausentes.txt"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    parada "teste obrigatório ausente no CTest (distro $DISTRO): $nome"
done < "$J/.obrig_ausentes.txt"

echo "pulos_testes: distro=$DISTRO ctest=$n_nomes com_junitxml=$n_com sem_junitxml=$n_sem excecoes=$n_exc funcoes=$total passou=$passou falhou=$falhou pulou=$pulou (pulos: $nesp esperados, $nobt obtidos, obrigatorios=$n_obrig)"
if [ "$ruins" -ne 0 ]; then
    echo "pulos_testes: REPROVADO ($ruins motivo(s))"
    exit 1
fi
echo "pulos_testes: VERDE, mundo fechado: todo teste do CTest tem junitxml ou exceção, e os pulos batem com a lista de $DISTRO"
exit 0
