#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portão dos testes fora do Flatpak (NL-2.15, DA-2.65; L-36). Confere, por igualdade exata, que cada teste que o build
# nativo registra e o build com NL_FLATPAK_BUILD=ON não registra (fora) está listado em flatpak/testes_fora_do_flatpak.txt,
# e que cada nome listado é mesmo um teste excluído do Flatpak.
# 1. As duas listas são do próprio CTest: ctest --show-only=json-v1 do build nativo (dir dado) e do configure ON feito aqui,
#    num diretório temporário, a partir da mesma fonte. Só configura, sem build.
# 2. fora = nativos - flatpak. Divergências: excluído e não listado; listado e registrado no ON; listado e ausente no nativo.
# 3. Cada linha da lista tem três colunas: <nome> | <motivo> | coberto no build nativo. Linha sem motivo reprova.
# 4. Imprime sempre: fora_do_flatpak: nativos=N flatpak=M fora=K listados=L divergencias=D, e uma linha por divergência.
# Só o autoteste define NL_FORA_JSON_NATIVO e NL_FORA_JSON_ON (JSON de mentira); em produção o portão roda o ctest.
# Sem processo por teste: sed, sort, comm e awk em passagens fixas (L-11).
#
# Uso: fora_do_flatpak.sh <dir_build_nativo> <lista>     (a fonte é a árvore onde este arquivo está)
set -u

BUILD="${1:-}"
LISTA="${2:-}"
if [ -z "$BUILD" ] || [ -z "$LISTA" ]; then
    echo "uso: fora_do_flatpak.sh <dir_build_nativo> <lista>"
    exit 2
fi
RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-fora-flatpak.XXXXXX")" || { echo "PARADA: mktemp falhou"; exit 1; }
trap 'rm -rf -- "$TMP"' EXIT
ruins=0
parada() {
    echo "PARADA: $*"
    ruins=$((ruins + 1))
}

if [ ! -r "$LISTA" ]; then
    echo "PARADA: lista ilegível: $LISTA"
    exit 1
fi

# 1. JSON do build nativo e do configure ON.
if [ -n "${NL_FORA_JSON_NATIVO:-}" ]; then
    JSON_NATIVO="$NL_FORA_JSON_NATIVO"
else
    JSON_NATIVO="$TMP/nativo.json"
    if ! ctest --test-dir "$BUILD" --show-only=json-v1 > "$JSON_NATIVO" 2> "$TMP/nativo.err"; then
        parada "ctest --show-only=json-v1 falhou no build nativo ($BUILD)"
        tail -n 5 "$TMP/nativo.err"
    fi
fi
if [ -n "${NL_FORA_JSON_ON:-}" ]; then
    JSON_ON="$NL_FORA_JSON_ON"
else
    ON="$TMP/on"
    JSON_ON="$TMP/on.json"
    if ! cmake -S "$RAIZ" -B "$ON" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DNL_FLATPAK_BUILD=ON > "$TMP/configure.log" 2>&1; then
        parada "configure com NL_FLATPAK_BUILD=ON falhou (últimas linhas abaixo)"
        tail -n 20 "$TMP/configure.log"
        : > "$JSON_ON"
    elif ! ctest --test-dir "$ON" --show-only=json-v1 > "$JSON_ON" 2> "$TMP/on.err"; then
        parada "ctest --show-only=json-v1 falhou no configure ON"
        tail -n 5 "$TMP/on.err"
    fi
fi

# Nomes de teste do JSON: linha "name" no nível do teste (seis espaços). Propriedades ficam mais fundo e não entram.
nomes() {
    sed -n 's/^      "name" : "\(.*\)",\{0,1\}$/\1/p' "$1"
}

