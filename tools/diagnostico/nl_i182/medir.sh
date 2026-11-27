#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# NL-I.182, medição em contêiner: GRANT mínimo e falhas do RENAME em lote. Uso: medir.sh mysql|mariadb
set -u
export TMPDIR=/var/tmp
FAM="$1"
AQUI="$(cd "$(dirname "$0")" && pwd)"
# Teto de contêiner pelo átomo do projeto (NL-3.K): a medição de 25/11/2026 usou 1 CPU e 2 GB, que ficam de padrão.
. "$AQUI/../../conteiner/cota_conteiner.sh"
NL_THREADS="${NL_THREADS:-1}"
NL_MEMORIA="${NL_MEMORIA:-2g}"
nl_cotas_docker || exit 2
SQLD="$AQUI/sql"
LOGD=/var/tmp/notalivre-logs
LOG="$LOGD/NL-I.182-$FAM.log"
REDE=nli182-net
SRV=nli182-srv-$FAM
CLI=nli182-cli-$FAM
case "$FAM" in
  mysql) IMG=mysql:8.4; CLIENTE=mysql; VARPW=MYSQL_ROOT_PASSWORD ;;
  mariadb) IMG=mariadb:11.8; CLIENTE=mariadb; VARPW=MARIADB_ROOT_PASSWORD ;;
  *) echo "familia invalida"; exit 2 ;;
esac
umask 077
: > "$LOG"
log() { printf '%s\n' "$*" >> "$LOG"; }
secao() { log ""; log "=================================================================="; log "== $*"; log "=================================================================="; }

PW_ROOT="$(openssl rand -hex 24)"
PW_U="$(openssl rand -hex 24)"
PW_U2="$(openssl rand -hex 24)"
PW_NOVA="$(openssl rand -hex 24)"

# cliente: sql CNF [args] < arquivo; registra comando e saída (com eco das instruções) no log
sql_log() {
  local cnf="$1"; shift
  log "\$ docker exec -i $CLI $CLIENTE --defaults-extra-file=/run/nl/$cnf --force --unbuffered -v -t $* < (SQL abaixo)"
  docker exec -i "$CLI" "$CLIENTE" --defaults-extra-file="/run/nl/$cnf" --force --unbuffered -v -t "$@" >> "$LOG" 2> "$AQUI/err.txt"
  local rc=$?
  if [ -s "$AQUI/err.txt" ]; then log "-- saída de erro (stderr) desta chamada:"; cat "$AQUI/err.txt" >> "$LOG"; fi
  log "rc=$rc"
  return $rc
}
# cliente sem eco, para SQL que leva senha: só o modelo vai ao log
sql_mudo() {
  local cnf="$1" modelo="$2"
  log "\$ docker exec -i $CLI $CLIENTE --defaults-extra-file=/run/nl/$cnf (SQL com senha, não ecoado): $modelo"
  docker exec -i "$CLI" "$CLIENTE" --defaults-extra-file="/run/nl/$cnf" -t >> "$LOG" 2>&1
  log "rc=$?"
}
grava_cnf() { # nome usuario senha banco
  printf '[client]\nuser=%s\npassword=%s\nhost=%s\nport=3306\nprotocol=TCP\ndatabase=%s\n' "$2" "$3" "$SRV" "$4" \
    | docker exec -i "$CLI" sh -c "umask 077; mkdir -p /run/nl; cat > /run/nl/$1"
  log "(arquivo de opções /run/nl/$1 gravado no cliente: user=$2, host=$SRV, banco=$4, senha omitida)"
}
cria_usuario() { # usuario senha
  if [ "$FAM" = mysql ]; then
    printf "DROP USER IF EXISTS '%s'@'%%';\nCREATE USER '%s'@'%%' IDENTIFIED WITH caching_sha2_password BY '%s';\n" "$1" "$1" "$2" \
      | sql_mudo root.cnf "DROP USER IF EXISTS '$1'@'%'; CREATE USER '$1'@'%' IDENTIFIED WITH caching_sha2_password BY '<senha>';"
  else
    printf "DROP USER IF EXISTS '%s'@'%%';\nCREATE USER '%s'@'%%' IDENTIFIED BY '%s';\n" "$1" "$1" "$2" \
      | sql_mudo root.cnf "DROP USER IF EXISTS '$1'@'%'; CREATE USER '$1'@'%' IDENTIFIED BY '<senha>';"
  fi
}
concede() { # usuario "lista" [lista_global]
  local g="GRANT $2 ON \`notalivre\\_%\`.* TO '$1'@'%';"
  { printf '%s\nSHOW WARNINGS;\n' "$g"
    [ -n "${3:-}" ] && printf "GRANT %s ON *.* TO '%s'@'%%';\n" "$3" "$1"
    printf "SHOW GRANTS FOR '%s'@'%%';\n" "$1"; } > "$AQUI/grant_tmp.sql"
  log "-- SQL: $(tr '\n' ' ' < "$AQUI/grant_tmp.sql")"
  sql_log root.cnf < "$AQUI/grant_tmp.sql"
}
resumo() { # marca arquivo_sql: recusas da última rodada, mapeadas à operação pelo número de linha do erro
  awk -v ini="$1" 'FNR==NR { if (match($0, /^SELECT .[OXP][0-9][0-9] /)) cur=substr($0, 9, 3); op[FNR]=cur; next }
    index($0, ini) {on=1}
    on && /^ERROR [0-9]+ .* at line [0-9]+:/ { n=$0; sub(/.* at line /, "", n); sub(/:.*/, "", n); print "  " op[n+0] ": " $0 }' "$2" "$LOG"
}

