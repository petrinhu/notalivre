# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Certificado do servidor do par (NL-3.1h, seção 1.3 da v3.5): copia o PEM da classe disco e confere a classe pela linha
# NL-IMPRESSAO da sonda. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_copiar_certificado CONTEINER DIR: copia /var/lib/mysql/server-cert.pem para DIR/certificado_servidor.pem (0600).
# Rc 0 só se o arquivo copiado tem BEGIN CERTIFICATE.
nl_banco_copiar_certificado() {
    umask 077
    docker exec "$1" cat /var/lib/mysql/server-cert.pem > "$2/certificado_servidor.pem" 2>/dev/null || return 1
    /usr/bin/grep -q 'BEGIN CERTIFICATE' "$2/certificado_servidor.pem"
}

# nl_banco_conferir_classe LINHA CLASSE: LINHA é a saída NL-IMPRESSAO da sonda; CLASSE é a coluna certificado do servidor.
# Imprime a impressão usada (vai ao executor.env) e sai 0 só na regra da seção 1.3; senão sai 1, sem imprimir.
#   disco:   sonda e PEM com a mesma impressão hexadecimal;
#   memoria: PEM nd, sonda com impressão hexadecimal, e ssl_cert vazio (conferido pelo chamador);
#   nenhum:  sonda sem_tls e PEM nd.
nl_banco_conferir_classe() {
    local linha="$1" classe="$2" c d s u
    linha="$(printf '%s\n' "$linha" | /usr/bin/grep '^NL-IMPRESSAO ' | tail -n 1)"
    [ -n "$linha" ] || return 1
    read -r c d s u <<<"$(awk '{ for (i = 2; i <= NF; i++) { split($i, kv, "="); v[kv[1]] = kv[2] } print v["certificado"], v["disco"], v["sonda"], v["usada"] }' <<<"$linha")"
    [ "$c" = "$classe" ] || return 1
    case "$c" in
        disco)
            [ "$d" = "$s" ] && [ "$u" = "$d" ] || return 1
            ;;
        memoria)
            [ "$d" = nd ] && [ "$u" = "$s" ] || return 1
            ;;
        nenhum)
            [ "$d" = nd ] && [ "$s" = sem_tls ] && [ "$u" = nenhuma ] || return 1
            printf '%s\n' "$u"
            return 0
            ;;
        *) return 1 ;;
    esac
    case "$u" in ''|*[!0-9A-Fa-f]*) return 1 ;; esac
    printf '%s\n' "$u"
}

# nl_banco_preparar_classe DISTRO CONTEINER FAMILIA DIR FONTE CERTIFICADO LOG: a classe do par antes do executor.
# Imprime a impressão usada e sai 0 só se a classe confere. Anexa a linha NL-IMPRESSAO ao LOG do par.
nl_banco_preparar_classe() {
    local distro="$1" cont="$2" fam="$3" dir="$4" fonte="$5" certificado="$6" log="$7" imp usada
    # As PARADA vão a stderr: quem chama captura só stdout, que é a impressão usada.
    if [ "$certificado" = disco ]; then
        nl_banco_copiar_certificado "$cont" "$dir" || { echo "PARADA: classe do servidor: PEM do disco ausente ($cont)" >&2; return 1; }
    fi
    if [ "$certificado" = memoria ]; then
        if [ -n "$(nl_banco_ler_ssl_cert "$cont" "$fam" | awk -F'\t' '{ print $2 }')" ]; then
            echo "PARADA: classe do servidor: ssl_cert não está vazio em $cont" >&2; return 1
        fi
    fi
    imp="$(nl_banco_rodar_sonda "$distro" "$cont" "$dir" "$fonte" "$certificado" 2>&1)" || true
    printf '%s\n' "$imp" >> "$log"
    usada="$(nl_banco_conferir_classe "$imp" "$certificado")" || { echo "PARADA: classe do servidor: impressão não confere em $cont" >&2; return 1; }
    printf '%s\n' "$usada"
}
