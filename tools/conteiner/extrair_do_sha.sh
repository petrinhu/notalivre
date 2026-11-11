#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Extrai de um repositório git, num SHA, só os caminhos pedidos, para uma pasta (NL-3.0j, modo mutante-mtls).
# Roda dentro do contêiner: o script do mutante é tirado do próprio SHA (o artefato sob teste), e não da árvore de quem
# chama. Não mexe em LISTA_FONTE (evidencia_sha.sh): o manifesto de todo SHA continua o mesmo, e os logs antigos seguem
# verificáveis.
# Uso: extrair_do_sha.sh REPO SHA DESTINO CAMINHO...
#   REPO     repositório git, ou a pasta .git dele, de onde sai o SHA
#   SHA      commit de 40 caracteres hexadecimais
#   DESTINO  pasta NOVA, criada aqui (mkdir -p); destino que já existe é erro
# Rc: 0 extraído; 2 uso, SHA inválido ou destino que já existe; 3 git archive falhou.
set -o pipefail

repo="${1:?uso: extrair_do_sha.sh REPO SHA DESTINO CAMINHO...}"
sha="${2:?uso: extrair_do_sha.sh REPO SHA DESTINO CAMINHO...}"
destino="${3:?uso: extrair_do_sha.sh REPO SHA DESTINO CAMINHO...}"
shift 3
if [ "$#" -lt 1 ]; then
    echo "PARADA: nenhum caminho pedido" >&2
    exit 2
fi
if [ "${#sha}" != 40 ] || [ -n "$(printf '%s' "$sha" | tr -d '0-9a-f')" ]; then
    echo "PARADA: SHA '$sha' não tem 40 caracteres hexadecimais minúsculos" >&2
    exit 2
fi
if [ -e "$destino" ]; then
    echo "PARADA: o destino já existe ($destino); a extração exige pasta nova" >&2
    exit 2
fi
mkdir -p -- "$destino" || exit 2
git -C "$repo" archive --format=tar "$sha" -- "$@" | tar -x -C "$destino" || {
    echo "PARADA: git archive de $sha falhou" >&2
    exit 3
}
