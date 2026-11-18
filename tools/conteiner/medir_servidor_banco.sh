#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Medição de um servidor do banco (NL-3.1h, passo 4). Uso: medir_servidor_banco.sh <nome>. Só orquestra: o SQL, a
# credencial e o ciclo do servidor vêm dos átomos de banco/. Rede none, senha por _FILE, sonda com teto de 90 tentativas.
set -u
NOME="${1:?uso: medir_servidor_banco.sh <nome>}"
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NL_BANCO_DIR="$RAIZ/tools/conteiner"
LOGS=/var/tmp/notalivre-logs
. "$NL_BANCO_DIR/cota_conteiner.sh"
. "$NL_BANCO_DIR/banco/listas.sh"
. "$NL_BANCO_DIR/banco/credencial.sh"
. "$NL_BANCO_DIR/banco/servidor.sh"
. "$NL_BANCO_DIR/banco/sql_root.sh"

linha="$(nl_banco_servidor_de "$NOME")" || { echo "PARADA: servidor $NOME fora de servidores_banco.txt"; exit 2; }
read -r _ FAM IMG METODO _ _ <<<"$linha"
CRED="/var/tmp/notalivre-nl31h-cred-$NOME-$$"
CONT="nl31h-med-$NOME-$$"
LOG="$LOGS/NL-3.1h-medicao-$NOME.log"

nl_banco_credencial_nova "$CRED" || { echo "PARADA: credencial de $NOME"; exit 2; }
nl_banco_subir_servidor "$NOME" "$CRED" "$CONT" || { echo "PARADA: docker run de $NOME recusado"; exit 2; }
trap 'nl_banco_parar "$CONT"' EXIT
if ! nl_banco_esperar_pronto "$CONT" "$FAM"; then
    echo "PARADA: servidor $NOME sem resposta em 127.0.0.1 em 90 tentativas"
    docker logs --tail 50 "$CONT" 2>&1 | tail -n 50
    exit 3
fi

{
    echo "== servidor=$NOME familia=$FAM imagem=$IMG metodo_da_ordem=$METODO"
    nl_banco_cliente "$CONT" "$FAM" -t -e 'SELECT VERSION();'
    nl_banco_cliente "$CONT" "$FAM" -t -e "SHOW GLOBAL VARIABLES LIKE 'require_secure_transport';"
    nl_banco_cliente "$CONT" "$FAM" -t -e "SHOW GLOBAL VARIABLES LIKE 'have_ssl';"
    nl_banco_cliente "$CONT" "$FAM" -t -e "SHOW SESSION STATUS LIKE 'Ssl_cipher';"
    nl_banco_cliente "$CONT" "$FAM" -t -e "CREATE USER 'nl_sem'@'%' IDENTIFIED BY 'nl31h-medicao';"
    nl_banco_cliente "$CONT" "$FAM" -t -e "SELECT user, plugin FROM mysql.user WHERE user='nl_sem';"
    if [ "$FAM" = mysql ]; then
        nl_banco_cliente "$CONT" "$FAM" -t -e "CREATE USER 'nl_com'@'%' IDENTIFIED WITH $METODO BY 'nl31h-medicao';"
    else
        nl_banco_cliente "$CONT" "$FAM" -t -e "CREATE USER 'nl_com'@'%' IDENTIFIED VIA $METODO USING PASSWORD('nl31h-medicao');"
    fi
    nl_banco_cliente "$CONT" "$FAM" -t -e "SELECT user, plugin FROM mysql.user WHERE user='nl_com';"
    nl_banco_cliente "$CONT" "$FAM" -t -e "SHOW COLLATION LIKE 'utf8mb4_bin';"
    if [ "$FAM" = mariadb ]; then
        nl_banco_cliente "$CONT" "$FAM" -t -e "SHOW GLOBAL VARIABLES LIKE 'innodb_snapshot_isolation';"
    fi
} > "$LOG" 2>&1
medicao_rc=$?

nl_banco_parar "$CONT"
echo "medicao_rc=$medicao_rc"
if nl_banco_conferir_senha_fora "$CRED" "$LOG"; then echo "senha_fora_do_log=ok"; else echo "senha_fora_do_log=PARADA"; fi
echo "credencial_mantida=$CRED (sem rm: apagar com o líder)"
