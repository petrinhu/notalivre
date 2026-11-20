# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Linha NL-PIDS do executor do banco (NL-3.1h, v3.19). O corpo ctest_banco_no_conteiner.sh a imprime de dentro do contêiner,
# direto e sem o prefixo do ctest, por isso a âncora é o começo da linha. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_conferir_pids LOG PAR: imprime NL-BANCO-PIDS do par e sai 0 só com exatamente uma linha NL-PIDS, pico inteiro e
# eventos_max=0 (o executor nunca bateu no teto). Senão, imprime a PARADA e sai 1. Um awk só (L-11).
nl_banco_conferir_pids() {
    local valores maximo pico eventos rc=0
    valores="$(awk '/^NL-PIDS max=[^ ]+ peak=[^ ]+ eventos_max=[^ ]+$/ { n++; v = $2 " " $3 " " $4 } END { if (n != 1) exit 1; gsub(/[a-z_]+=/, "", v); print v }' "$1")" || {
        echo "PARADA: linha NL-PIDS (par $2: ausente ou repetida)"
        return 1
    }
    read -r maximo pico eventos <<<"$valores"
    echo "NL-BANCO-PIDS par=$2 max=$maximo peak=$pico eventos_max=$eventos"
    case "$pico" in
        ''|*[!0-9]*) echo "PARADA: linha NL-PIDS (par $2: peak=$pico, sem número)"; rc=1 ;;
    esac
    if [ "$eventos" != 0 ]; then
        echo "PARADA: linha NL-PIDS (par $2: eventos_max=$eventos, o executor bateu no teto)"; rc=1
    fi
    return "$rc"
}
