#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Teste do EXPECTED_HASH do ZIP do XSD (NL-1.6a, mutação M3). Configura o projeto mínimo hash_zip/ com NL_XSD_URL em
# file:// (hermético, sem rede). Modo adulterado: o ZIP tem um byte trocado; o configure TEM de falhar com a mensagem
# "HASH mismatch" ou "SHA-256 esperado". Modo controle: o ZIP certo; o configure TEM de passar.
# O adulterado muda SÓ o comentário do ZIP: os membros (.xsd) são idênticos e extraíveis, então só o hash pode recusar.
# Sem o EXPECTED_HASH o adulterado passa e o teste fica vermelho.
# Uso: teste_hash_zip.sh <adulterado|controle> <raiz_fonte> <zip_certo> <dir_trabalho> <gerador_cmake>
# O gerador vem do projeto pai (CMAKE_GENERATOR, p.ex. Ninja): sem -G, o configure aninhado cai no Unix Makefiles,
# que não existe nas imagens sem make (ubuntu26.04 e manjaro, medido em 25/10 e 26/10/2026).
set -u

MODO="${1:?uso: teste_hash_zip.sh <adulterado|controle> <raiz_fonte> <gerador>}"
FONTE="${2:?raiz da fonte}"
GERADOR="${3:?gerador do CMake pai (CMAKE_GENERATOR)}"
TRABALHO="${NL_HASH_ZIP_TRABALHO:?dir de trabalho no ambiente (NL_HASH_ZIP_TRABALHO, NL-2.14 adendo 5)}"
ZIP_CERTO="${NL_XSD_ZIP_CERTO:?ZIP certo no ambiente (NL_XSD_ZIP_CERTO, NL-2.14 adendo 5)}"
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

# Configure aninhado HERMÉTICO: sem NL_XSD_ZIP_CACHE no ambiente e com o cache vazio por -D. Se o ambiente trouxer
# o cache (o CI exporta NL_XSD_ZIP_CACHE para o passo do XSD), o CMake copiaria o ZIP BOM para o destino e o
# file(DOWNLOAD) com EXPECTED_HASH não leria o ZIP adulterado (NL-1.6c, run 40270511251). Sem isto, o teste não mede nada.
env -u NL_XSD_ZIP_CACHE cmake -G "$GERADOR" -S "$PROJETO" -B "$TRABALHO/bin" -DNL_FONTE="$FONTE" -DNL_XSD_URL="file://$ZIP" \
    -DNL_XSD_ZIP_CACHE= > "$TRABALHO/configure.log" 2>&1
rc=$?
echo "modo=$MODO configure_rc=$rc zip=$ZIP"
grep -n 'NL-1.6a' "$TRABALHO/configure.log" | head -n 3
# Saída aninhada inteira quando o caso falha, para o CI mostrar a causa e não só o veredito.
if { [ "$MODO" = adulterado ] && [ "$rc" -eq 0 ]; } || { [ "$MODO" = controle ] && [ "$rc" -ne 0 ]; }; then
    echo "--- configure.log aninhado ($TRABALHO/configure.log) ---"
    cat "$TRABALHO/configure.log"
    echo "--- fim do configure.log aninhado ---"
fi

if [ "$MODO" = adulterado ]; then
    if [ "$rc" -eq 0 ]; then
        echo "FALHA: configure aceitou ZIP adulterado (EXPECTED_HASH ausente ou não efetivo)"
        exit 1
    fi
    if ! grep -q -e 'HASH mismatch' -e 'SHA-256 esperado' "$TRABALHO/configure.log"; then
        echo "FALHA: configure falhou, mas sem a mensagem de hash (causa errada)"
        cat "$TRABALHO/configure.log"
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
