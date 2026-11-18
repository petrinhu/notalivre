#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Impressão do certificado do servidor do par (NL-3.1h, C4' e seção 1.3). Uso: impressao_servidor_banco.sh <certificado> <pem|->.
# Roda só dentro de contêiner (a sonda sobe na rede do servidor). Imprime NL-IMPRESSAO certificado=<c> disco=<hex|nd>
# sonda=<hex|sem_tls|erro> usada=<hex|nenhuma> e sai 0 só se a classe confere; senão sai 3.
set -u
[ -f /.dockerenv ] || { echo "PARADA: só em contêiner"; exit 2; }
CERT="${1:?uso: impressao_servidor_banco.sh <disco|memoria|nenhum> <pem|->}"
PEM="${2:?uso: impressao_servidor_banco.sh <disco|memoria|nenhum> <pem|->}"
DISCO=nd
SONDA=erro
USADA=nenhuma
ERRO_TMP="$(mktemp)" || { echo "PARADA: mktemp"; exit 2; }
trap 'rm -f -- "$ERRO_TMP"' EXIT

if [ "$CERT" = disco ]; then
    DISCO="$(openssl x509 -in "$PEM" -noout -fingerprint -sha256 2>/dev/null | sed 's/^.*=//; s/://g')"
    [ -n "$DISCO" ] || DISCO=erro
fi

# A sonda fala STARTTLS com o servidor em 127.0.0.1. Sem impressão: sem_tls só se o erro disser "does not support SSL".
HEX_SONDA="$(openssl s_client -connect 127.0.0.1:3306 -starttls mysql </dev/null 2>"$ERRO_TMP" \
    | openssl x509 -noout -fingerprint -sha256 2>/dev/null | sed 's/^.*=//; s/://g')"
if [ -n "$HEX_SONDA" ]; then
    SONDA="$HEX_SONDA"
elif /usr/bin/grep -q 'does not support SSL' "$ERRO_TMP"; then
    SONDA=sem_tls
fi

rc=3
case "$CERT" in
    disco)
        if [ "$DISCO" != erro ] && [ "$DISCO" = "$SONDA" ]; then USADA="$SONDA"; rc=0; fi ;;
    memoria)
        if [ "$DISCO" = nd ] && [ "$SONDA" != erro ] && [ "$SONDA" != sem_tls ]; then USADA="$SONDA"; rc=0; fi ;;
    nenhum)
        if [ "$DISCO" = nd ] && [ "$SONDA" = sem_tls ]; then USADA=nenhuma; rc=0; fi ;;
esac
echo "NL-IMPRESSAO certificado=$CERT disco=$DISCO sonda=$SONDA usada=$USADA"
exit "$rc"
