#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portão de pulos de teste (NL-2.14, DA-2.61; L-36). Agrega, por função, o junitxml que cada teste Qt Test grava (o CMake
# registra o arquivo em <build>/junit/<teste>.xml, pela função nl_registra_teste_qt). O CTest conta um teste que pulou
# como passou (QSKIP sai com 0), por isso o pulo é lido aqui, função a função.
#
# Uso: pulos_testes.sh <dir_build> <distro> <lista>
#   dir_build  pasta de build do CMake; o manifesto <build>/junit/esperados.txt lista os testes registrados
#   distro     NL_DISTRO: fedora44, ubuntu26.04, manjaro, mint22.3 ou flatpak. Vazio reprova (fechado)
#   lista      tests/pulos_esperados_por_distro.txt: uma linha por pulo, "distro teste motivo"
#
# Reprova (rc 1) se: a distro vier vazia; a lista não abrir ou tiver linha sem motivo; o manifesto não abrir ou vier
# vazio; um junitxml faltar, vier sem </testsuite> ou tiver contagem diferente dos registros lidos; um registro não tiver
# nome legível; uma função falhar; um pulo ocorrer fora da lista da distro; ou uma linha da lista da distro não ocorrer.
# Comparação por igualdade exata, nas duas direções. O motivo é documentação e não é comparado.
# Contagem: funcoes inclui initTestCase e cleanupTestCase, porque o QTest os conta como funções. A linha de contagens sai
# sempre, mesmo com zero. Não confundir com o piso de testes, que continua na contagem do ctest -N.
# Só POSIX sh, sem python: roda também no sandbox do SDK Flatpak.
set -u

BUILD="${1:-}"
DISTRO="${2:-}"
LISTA="${3:-}"
# Quarto argumento, opcional: lista de testes obrigatórios por distro (padrão: a do repositório, NL-2.14 MÉDIA-2).
OBRIG="${4:-tests/testes_obrigatorios_por_distro.txt}"
if [ -z "$BUILD" ] || [ -z "$LISTA" ]; then
    echo "uso: pulos_testes.sh <dir_build> <distro> <lista> [obrigatorios]"
    exit 2
fi
# Os portões de registro (NL-2.14, revisão devops ALTA-1) ficam ao lado deste script.
PORTOES_DIR="$(dirname "$0")"

JUNIT="$BUILD/junit"
MANIFESTO="$JUNIT/esperados.txt"
REGISTROS="$JUNIT/.registros.txt"
OBTIDOS="$JUNIT/.pulos_obtidos.txt"
ESPERADOS="$JUNIT/.pulos_esperados.txt"

ruins=0
parada() {
    echo "PARADA: $*"
    ruins=$((ruins + 1))
}

if [ -z "$DISTRO" ]; then
    echo "PARADA: distro ausente (NL_DISTRO vazio): sem distro não há lista de pulos esperados"
    exit 1
fi
if [ ! -r "$LISTA" ]; then
    echo "PARADA: lista de pulos ilegível: $LISTA"
    exit 1
fi
if [ ! -r "$MANIFESTO" ]; then
    echo "PARADA: manifesto de testes ausente ($MANIFESTO): o configure grava o manifesto"
    exit 1
fi

total=0
passou=0
falhou=0
pulou=0
nomes=0
nesp=0
nobt=0

# Lista da distro. Toda linha tem de ter distro, teste e motivo, mesmo as de outra distro (formato fechado).
: > "$ESPERADOS"
n_linha=0
while IFS= read -r linha; do
    n_linha=$((n_linha + 1))
    case "$linha" in
        ''|'#'*) continue ;;
    esac
    dist=$(printf '%s\n' "$linha" | sed -n 's/^\([^ ][^ ]*\) \([^ ][^ ]*\) \([^ ].*\)$/\1/p')
    teste=$(printf '%s\n' "$linha" | sed -n 's/^\([^ ][^ ]*\) \([^ ][^ ]*\) \([^ ].*\)$/\2/p')
    motivo=$(printf '%s\n' "$linha" | sed -n 's/^\([^ ][^ ]*\) \([^ ][^ ]*\) \([^ ].*\)$/\3/p')
    if [ -z "$dist" ] || [ -z "$teste" ] || [ -z "$motivo" ]; then
        parada "linha $n_linha da lista sem motivo (formato: distro teste motivo): $linha"
        continue
    fi
    if [ "$dist" = "$DISTRO" ]; then
        echo "$teste" >> "$ESPERADOS"
        nesp=$((nesp + 1))
    fi
