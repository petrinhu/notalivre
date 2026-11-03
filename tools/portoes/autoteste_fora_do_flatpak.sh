#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão dos testes fora do Flatpak (NL-2.15, DA-2.65; L-36). Cinco casos, cada um com o veredito esperado e
# um trecho obrigatório da saída: lista completa passa; excluído não listado reprova; listado e registrado no ON reprova;
# linha sem motivo reprova; JSON vazio reprova. Os JSON do CTest e as listas são gerados aqui, numa pasta temporária, como
# no autoteste do portão de pulos; não são arquivos versionados. Sem prova de vermelho o portão não conta.
#
# Uso: autoteste_fora_do_flatpak.sh     (usa o portão da árvore onde este arquivo está)
set -u

RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
PORTAO="$RAIZ/tools/portoes/fora_do_flatpak.sh"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-fora-flatpak.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
CASOS=0
FALHAS=0

# JSON de mentira no layout do ctest --show-only=json-v1: "name" no nível do teste (seis espaços).
# json_ctest ARQUIVO NOME...
json_ctest() {
    arq="$1"
    shift
    {
        printf '%s\n' '{' '  "kind" : "ctestInfo",' '  "tests" :' '  ['
        for nome in "$@"; do
            printf '%s\n' '    {' '      "backtrace" : 2,' '      "command" :' '      [' "        \"/x/$nome\"" '      ],' "      \"name\" : \"$nome\"" '    },'
        done
        printf '%s\n' '  ]' '}'
    } > "$arq"
}

# Linhas da lista de fixture: LINHA... (cada argumento é uma linha, já no formato nome | motivo | coluna fixa).
lista_de() {
    arq="$1"
    shift
    : > "$arq"
    for linha in "$@"; do
        printf '%s\n' "$linha" >> "$arq"
    done
}

NATIVO="$BASE/nativo.json"
ON="$BASE/on.json"
ON_VAZIO="$BASE/on_vazio.json"
json_ctest "$NATIVO" A B C D
json_ctest "$ON" A D
: > "$ON_VAZIO"

LISTA_COMPLETA="$BASE/lista_completa.txt"
LISTA_FALTANDO="$BASE/lista_faltando.txt"
LISTA_REGISTRADA="$BASE/lista_registrada.txt"
LISTA_SEM_MOTIVO="$BASE/lista_sem_motivo.txt"
lista_de "$LISTA_COMPLETA" "# comentario" "B | motivo B | coberto no build nativo" "C | motivo C | coberto no build nativo"
lista_de "$LISTA_FALTANDO" "B | motivo B | coberto no build nativo"
lista_de "$LISTA_REGISTRADA" "B | motivo B | coberto no build nativo" "C | motivo C | coberto no build nativo" "A | motivo A | coberto no build nativo"
lista_de "$LISTA_SEM_MOTIVO" "B |  | coberto no build nativo" "C | motivo C | coberto no build nativo"

# caso NOME RC_ESPERADO TRECHO LISTA JSON_ON
caso() {
    CASOS=$((CASOS + 1))
    saida="$BASE/saida-$CASOS.txt"
    NL_FORA_JSON_NATIVO="$NATIVO" NL_FORA_JSON_ON="$5" sh "$PORTAO" "$BASE/sem-build" "$4" > "$saida" 2>&1
    rc=$?
    if [ "$rc" -ne "$2" ] || ! grep -q -- "$3" "$saida"; then
        FALHAS=$((FALHAS + 1))
        echo "FALHA caso $CASOS ($1): rc=$rc (esperado $2), trecho '$3' ausente na saída:"
        sed 's/^/  /' "$saida"
    else
        echo "ok caso $CASOS ($1): rc=$rc, trecho '$3'"
    fi
}

caso "lista completa passa" 0 "divergencias=0" "$LISTA_COMPLETA" "$ON"
caso "excluido nao listado reprova" 1 "DIVERGENCIA excluido-nao-listado: C" "$LISTA_FALTANDO" "$ON"
caso "listado e registrado no ON reprova" 1 "DIVERGENCIA listado-registrado-no-flatpak: A" "$LISTA_REGISTRADA" "$ON"
caso "linha sem motivo reprova" 1 "sem motivo: B" "$LISTA_SEM_MOTIVO" "$ON"
caso "JSON vazio reprova" 1 "JSON vazio (flatpak)" "$LISTA_COMPLETA" "$ON_VAZIO"

echo "autoteste_fora_do_flatpak: casos=$CASOS falhas=$FALHAS"
if [ "$CASOS" -ne 5 ] || [ "$FALHAS" -ne 0 ]; then
    exit 1
fi
exit 0