secao "NL-I.182, família $FAM, imagem $IMG, início $(date -u +%FT%TZ)"
log "Memória do host antes: $(grep -E 'MemAvailable' /proc/meminfo)"
log "\$ docker image inspect $IMG --format '{{.Id}} {{.Size}}'"
docker image inspect "$IMG" --format '{{.Id}} {{.Size}}' >> "$LOG" 2>&1
log "\$ docker network create --internal $REDE"
docker network create --internal "$REDE" >> "$LOG" 2>&1
log "\$ docker run -d --name $SRV --network $REDE ${NL_COTAS_DOCKER[*]} --env-file <(senha root, não registrada) $IMG"
docker run -d --name "$SRV" --network "$REDE" "${NL_COTAS_DOCKER[@]}" --env-file <(printf '%s=%s\n' "$VARPW" "$PW_ROOT") "$IMG" >> "$LOG" 2>&1
log "\$ docker run -d --name $CLI --network $REDE ${NL_COTAS_DOCKER[*]} --entrypoint sleep $IMG infinity"
docker run -d --name "$CLI" --network "$REDE" "${NL_COTAS_DOCKER[@]}" --entrypoint sleep "$IMG" infinity >> "$LOG" 2>&1
grava_cnf root.cnf root "$PW_ROOT" mysql
log "Esperando o servidor aceitar TCP (até 180 s)"
pronto=0
for i in $(seq 1 90); do
  if docker exec "$CLI" "$CLIENTE" --defaults-extra-file=/run/nl/root.cnf -N -e 'SELECT 1' >/dev/null 2>&1; then pronto=1; break; fi
  sleep 2
done
log "pronto=$pronto apos $i tentativas"
[ "$pronto" = 1 ] || { log "PARADA: servidor não ficou pronto"; docker logs "$SRV" >> "$LOG" 2>&1; }

secao "Versões"
printf "SELECT VERSION(), @@version_comment, @@lock_wait_timeout, @@innodb_lock_wait_timeout;\nSHOW VARIABLES LIKE 'partial_revokes';\nSHOW VARIABLES LIKE 'log_bin';\nSHOW VARIABLES LIKE 'log_bin_trust_function_creators';\n" > "$AQUI/v.sql"
sql_log root.cnf < "$AQUI/v.sql"
log "\$ docker exec $CLI $CLIENTE --version"
docker exec "$CLI" "$CLIENTE" --version >> "$LOG" 2>&1

cria_usuario nl182 "$PW_U"
grava_cnf u.cnf nl182 "$PW_U" notalivre_principal

M2="CREATE, DROP, ALTER, SELECT, INSERT, UPDATE, DELETE, INDEX"
rodada() { # rotulo lista [global]
  secao "ITEM 1, rodada: $1"
  log "Privilégios no prefixo: $2${3:+; globais: $3}"
  sql_log root.cnf < "$SQLD/fixture_ops.sql" > /dev/null
  sql_log root.cnf -e "REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'nl182'@'%'"
  concede nl182 "$2" "${3:-}"
  local marca="### OPERACOES $1 ###"
  log "$marca"
  sql_log u.cnf < "$SQLD/ops_app.sql"
  log "### RESUMO $1 (recusas por operação) ###"
  resumo "$marca" "$SQLD/ops_app.sql" > "$AQUI/r.txt"; cat "$AQUI/r.txt" >> "$LOG"; [ -s "$AQUI/r.txt" ] || log "  (nenhuma recusa)"
}
rodada "m2 completo" "$M2"
for P in CREATE DROP ALTER SELECT INSERT UPDATE DELETE INDEX; do
  L="$(printf '%s' "$M2" | tr -d ' ' | tr ',' '\n' | grep -vx "$P" | paste -sd, | sed 's/,/, /g')"
  rodada "m2 sem $P" "$L"
