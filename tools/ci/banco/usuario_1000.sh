#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Usuário de uid 1000 no runner do CI (NL-3.1v, seção 1.2 do plano do CTO). Todo contêiner do harness e do build_local.sh
# roda --user 1000:1000 e a credencial do par nasce 0700 do usuário do host (NF-30): o build e o harness rodam, por sudo -u,
# como um usuário de uid 1000. Se o uid 1000 está livre, cria o nlci (uid e gid 1000); se existe, usa esse usuário.
# O usuário fica sem sudo (sudo -l -U). Se o uid 1000 que já existe tem sudo (run 45986750884: o packer da imagem do
# runner), tira o sudo dele nesta VM descartável: sai dos grupos sudo, admin e wheel (gpasswd -d) e perde só as linhas de
# /etc/sudoers.d/ cujo primeiro campo é o nome dele, com visudo -cf em cada arquivo novo antes da troca e visudo -c no conjunto
# depois; o log diz o que saiu. Se ainda tiver sudo, PARADA. Nada de outro usuário muda (o runner segue com o sudo dele).
# Põe o usuário no grupo docker e no grupo do dono do workspace (para atravessar a pasta do runner), declara o workspace
# seguro no git dele e confere que ele lê o workspace. Nenhum script do caso local muda.
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

# nl_u1000_nome_valido USUARIO: rc 0 só para nome de usuário POSIX simples (o nome entra em comparação com sudoers).
nl_u1000_nome_valido() {
    [[ "${1:-}" =~ ^[a-z_][a-z0-9_-]{0,31}$ ]]
}

# nl_u1000_tirar_grupos USUARIO: tira USUARIO dos grupos sudo, admin e wheel em que ele estiver, com o log de cada um.
nl_u1000_tirar_grupos() {
    local g
    for g in sudo admin wheel; do
        if id -nG "$1" | tr ' ' '\n' | grep -qxF "$g"; then
            sudo -n gpasswd -d "$1" "$g" >/dev/null || return 1
            echo "sudo do uid 1000: $1 saiu do grupo $g" >&2
        fi
    done
}

# nl_u1000_linhas_do_usuario USUARIO ARQUIVO: imprime as linhas do ARQUIVO (lido como root) cujo primeiro campo é USUARIO.
nl_u1000_linhas_do_usuario() {
    sudo -n awk -v u="$1" '{ s = $0; sub(/^[ \t]+/, "", s); split(s, c, /[ \t]+/); if (c[1] == u) print }' "$2"
}

# nl_u1000_tirar_linhas USUARIO ARQUIVO: troca ARQUIVO pela cópia sem as linhas de USUARIO, só se a cópia passa no visudo -cf.
# A cópia nasce numa pasta temporária do root, 0440 e de root, como o arquivo de /etc/sudoers.d/.
nl_u1000_tirar_linhas() {
    local tmp rc=0
    tmp="$(sudo -n mktemp -d)" || return 1
    case "$tmp" in /tmp/tmp.*) ;; *) echo "PARADA: pasta temporária fora do padrão: $tmp" >&2; return 1 ;; esac
    sudo -n awk -v u="$1" '{ s = $0; sub(/^[ \t]+/, "", s); split(s, c, /[ \t]+/); if (c[1] != u) print }' "$2" \
        | sudo -n tee "$tmp/novo" >/dev/null || rc=1
    [ "$rc" -eq 0 ] && sudo -n chmod 0440 "$tmp/novo" || rc=1
    [ "$rc" -eq 0 ] && sudo -n visudo -cf "$tmp/novo" >&2 || rc=1
    [ "$rc" -eq 0 ] && sudo -n install -m 0440 -o root -g root "$tmp/novo" "$2" || rc=1
    sudo -n rm -rf -- "$tmp"
    return "$rc"
}

# nl_u1000_tirar_sudoers_d USUARIO: em cada arquivo de /etc/sudoers.d/ com linha de USUARIO, tira só essas linhas, com o log
# de cada linha; depois, visudo -c no conjunto.
nl_u1000_tirar_sudoers_d() {
    local f linhas
    while IFS= read -r f; do
        [ -n "$f" ] || continue
        linhas="$(nl_u1000_linhas_do_usuario "$1" "$f")" || return 1
        [ -n "$linhas" ] || continue
        nl_u1000_tirar_linhas "$1" "$f" || { echo "PARADA: $f sem as linhas de $1 não passou no visudo -cf" >&2; return 1; }
        printf '%s\n' "$linhas" | sed "s|^|sudo do uid 1000: saiu de $f: |" >&2
    done <<<"$(sudo -n find /etc/sudoers.d -maxdepth 1 -type f)"
    sudo -n visudo -c >&2 || { echo "PARADA: visudo -c reprovou o conjunto depois da troca" >&2; return 1; }
}

# nl_u1000_tirar_sudo USUARIO: tira o sudo do USUARIO (grupos e /etc/sudoers.d/), só com nome válido.
nl_u1000_tirar_sudo() {
    nl_u1000_nome_valido "$1" || { echo "PARADA: nome de usuário fora do padrão: $1" >&2; return 1; }
    nl_u1000_tirar_grupos "$1" || return 1
    nl_u1000_tirar_sudoers_d "$1"
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
if ! nl_u1000_sem_sudo "$usuario"; then
    echo "sudo do uid 1000: $usuario tem sudo neste runner; tirando (VM descartável)" >&2
    nl_u1000_tirar_sudo "$usuario" || { echo "PARADA: não tirei o sudo de $usuario" >&2; exit 1; }
    nl_u1000_sem_sudo "$usuario" || { echo "PARADA: $usuario tem regra de sudo neste runner; o build e o harness não rodam com ele" >&2; exit 1; }
    echo "sudo do uid 1000: $usuario sem sudo (sudo -l -U conferido de novo)" >&2
fi
nl_u1000_grupos "$usuario" "$WORKSPACE" || { echo "PARADA: grupos de $usuario não ajustados" >&2; exit 1; }
nl_u1000_git_seguro "$usuario" "$WORKSPACE" || { echo "PARADA: safe.directory de $usuario não gravado" >&2; exit 1; }
nl_u1000_le_workspace "$usuario" "$WORKSPACE" || { echo "PARADA: $usuario não lê o workspace $WORKSPACE" >&2; exit 1; }
id "$usuario" >&2
printf '%s\n' "$usuario"
