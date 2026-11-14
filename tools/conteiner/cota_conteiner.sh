# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Teto de contêiner (NL-3.K; NL-I.266, NL-I.275). Ponto único do teto de CPU e de memória de todo docker run e docker build,
# para que nenhum build tome a máquina (decisão do líder, 13/11/2026: "Consertar só o teto (Recomendado)"). Nenhum núcleo é
# preso por padrão; NL_CPUSET explícita continua aceita. Valor do teto: 7 CPUs e 12 GB, o que o projeto mede desde 30/10/2026;
# o porquê está no plano da NL-3.K (seção 1). A regra de CPU (NL_COTA_PADRAO, nl_threads_valido, nl_cpuset_valido) mora em
# cota_nucleos.sh. Carregado por ponto. Não é executável.
# Uso: nl_cotas_docker || exit 2; docker run --pull=never "${NL_COTAS_DOCKER[@]}" ...; docker build -t X "${NL_COTAS_DOCKER_BUILD[@]}" ...
# O docker build não aceita --cpus; o teto de CPU dele vai por --cpu-period e --cpu-quota. Medido em 13/11/2026 (NL-3.K, passo 0,
# Docker 29.9.3): o BuildKit e o builder legado aplicam cpu.max "700000 100000" e memory.max 12884901888 ao passo RUN.
. "$(dirname "${BASH_SOURCE[0]}")/cota_nucleos.sh"

# Memória por contêiner, igual em --memory e --memory-swap (o contêiner não usa swap além dela). Teto em MiB.
NL_COTA_MEMORIA=12g
NL_COTA_MEMORIA_MIB=12288

# nl_memoria_valida VALOR: imprime a memória efetiva e sai 0; sai 1 com REPROVA em stderr. VALOR vazio vale NL_COTA_MEMORIA.
# Só passa inteiro positivo sem zero à esquerda, de até 5 dígitos, seguido de m ou g, até o teto.
nl_memoria_valida() {
    local valor="${1:-}" numero mib
    [ -n "$valor" ] || valor="$NL_COTA_MEMORIA"
    if ! [[ "$valor" =~ ^([1-9][0-9]{0,4})([mg])$ ]]; then
        echo "REPROVA: NL_MEMORIA=$valor fora do formato (ex.: 12g ou 4096m)" >&2
        return 1
    fi
    numero="${BASH_REMATCH[1]}"
    mib="$numero"
    if [ "${BASH_REMATCH[2]}" = g ]; then
        mib=$((numero * 1024))
    fi
    if [ "$mib" -gt "$NL_COTA_MEMORIA_MIB" ]; then
        echo "REPROVA: NL_MEMORIA=$valor acima do teto de $NL_COTA_MEMORIA (NL-3.K)" >&2
        return 1
    fi
    echo "$valor"
}

# nl_cotas_docker: monta os vetores NL_COTAS_DOCKER (run) e NL_COTAS_DOCKER_BUILD (build) e os efetivos NL_COTA_CPUSET,
# NL_COTA_THREADS e NL_COTA_MEM.
# Lê NL_THREADS, NL_MEMORIA e NL_CPUSET. Sem NL_THREADS, o teto é NL_COTA_PADRAO, reduzido com AVISO em máquina com menos
# CPUs (o docker recusa --cpus acima do que existe; o runner do CI tem 4). O número de CPUs é o da máquina (nproc --all):
# dentro de um contêiner com --cpus, o nproc simples devolve a cota e não as CPUs que o docker vê (NL-3.K, revisão, A-3).
# Sem NL_CPUSET, nenhum núcleo é preso.
# Costuras só dos autotestes: NL_NPROC_TESTE (no lugar de nproc) e NL_CPUS_ONLINE_ARQ (no lugar de /sys/.../online).
# Valor inválido: PARADA em stderr, rc 2, e o vetor fica vazio (quem chama sai antes de qualquer docker run).
nl_cotas_docker() {
    local online="" arq="${NL_CPUS_ONLINE_ARQ:-/sys/devices/system/cpu/online}" np threads memoria cpuset=todas
    NL_COTAS_DOCKER=()
    NL_COTAS_DOCKER_BUILD=()
    np="${NL_NPROC_TESTE:-$(nproc --all)}"
    if [ -n "${NL_THREADS:-}" ]; then
        threads="$(nl_threads_valido "$NL_THREADS" "$np")" || { echo "PARADA: NL_THREADS inválida" >&2; return 2; }
    else
        threads="$NL_COTA_PADRAO"
        if [ "$threads" -gt $((np - 1)) ]; then
            threads=$((np - 1))
            if [ "$threads" -lt 1 ]; then
                threads=1
            fi
            echo "AVISO: máquina com $np CPUs; teto de CPU do contêiner reduzido de $NL_COTA_PADRAO para $threads" >&2
        fi
    fi
    memoria="$(nl_memoria_valida "${NL_MEMORIA:-}")" || { echo "PARADA: NL_MEMORIA inválida" >&2; return 2; }
    if [ -n "${NL_CPUSET:-}" ]; then
        if [ -r "$arq" ]; then
            read -r online < "$arq" || true
        fi
        cpuset="$(nl_cpuset_valido "$NL_CPUSET" "$online")" || { echo "PARADA: NL_CPUSET inválida" >&2; return 2; }
    fi
    if [ "$cpuset" != todas ]; then
        NL_COTAS_DOCKER=(--cpuset-cpus "$cpuset")
        NL_COTAS_DOCKER_BUILD=(--cpuset-cpus "$cpuset")
    fi
    NL_COTAS_DOCKER+=(--cpus "$threads" --memory "$memoria" --memory-swap "$memoria")
    NL_COTAS_DOCKER_BUILD+=(--cpu-period 100000 --cpu-quota $((threads * 100000)) --memory "$memoria" --memory-swap "$memoria")
    NL_COTA_CPUSET="$cpuset"
    NL_COTA_THREADS="$threads"
    NL_COTA_MEM="$memoria"
}
