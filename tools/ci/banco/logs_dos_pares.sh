#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Fim do log de cada par vermelho no log do job banco (NL-3.1v, seção 1.7 do plano do CTO). O harness grava o log de cada
# par em /var/tmp/notalivre-logs, 0600 do usuário de uid 1000, e no log da distro deixa só "PARADA: par <d>/<s> vermelho".
# Este passo lê, como esse usuário (sudo -u), as linhas de falha do QTest e do CTest e o fim de cada log de par vermelho.
# O log do CI é público: se o harness acusou "PARADA: senha encontrada" em qualquer par, nenhum log de par é mostrado.
# Não muda o rc do job: o passo do harness já reprovou. Uso: logs_dos_pares.sh LOG DISTRO SHA USUARIO
set -u
LOGS=/var/tmp/notalivre-logs
LINHAS_FIM=60

# nl_logs_vermelhos LOG DISTRO: imprime o nome do servidor de cada par vermelho da DISTRO, só com [a-z0-9].
nl_logs_vermelhos() {
    sed -n "s|^PARADA: par $2/\([a-z0-9]*\) vermelho.*|\1|p" "$1"
}

# nl_logs_falhas USUARIO ARQUIVO: imprime, lidas como USUARIO, as linhas de falha do QTest e o resumo do CTest.
nl_logs_falhas() {
    sudo -n -u "$1" grep -E 'FAIL!  :|^ *Loc: |\*\*\*Failed|tests passed|PARADA' -- "$2" || true
}

# nl_logs_fim USUARIO ARQUIVO: imprime, lidas como USUARIO, as últimas LINHAS_FIM linhas do ARQUIVO.
nl_logs_fim() {
    sudo -n -u "$1" tail -n "$LINHAS_FIM" -- "$2"
}

LOG="${1:-}"
DISTRO="${2:-}"
SHA="${3:-}"
USUARIO="${4:-}"
[ -n "$LOG" ] && [ -n "$DISTRO" ] && [ -n "$SHA" ] && [ -n "$USUARIO" ] || { echo "PARADA: uso: logs_dos_pares.sh LOG DISTRO SHA USUARIO"; exit 2; }
[ -f "$LOG" ] || { echo "AVISO: log do harness ausente ($LOG); nada a mostrar"; exit 0; }
if grep -q 'PARADA: senha encontrada' "$LOG"; then
    echo "AVISO: o harness acusou senha em log; nenhum log de par é mostrado (log do CI é público)"
    exit 0
fi
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    arquivo="$LOGS/NL-3.1h-par-$DISTRO-$nome-$SHA.log"
    echo "::group::par $DISTRO/$nome vermelho: $arquivo"
    echo "--- linhas de falha"
    nl_logs_falhas "$USUARIO" "$arquivo"
    echo "--- últimas $LINHAS_FIM linhas"
    nl_logs_fim "$USUARIO" "$arquivo" || echo "AVISO: log do par ausente ou ilegível"
    echo "::endgroup::"
done <<<"$(nl_logs_vermelhos "$LOG" "$DISTRO")"
exit 0
