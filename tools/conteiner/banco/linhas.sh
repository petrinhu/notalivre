# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Linhas de saída do harness do banco (NL-3.1h): só imprimem. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_linha_par DISTRO NOME VERSAO METODO TLS_CLIENTE TLS_SERVIDOR CONTAGEM RC
nl_banco_linha_par() {
    echo "NL-BANCO par=$1/$2 versao=$3 metodo=$4 tls_cliente=$5 tls_servidor=$6 $7 rc=$8"
}

# nl_banco_linha_resumo ESPERADOS EXECUTADOS VERDES VERMELHOS
nl_banco_linha_resumo() {
    echo "NL-BANCO-RESUMO pares_esperados=$1 executados=$2 verdes=$3 vermelhos=$4"
}
