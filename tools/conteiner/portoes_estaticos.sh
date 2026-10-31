#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portões estáticos no build local (NL-2.11, DA-2.55; L-36: rodar local o que o CI roda, antes do push).
# Até aqui esses portões só rodavam nos jobs higiene e camadas do ci.yml, que disparam em onda-* e main. O build_local
# chama este script antes do configure, sobre o mesmo SHA que vai compilar.
#
# Uso: portoes_estaticos.sh REPO SHA DESTINO
#   REPO     repositório git de onde sai o SHA (o mesmo NL_REPO do build_local)
#   SHA      commit a verificar (40 hex)
#   DESTINO  pasta NOVA, fora do repositório; recebe o clone do SHA (gitleaks git e os portões de git exigem histórico)
# Variável: NL_HIGIENE_IMAGEM (padrão notalivre-higiene:ci, a mesma imagem do job higiene do CI)
# Cota desta máquina (ordem do líder de 30/10/2026, dividida com outra sessão): cada contêiner tem 7 núcleos e 12 GiB.
# Saída: uma linha "portao=<nome> rc=<rc>" por portão e o total "portoes=N falharam=K (piso P)".
# Piso versionado em tools/conteiner/piso_portoes.txt (1ª linha). Reprova (rc 1) se a lista de PORTOES não tiver
# exatamente P itens ou se o laço não rodar P portões. Piso 0 ou ilegível sai com rc 2 (zero portões é varredura
# quebrada, L-36). Autoteste de estreia: autoteste_piso_portoes.sh.
set -u

REPO="${1:?uso: portoes_estaticos.sh REPO SHA DESTINO}"
SHA="${2:?uso: portoes_estaticos.sh REPO SHA DESTINO}"
DESTINO="${3:?uso: portoes_estaticos.sh REPO SHA DESTINO}"
IMG="${NL_HIGIENE_IMAGEM:-notalivre-higiene:ci}"

PISO_ARQ="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/piso_portoes.txt"
piso="$(sed -n '1p' "$PISO_ARQ" 2>/dev/null)"
case "$piso" in
    ''|*[!0-9]*) echo "PARADA: piso ilegível em $PISO_ARQ ('$piso')"; exit 2 ;;
esac
if [ "$piso" -eq 0 ]; then
    echo "PARADA: piso 0 recusado em $PISO_ARQ (zero portões é varredura quebrada, L-36)"
    exit 2
fi

case "$SHA" in
    [0-9a-f]*) [ "${#SHA}" = 40 ] || { echo "PARADA: SHA com ${#SHA} caracteres, esperado 40"; exit 2; } ;;
    *) echo "PARADA: SHA não é hexadecimal"; exit 2 ;;
esac

if [ -e "$DESTINO" ]; then
    echo "PARADA: destino já existe ($DESTINO); os portões precisam de pasta nova"
    exit 2
fi
mkdir -p "$DESTINO" || exit 2
ARVORE="$DESTINO/arvore"
git clone --quiet --no-hardlinks "$REPO" "$ARVORE" || { echo "PARADA: clone de $REPO falhou"; exit 2; }
git -C "$ARVORE" checkout --quiet "$SHA" || { echo "PARADA: checkout de $SHA falhou"; exit 2; }

# Portões: nome e comando (rodado com sh -c dentro da imagem, com a árvore montada em /src, só leitura).
PORTOES=(
    "camadas|python3 -I /src/tools/portoes/camadas.py /src"
    "autoteste_camadas|bash /src/tools/portoes/autoteste_camadas.sh /src"
    "segredos_arvore|bash /src/tools/portoes/higiene_segredos.sh /src"
    "segredos_historico|gitleaks git /src --config /src/.gitleaks.toml --redact --no-banner --exit-code 1"
    "travessao|python3 -I /src/tools/portoes/higiene_travessao.py /src"
    "spdx|git config --global --add safe.directory /src && python3 -I /src/tools/portoes/spdx.py /src && bash /src/tools/portoes/autoteste_spdx.sh"
    "autoteste_higiene|git config --global --add safe.directory /src && bash /src/tools/portoes/autoteste_higiene.sh"
    "hex_fora_do_theme|python3 -I /src/tools/portoes/nenhum_hex_fora_do_theme.py /src"
    "tema_gerado|python3 -I /src/tools/tema/gerar_theme.py --raiz /src --verificar"
    "sha1_env|git config --global --add safe.directory /src && python3 -I /src/tools/portoes/portao_sha1_env.py /src"
    "fixacao_ci|python3 -I /src/tools/portoes/fixacao_ci.py /src && python3 -I /src/tools/portoes/fixacao_ci.py --autoteste"
)
lista=${#PORTOES[@]}
if [[ "$lista" == "$piso" ]]; then :; else
    echo "PARADA: a lista de PORTOES tem $lista portões, o piso é $piso ($PISO_ARQ)"
    exit 1
fi

rodados=0
falharam=0
for item in "${PORTOES[@]}"; do
    nome="${item%%|*}"
    cmd="${item#*|}"
    rodados=$((rodados + 1))
    docker run --rm --cpus 7 --memory 12g --memory-swap 12g --network none --user 1000:1000 -e HOME=/tmp \
        -v "$ARVORE:/src:ro,z" -w /src "$IMG" sh -c "$cmd" > "$DESTINO/$nome.log" 2>&1
    rc=$?
    echo "portao=$nome rc=$rc"
    if [ "$rc" -ne 0 ]; then
        falharam=$((falharam + 1))
        echo "  saida: $DESTINO/$nome.log"
    fi
done

echo "portoes=$rodados falharam=$falharam (piso $piso)"
if [[ "$rodados" != "$piso" ]]; then
    echo "PARADA: piso de portões não rodou inteiro (rodados=$rodados, piso=$piso)"
    exit 1
fi
[ "$falharam" = 0 ] || exit 1
exit 0