done
rodada "m2 com CREATE global no lugar de CREATE no prefixo" "DROP, ALTER, SELECT, INSERT, UPDATE, DELETE, INDEX" "CREATE"

secao "ITEM 1, operações extras (temporária, FOREIGN KEY, LOCK TABLES, TRIGGER), com m2"
sql_log root.cnf < "$SQLD/fixture_ops.sql" > /dev/null
sql_log root.cnf -e "REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'nl182'@'%'"
concede nl182 "$M2"
log "### EXTRAS m2 ###"
sql_log u.cnf < "$SQLD/ops_extra.sql"
log "### RESUMO EXTRAS m2 ###"; resumo "### EXTRAS m2 ###" "$SQLD/ops_extra.sql" >> "$LOG"
for X in "CREATE TEMPORARY TABLES" "REFERENCES" "LOCK TABLES" "TRIGGER"; do
  secao "ITEM 1, extras com m2 + $X"
  sql_log root.cnf < "$SQLD/fixture_ops.sql" > /dev/null
  sql_log root.cnf -e "REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'nl182'@'%'"
  concede nl182 "$M2, $X"
  log "### EXTRAS m2+$X ###"
  sql_log u.cnf < "$SQLD/ops_extra.sql"
  log "### RESUMO EXTRAS m2+$X ###"; resumo "### EXTRAS m2+$X ###" "$SQLD/ops_extra.sql" >> "$LOG"
done
secao "ITEM 1, extras com m2 + os quatro"
sql_log root.cnf < "$SQLD/fixture_ops.sql" > /dev/null
sql_log root.cnf -e "REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'nl182'@'%'"
concede nl182 "$M2, CREATE TEMPORARY TABLES, REFERENCES, LOCK TABLES, TRIGGER"
log "### EXTRAS m2+4 ###"
sql_log u.cnf < "$SQLD/ops_extra.sql"
log "### RESUMO EXTRAS m2+4 ###"; resumo "### EXTRAS m2+4 ###" "$SQLD/ops_extra.sql" >> "$LOG"

secao "ITEM 1, troca da própria senha (arnês de teste, tst_executor_banco.cpp:447-452) com m2 sem privilégio extra"
sql_log root.cnf -e "REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'nl182'@'%'"
concede nl182 "$M2"
if [ "$FAM" = mysql ]; then
  printf "ALTER USER CURRENT_USER() IDENTIFIED BY '%s';\nSELECT 'troca ok' AS r;\n" "$PW_NOVA" | sql_mudo u.cnf "ALTER USER CURRENT_USER() IDENTIFIED BY '<senha nova>';"
else
  printf "SET PASSWORD = PASSWORD('%s');\nSELECT 'troca ok' AS r;\n" "$PW_NOVA" | sql_mudo u.cnf "SET PASSWORD = PASSWORD('<senha nova>');"
fi
grava_cnf u.cnf nl182 "$PW_NOVA" notalivre_principal
sql_log u.cnf -e "SELECT CURRENT_USER() AS login_com_senha_nova"

secao "ITEM 1, preparabilidade (o executor prepara tudo por mysql_stmt_prepare; aqui PREPARE do SQL), com m2 + CREATE TEMPORARY TABLES + LOCK TABLES"
sql_log root.cnf < "$SQLD/fixture_ops.sql" > /dev/null
sql_log root.cnf -e "REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'nl182'@'%'"
concede nl182 "$M2, CREATE TEMPORARY TABLES, LOCK TABLES"
log "### PREPARE ###"
sql_log u.cnf < "$SQLD/ops_prepare.sql"
log "### RESUMO PREPARE ###"; resumo "### PREPARE ###" "$SQLD/ops_prepare.sql" >> "$LOG"

# ---------------- ITEM 2 ----------------
estado() { log "-- estado final:"; sql_log root.cnf < "$SQLD/estado_rn.sql"; }
fixture_rn() { sql_log root.cnf < "$SQLD/fixture_rn.sql" > /dev/null; }
caso() { secao "ITEM 2, caso $1"; }

