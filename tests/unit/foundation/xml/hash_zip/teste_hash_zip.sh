#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Teste do EXPECTED_HASH do ZIP do XSD (NL-1.6a, mutação M3). Configura o projeto mínimo hash_zip/ com NL_XSD_URL em
# file:// (hermético, sem rede). Modo adulterado: o ZIP tem um byte trocado; o configure TEM de falhar com a mensagem
# "HASH mismatch" ou "SHA-256 esperado". Modo controle: o ZIP certo; o configure TEM de passar.
# O adulterado muda SÓ o comentário do ZIP: os membros (.xsd) são idênticos e extraíveis, então só o hash pode recusar.
# Sem o EXPECTED_HASH o adulterado passa e o teste fica vermelho.
# Uso: teste_hash_zip.sh <adulterado|controle> <raiz_fonte> <zip_certo> <dir_trabalho>
set -u

MODO="${1:?uso: teste_hash_zip.sh <adulterado|controle> <raiz_fonte> <zip_certo> <dir_trabalho>}"
FONTE="${2:?raiz da fonte}"
ZIP_CERTO="${3:?ZIP certo}"
TRABALHO="${4:?dir de trabalho}"
PROJETO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

rm -rf -- "$TRABALHO"
mkdir -p "$TRABALHO/zip"

case "$MODO" in
    controle)
        ZIP="$ZIP_CERTO"
        ;;
    adulterado)
        # Mesmo conteúdo extraível, outro comentário do arquivo (outro SHA-256, mesmos .xsd).
        ZIP="$TRABALHO/zip/esquemas_adulterado.zip"
        if ! python3 -I -c 'import sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as zin, zipfile.ZipFile(sys.argv[2], "w", zipfile.ZIP_DEFLATED) as zout:
    zout.comment = b"adulterado: so o comentario do ZIP muda"
    for info in zin.infolist():
        zout.writestr(info, zin.read(info.filename))' "$ZIP_CERTO" "$ZIP"; then
            echo "ERRO: não consegui gerar o ZIP adulterado a partir de $ZIP_CERTO"
            exit 1
        fi
        ;;
    *)
        echo "ERRO: modo desconhecido: $MODO"
        exit 2
        ;;
esac

cmake -S "$PROJETO" -B "$TRABALHO/bin" -DNL_FONTE="$FONTE" -DNL_XSD_URL="file://$ZIP" > "$TRABALHO/configure.log" 2>&1
rc=$?
echo "modo=$MODO configure_rc=$rc zip=$ZIP"
grep -n 'NL-1.6a' "$TRABALHO/configure.log" | head -n 3

if [ "$MODO" = adulterado ]; then
    if [ "$rc" -eq 0 ]; then
        echo "FALHA: configure aceitou ZIP adulterado (EXPECTED_HASH ausente ou não efetivo)"
        exit 1
    fi
    if ! grep -q -e 'HASH mismatch' -e 'SHA-256 esperado' "$TRABALHO/configure.log"; then
        echo "FALHA: configure falhou, mas sem a mensagem de hash (causa errada)"
        exit 1
    fi
    echo "OK: ZIP adulterado recusado pela conferência de SHA-256"
    exit 0
fi

if [ "$rc" -ne 0 ]; then
    echo "FALHA: controle com ZIP certo não configurou"
    exit 1
fi
echo "OK: controle com ZIP certo configurou"
exit 0
