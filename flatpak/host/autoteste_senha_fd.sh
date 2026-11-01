#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do medidor flatpak/host/senha_fd.sh (NL-2.27, passo 6; L-36): um portão só conta depois de visto reprovar. Roda o
# medidor com dois flatpaks falsos e exige reprovação nos dois: (1) um que não conhece --senha-certificado-fd; (2) um que trava
# (a CLI parada). Sem essas reprovações, o verde da medição real não prova nada. O controle positivo é a própria medição no
# Flatpak real, no job smoke-host do CI.
#
# Uso: autoteste_senha_fd.sh   (sai com 0 só se o medidor reprovar nos dois casos)
set -u

AQUI="$(cd "$(dirname "$0")" && pwd)"
BASE="${TMPDIR:-/var/tmp}"
DIR="$(mktemp -d "$BASE/senha-fd-autoteste.XXXXXX")" || { echo "FALHA: não consegui criar a pasta do autoteste"; exit 1; }

# Stub 1: opção desconhecida. O flatpak recusa --senha-certificado-fd antes de chegar à CLI.
cat > "$DIR/flatpak-opcao" <<'STUB'
#!/bin/sh
case "$1" in
    --version) echo "Flatpak 0.0 (stub: opção desconhecida)"; exit 0 ;;
esac
echo "error: unknown option --senha-certificado-fd" >&2
exit 2
STUB

# Stub 2: a CLI para. Simula o processo parado (o medidor tem de sair pelo teto, sem ficar preso).
cat > "$DIR/flatpak-para" <<'STUB'
#!/bin/sh
case "$1" in
    --version) echo "Flatpak 0.0 (stub: parada)"; exit 0 ;;
esac
exec sleep 60
STUB
chmod 755 "$DIR/flatpak-opcao" "$DIR/flatpak-para"

REPROVOU=0
TOTAL=0
# Cada stub tem de reprovar pelo motivo que ele existe para provar, não por qualquer falha: o autoteste confere a mensagem.
#   opcao: o flatpak recusa a opção, então o descritor 3 não chega à CLI (medida (c));
#   para: a CLI trava, e o teto corta a leitura dos grupos dentro do sandbox (medida (b)).
for par in "opcao|o descritor 3 não chegou à CLI" "para|não consegui ler os grupos de processo dentro do sandbox"; do
    stub="${par%%|*}"
    motivo="${par#*|}"
    TOTAL=$(( TOTAL + 1 ))
    saida="$(env -u GITHUB_STEP_SUMMARY RUNNER_TEMP="$DIR/runner-$stub" NL_FLATPAK_BIN="$DIR/flatpak-$stub" NL_SENHA_FD_TIMEOUT=3 \
        bash "$AQUI/senha_fd.sh" io.github.petrinhu.notalivre 2>&1)"
    rc=$?
    if [ "$rc" -ne 0 ] && printf '%s\n' "$saida" | grep -q -F -- "$motivo"; then
        REPROVOU=$(( REPROVOU + 1 ))
        echo "ok: o medidor reprovou com o stub $stub, pelo motivo esperado (rc=$rc): $motivo"
    else
        echo "FALHA: o medidor não reprovou com o stub $stub pelo motivo esperado (rc=$rc, esperado: $motivo)"
        printf '%s\n' "$saida"
    fi
done

case "$DIR" in
    "$BASE"/senha-fd-autoteste.*) rm -rf -- "$DIR" ;;
esac

echo "autoteste senha_fd: reprovou em $REPROVOU de $TOTAL casos negativos"
[ "$REPROVOU" -eq "$TOTAL" ]