caso "C0 destino existente (controle, repete a seção 4 item c)"
fixture_rn
sql_log root.cnf -e "CREATE TABLE notalivre_rb.t2 (origem VARCHAR(32)) ENGINE=InnoDB; RENAME TABLE notalivre_ra.t1 TO notalivre_rb.t1, notalivre_ra.t2 TO notalivre_rb.t2, notalivre_ra.t3 TO notalivre_rb.t3"
estado

caso "C1 origem ausente no par do meio"
fixture_rn
sql_log root.cnf -e "RENAME TABLE notalivre_ra.t1 TO notalivre_rb.t1, notalivre_ra.ausente TO notalivre_rb.x, notalivre_ra.t3 TO notalivre_rb.t3"
estado

caso "C2 banco de destino ausente no par do meio"
fixture_rn
sql_log root.cnf -e "RENAME TABLE notalivre_ra.t1 TO notalivre_rb.t1, notalivre_ra.t2 TO notalivre_rz.t2, notalivre_ra.t3 TO notalivre_rb.t3"
estado

caso "C3a sem privilégio algum no banco de destino do par do meio (usuário nl182 com m2 no prefixo; outro_rn fora do prefixo)"
fixture_rn
sql_log root.cnf -e "REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'nl182'@'%'"
concede nl182 "$M2"
grava_cnf urn.cnf nl182 "$PW_NOVA" notalivre_ra
sql_log urn.cnf -e "RENAME TABLE notalivre_ra.t1 TO notalivre_rb.t1, notalivre_ra.t2 TO outro_rn.t2, notalivre_ra.t3 TO notalivre_rb.t3"
estado

caso "C3b destino do par do meio sem INSERT (usuário nl182b com GRANT por banco)"
fixture_rn
cria_usuario nl182b "$PW_U2"
{ echo "GRANT CREATE, DROP, ALTER, SELECT, INSERT ON notalivre_ra.* TO 'nl182b'@'%';"
  echo "GRANT CREATE, DROP, ALTER, SELECT, INSERT ON notalivre_rd.* TO 'nl182b'@'%';"
  echo "GRANT CREATE, DROP, ALTER, SELECT ON notalivre_rb.* TO 'nl182b'@'%';"
  echo "SHOW GRANTS FOR 'nl182b'@'%';"; } > "$AQUI/g2.sql"
sql_log root.cnf < "$AQUI/g2.sql"
grava_cnf u2.cnf nl182b "$PW_U2" notalivre_ra
sql_log u2.cnf -e "RENAME TABLE notalivre_ra.t1 TO notalivre_rd.t1, notalivre_ra.t2 TO notalivre_rb.t2, notalivre_ra.t3 TO notalivre_rd.t3"
estado

trava_caso() { # rotulo sql_A
  caso "$1"
  fixture_rn
  log "Sessão A (root, em segundo plano): $2"
  ( docker exec -i "$CLI" "$CLIENTE" --defaults-extra-file=/run/nl/root.cnf --unbuffered -v -t -e "$2" > "$AQUI/a.out" 2>> "$AQUI/a.err"; echo "rc_A=$?" >> "$AQUI/a.out" ) &
  local pa=$!
  sleep 2
  local B="SET SESSION lock_wait_timeout = 3; SELECT NOW(6) AS antes; RENAME TABLE notalivre_ra.t1 TO notalivre_rb.t1, notalivre_ra.t2 TO notalivre_rb.t2, notalivre_ra.t3 TO notalivre_rb.t3; SELECT NOW(6) AS depois"
  log "Sessão B (root, em segundo plano), 2 s depois: $B"
  ( docker exec -i "$CLI" "$CLIENTE" --defaults-extra-file=/run/nl/root.cnf --force --unbuffered -v -t -e "$B" > "$AQUI/b.out" 2>> "$AQUI/b.err"; echo "rc_B=$?" >> "$AQUI/b.out" ) &
  local pb=$!
  sleep 1
  log "Root, 1 s depois de B começar:"
  sql_log root.cnf -e "SELECT id, command, state, LEFT(info, 60) AS info FROM information_schema.processlist WHERE info LIKE 'RENAME%' OR info LIKE '%SLEEP%' ORDER BY id"
  log "-- estado durante a espera de B:"
  sql_log root.cnf < "$SQLD/estado_rn.sql"
  wait "$pb"; log "-- saída da sessão B:"; cat "$AQUI/b.out" >> "$LOG"; log "-- stderr da sessão B:"; cat "$AQUI/b.err" >> "$LOG" 2>/dev/null; rm -f "$AQUI/b.err"
  log "-- estado logo depois da recusa de B, com A ainda viva:"
  sql_log root.cnf < "$SQLD/estado_rn.sql"
  wait "$pa"; log "-- saída da sessão A:"; cat "$AQUI/a.out" >> "$LOG"; log "-- stderr da sessão A:"; cat "$AQUI/a.err" >> "$LOG" 2>/dev/null; rm -f "$AQUI/a.err"
  estado
}
trava_caso "C4 tabela do par do meio travada por LOCK TABLES READ em outra sessão, lock_wait_timeout 3" \
  "LOCK TABLES notalivre_ra.t2 READ; SELECT NOW(6) AS travou; SELECT SLEEP(10); UNLOCK TABLES; SELECT NOW(6) AS soltou"
