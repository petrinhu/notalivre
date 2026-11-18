# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Sonda de impressão do certificado do servidor do par (NL-3.1h): um contêiner curto na rede do servidor, com a imagem de
# build da distro (que traz o openssl; FATO: tools/conteiner/instalar/*.sh). Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_rodar_sonda DISTRO SERVIDOR DIR FONTE CERTIFICADO: imprime a linha NL-IMPRESSAO do par. Roda bash (nunca sh: no
# Ubuntu e no Mint, sh é dash). O PEM vai só na classe disco; nas outras, o argumento é "-".
nl_banco_rodar_sonda() {
    local distro="$1" servidor="$2" dir="$3" fonte="$4" certificado="$5" pem=-
    NL_THREADS=1 NL_MEMORIA=1g nl_cotas_docker || return 2
    if [ "$certificado" = disco ]; then
        pem=/run/nl/certificado_servidor.pem
    fi
    docker run --pull=never --rm "${NL_COTAS_DOCKER[@]}" --pids-limit 128 --network "container:$servidor" \
        --user 1000:1000 -v "$dir:/run/nl:ro,z" -v "$fonte:/src:ro,z" "notalivre/build-$distro:nl-2.0" \
        bash /src/tools/conteiner/impressao_servidor_banco.sh "$certificado" "$pem"
}
