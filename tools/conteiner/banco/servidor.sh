# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Ciclo de vida do servidor do par (NL-3.1h): subir com rede none e senha por _FILE, prontidão, parar. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_subir_servidor NOME DIR CONTEINER: contêiner do servidor, com a cota de nl_cotas_docker (1 CPU, 2g).
nl_banco_subir_servidor() {
    local linha fam img var
    linha="$(nl_banco_servidor_de "$1")" || return 1
    read -r _ fam img _ <<<"$linha"
    case "$fam" in
        mysql) var=MYSQL_ROOT_PASSWORD_FILE ;;
        mariadb) var=MARIADB_ROOT_PASSWORD_FILE ;;
        *) return 1 ;;
    esac
    NL_THREADS=1 NL_MEMORIA=2g nl_cotas_docker || return 2
    docker run --pull=never -d --rm "${NL_COTAS_DOCKER[@]}" --pids-limit 512 --network none --name "$3" \
        -e "$var=/run/nl/senha_root" -v "$2:/run/nl:ro,z" "$img" >/dev/null
}

# nl_banco_esperar_pronto CONTEINER FAMILIA: sonda TCP em 127.0.0.1, até 90 tentativas, 2 s entre elas (teto fixo).
nl_banco_esperar_pronto() {
    local i
    for i in $(seq 1 90); do
        if nl_banco_cliente "$1" "$2" -N -e 'SELECT 1' >/dev/null 2>&1; then return 0; fi
        sleep 2
    done
    return 1
}

# nl_banco_parar CONTEINER: docker stop do contêiner que o harness criou (o --rm remove depois).
nl_banco_parar() {
    docker stop -t 10 "$1" >/dev/null 2>&1
}

# nl_banco_reiniciar CONTEINER: docker restart do servidor do par (medição de reinício, passo 4b do NL-3.1h).
nl_banco_reiniciar() {
    docker restart -t 10 "$1" >/dev/null 2>&1
}
