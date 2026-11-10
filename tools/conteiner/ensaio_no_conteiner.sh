#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Corpo do ensaio de repetição (NL-3.0j, decisão do CTO de 10/11/2026). Roda DENTRO do contêiner, com cwd /src, chamado
# pelo build_local.sh ensaio como: bash /ensaio/ensaio_no_conteiner.sh <preset>, com NL_ENSAIO='nome:N nome:N ...'.
# Ordem, sem laço dentro de slot:
#   1. confere RLIMIT_CORE 1 1 (o contêiner foi criado com --ulimit core=1); outro valor aborta antes de tudo;
#   2. configura e constrói o preset;
#   3. roda a fixture certificados.gerar UMA vez, fora da repetição;
#   4. para cada slot nome:N, um ctest com --repeat until-fail:N: um processo novo por repetição, -FS da fixture,
#      --no-tests=error, -j 1 e -V (a saída inteira vai para o log);
#   5. para na primeira falha de slot.
# Cada slot imprime "ensaio teste=... pedidas=... processos=... aprovadas=... medicao=... rc=..."; o resumo sai sempre.
# Códigos: 0 verde; 1 slot reprovado; 2 core fora de 1 1; 3 configure ou build; 4 fixture; 5 lista ou uso inválidos;
# 6 temporário não criado.
# Nenhum limite de tempo é alterado aqui: o limite por função do Qt Test não se mexe em lugar nenhum (regra do CTO).
set -u

if [ "$#" -ne 1 ]; then
    echo "uso: ensaio_no_conteiner.sh <preset>" >&2
    exit 5
fi
preset="$1"

limite_core="$(awk '/^Max core file size/ {print $5 " " $6}' /proc/self/limits)"
echo "limite de core: $limite_core (esperado 1 1)"
if [ "$limite_core" != "1 1" ]; then
    echo "ENSAIO PARADA: RLIMIT_CORE '$limite_core', esperado '1 1' (L-36)"
    exit 2
fi
echo "pids.max: $(cat /sys/fs/cgroup/pids.max 2>/dev/null || echo ausente)"

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

if ! cmake --preset "$preset" || ! cmake --build --preset "$preset"; then
    echo "ENSAIO PARADA: configure ou build reprovou"
    exit 3
fi

# Fixture: uma vez, fora da repetição (o -FS de cada slot a tira do conjunto).
if ! ctest --preset "$preset" --no-tests=error --output-on-failure -R '^certificados[.]gerar$'; then
    echo "ENSAIO PARADA: fixture certificados.gerar reprovou"
    exit 4
fi

falhas=0
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
    a=$(grep -c 'Start testing of' "$saida") || true
    b=$(grep -c -E 'Totals: [0-9]+ passed, 0 failed' "$saida") || true
    c=$(grep -c 'medicao ' "$saida") || true
    echo "loadavg fim teste=$nome: $(cat /proc/loadavg)"
    echo "ensaio teste=$nome pedidas=$n processos=${a:-0} aprovadas=${b:-0} medicao=${c:-0} rc=$rc"
    if [ "$rc" -ne 0 ] || [ "${a:-0}" != "$n" ] || [ "${b:-0}" != "$n" ]; then
        falhas=$((falhas + 1))
        break
    fi
done

echo "pids.peak: $(cat /sys/fs/cgroup/pids.peak 2>/dev/null || echo ausente)"
echo "ensaio resumo slots=$slots reprovados=$falhas"
if [ "$falhas" -ne 0 ]; then
    echo "ENSAIO: VERMELHO"
    exit 1
fi
echo "ENSAIO: VERDE"
exit 0
