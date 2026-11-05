#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do executor do espelho (NL-3.0g, revisão de segurança, achado 1; L-35 e L-36): a conferência --sem-run de
# rodar_espelho.sh roda na área própria do espelho (copiar_arvore, pasta 700 do usuário), nunca num caminho previsível
# dentro de NL_TMP_BASE. Controles:
#   C1 área com escrita de grupo já existente: rodar_espelho REPROVA antes do clone e antes do primeiro job;
#   C2 área privada 700: a conferência passa, a cópia fica dentro da área e o laço de jobs é alcançado (controle:
#      a guarda não pode bloquear o caso bom).
# Sem trabalho pesado: um bash de mentira no PATH registra cada job em vez de rodá-lo, e um docker de mentira faz a
# conferência de lista passar. Nenhum job de verdade roda aqui.
# Uso: autoteste_rodar_espelho.sh   (sem argumentos; sai 0 só com todos os controles verdes)
set -u

DIR="$(cd "$(dirname "$0")" && pwd)"
RAIZ="$(cd "$DIR/../.." && pwd)"
REAL_BASH="$(command -v bash)"
TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/rodar-autoteste.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT
FALHAS=0
export REAL_BASH

verde() { echo "[VERDE] $1"; }
vermelho() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# Bash de mentira: `bash -c CMD` (o que rodar_espelho usa para cada job) só anota CMD em $MARCA e sai 0.
# Qualquer outra chamada segue para o bash de verdade. Docker de mentira: sai 0 (a conferência de lista passa).
STUB="$TMP/stub"
mkdir -p "$STUB"
cat > "$STUB/bash" <<'STUB_BASH'
#!/bin/sh
if [ "$1" = "-c" ]; then echo "$2" >> "$MARCA"; exit 0; fi
exec "$REAL_BASH" "$@"
STUB_BASH
cat > "$STUB/docker" <<'STUB_DOCKER'
#!/bin/sh
exit 0
STUB_DOCKER
chmod +x "$STUB/bash" "$STUB/docker"

# C1: pasta do espelho com escrita de grupo (775) já existente. Nada pode ser clonado nem rodado.
BASE1="$TMP/base1"
mkdir -p "$BASE1/notalivre-espelho" && chmod 775 "$BASE1/notalivre-espelho"
MARCA1="$TMP/marca1"
MARCA="$MARCA1" PATH="$STUB:$PATH" NL_TMP_BASE="$BASE1" bash "$RAIZ/tools/espelho/rodar_espelho.sh" > "$TMP/saida1.log" 2>&1
rc1=$?
residuo1=nao
set -- "$BASE1"/notalivre-espelho-semrun*
[ -e "$1" ] && residuo1=sim
if [ "$rc1" -ne 0 ] && [ "$residuo1" = nao ] && [ ! -e "$BASE1/notalivre-espelho/semrun" ] && [ ! -e "$MARCA1" ]; then
    verde "C1 área 775 existente: REPROVA antes do clone e antes do primeiro job (rc=$rc1, sem residuo, sem job)"
else
    vermelho "C1 área 775 existente: rc=$rc1, residuo_em_base=$residuo1, job alcançado=$([ -e "$MARCA1" ] && echo sim || echo nao)"
    sed 's/^/   | /' "$TMP/saida1.log"
fi

# C2: pasta do espelho privada (700). A conferência passa, a cópia fica dentro da área e os jobs são alcançados.
BASE2="$TMP/base2"
mkdir -p "$BASE2/notalivre-espelho" && chmod 700 "$BASE2/notalivre-espelho"
MARCA2="$TMP/marca2"
MARCA="$MARCA2" PATH="$STUB:$PATH" NL_TMP_BASE="$BASE2" bash "$RAIZ/tools/espelho/rodar_espelho.sh" > "$TMP/saida2.log" 2>&1
rc2=$?
if [ "$rc2" -eq 0 ] && [ -d "$BASE2/notalivre-espelho/semrun" ] && [ -s "$MARCA2" ] && grep -q '^espelho: jobs=.* falhas=0 ' "$TMP/saida2.log"; then
    verde "C2 área 700 do usuário: conferência na área própria, jobs alcançados, rc=$rc2"
else
    vermelho "C2 área 700 do usuário: rc=$rc2, cópia na área ausente ou job não alcançado"
    sed 's/^/   | /' "$TMP/saida2.log"
fi

echo "FALHAS=$FALHAS"
[ "$FALHAS" -eq 0 ] || { echo "AUTOTESTE RODAR ESPELHO: REPROVADO"; exit 1; }
echo "AUTOTESTE RODAR ESPELHO: todos os controles conferem"
exit 0
