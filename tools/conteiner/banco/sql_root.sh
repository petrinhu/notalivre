# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# SQL que o root roda no servidor do par (NL-3.1h): cliente, conta de teste, método e privilégios. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_cliente CONTEINER FAMILIA [args]: cliente da família com o arquivo de opções do root, por TCP em 127.0.0.1.
nl_banco_cliente() {
    local cont="$1" fam="$2" cli
    case "$fam" in
        mysql) cli=mysql ;;
        mariadb) cli=mariadb ;;
        *) return 1 ;;
    esac
    shift 2
    docker exec -i "$cont" "$cli" --defaults-extra-file=/run/nl/root.cnf --protocol=TCP -h 127.0.0.1 "$@"
}

# nl_banco_criar_conta_teste CONTEINER FAMILIA METODO DIR: banco notalivre_teste e conta com o método e 8 privilégios.
# O SQL vai para um arquivo 0600 dentro de DIR e entra pelo stdin; a senha só passa pelo arquivo, nunca por argv.
nl_banco_criar_conta_teste() {
    local cont="$1" fam="$2" metodo="$3" dir="$4" sql senha
    sql="$dir/conta.sql"
    senha="$(cat "$dir/senha_app")"
    umask 077
    printf 'CREATE DATABASE IF NOT EXISTS notalivre_teste;\n' > "$sql"
    if [ "$fam" = mysql ]; then
        printf "CREATE USER IF NOT EXISTS 'notalivre_teste'@'%%' IDENTIFIED WITH %s BY '%s';\n" "$metodo" "$senha" >> "$sql"
    else
        printf "CREATE USER IF NOT EXISTS 'notalivre_teste'@'%%' IDENTIFIED VIA %s USING PASSWORD('%s');\n" "$metodo" "$senha" >> "$sql"
    fi
    printf '%s\n' "GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, ALTER, INDEX ON `notalivre\_%`.* TO 'notalivre_teste'@'%';" >> "$sql"
    printf 'FLUSH PRIVILEGES;\n' >> "$sql"
    nl_banco_cliente "$cont" "$fam" < "$sql"
}

# nl_banco_ler_metodo CONTEINER FAMILIA: imprime o plugin da conta de teste, lido pelo root.
nl_banco_ler_metodo() {
    printf "SELECT plugin FROM mysql.user WHERE user='notalivre_teste' AND host='%%';\n" | nl_banco_cliente "$1" "$2" -N
}
