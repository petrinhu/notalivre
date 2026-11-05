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
# NL-3.0g (CISO, 05/11/2026): a lista de portões (tools/conteiner/lista_portoes.txt) e o piso (piso_portoes.txt) são lidos
# SÓ do SHA, por git show. Nunca de arquivo da árvore de trabalho nem de dirname: quem muda a lista ou o piso muda o SHA.
# Variável: NL_HIGIENE_IMAGEM (padrão notalivre-higiene:ci, a mesma imagem do job higiene do CI)
# Cota desta máquina (ordem do líder de 30/10/2026, dividida com outra sessão): cada contêiner tem 7 núcleos e 12 GiB.
# Saída: uma linha "portao=<nome> rc=<rc>" por portão e o total "portoes=N falharam=K (piso P)".
# Rc: 2 para piso ilegível, piso 0, lista ou piso não lidos do SHA, e para os erros de SHA e destino;
# 1 para lista com número diferente do piso (lista vazia reprova por aí) e para portão que falha.
# Autoteste de estreia: autoteste_piso_portoes.sh.
set -u

REPO="${1:?uso: portoes_estaticos.sh REPO SHA DESTINO}"
SHA="${2:?uso: portoes_estaticos.sh REPO SHA DESTINO}"
DESTINO="${3:?uso: portoes_estaticos.sh REPO SHA DESTINO}"
IMG="${NL_HIGIENE_IMAGEM:-notalivre-higiene:ci}"

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

PISO_TXT="$(git -C "$ARVORE" show "$SHA:tools/conteiner/piso_portoes.txt" 2>/dev/null)" \
    || { echo "PARADA: piso_portoes.txt não lido do SHA $SHA (rc 2)"; exit 2; }
piso="$(printf '%s\n' "$PISO_TXT" | sed -n '1p')"
case "$piso" in
    ''|*[!0-9]*) echo "PARADA: piso ilegível no SHA ('$piso')"; exit 2 ;;
esac
if [ "$piso" -eq 0 ]; then
    echo "PARADA: piso 0 recusado no SHA (zero portões é varredura quebrada, L-36)"
    exit 2
fi

# Portões: uma linha "nome|comando" por portão em tools/conteiner/lista_portoes.txt, do próprio SHA.
LISTA_TXT="$(git -C "$ARVORE" show "$SHA:tools/conteiner/lista_portoes.txt" 2>/dev/null)" \
    || { echo "PARADA: lista_portoes.txt não lida do SHA $SHA (rc 2)"; exit 2; }
PORTOES=()
while IFS= read -r linha; do
    case "$linha" in
        ''|'#'*) continue ;;
    esac
    PORTOES+=("$linha")
done <<< "$LISTA_TXT"
lista=${#PORTOES[@]}
if [[ "$lista" != "$piso" ]]; then
    echo "PARADA: a lista de PORTOES tem $lista portões, o piso é $piso (SHA $SHA)"
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
