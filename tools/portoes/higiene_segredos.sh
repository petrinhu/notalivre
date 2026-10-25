#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Portão de higiene de segredos do NotaLivre (NL-0.6), com gitleaks em modo dir.
# Uso: higiene_segredos.sh RAIZ
#   Lê RAIZ/.gitleaks.toml (fail-closed: sem o arquivo, reprova).
#   Modo dir: enxerga só os arquivos da árvore, sem histórico git. O histórico é
#   coberto pelo job de CI com o modo git.
# Saída: uma linha "encontrados=N gitleaks_rc=R", depois PASSA ou REPROVA.
# Termina com exit 1 em qualquer reprovação, antes de qualquer ação que proteja.
set -u

RAIZ="${1:-}"
if [ -z "$RAIZ" ] || [ ! -d "$RAIZ" ]; then
    echo "encontrados=0 gitleaks_rc=-"
    echo "REPROVA: raiz ausente ou inexistente: '$RAIZ'"
    exit 1
fi

CONFIG="$RAIZ/.gitleaks.toml"
if [ ! -f "$CONFIG" ]; then
    echo "encontrados=0 gitleaks_rc=-"
    echo "REPROVA: .gitleaks.toml ausente em $RAIZ (fail-closed)"
    exit 1
fi

# Piso de varredura: zero arquivos é varredura quebrada, não árvore limpa (L-36)
ENC=$(/usr/bin/find "$RAIZ" -path '*/.git' -prune -o -type f -print | /usr/bin/wc -l | tr -d ' ')
if [ "${ENC:-0}" -eq 0 ]; then
    echo "encontrados=0 gitleaks_rc=-"
    echo "REPROVA: varredura vazia (zero arquivos em $RAIZ)"
    exit 1
fi

gitleaks dir "$RAIZ" --config "$CONFIG" --redact --no-banner --exit-code 1
GL=$?
echo "encontrados=$ENC gitleaks_rc=$GL"

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
