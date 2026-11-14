#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Corpo do ensaio de repetição (NL-3.0j, decisões do CTO de 10/11 e 11/11/2026). Roda DENTRO do contêiner, com cwd /src,
# chamado pelo build_local.sh ensaio como: bash /ensaio/ensaio_no_conteiner.sh <preset>, com NL_ENSAIO='nome:N nome:N ...'
# e NL_ENSAIO_SHA=<40 hex>.
# Ordem, sem laço dentro de slot:
#   1. imprime o comando recebido (preset, SHA e lista), para o log provar o que rodou;
#   2. confere RLIMIT_CORE 1 1 e pids.max 512 (o contêiner foi criado com --ulimit core=1 e --pids-limit 512); outro valor
#      aborta antes de tudo;
#   3. configura e constrói o preset;
#   4. roda a fixture certificados.gerar UMA vez, fora da repetição;
#   5. para cada slot nome:N, um ctest com --repeat until-fail:N: um processo novo por repetição, -FS da fixture,
#      --no-tests=error, -j 1 e -V (a saída inteira vai para o log);
#   6. para no primeiro slot que não for aprovado.
# Contagem: só linhas com o prefixo "<número>: " que o -V põe na saída do processo de teste. O bloco que o CTest repete
# numa falha não tem o prefixo e não conta. Num slot reprovado, processos é o número da repetição que falhou.
# Cada slot imprime "ensaio teste=... pedidas=... processos=... aprovadas=... medicao=... rc=... classe=... motivo=...".
# Classe, pelo rc do ctest e pelas contagens (L-49):
#   aprovado       rc 0, processos e aprovadas iguais a N;
#   reprovado      rc de 1 a 125 com pelo menos um processo de teste iniciado: o CTest julgou falha;
#   indeterminado  motivo=ferramenta (rc 126 ou mais: o ctest não rodou ou morreu por sinal), motivo=nao_rodou (rc diferente
#                  de 0 e nenhum processo de teste imprimiu o início: nome errado, teste não registrado neste SHA ou
#                  processo que morreu antes da primeira linha) ou motivo=contagem (rc 0 com contagem diferente de N).
#                  Não é reprovação nem aprovação do slot.
# Veredito (CTO, 11/11/2026; NL-3.A, 13/11/2026): o veredito de cada repetição é o do CTest. Desde a NL-3.A nenhum teste
# usa regex de passe, então o código de saída do processo conta; sinal, tempo esgotado e FAIL_REGULAR_EXPRESSION reprovam.
# Códigos: 0 verde; 1 slot reprovado; 2 core fora de 1 1 ou pids.max fora de 512; 3 configure ou build; 4 fixture;
# 5 lista, SHA ou uso inválidos; 6 temporário não criado; 7 indeterminado.
# NL_ENSAIO_ARQ_PIDS_MAX é costura só do autoteste (arquivo lido no lugar de /sys/fs/cgroup/pids.max). O build_local.sh
# nunca a passa ao contêiner, e o autoteste confere isso.
# Nenhum limite de tempo é alterado aqui: o limite por função do Qt Test não se mexe em lugar nenhum (regra do CTO).
set -u

if [ "$#" -ne 1 ]; then
    echo "uso: ensaio_no_conteiner.sh <preset>" >&2
    exit 5
fi
preset="$1"

echo "NL-ENSAIO-PRESET=$preset"
echo "NL-ENSAIO-SHA=${NL_ENSAIO_SHA:-ausente}"
echo "NL-ENSAIO-LISTA=${NL_ENSAIO:-}"
echo "ensaio veredito: o do CTest; sem regex de passe (NL-3.A), o código de saída do processo conta"

grep '^Max core file size' /proc/self/limits || echo "Max core file size: linha ausente em /proc/self/limits"
limite_core="$(awk '/^Max core file size/ {print $5 " " $6}' /proc/self/limits)"
echo "limite de core: $limite_core (esperado 1 1)"
if [ "$limite_core" != "1 1" ]; then
    echo "ENSAIO PARADA: RLIMIT_CORE '$limite_core', esperado '1 1' (L-36)"
    exit 2
fi
arq_pids="${NL_ENSAIO_ARQ_PIDS_MAX:-/sys/fs/cgroup/pids.max}"
pids_max="$(cat "$arq_pids" 2>/dev/null || echo ausente)"
echo "pids.max: $pids_max (esperado 512)"
if [ "$pids_max" != "512" ]; then
    echo "ENSAIO PARADA: pids.max '$pids_max', esperado '512' (exceção do líder à L-11, 10/11/2026)"
    exit 2
