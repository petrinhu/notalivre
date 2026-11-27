#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Servidores de banco dos pares de uma distro, por digest (NL-3.1v, seção 1.6 do plano do CTO). O harness roda os servidores
# com --pull=never: este passo os traz antes. Pares e imagens vêm das listas pelas funções de tools/conteiner/banco/listas.sh
# (sem segunda leitura das listas); só entra imagem de servidores_banco.txt (NF-05) e só por @sha256. Depois do pull, o
# digest local tem de ser o da lista. Saída: NL-SERVIDORES distro=<d> pedidos=N presentes=M; N diferente de M, ou N zero,
# reprova.
# Uso: puxar_servidores.sh DISTRO
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
NL_BANCO_DIR="$RAIZ/tools/conteiner"
. "$NL_BANCO_DIR/banco/listas.sh"

# nl_servidores_imagem_de NOME: imprime a imagem@digest do servidor NOME; rc 1 se o nome não existe ou se não é por digest.
nl_servidores_imagem_de() {
    local linha img
    linha="$(nl_banco_servidor_de "$1")" || return 1
    read -r _ _ img _ <<<"$linha"
    case "$img" in
        *@sha256:*) printf '%s\n' "$img" ;;
        *) return 1 ;;
    esac
}

# nl_servidores_presente IMAGEM: rc 0 só se o RepoDigests local traz exatamente IMAGEM.
nl_servidores_presente() {
    docker image inspect --format '{{range .RepoDigests}}{{println .}}{{end}}' "$1" 2>/dev/null | grep -qxF -- "$1"
}

DISTRO="${1:-}"
[ -n "$DISTRO" ] || { echo "PARADA: uso: puxar_servidores.sh DISTRO"; exit 2; }
pedidos=0
presentes=0
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    pedidos=$((pedidos + 1))
    if ! img="$(nl_servidores_imagem_de "$nome")"; then
        echo "PARADA: servidor $nome fora de servidores_banco.txt ou sem digest"
        continue
    fi
    docker pull --quiet "$img" </dev/null || echo "PARADA: docker pull de $nome ($img) falhou"
    if nl_servidores_presente "$img"; then
        presentes=$((presentes + 1))
    else
        echo "PARADA: servidor $nome sem o digest da lista no host ($img)"
    fi
done <<<"$(nl_banco_pares_de "$DISTRO")"
echo "NL-SERVIDORES distro=$DISTRO pedidos=$pedidos presentes=$presentes"
[ "$pedidos" -gt 0 ] && [ "$pedidos" -eq "$presentes" ]
