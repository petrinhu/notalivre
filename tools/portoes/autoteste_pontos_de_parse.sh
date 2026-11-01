#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de pontos de parse (NL-2.21, DA-2.67; L-36: portão só conta depois de visto reprovar).
# Cada caso monta uma árvore nova numa pasta temporária e confere o rc do portão. Saída: "casos=N falharam=K".
# Casos: árvore limpa passa; chamada nova fora da lista reprova; linha da lista sem chamada reprova; varredura vazia
# reprova; nome de função dentro de comentário não conta.
set -u

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GATE="$DIR/pontos_de_parse.py"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/pontos_parse_autoteste.XXXXXX")" || { echo "PARADA: mktemp falhou"; exit 2; }
trap 'rm -rf "$TMP"' EXIT

casos=0
falharam=0

# Lista de referência dos casos: um ponto por arquivo de fixture.
LISTA="$TMP/lista.txt"
printf '%s\n' \
    'src/a.cpp | xmlCreateMemoryParserCtxt | caso' \
    'src/b.cpp | xmlSchemaNewParserCtxt | caso' > "$LISTA"

# Monta a árvore limpa: as duas chamadas da lista, e um comentário com um nome proibido que não pode contar.
limpa="$TMP/limpa"
mkdir -p "$limpa/src"
printf '%s\n' '// xmlReadMemory(x) citado em comentário, não é chamada' \
    'void f() { xmlCreateMemoryParserCtxt("a", 1); }' > "$limpa/src/a.cpp"
printf '%s\n' 'void g() { xmlSchemaNewParserCtxt("b"); }' > "$limpa/src/b.cpp"

# Caso: espera rc esperado do portão sobre a árvore dada.
caso() {
    local nome="$1" esperado="$2" arvore="$3" lista="$4"
    casos=$((casos + 1))
    python3 -I "$GATE" "$arvore" "$lista" > "$TMP/saida.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$esperado" ]; then
        echo "caso=$nome rc=$rc (esperado $esperado) OK"
    else
        echo "caso=$nome rc=$rc (esperado $esperado) FALHOU"
        cat "$TMP/saida.txt"
        falharam=$((falharam + 1))
    fi
}

caso limpa 0 "$limpa" "$LISTA"

nova="$TMP/chamada_nova"
cp -r "$limpa" "$nova"
printf '%s\n' 'void h() { xmlReadMemory("c", 1, "u", nullptr, 0); }' > "$nova/src/c.cpp"
caso chamada_nova_fora_da_lista 1 "$nova" "$LISTA"

sem="$TMP/linha_sem_chamada"
cp -r "$limpa" "$sem"
cp "$LISTA" "$TMP/lista_sem.txt"
printf '%s\n' 'src/d.cpp | xmlReadFile | caso sem chamada' >> "$TMP/lista_sem.txt"
caso linha_da_lista_sem_chamada 1 "$sem" "$TMP/lista_sem.txt"

vazia="$TMP/vazia"
mkdir -p "$vazia/src"
printf '%s\n' 'int nada() { return 0; }' > "$vazia/src/z.cpp"
caso varredura_vazia 1 "$vazia" "$LISTA"

echo "casos=$casos falharam=$falharam"
[ "$falharam" -eq 0 ] || exit 1
[ "$casos" -gt 0 ] || exit 2
exit 0
