#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Conferência do ZIP do XSD de Produção Restrita no CI (NL-3.1v, seção 1.4, passo 3, do plano do CTO), na regra do passo do
# job nativo (NL-1.6c): o SHA-256 do ci.yml tem de ser o do manifesto da árvore, e o ZIP em PASTA tem de ter esse SHA-256;
# divergência apaga o ZIP. Só confere, nunca baixa: o gov.br caiu 5 vezes com "SSL connect error" a partir do runner (run
# 45985663535), e o ZIP vem do cache dos jobs nativos, pelo job xsd-cache e por artefato. ZIP ausente reprova.
# Lê NL_XSD_ZIP_SHA256 do ambiente do ci.yml. Uso: zip_xsd.sh PASTA
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ZIP_NOME=esquemas_prod_restrita_20261020.zip
MANIFESTO="$RAIZ/third_party/nfse-xsd/prodrest-20261020/SHA256SUMS"

# nl_zip_xsd_manifesto_confere: rc 0 só se o SHA-256 do ci.yml é o do manifesto da árvore (cache e código falam do mesmo ZIP).
nl_zip_xsd_manifesto_confere() {
    grep -q -F "# ZIP SHA-256: $NL_XSD_ZIP_SHA256 " "$MANIFESTO"
}

# nl_zip_xsd_conferir PASTA: sha256sum -c do ZIP em PASTA; divergência apaga o ZIP e sai 1.
nl_zip_xsd_conferir() {
    if ! (cd "$1" && echo "$NL_XSD_ZIP_SHA256  $ZIP_NOME" | sha256sum -c -); then
        rm -f -- "$1/$ZIP_NOME"
        echo "::error::SHA-256 do ZIP do XSD divergente do esperado ($NL_XSD_ZIP_SHA256); cache ou artefato corrompido. Chave do cache não pode ser sobrescrita: trocar a chave."
        return 1
    fi
}

PASTA="${1:-}"
[ -n "$PASTA" ] || { echo "PARADA: uso: zip_xsd.sh PASTA"; exit 2; }
[ -n "${NL_XSD_ZIP_SHA256:-}" ] || { echo "PARADA: NL_XSD_ZIP_SHA256 é obrigatório"; exit 2; }
nl_zip_xsd_manifesto_confere || { echo "::error::SHA-256 do ci.yml ($NL_XSD_ZIP_SHA256) fora do manifesto $MANIFESTO"; exit 1; }
if [ ! -f "$PASTA/$ZIP_NOME" ]; then
    echo "::error::ZIP do XSD ausente em $PASTA (sem cache dos jobs nativos ou sem artefato); este passo não baixa do gov.br"
    exit 1
fi
nl_zip_xsd_conferir "$PASTA" || exit 1
echo "ZIP do XSD conferido: $PASTA/$ZIP_NOME"
