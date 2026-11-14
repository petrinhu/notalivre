# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Cota de núcleos do espelho e da sabotagem (NL-3.0k, ordem do CTO de 07/11/2026) e reserva de CPUs do build (NL-3.0j, ordem
# do líder de 11/11/2026). Átomo único: o padrão e a regra de validação moram aqui. Carregado por ponto, com caminho
# relativo ao próprio script: `. .../cota_nucleos.sh`.
# Não é executável. Chamadores: tools/espelho/job_flatpak.sh e tools/mutacao/sabotar_portoes.sh (modo flatpak), que usam
# nl_threads_valido; e tools/conteiner/build_local.sh, que usa nl_cpuset_valido.

# Teto de CPU por contêiner e cota do flatpak-builder, em núcleos (NL-3.K; o porquê está no plano da NL-3.K, seção 1).
NL_COTA_PADRAO=7

# nl_threads_valido VALOR NPROC: imprime a cota efetiva em stdout e sai 0; sai 1 com REPROVA em stderr se for inválida.
# VALOR vazio vale NL_COTA_PADRAO. Só passa inteiro positivo sem zero à esquerda, até NPROC-1 (um núcleo fica ao sistema).
# Acima de NL_COTA_PADRAO reprova: é o teto (NL-3.K, decisão do líder de 13/11/2026, "Consertar só o teto (Recomendado)").
# Valor com mais de 4 dígitos reprova antes da comparação: a aritmética do bash estoura em número longo e daria verde falso.
nl_threads_valido() {
    local valor="${1:-}" np="${2:?uso: nl_threads_valido VALOR NPROC}" teto
    [ -n "$valor" ] || valor="$NL_COTA_PADRAO"
    teto=$((np - 1))
    if ! [[ "$valor" =~ ^[1-9][0-9]*$ ]]; then
        echo "REPROVA: NL_THREADS=$valor não é inteiro positivo sem zero à esquerda (ex.: NL_THREADS=7)" >&2
        return 1
    fi
    if [ "${#valor}" -gt 4 ] || [ "$valor" -gt "$teto" ]; then
        echo "REPROVA: NL_THREADS=$valor acima do teto de $teto núcleos (nproc $np, um fica ao sistema)" >&2
        return 1
    fi
    if [ "$valor" -gt "$NL_COTA_PADRAO" ]; then
        echo "REPROVA: NL_THREADS=$valor acima do teto de $NL_COTA_PADRAO núcleos (NL-3.K)" >&2
        return 1
    fi
    echo "$valor"
}

# Reserva de CPUs (NL-3.0j, ordem do líder de 11/11/2026): as primeiras NL_CPUS_RESERVADAS CPUs ficam livres para a sessão do
# líder. Em máquina com menos de NL_CPUS_MINIMO_PARA_RESERVA CPUs (runner do CI, notebook) não há reserva, e isso sai em AVISO.
NL_CPUS_RESERVADAS=4
NL_CPUS_MINIMO_PARA_RESERVA=8

# nl_cpuset_valido VALOR ONLINE: imprime em stdout a lista para --cpuset-cpus, ou a palavra "todas" (sem a opção), e sai 0;
# sai 1 com REPROVA em stderr se VALOR for inválido. ONLINE é o conteúdo de /sys/devices/system/cpu/online.
# VALOR vazio: padrão calculado, NL_CPUS_RESERVADAS até a última CPU. VALOR "todas": sem reserva, por escolha explícita.
# VALOR lista (ex.: 4-15 ou 4,6,8-15): só dígitos, vírgula e hífen, sem zero à esquerda, cada CPU até a última online.
nl_cpuset_valido() {
    local valor="${1:-}" online="${2:-}" ultima="" parte ini fim
    local numero='(0|[1-9][0-9]{0,3})'
    if [[ "$online" =~ ^0-([1-9][0-9]{0,3})$ ]]; then
        ultima="${BASH_REMATCH[1]}"
    elif [ "$online" = 0 ]; then
        ultima=0
    fi
    if [ -z "$valor" ]; then
        if [ -z "$ultima" ]; then
            echo "AVISO: CPUs online '$online' fora da forma 0-N; sem reserva de CPU" >&2
            echo todas
            return 0
        fi
        if [ $((ultima + 1)) -lt "$NL_CPUS_MINIMO_PARA_RESERVA" ]; then
            echo "AVISO: máquina com $((ultima + 1)) CPUs, abaixo de $NL_CPUS_MINIMO_PARA_RESERVA; sem reserva de CPU" >&2
            echo todas
            return 0
        fi
        echo "$NL_CPUS_RESERVADAS-$ultima"
        return 0
    fi
    if [ "$valor" = todas ]; then
        echo "AVISO: NL_CPUSET=todas, sem reserva de CPU" >&2
        echo todas
        return 0
    fi
    if [ "${#valor}" -gt 64 ] || ! [[ "$valor" =~ ^$numero(-$numero)?(,$numero(-$numero)?)*$ ]]; then
        echo "REPROVA: NL_CPUSET fora do formato (lista de CPUs como 4-15, ou a palavra todas)" >&2
        return 1
    fi
    if [ -z "$ultima" ]; then
        echo "REPROVA: CPUs online '$online' fora da forma 0-N; NL_CPUSET explícito não pode ser conferido" >&2
        return 1
    fi
    local IFS=,
    for parte in $valor; do
        ini="${parte%-*}"
        fim="${parte#*-}"
        if [ "$ini" -gt "$fim" ] || [ "$fim" -gt "$ultima" ]; then
            echo "REPROVA: NL_CPUSET=$valor tem faixa invertida ou CPU acima de $ultima" >&2
            return 1
        fi
        if [ "$ini" -lt "$NL_CPUS_RESERVADAS" ]; then
            echo "AVISO: NL_CPUSET=$valor inclui CPU reservada (0 a $((NL_CPUS_RESERVADAS - 1)))" >&2
        fi
    done
    echo "$valor"
}