done < "$LISTA"

# Resultado por função, a partir do junitxml de cada teste registrado.
: > "$OBTIDOS"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    nomes=$((nomes + 1))
    xml="$JUNIT/$nome.xml"
    if [ ! -r "$xml" ]; then
        parada "xml ausente para o teste $nome ($xml): o teste não gravou resultado"
        continue
    fi
    if ! grep -q '</testsuite>' "$xml"; then
        parada "xml incompleto (sem </testsuite>), o processo morreu no meio: $nome"
        continue
    fi
    declarados=$(sed -n 's/^<testsuite .* tests="\([0-9][0-9]*\)".*/\1/p' "$xml" | head -n 1)
    if [ -z "$declarados" ]; then
        parada "xml sem contagem de testes (tests=): $nome"
        continue
    fi
    tr '\n' ' ' < "$xml" | sed 's/<testcase /\n<testcase /g' | grep '^<testcase ' > "$REGISTROS"
    lidos=$(wc -l < "$REGISTROS" | tr -d ' ')
    if [ "$lidos" -eq 0 ] || [ "$lidos" -ne "$declarados" ]; then
        parada "xml inconsistente: $nome declara $declarados testes e tem $lidos registros"
        continue
    fi
    while IFS= read -r registro; do
        total=$((total + 1))
        chave=$(printf '%s\n' "$registro" | sed -n 's/^<testcase name="\([^"]*\)" classname="\([^"]*\)".*/\2.\1/p')
        if [ -z "$chave" ]; then
            parada "registro sem nome legível em $nome"
            continue
        fi
        case "$registro" in
            *'<failure'*|*'<error'*) falhou=$((falhou + 1)) ;;
            *' result="pass"'*) passou=$((passou + 1)) ;;
            *'<skipped'*|*' result="skip"'*)
                pulou=$((pulou + 1))
                printf '%s\n' "$chave" >> "$OBTIDOS"
                ;;
            *' result="'*) falhou=$((falhou + 1)) ;;
            *) passou=$((passou + 1)) ;;
        esac
    done < "$REGISTROS"
done < "$MANIFESTO"

if [ "$nomes" -eq 0 ]; then
    parada "manifesto vazio: nenhum teste registrado (zero funções não é varredura)"
fi
nobt=$(wc -l < "$OBTIDOS" | tr -d ' ')

# Igualdade exata nas duas direções, por distro.
while IFS= read -r chave; do
    [ -n "$chave" ] || continue
    if ! grep -qxF -- "$chave" "$ESPERADOS"; then
        parada "pulo fora da lista ($DISTRO): $chave"
    fi
done < "$OBTIDOS"
while IFS= read -r chave; do
    [ -n "$chave" ] || continue
    if ! grep -qxF -- "$chave" "$OBTIDOS"; then
        parada "pulo listado que não ocorreu ($DISTRO): $chave"
    fi
done < "$ESPERADOS"

echo "pulos_testes: distro=$DISTRO funcoes=$total passou=$passou falhou=$falhou pulou=$pulou (lista: $nesp esperados, $nobt obtidos)"
if [ "$falhou" -gt 0 ]; then
    parada "$falhou funcao(oes) falharam"
fi
# Registro Qt Test: todo Qt Test do CTest no manifesto, obrigatórios presentes, e a checagem estática (reforço).
if ! sh "$PORTOES_DIR/registro_qt.sh" "$BUILD" "$DISTRO" "$OBRIG"; then
    parada "registro Qt Test reprovou (linhas acima)"
fi
if ! sh "$PORTOES_DIR/registro_qt_estatico.sh" "${PULOS_RAIZ:-.}"; then
    parada "checagem estática do registro Qt Test reprovou (linhas acima)"
fi
if [ "$ruins" -ne 0 ]; then
    echo "pulos_testes: REPROVADO ($ruins motivo(s))"
    exit 1
fi
echo "pulos_testes: VERDE, pulos batem com a lista de $DISTRO"
exit 0
