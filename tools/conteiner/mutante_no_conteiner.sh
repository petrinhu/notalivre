#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Corpo do modo mutante-mtls (NL-3.0j, decisão do CTO de 11/11/2026, I1 da revisão). Roda DENTRO do contêiner, chamado pelo
# build_local.sh mutante-mtls como: bash /extrator/mutante_no_conteiner.sh REPO SHA BASE, com NL_MUTANTE_SCRIPT_SHA256.
# O script do mutante (tools/mutacao/mutante_mtls.sh) é o artefato sob teste: sai do SHA pedido, nunca da árvore de quem
# chama. A prova fica no log, por conteúdo: o sha256 do arquivo extraído tem de ser igual ao do blob do SHA, calculado
# pelo build_local.sh no host. Se diferir, nada é executado.
# Ordem:
#   1. confere RLIMIT_CORE 1 1 (mutante derruba teste de propósito; core maior que 1 chega ao coletor do host);
#   2. extrai tools/mutacao do SHA para BASE/mutante-fonte, pasta NOVA (resto de rodada anterior aborta);
#   3. imprime o sha256 esperado e o do arquivo extraído, e compara;
#   4. executa o arquivo extraído, e só ele: bash <extraído> REPO SHA BASE/mutante-mtls;
#   5. sai com o código do script do mutante.
# Uso: mutante_no_conteiner.sh REPO SHA BASE
#   REPO  repositório git, ou a pasta .git dele (no contêiner: /gitrepo)
#   SHA   commit de 40 caracteres hexadecimais
#   BASE  pasta de trabalho gravável (no contêiner: /var/tmp/notalivre-build)
# Códigos próprios, fora da faixa do mutante_mtls.sh (0, 1 e 2): 20 uso ou hash esperado inválido; 21 core fora de 1 1;
# 23 extração falhou (caminho ausente no SHA, destino que já existe, git); 24 arquivo extraído diferente do blob do SHA.
set -u

if [ "$#" -ne 3 ]; then
    echo "uso: mutante_no_conteiner.sh REPO SHA BASE" >&2
    exit 20
fi
repo="$1"
sha="$2"
base="$3"
esperado="${NL_MUTANTE_SCRIPT_SHA256:-}"
aqui="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

grep '^Max core file size' /proc/self/limits || echo "Max core file size: linha ausente em /proc/self/limits"
limite_core="$(awk '/^Max core file size/ {print $5 " " $6}' /proc/self/limits)"
echo "limite de core: $limite_core (esperado 1 1)"
if [ "$limite_core" != "1 1" ]; then
    echo "MUTANTE PARADA: RLIMIT_CORE '$limite_core', esperado '1 1' (L-36)"
    exit 21
fi
if ! [[ "$esperado" =~ ^[0-9a-f]{64}$ ]]; then
    echo "MUTANTE PARADA: NL_MUTANTE_SCRIPT_SHA256 ausente ou fora de 64 hexadecimais"
    exit 20
fi

fonte="$base/mutante-fonte"
if ! bash "$aqui/extrair_do_sha.sh" "$repo" "$sha" "$fonte" tools/mutacao; then
    echo "MUTANTE PARADA: extração de tools/mutacao do SHA $sha falhou"
    exit 23
fi
script="$fonte/tools/mutacao/mutante_mtls.sh"
if [ ! -f "$script" ]; then
    echo "MUTANTE PARADA: tools/mutacao/mutante_mtls.sh não veio do SHA $sha"
    exit 23
fi
obtido="$(sha256sum < "$script" | cut -d' ' -f1)"
echo "NL-MUTANTE-SCRIPT-SHA256-ESPERADO=$esperado"
echo "NL-MUTANTE-SCRIPT-SHA256-EXTRAIDO=$obtido"
if [ "$obtido" != "$esperado" ]; then
    echo "MUTANTE PARADA: o arquivo extraído difere do blob do SHA $sha; nada foi executado"
    exit 24
fi
echo "NL-MUTANTE-SCRIPT-EXECUTADO=$script sha256=$obtido"
bash "$script" "$repo" "$sha" "$base/mutante-mtls"
rc=$?
echo "mutante corpo: script do SHA $sha saiu com rc=$rc"
exit "$rc"
