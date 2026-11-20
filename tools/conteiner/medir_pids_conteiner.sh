# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Medição de processos dentro do contêiner (NL-3.1h, emenda v3.11): a linha NL-PIDS, com o teto (pids.max), o pico (pids.peak)
# e as negações do teto (eventos_max, o campo "max" de pids.events). Carregado por ponto pelo executar_no_conteiner.sh e pelo
# autoteste_paralelo_conteiner.sh. Não é executável. Leitura por read do bash, sem processo novo (L-11). Arquivo ausente ou
# ilegível vira "ausente", nunca zero. Costura NL_ARQ_CGROUP (só do autoteste); o padrão é /sys/fs/cgroup.

# nl_pids_ler ARQUIVO: imprime a primeira linha do arquivo, ou ausente.
nl_pids_ler() {
    local valor=""
    if [ -r "$1" ] && read -r valor < "$1" && [ -n "$valor" ]; then
        echo "$valor"
    else
        echo ausente
    fi
}

# nl_pids_eventos_max ARQUIVO: imprime o valor da linha "max" de pids.events, ou ausente.
nl_pids_eventos_max() {
    local arquivo="$1" chave="" valor=""
    if [ ! -r "$arquivo" ]; then
        echo ausente
        return 0
    fi
    while read -r chave valor; do
        if [ "$chave" = max ] && [ -n "$valor" ]; then
            echo "$valor"
            return 0
        fi
    done < "$arquivo"
    echo ausente
}

# nl_pids_linha: imprime "NL-PIDS max=<pids.max> peak=<pids.peak> eventos_max=<pids.events max>".
nl_pids_linha() {
    local dir="${NL_ARQ_CGROUP:-/sys/fs/cgroup}" maximo pico eventos
    maximo="$(nl_pids_ler "$dir/pids.max")"
    pico="$(nl_pids_ler "$dir/pids.peak")"
    eventos="$(nl_pids_eventos_max "$dir/pids.events")"
    echo "NL-PIDS max=$maximo peak=$pico eventos_max=$eventos"
}
