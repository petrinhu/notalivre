#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Medição de reinício do servidor do par (NL-3.1h, passo 4b): a impressão do certificado muda depois de docker restart?
# Uso: medir_reinicio_banco.sh <nome>. Só orquestra: sobe o servidor, tira a impressão pela sonda, reinicia, tira de novo e
# para o servidor. Não apaga a credencial: ela fica listada no relatório (ordem do líder: sem rm por conta própria).
set -u
NOME="${1:?uso: medir_reinicio_banco.sh <nome>}"
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NL_BANCO_DIR="$RAIZ/tools/conteiner"
. "$NL_BANCO_DIR/cota_conteiner.sh"
. "$NL_BANCO_DIR/banco/listas.sh"
. "$NL_BANCO_DIR/banco/credencial.sh"
. "$NL_BANCO_DIR/banco/servidor.sh"
. "$NL_BANCO_DIR/banco/sql_root.sh"
. "$NL_BANCO_DIR/banco/certificado.sh"
. "$NL_BANCO_DIR/banco/sonda.sh"

# sonda_do_par DIR_CERTIFICADO: tira a impressão do servidor pela sonda e imprime só o campo sonda= (hex, sem_tls ou erro).
sonda_do_par() {
    local saida
    if [ "$CERTC" = disco ]; then
        nl_banco_copiar_certificado "$CONT" "$CRED" || { echo erro; return 0; }
    fi
    saida="$(nl_banco_rodar_sonda fedora45 "$CONT" "$CRED" "$RAIZ" "$CERTC" 2>&1)"
    printf '%s\n' "$saida" | awk '/^NL-IMPRESSAO/ { for (i = 2; i <= NF; i++) { split($i, kv, "="); if (kv[1] == "sonda") print kv[2] } }' | awk 'NR == 1 { print; found = 1 } END { if (!found) print "erro" }'
}

linha="$(nl_banco_servidor_de "$NOME")" || { echo "PARADA: servidor $NOME fora de servidores_banco.txt"; exit 2; }
read -r _ FAM IMG _ TLSC CERTC <<<"$linha"
CRED="/var/tmp/notalivre-nl31h-cred-reinicio-$NOME-$$"
CONT="nl31h-reinicio-$NOME-$$"

nl_banco_credencial_nova "$CRED" || { echo "PARADA: credencial de $NOME"; exit 2; }
nl_banco_subir_servidor "$NOME" "$CRED" "$CONT" || { echo "PARADA: docker run de $NOME recusado"; exit 2; }
trap 'nl_banco_parar "$CONT"' EXIT
if ! nl_banco_esperar_pronto "$CONT" "$FAM"; then
    echo "PARADA: servidor $NOME sem resposta em 127.0.0.1 em 90 tentativas"
    exit 3
fi
echo "NL-UID servidor=$NOME uid_do_servidor=$(docker top "$CONT" -o uid,pid 2>/dev/null | awk 'NR == 2 { print $1 }')"

antes="$(sonda_do_par)"
nl_banco_reiniciar "$CONT"
nl_banco_esperar_pronto "$CONT" "$FAM" || echo "AVISO: servidor $NOME sem resposta depois do reinício"
vivo="$(docker inspect -f '{{.State.Running}}' "$CONT" 2>/dev/null | awk '/^true$/ { print "sim"; found = 1 } END { if (!found) print "nao" }')"
depois="$(sonda_do_par)"

igual=nao
[ "$antes" = "$depois" ] && igual=sim
echo "NL-REINICIO servidor=$NOME antes=$antes depois=$depois igual=$igual conteiner_vivo=$vivo"
echo "NL-CREDENCIAL $CRED (mantida, sem rm)"
