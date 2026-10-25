#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão SPDX (NL-0.2, achado A0-06; L-36: portão só conta depois de visto reprovar).
# Cada controle roda spdx.py numa árvore de teste em TMPDIR e exige rc EXATO e a linha de contagem.
# Controles: C1 positivo (código com cabeçalho e JSON declarado passa); C2 código sem cabeçalho reprova;
# C3 varredura vazia reprova; C4 JSON não declarado reprova; C5 exceção obsoleta reprova;
# C6 piso de código (só JSON declarado) reprova.
# Uso: autoteste_spdx.sh        (sem argumentos; roda no contêiner de higiene)
set -u

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORTAO="$DIR/spdx.py"
TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/spdx-autoteste.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT
CAB='# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa'
FALHAS=0

# controle NOME RC_ESPERADO REGEX_ESPERADA ARVORE EXCECOES
controle() {
    local nome="$1" rc_esp="$2" regex="$3" arvore="$4" exc="$5" saida rc
    saida="$TMP/$nome.out"
    python3 -I "$PORTAO" "$arvore" "$exc" > "$saida" 2>&1
    rc=$?
    if [ "$rc" -eq "$rc_esp" ] && grep -qE "$regex" "$saida" \
        && grep -qE '^encontrados=[0-9]+ analisados=[0-9]+ codigo=[0-9]+ violacoes=[0-9]+ excecoes=[0-9]+$' "$saida"; then
        echo "[VERDE] $nome: rc=$rc (esperado $rc_esp)"
    else
        echo "[VERMELHO] $nome: rc=$rc (esperado $rc_esp), saída:"
        sed 's/^/   | /' "$saida"
        FALHAS=$((FALHAS + 1))
    fi
}

# Árvores de teste: arquivos com e sem cabeçalho, sem .git (caminho de percurso de diretório).
arvore_base() { mkdir -p "$TMP/$1/src" && printf '%s\nint ok = 1;\n' "$CAB" > "$TMP/$1/src/ok.cpp"; }
exc_vazia() { : > "$TMP/exc_vazia.txt"; }
exc_json() { printf 'ex.json|teste do autoteste\n' > "$TMP/exc_json.txt"; }
exc_obsoleta() { printf 'fantasma.json|nao existe\n' > "$TMP/exc_obsoleta.txt"; }
exc_vazia

arvore_base c1; printf '{}\n' > "$TMP/c1/ex.json"; exc_json
controle c1-positivo 0 'encontrados=2 analisados=2 codigo=1 violacoes=0 excecoes=1|^PASSA$' "$TMP/c1" "$TMP/exc_json.txt"

arvore_base c2; printf 'int sem = 0;\n' > "$TMP/c2/src/sem.cpp"
controle c2-codigo-sem-cabecalho 1 'VIOLACAO src/sem.cpp: sem cabeçalho SPDX' "$TMP/c2" "$TMP/exc_vazia.txt"

mkdir -p "$TMP/c3"
controle c3-varredura-vazia 1 'REPROVA: varredura vazia' "$TMP/c3" "$TMP/exc_vazia.txt"

arvore_base c4; printf '{}\n' > "$TMP/c4/ex.json"
controle c4-json-nao-declarado 1 'VIOLACAO ex.json: JSON sem exceção declarada' "$TMP/c4" "$TMP/exc_vazia.txt"

arvore_base c5; exc_obsoleta
controle c5-excecao-obsoleta 1 'VIOLACAO fantasma.json: exceção obsoleta' "$TMP/c5" "$TMP/exc_obsoleta.txt"

mkdir -p "$TMP/c6"; printf '{}\n' > "$TMP/c6/ex.json"; exc_json
controle c6-piso-codigo 1 'REPROVA: piso de código' "$TMP/c6" "$TMP/exc_json.txt"

echo "FALHAS=$FALHAS"
[ "$FALHAS" -eq 0 ] || { echo "AUTOTESTE SPDX: REPROVADO"; exit 1; }
echo "AUTOTESTE SPDX: todos os controles conferem"
exit 0
