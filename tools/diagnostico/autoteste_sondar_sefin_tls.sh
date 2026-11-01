#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da trava da sonda sondar_sefin_tls.sh (NL-2.10e, revisão de segurança). Prova, sem conexão nenhuma: (1) sem
# NL_SONDA_SEFIN_CONFIRMO=1 a sonda recusa e não chama o openssl; (2) com CI definida, recusa mesmo com a confirmação;
# (3) com a confirmação e sem CI, a trava abre e o openssl é chamado. O openssl do PATH é um stub que só registra a chamada.
# Também prova que o script não usa /tmp literal. Roda dentro de contêiner com --network none (sem rede no autoteste).
#
# Uso: autoteste_sondar_sefin_tls.sh   (sai com 0 só se os quatro casos passarem)
set -u

AQUI="$(cd "$(dirname "$0")" && pwd)"
SONDA="$AQUI/sondar_sefin_tls.sh"
BASE="${NL_TMP_BASE:-/var/tmp}"
DIR="$(mktemp -d "$BASE/autoteste-sonda.XXXXXX")" || { echo "FALHA: não criei a pasta do autoteste"; exit 1; }
mkdir -p "$DIR/stub" "$DIR/home"

# Stub do openssl: nunca conecta. Só grava a marca e sai com 0.
cat > "$DIR/stub/openssl" <<'STUB'
#!/bin/sh
: > "$MARCA"
exit 0
STUB
chmod 755 "$DIR/stub/openssl"

FALHAS=0
TOTAL=0

# $1 nome; $2 "chamou" ou "nao_chamou"; $3 "recusa" ou "segue"; resto: variáveis de ambiente do caso (além do ambiente limpo).
caso() {
    local nome="$1" esperado_marca="$2" esperado_saida="$3"; shift 3
    local marca="$DIR/marca-$nome" saida rc achou_marca=nao_chamou
    TOTAL=$(( TOTAL + 1 ))
    saida="$(env -i PATH="$DIR/stub:/usr/bin:/bin" HOME="$DIR/home" NL_TMP_BASE="$BASE" MARCA="$marca" "$@" \
        bash "$SONDA" 127.0.0.1 2>&1 </dev/null)"
    rc=$?
    [ -e "$marca" ] && achou_marca=chamou
    local problema=""
    [ "$achou_marca" = "$esperado_marca" ] || problema="$problema openssl=$achou_marca(esperado $esperado_marca)"
    if [ "$esperado_saida" = recusa ]; then
        [ "$rc" -eq 2 ] || problema="$problema rc=$rc(esperado 2)"
        printf '%s\n' "$saida" | grep -q 'RECUSADA' || problema="$problema sem_RECUSADA"
    else
        printf '%s\n' "$saida" | grep -q 'RECUSADA' && problema="$problema recusou_com_confirmacao"
    fi
    if [ -z "$problema" ]; then
        echo "ok $nome: rc=$rc openssl=$achou_marca"
    else
        echo "FALHA $nome:$problema"
        printf '%s\n' "$saida" | head -3 | sed 's/^/    /'
        FALHAS=$(( FALHAS + 1 ))
    fi
}

caso sem_confirmacao nao_chamou recusa
caso confirmacao_zero nao_chamou recusa NL_SONDA_SEFIN_CONFIRMO=0
caso ci_definida_recusa nao_chamou recusa CI=1 NL_SONDA_SEFIN_CONFIRMO=1
caso confirmada_sem_ci chamou segue NL_SONDA_SEFIN_CONFIRMO=1

# Sem /tmp literal no script (regra de temporários, L-11): só NL_TMP_BASE. A regex não casa /var/tmp (o caminho do padrão).
# Controle positivo e negativo antes da varredura real: se a regex não pegar /tmp ou pegar /var/tmp, o teste está quebrado.
PADRAO_TMP='(^|[^[:alnum:]_])/tmp([/ "]|$)'
TOTAL=$(( TOTAL + 1 ))
ctl_pos=$(printf 'mk /tmp/sonda\n' | grep -c -E "$PADRAO_TMP")
ctl_neg=$(printf 'base /var/tmp;\n' | grep -c -E "$PADRAO_TMP")
if [ "$ctl_pos" -ne 1 ] || [ "$ctl_neg" -ne 0 ]; then
    echo "FALHA controle_regex_tmp: positivo=$ctl_pos (esperado 1) negativo=$ctl_neg (esperado 0)"
    FALHAS=$(( FALHAS + 1 ))
elif grep -q -E "$PADRAO_TMP" "$SONDA"; then
    echo "FALHA sem_tmp_literal: o script ainda contém /tmp"
    FALHAS=$(( FALHAS + 1 ))
else
    echo "ok sem_tmp_literal (controles: positivo=1 negativo=0)"
fi

echo "autoteste sonda Sefin: casos=$TOTAL falhas=$FALHAS (pasta: $DIR)"
[ "$FALHAS" -eq 0 ]