trava_caso "C5 tabela do par do meio lida em transação aberta em outra sessão, lock_wait_timeout 3" \
  "START TRANSACTION; SELECT COUNT(*) FROM notalivre_ra.t2; SELECT NOW(6) AS abriu; SELECT SLEEP(10); COMMIT; SELECT NOW(6) AS confirmou"

caso "C6a lote entre quatro bancos, sem falha"
fixture_rn
sql_log root.cnf -e "RENAME TABLE notalivre_ra.t1 TO notalivre_rb.t1, notalivre_rc.t1 TO notalivre_rd.t1, notalivre_ra.t2 TO notalivre_rc.t2; SELECT origem FROM notalivre_rb.t1; SELECT origem FROM notalivre_rd.t1; SELECT origem FROM notalivre_rc.t2"
estado

caso "C6b troca no formato da NL-3.1d (principal ra para antigo rc, cópia rb para principal ra), sem falha"
fixture_rn
sql_log root.cnf -e "RENAME TABLE notalivre_ra.t1 TO notalivre_rc.t1x, notalivre_rb.copia TO notalivre_ra.t1; SELECT origem FROM notalivre_ra.t1; SELECT origem FROM notalivre_rc.t1x"
estado

caso "C6c lote entre quatro bancos com falha no último par (destino existente em outro banco)"
fixture_rn
sql_log root.cnf -e "CREATE TABLE notalivre_rd.t9 (origem VARCHAR(32)) ENGINE=InnoDB; RENAME TABLE notalivre_ra.t1 TO notalivre_rb.t1, notalivre_rc.t1 TO notalivre_rd.t1, notalivre_ra.t2 TO notalivre_rd.t9"
estado

caso "C6d troca no formato da NL-3.1d com a cópia ausente no segundo par"
fixture_rn
sql_log root.cnf -e "DROP TABLE notalivre_rb.copia; RENAME TABLE notalivre_ra.t1 TO notalivre_rc.t1x, notalivre_rb.copia TO notalivre_ra.t1"
sql_log root.cnf -e "SELECT origem FROM notalivre_ra.t1"
estado

secao "Encerramento"
log "\$ docker stop $CLI $SRV"; docker stop "$CLI" "$SRV" >> "$LOG" 2>&1
log "\$ docker rm $CLI $SRV"; docker rm "$CLI" "$SRV" >> "$LOG" 2>&1
log "\$ docker network rm $REDE"; docker network rm "$REDE" >> "$LOG" 2>&1
log "\$ docker ps -a --filter name=nli182 (deve vir vazio)"; docker ps -a --filter name=nli182 >> "$LOG" 2>&1
rm -f "$AQUI/err.txt" "$AQUI/a.out" "$AQUI/b.out" "$AQUI/r.txt" "$AQUI/g2.sql" "$AQUI/grant_tmp.sql" "$AQUI/v.sql"
sed -i "s/IDENTIFIED BY PASSWORD '[^']*'/IDENTIFIED BY PASSWORD '<hash omitido>'/g" "$LOG"
log "(hash de senha das linhas SHOW GRANTS substituído por <hash omitido>: $(grep -c 'hash omitido' "$LOG") linhas)"
n=0
for s in "$PW_ROOT" "$PW_U" "$PW_U2" "$PW_NOVA"; do
  c=$(grep -cF -- "$s" "$LOG" "$LOGD"/NL-I.182-*.log 2>/dev/null | awk -F: '{t+=$NF} END {print t+0}')
  n=$((n + c))
done
log "Conferência grep -F das 4 senhas desta família nos logs NL-I.182-*.log: $n ocorrências"
log "Fim $(date -u +%FT%TZ)"
echo "ocorrencias_senha=$n"