checa_json() {
    arq="$1"
    rotulo="$2"
    if [ ! -s "$arq" ]; then
        parada "JSON vazio ($rotulo): zero testes não é varredura"
        return 0
    fi
    n_nomes=$(nomes "$arq" | wc -l | tr -d ' ')
    n_obj=$(grep -c '^    {$' "$arq" || true)
    if [ "$n_nomes" -eq 0 ]; then
        parada "JSON sem nenhum teste ($rotulo)"
    elif [ "$n_obj" -ne "$n_nomes" ]; then
        parada "formato do JSON não reconhecido ($rotulo): $n_obj objetos, $n_nomes nomes"
    fi
    nomes "$arq" | LC_ALL=C sort | uniq -d > "$TMP/dup.$rotulo"
    if [ -s "$TMP/dup.$rotulo" ]; then
        parada "nome de teste repetido no JSON ($rotulo): $(head -n 1 "$TMP/dup.$rotulo")"
    fi
}
checa_json "$JSON_NATIVO" nativo
checa_json "$JSON_ON" flatpak

nomes "$JSON_NATIVO" | LC_ALL=C sort -u > "$TMP/nativo.set"
nomes "$JSON_ON" | LC_ALL=C sort -u > "$TMP/on.set"
if [ ! -s "$TMP/nativo.set" ]; then
    parada "build nativo sem testes listados"
fi

# 2. Lista versionada: uma linha por teste, três colunas, motivo obrigatório. Linhas de comentário e vazias são ignoradas.
: > "$TMP/lista.raw"
if ! awk -F'|' -v OUT="$TMP/lista.raw" '
    /^[[:space:]]*#/ || /^[[:space:]]*$/ { next }
    {
        n = $1; m = $2; c = $3
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", n)
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", m)
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", c)
        if (NF != 3) { print "PARADA: linha " NR " com " NF " campos (esperado: nome | motivo | coberto no build nativo)"; bad++; next }
        if (n == "") { print "PARADA: linha " NR " sem nome"; bad++; next }
        if (m == "") { print "PARADA: linha " NR " sem motivo: " n; bad++; next }
        if (c != "coberto no build nativo") { print "PARADA: linha " NR " sem a coluna fixa coberto no build nativo: " n; bad++; next }
        print n > OUT
    }
    END { if (bad > 0) exit 1 }
' "$LISTA"; then
    ruins=$((ruins + 1))
fi
LC_ALL=C sort -u "$TMP/lista.raw" > "$TMP/lista.set"

# 3. Divergências, por comparação de conjuntos ordenados (comm exige a mesma ordem de sort).
comm -23 "$TMP/nativo.set" "$TMP/on.set" > "$TMP/fora.set"
comm -23 "$TMP/fora.set" "$TMP/lista.set" > "$TMP/d_nao_listado.txt"
comm -12 "$TMP/lista.set" "$TMP/on.set" > "$TMP/d_registrado.txt"
comm -23 "$TMP/lista.set" "$TMP/nativo.set" > "$TMP/d_inexistente.txt"

conta() {
    wc -l < "$1" | tr -d ' '
}
nativos=$(conta "$TMP/nativo.set")
flatpak=$(conta "$TMP/on.set")
fora=$(conta "$TMP/fora.set")
listados=$(conta "$TMP/lista.set")
divergencias=$(cat "$TMP/d_nao_listado.txt" "$TMP/d_registrado.txt" "$TMP/d_inexistente.txt" | wc -l | tr -d ' ')

sed 's/^/DIVERGENCIA excluido-nao-listado: /' "$TMP/d_nao_listado.txt"
sed 's/^/DIVERGENCIA listado-registrado-no-flatpak: /' "$TMP/d_registrado.txt"
sed 's/^/DIVERGENCIA listado-inexistente-no-nativo: /' "$TMP/d_inexistente.txt"
echo "fora_do_flatpak: nativos=$nativos flatpak=$flatpak fora=$fora listados=$listados divergencias=$divergencias"

if [ "$ruins" -ne 0 ] || [ "$divergencias" -ne 0 ]; then
    exit 1
fi
exit 0
