#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# ZIP do XSD de Produção Restrita no CI (NL-3.1v, seção 1.4, passo 3, do plano do CTO). A mesma regra do passo do job nativo
# (NL-1.6c): o SHA-256 do ci.yml tem de ser o do manifesto da árvore; o ZIP vem do cache ou, na falta, do gov.br por
# tools/ci/baixar_zip_xsd.cmake (5 tentativas); o SHA-256 é conferido antes de qualquer build, e divergência apaga o ZIP.
# Lê NL_XSD_ZIP_SHA256 e NL_XSD_ZIP_URL do ambiente do ci.yml. O build do banco roda sem rede e lê o ZIP desta PASTA.
# Uso: zip_xsd.sh PASTA
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ZIP_NOME=esquemas_prod_restrita_20261020.zip
MANIFESTO="$RAIZ/third_party/nfse-xsd/prodrest-20261020/SHA256SUMS"

# nl_zip_xsd_manifesto_confere: rc 0 só se o SHA-256 do ci.yml é o do manifesto da árvore (cache e código falam do mesmo ZIP).
nl_zip_xsd_manifesto_confere() {
    grep -q -F "# ZIP SHA-256: $NL_XSD_ZIP_SHA256 " "$MANIFESTO"
}

# nl_zip_xsd_baixar ZIP: baixa do gov.br com nova tentativa e EXPECTED_HASH; na falha apaga o ZIP e sai 1.
nl_zip_xsd_baixar() {
    if ! cmake -DNL_URL="$NL_XSD_ZIP_URL" -DNL_DESTINO="$1" -DNL_SHA256="$NL_XSD_ZIP_SHA256" -P "$RAIZ/tools/ci/baixar_zip_xsd.cmake"; then
        rm -f -- "$1"
        echo "::error::ZIP do XSD não baixado do gov.br (5 tentativas, ou hash divergente)"
        return 1
    fi
}

# nl_zip_xsd_conferir PASTA: sha256sum -c do ZIP em PASTA; divergência apaga o ZIP e sai 1.
nl_zip_xsd_conferir() {
    if ! (cd "$1" && echo "$NL_XSD_ZIP_SHA256  $ZIP_NOME" | sha256sum -c -); then
        rm -f -- "$1/$ZIP_NOME"
        echo "::error::SHA-256 do ZIP do XSD divergente do esperado ($NL_XSD_ZIP_SHA256); cache ou download corrompido. Chave do cache não pode ser sobrescrita: trocar a chave."
        return 1
    fi
}

PASTA="${1:-}"
[ -n "$PASTA" ] || { echo "PARADA: uso: zip_xsd.sh PASTA"; exit 2; }
[ -n "${NL_XSD_ZIP_SHA256:-}" ] && [ -n "${NL_XSD_ZIP_URL:-}" ] || { echo "PARADA: NL_XSD_ZIP_SHA256 e NL_XSD_ZIP_URL são obrigatórios"; exit 2; }
nl_zip_xsd_manifesto_confere || { echo "::error::SHA-256 do ci.yml ($NL_XSD_ZIP_SHA256) fora do manifesto $MANIFESTO"; exit 1; }
mkdir -p -- "$PASTA" || exit 1
if [ -f "$PASTA/$ZIP_NOME" ]; then
    echo "ZIP do XSD: cache hit ($PASTA/$ZIP_NOME)"
else
    echo "ZIP do XSD: cache miss, baixando de $NL_XSD_ZIP_URL"
    nl_zip_xsd_baixar "$PASTA/$ZIP_NOME" || exit 1
fi
nl_zip_xsd_conferir "$PASTA" || exit 1
