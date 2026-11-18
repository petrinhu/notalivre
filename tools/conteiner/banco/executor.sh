# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Contêiner que roda o CTest do banco (NL-3.1h), na rede do servidor do par. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_rodar_executor DISTRO SERVIDOR DIR FONTE BUILD: ctest --preset banco com rede do servidor, sem senha no ambiente.
nl_banco_rodar_executor() {
    NL_THREADS=3 NL_MEMORIA=4g nl_cotas_docker || return 2
    docker run --pull=never --rm "${NL_COTAS_DOCKER_SEM_PIDS[@]}" --pids-limit 400 --ulimit core=1 \
        --network "container:$2" --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -e NL_TMP_BASE=/var/tmp \
        --env-file "$3/executor.env" -v "$3:/run/nl:ro,z" -v "$4:/src:ro,z" -v "$5:/var/tmp/notalivre-build:z" \
        -v notalivre-qt-6120:/opt/qt:ro -w /src "notalivre/build-$1:nl-2.0" \
        sh -c 'mkdir -p "$HOME" && ctest --preset banco --no-tests=error --output-on-failure --verbose'
}

# nl_banco_rodar_portao_funcoes DISTRO FONTE BUILD: confere o registro do banco (C15') num contêiner sem rede.
nl_banco_rodar_portao_funcoes() {
    NL_THREADS=1 NL_MEMORIA=2g nl_cotas_docker || return 2
    docker run --pull=never --rm "${NL_COTAS_DOCKER_SEM_PIDS[@]}" --pids-limit 256 --network none --user 1000:1000 \
        -e HOME=/tmp/home -e TMPDIR=/var/tmp -v "$2:/src:ro,z" -v "$3:/var/tmp/notalivre-build:z" -w /src \
        "notalivre/build-$1:nl-2.0" \
        sh -c 'mkdir -p "$HOME" && python3 -I tools/portoes/funcoes_no_ctest.py --raiz . --build /var/tmp/notalivre-build/banco --excecoes tests/funcoes_fora_do_ctest.txt'
}
