# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Credencial de um par (NL-3.1h): senhas aleatórias e root.cnf, sempre em pasta 0700. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_credencial_nova DIR: cria DIR (0700), senha_root, senha_app e root.cnf (0600). Nenhuma senha vai a argv.
nl_banco_credencial_nova() {
    umask 077
    mkdir -- "$1" || return 1
    od -An -tx1 -N24 /dev/urandom | tr -d ' \n' > "$1/senha_root" || return 1
    od -An -tx1 -N24 /dev/urandom | tr -d ' \n' > "$1/senha_app" || return 1
    { printf '[client]\nuser=root\npassword='; cat "$1/senha_root"; printf '\n'; } > "$1/root.cnf" || return 1
}

# nl_banco_conferir_senha_fora DIR LOG...: rc 0 só se nenhuma das duas senhas aparece nos logs.
nl_banco_conferir_senha_fora() {
    local dir="$1"
    shift
    if /usr/bin/grep -qF -f "$dir/senha_root" "$@" 2>/dev/null; then return 1; fi
    if /usr/bin/grep -qF -f "$dir/senha_app" "$@" 2>/dev/null; then return 1; fi
    return 0
}

# nl_banco_apagar_credencial DIR: só apaga pasta com o prefixo de credencial do NL-3.1h (L-53: ls antes do rm).
nl_banco_apagar_credencial() {
    case "$1" in
        /var/tmp/notalivre-nl31h-cred-*) ;;
        *) echo "PARADA: caminho de credencial fora do padrão: $1"; return 1 ;;
    esac
    ls -d -- "$1" >/dev/null || return 1
    rm -rf -- "$1"
}
