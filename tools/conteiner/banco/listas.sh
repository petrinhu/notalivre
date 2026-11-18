# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Leitura das listas do banco (NL-3.1h): servidores e pares. Carregado por ponto. Exige NL_BANCO_DIR (tools/conteiner).
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_servidor_de NOME: imprime "nome familia imagem metodo" do servidor; rc 1 se o nome não existe.
nl_banco_servidor_de() {
    awk -v n="$1" '$0 !~ /^#/ && $1 == n { print; achou = 1 } END { exit !achou }' "$NL_BANCO_DIR/servidores_banco.txt"
}

# nl_banco_pares_de DISTRO: imprime o nome de cada servidor do par da distro, um por linha.
nl_banco_pares_de() {
    awk -v d="$1" '$0 !~ /^#/ && $1 == d { print $2 }' "$NL_BANCO_DIR/matriz_banco.txt"
}
