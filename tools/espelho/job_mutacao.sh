#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do job `mutacao` do ci.yml (R2): sabotagem dos portões (A2, gitleaks, travessão, SPDX, Flatpak 6.11)
# e controle do próprio harness (A0-15). Local: modo flatpak (org.flatpak.Builder, sem --privileged).
# ESPELHO_REF: REF do harness. WORKTREE (padrão local) = árvore de trabalho sem commit, declarado no log.
#              No CI o job usa $GITHUB_SHA (blob commitado).
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
travar
REF="${ESPELHO_REF:-WORKTREE}"
T="$ESPELHO_TRAB/mutacao"
mkdir -p "$T" && cd "$T" || exit 1
passo "imagem-higiene" garantir_imagem_higiene
passo "sabotagens" env HIGIENE_IMAGEM="$IMAGEM_HIGIENE" bash "$RAIZ/tools/mutacao/sabotar_portoes.sh" "$REF" "$T/sabotagens" flatpak
# Controle do harness: o veredito TEM de sair 1. Rc 0 aqui seria verde falso do próprio harness.
rc=0
env HIGIENE_IMAGEM="$IMAGEM_HIGIENE" MUTACAO_CONTROLE_HARNESS=1 bash "$RAIZ/tools/mutacao/sabotar_portoes.sh" "$REF" "$T/controle" flatpak || rc=$?
echo "passo=controle-harness rc=$rc (esperado 1)"
[ "$rc" -eq 1 ] || PASSOS_FALHOS=$((PASSOS_FALHOS + 1))
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "JOB mutacao: VERDE"; exit 0; }
echo "JOB mutacao: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente do esperado)"; exit 1
