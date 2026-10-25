#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Portão de higiene de segredos do NotaLivre (NL-0.6), com gitleaks em modo dir.
# Uso: higiene_segredos.sh RAIZ
#   Lê RAIZ/.gitleaks.toml (fail-closed: sem o arquivo, reprova).
#   Modo dir: enxerga só os arquivos da árvore, sem histórico git. O histórico é
#   coberto pelo job de CI com o modo git.
# Saída (A0-10): "encontrados=N analisados=M segredos=K gitleaks_rc=R", depois PASSA ou REPROVA.
#   encontrados: arquivos da árvore (piso de varredura, L-36).
#   analisados:  arquivos que o gitleaks recebeu. Vale N só quando ele termina com rc 0 ou 1; com erro, vale 0.
#   segredos:    achados no relatório JSON (gravado em arquivo, nunca no log: o valor fica redigido).
# Termina com exit 1 em qualquer reprovação, antes de qualquer ação que proteja.
set -u

RAIZ="${1:-}"
if [ -z "$RAIZ" ] || [ ! -d "$RAIZ" ]; then
    echo "encontrados=0 analisados=0 segredos=0 gitleaks_rc=-"
    echo "REPROVA: raiz ausente ou inexistente: '$RAIZ'"
    exit 1
fi

CONFIG="$RAIZ/.gitleaks.toml"
if [ ! -f "$CONFIG" ]; then
    echo "encontrados=0 analisados=0 segredos=0 gitleaks_rc=-"
    echo "REPROVA: .gitleaks.toml ausente em $RAIZ (fail-closed)"
    exit 1
fi

# Piso de varredura: zero arquivos é varredura quebrada, não árvore limpa (L-36)
ENC=$(/usr/bin/find "$RAIZ" -path '*/.git' -prune -o -type f -print | /usr/bin/wc -l | tr -d ' ')
if [ "${ENC:-0}" -eq 0 ]; then
    echo "encontrados=0 analisados=0 segredos=0 gitleaks_rc=-"
    echo "REPROVA: varredura vazia (zero arquivos em $RAIZ)"
    exit 1
fi

RELATORIO="$(mktemp "${TMPDIR:-/var/tmp}/gitleaks-relatorio.XXXXXX")" || { echo "REPROVA: sem arquivo de relatório"; exit 1; }
trap 'rm -f -- "$RELATORIO"' EXIT
gitleaks dir "$RAIZ" --config "$CONFIG" --redact --no-banner --exit-code 1 \
    --report-format json --report-path "$RELATORIO"
GL=$?

if [ "$GL" -eq 0 ] || [ "$GL" -eq 1 ]; then
    ANA="$ENC"
else
    ANA=0
fi
SEG=$(/usr/bin/grep -c '"RuleID"' "$RELATORIO" 2>/dev/null || true)
echo "encontrados=$ENC analisados=$ANA segredos=${SEG:-0} gitleaks_rc=$GL"

case "$GL" in
    0)
        echo "PASSA: nenhum segredo detectado"
        exit 0
        ;;
    1)
        echo "REPROVA: segredo detectado (valor redigido acima)"
        exit 1
        ;;
    *)
        echo "REPROVA: erro da ferramenta gitleaks, rc=$GL (L-49: não é reprovação legítima)"
        exit 1
        ;;
esac
