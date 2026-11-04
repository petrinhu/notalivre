#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portão do histórico de segredos (NL-3.E; L-28 e L-36): gitleaks git sobre RAIZ, com piso de varredura.
# Um passo de histórico só é verde se leu commits. Medido em 04/11/2026, imagem de higiene, gitleaks 8.30.0:
# .git-arquivo pendurado e repositório sem commit saem com rc 0 e "0 commits scanned" (falso verde).
# Uso: sh higiene_historico.sh RAIZ   (roda no contêiner de higiene; sai 1 em qualquer reprova)
RAIZ="${1:?uso: higiene_historico.sh RAIZ}"
CFG="$RAIZ/.gitleaks.toml"
[ -f "$CFG" ] || { echo "REPROVA: falta $CFG (configuração do gitleaks, fail-closed)"; exit 1; }
LOG="$(mktemp "${TMPDIR:-/var/tmp}/historico.XXXXXX")" || exit 1
trap 'rm -f -- "$LOG"' EXIT
gitleaks git "$RAIZ" --config "$CFG" --redact --no-banner --no-color --exit-code 1 > "$LOG" 2>&1
rc=$?
cat "$LOG"
lidos="$(grep -oE '[0-9]+ commits scanned' "$LOG" | tail -n 1 | grep -oE '^[0-9]+')"
if [ "$rc" -ne 0 ]; then
    echo "REPROVA: gitleaks rc=$rc (segredo encontrado ou erro; ver o log acima)"
    exit 1
fi
if [ -z "$lidos" ] || [ "$lidos" -lt 1 ]; then
    echo "REPROVA: piso de varredura: commits_lidos=${lidos:-ausente} (zero commit lido é falso verde, L-36)"
    exit 1
fi
echo "commits_lidos=$lidos gitleaks_rc=$rc"
exit 0
