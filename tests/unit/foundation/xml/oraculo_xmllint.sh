#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Oráculo independente do validador XSD (NL-1.6a): roda xmllint --schema sobre cada fixture e exige o veredito
# do nome do arquivo (valida_* tem rc 0; invalida_* tem rc diferente de 0). Régua igual à do tst_validador_xsd.
# Uso: oraculo_xmllint.sh <xmllint> <dir_xsd> <dir_fixtures>
# Saída: "encontrados=N analisados=M falharam=F". Sai com 1 se houver divergência, nome sem veredito, varredura
# vazia ou analisados diferente de encontrados. Cada fixture é um processo xmllint (4 hoje, sem recursão).
set -u

XMLLINT="${1:?uso: oraculo_xmllint.sh <xmllint> (XSD e fixtures no ambiente: NL_XSD_DIR, NL_FIXTURES_ORACULO)}"
XSD_DIR="${NL_XSD_DIR:?dir xsd no ambiente (NL_XSD_DIR, NL-2.14 adendo 5)}"
FIXTURES="${NL_FIXTURES_ORACULO:?dir fixtures no ambiente (NL_FIXTURES_ORACULO, NL-2.14 adendo 5)}"
RAIZ_XSD="$XSD_DIR/DPS_v1.01.xsd"

if [ ! -f "$RAIZ_XSD" ]; then
    echo "ERRO: esquema raiz ausente: $RAIZ_XSD"
    exit 1
fi

versao="$("$XMLLINT" --version 2>&1 | head -n 1)"
echo "oraculo: $versao"

encontrados=0
analisados=0
falharam=0
for caminho in "$FIXTURES"/*.xml; do
    if [ ! -f "$caminho" ]; then
        continue
    fi
    encontrados=$((encontrados + 1))
    nome="$(basename "$caminho")"
    case "$nome" in
        valida_*) esperado=valida ;;
        invalida_*) esperado=invalida ;;
        *)
            echo "FALHA: nome sem veredito (use valida_* ou invalida_*): $nome"
            falharam=$((falharam + 1))
            continue
            ;;
    esac
    saida="$("$XMLLINT" --noout --nonet --schema "$RAIZ_XSD" "$caminho" 2>&1)"
    rc=$?
    analisados=$((analisados + 1))
    if [ "$esperado" = valida ] && [ "$rc" -ne 0 ]; then
        echo "DIVERGE: $nome deveria validar, xmllint rc=$rc"
        printf '%s\n' "$saida" | head -n 5
        falharam=$((falharam + 1))
    elif [ "$esperado" = invalida ] && [ "$rc" -eq 0 ]; then
        echo "DIVERGE: $nome deveria falhar, xmllint rc=0"
        printf '%s\n' "$saida" | head -n 5
        falharam=$((falharam + 1))
    else
        echo "OK: $nome esperado=$esperado xmllint_rc=$rc"
    fi
done

echo "encontrados=$encontrados analisados=$analisados falharam=$falharam"
if [ "$encontrados" -eq 0 ]; then
    echo "ERRO: varredura vazia (zero fixtures). Zero não é sinal de código limpo."
    exit 1
fi
if [ "$analisados" -ne "$encontrados" ] || [ "$falharam" -ne 0 ]; then
    exit 1
fi
exit 0
