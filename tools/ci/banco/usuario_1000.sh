#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Usuário de uid 1000 no runner do CI (NL-3.1v, seção 1.2 do plano do CTO). Todo contêiner do harness e do build_local.sh
# roda --user 1000:1000 e a credencial do par nasce 0700 do usuário do host (NF-30): o build e o harness rodam, por sudo -u,
# como um usuário de uid 1000. Se o uid 1000 está livre, cria o nlci (uid e gid 1000); se existe, usa esse usuário.
# Recusa um uid 1000 que tenha sudo (sudo -l -U); o usuário fica sem sudo. Põe o usuário no grupo docker e no grupo do dono
# do workspace (para atravessar a pasta do runner), declara o workspace seguro no git dele e confere que ele lê o workspace.
# Nenhum script do caso local muda.
# Saída: o nome do usuário em stdout (o resto vai a stderr). Uso: usuario_1000.sh WORKSPACE
set -u
NOME_NOVO=nlci

# nl_u1000_nome: imprime o nome do usuário de uid 1000; rc 1 se o uid está livre.
nl_u1000_nome() {
    getent passwd 1000 | cut -d: -f1 | grep .
}

# nl_u1000_criar: cria o nlci com uid 1000 e gid 1000 (o grupo 1000 é criado se não existe).
nl_u1000_criar() {
    if ! getent group 1000 >/dev/null; then
        sudo -n groupadd -g 1000 "$NOME_NOVO" || return 1
    fi
    sudo -n useradd -m -u 1000 -g 1000 -s /bin/bash "$NOME_NOVO"
}

# nl_u1000_grupos USUARIO WORKSPACE: põe USUARIO no grupo docker e no grupo do dono do WORKSPACE.
nl_u1000_grupos() {
    local grupo
    grupo="$(stat -c %G -- "$2")" || return 1
    sudo -n usermod -aG docker "$1" || return 1
    if [ "$grupo" != docker ]; then
        sudo -n usermod -aG "$grupo" "$1" || return 1
    fi
}

# nl_u1000_git_seguro USUARIO WORKSPACE: o git do USUARIO aceita o WORKSPACE, que é de outro dono (o runner).
nl_u1000_git_seguro() {
    sudo -n -H -u "$1" git config --global --add safe.directory "$2"
}

# nl_u1000_sem_sudo USUARIO: rc 0 só se o sudo diz que USUARIO não pode rodar nada pelo sudo neste host.
nl_u1000_sem_sudo() {
    LC_ALL=C sudo -n -l -U "$1" 2>&1 | grep -q 'is not allowed to run sudo'
}

# nl_u1000_le_workspace USUARIO WORKSPACE: rc 0 só se USUARIO lê o HEAD do git e o harness do WORKSPACE.
nl_u1000_le_workspace() {
    sudo -n -u "$1" test -r "$2/.git/HEAD" && sudo -n -u "$1" test -r "$2/tools/conteiner/integracao_banco.sh"
}

WORKSPACE="${1:-}"
[ -n "$WORKSPACE" ] && [ -d "$WORKSPACE" ] || { echo "PARADA: uso: usuario_1000.sh WORKSPACE" >&2; exit 2; }
if usuario="$(nl_u1000_nome)"; then
    echo "uid 1000 já existe: $usuario (usado como está, só com os grupos)" >&2
else
    nl_u1000_criar || { echo "PARADA: não criei o usuário $NOME_NOVO com uid 1000" >&2; exit 1; }
    usuario="$NOME_NOVO"
    echo "uid 1000 livre: criado $usuario" >&2
fi
nl_u1000_sem_sudo "$usuario" || { echo "PARADA: $usuario tem regra de sudo neste runner; o build e o harness não rodam com ele" >&2; exit 1; }
nl_u1000_grupos "$usuario" "$WORKSPACE" || { echo "PARADA: grupos de $usuario não ajustados" >&2; exit 1; }
nl_u1000_git_seguro "$usuario" "$WORKSPACE" || { echo "PARADA: safe.directory de $usuario não gravado" >&2; exit 1; }
nl_u1000_le_workspace "$usuario" "$WORKSPACE" || { echo "PARADA: $usuario não lê o workspace $WORKSPACE" >&2; exit 1; }
id "$usuario" >&2
printf '%s\n' "$usuario"