fi

if ! [[ "${NL_ENSAIO_SHA:-}" =~ ^[0-9a-f]{40}$ ]]; then
    echo "ENSAIO PARADA: NL_ENSAIO_SHA ausente ou fora de 40 hexadecimais"
    exit 5
fi

# Lista validada antes de qualquer build: nome só com [A-Za-z0-9_.] e N de 1 a 9999 sem zero à esquerda.
read -r -a itens <<< "${NL_ENSAIO:-}"
if [ "${#itens[@]}" -eq 0 ]; then
    echo "ENSAIO PARADA: NL_ENSAIO vazia"
    exit 5
fi
for item in "${itens[@]}"; do
    case "$item" in *:*) ;; *) echo "ENSAIO PARADA: '$item' sem ':'"; exit 5 ;; esac
    nome="${item%:*}"
    n="${item##*:}"
    case "$nome" in ''|*[!A-Za-z0-9_.]*) echo "ENSAIO PARADA: nome '$nome' inválido"; exit 5 ;; esac
    case "$n" in
        [1-9]|[1-9][0-9]|[1-9][0-9][0-9]|[1-9][0-9][0-9][0-9]) ;;
        *) echo "ENSAIO PARADA: N '$n' inválido"; exit 5 ;;
    esac
done
pedidos="${#itens[@]}"

if ! cmake --preset "$preset" || ! cmake --build --preset "$preset"; then
    echo "ENSAIO PARADA: configure ou build reprovou"
    exit 3
fi

# Fixture: uma vez, fora da repetição (o -FS de cada slot a tira do conjunto).
if ! ctest --preset "$preset" --no-tests=error --output-on-failure -R '^certificados[.]gerar$'; then
    echo "ENSAIO PARADA: fixture certificados.gerar reprovou"
    exit 4
fi

reprovados=0
indeterminados=0
slots=0
for item in "${itens[@]}"; do
    nome="${item%:*}"
    n="${item##*:}"
    re="^${nome//./[.]}\$"
    slots=$((slots + 1))
    saida="$(mktemp "${TMPDIR:-/var/tmp}/nl-ensaio-XXXXXX")" || { echo "ENSAIO PARADA: mktemp falhou"; exit 6; }
    echo "loadavg inicio teste=$nome: $(cat /proc/loadavg)"
    ctest --preset "$preset" --no-tests=error -j 1 -V -FS '^certificados_teste$' -R "$re" --repeat "until-fail:$n" > "$saida" 2>&1
    rc=$?
    cat "$saida"
    a=$(grep -c -E '^[0-9]+: [*]+ Start testing of ' "$saida") || true
    b=$(grep -c -E '^[0-9]+: Totals: [0-9]+ passed, 0 failed' "$saida") || true
    c=$(grep -c -E '^[0-9]+: .*medicao ' "$saida") || true
    a="${a:-0}"
    b="${b:-0}"
    c="${c:-0}"
    if [ "$rc" -ge 126 ]; then
        classe=indeterminado
        motivo=ferramenta
    elif [ "$rc" -ne 0 ] && [ "$a" -eq 0 ]; then
        classe=indeterminado
        motivo=nao_rodou
    elif [ "$rc" -ne 0 ]; then
        classe=reprovado
        motivo=ctest
    elif [ "$a" != "$n" ] || [ "$b" != "$n" ]; then
        classe=indeterminado
        motivo=contagem
    else
        classe=aprovado
        motivo=nenhum
    fi
    echo "loadavg fim teste=$nome: $(cat /proc/loadavg)"
    echo "ensaio teste=$nome pedidas=$n processos=$a aprovadas=$b medicao=$c rc=$rc classe=$classe motivo=$motivo"
    case "$classe" in
        aprovado) ;;
        reprovado) reprovados=$((reprovados + 1)); break ;;
        *) indeterminados=$((indeterminados + 1)); break ;;
    esac
done

echo "pids.peak: $(cat /sys/fs/cgroup/pids.peak 2>/dev/null || echo ausente)"
echo "ensaio resumo slots_pedidos=$pedidos slots=$slots reprovados=$reprovados indeterminados=$indeterminados"
if [ "$reprovados" -ne 0 ]; then
    echo "ENSAIO: VERMELHO"
    exit 1
fi
if [ "$indeterminados" -ne 0 ] || [ "$pedidos" -eq 0 ] || [ "$slots" -ne "$pedidos" ]; then
    echo "ENSAIO: INDETERMINADO"
    exit 7
fi
echo "ENSAIO: VERDE"
exit 0
