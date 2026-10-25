#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Mutação dos portões da Onda 0 (definição de fechamento, passo 2; L-36 e L-27).
# Cada portão TEM de reprovar a própria sabotagem, pelo motivo esperado. Se um deles não reprovar,
# o portão é um verde falso e o job falha. Sabota só CÓPIAS dentro de TRABALHO; a árvore de origem não é tocada.
#
# Uso: sabotar_portoes.sh RAIZ TRABALHO MODO_FLATPAK
#   RAIZ          árvore do repositório (somente leitura para este script).
#   TRABALHO      pasta de rascunho fora da árvore (ex.: $RUNNER_TEMP/mutacao).
#   MODO_FLATPAK  docker  = contêiner ghcr flathub kde-6.11 com --privileged (CI);
#                 flatpak = org.flatpak.Builder via flatpak run (execução local, mesmo flatpak-builder).
# Variável HIGIENE_IMAGEM: imagem do portão de higiene (padrão notalivre-higiene:ci).
# Saída: uma linha por portão "portao=<nome> rc=<rc> motivo=<ok|nao>" e o veredito. Sempre termina com exit 1
# se qualquer portão não reprovar pelo motivo esperado, antes de qualquer ação que proteja.
set -u

RAIZ="${1:?uso: sabotar_portoes.sh RAIZ TRABALHO MODO_FLATPAK}"
TRABALHO="${2:?uso: sabotar_portoes.sh RAIZ TRABALHO MODO_FLATPAK}"
MODO="${3:?uso: sabotar_portoes.sh RAIZ TRABALHO MODO_FLATPAK}"
IMAGEM="${HIGIENE_IMAGEM:-notalivre-higiene:ci}"
FLATHUB_KDE="ghcr.io/flathub-infra/flatpak-github-actions:kde-6.11"
RESUMO="${GITHUB_STEP_SUMMARY:-/dev/null}"
FALHAS=0

[ -d "$RAIZ" ] || { echo "REPROVA: raiz ausente: $RAIZ"; exit 1; }
mkdir -p "$TRABALHO"

# Cópia limpa da árvore, sem .git, para a sabotagem. Uma chamada de tar, nunca um processo por arquivo.
copiar()
{
    local destino="$TRABALHO/$1"
    rm -rf "$destino" && mkdir -p "$destino" || return 1
    (cd "$RAIZ" && tar --exclude=./.git -cf - .) | tar -C "$destino" -xf - || return 1
    echo "$destino"
}

# Registra o resultado de um portão e confere o motivo esperado (rc != 0 E texto de motivo presente).
registrar()
{
    local nome="$1" rc="$2" motivo_ok="$3" log="$4"
    local ok=nao
    if [ "$rc" -ne 0 ] && [ "$rc" -ne 126 ] && [ "$rc" -ne 127 ] && [ "$motivo_ok" = sim ]; then
        ok=ok
    fi
    echo "portao=$nome rc=$rc motivo=$ok"
    echo "| $nome | $rc | $ok |" >> "$RESUMO"
    if [ "$ok" != ok ]; then
        FALHAS=$((FALHAS + 1))
        echo "--- saída de $nome (não reprovou pelo motivo esperado):" && tail -n 15 "$log"
    fi
}

echo "## Mutação dos portões (Onda 0, L-36)" >> "$RESUMO"
echo "| Portão | rc da sabotagem | Reprovou pelo motivo esperado |" >> "$RESUMO"
echo "| --- | --- | --- |" >> "$RESUMO"

# 1. A2 de camadas: include de camada proibida (back incluindo mid).
d=$(copiar a2) || { echo "copia a2 falhou"; exit 1; }
echo '#include <nl/mid/camada.h>' >> "$d/src/back/camada.cpp"
log="$TRABALHO/a2.log"
python3 -I "$d/tools/portoes/camadas.py" "$d" > "$log" 2>&1
rc=$?
grep -q 'VIOLACAO src/back/camada.cpp' "$log" && m=sim || m=nao
registrar a2 "$rc" "$m" "$log"

# 2. gitleaks: chave privada gerada na hora, fora da árvore, colocada só na cópia.
d=$(copiar gitleaks) || { echo "copia gitleaks falhou"; exit 1; }
chave="$TRABALHO/chave_gerada"
rm -f "$chave" "$chave.pub"
ssh-keygen -q -t ed25519 -N '' -C 'mutacao-nao-real' -f "$chave" || { echo "ssh-keygen falhou"; exit 1; }
cp "$chave" "$d/chave_de_mutacao"
log="$TRABALHO/gitleaks.log"
docker run --rm -v "$d:/src:ro,z" "$IMAGEM" bash /src/tools/portoes/higiene_segredos.sh /src > "$log" 2>&1
rc=$?
grep -q 'REPROVA' "$log" && m=sim || m=nao
registrar gitleaks "$rc" "$m" "$log"
rm -f "$chave" "$chave.pub"

# 3. Travessão: U+2014 (bytes E2 80 94) escrito em octal, para este arquivo não conter o caractere.
d=$(copiar travessao) || { echo "copia travessao falhou"; exit 1; }
printf 'Texto de mutacao com travess\303\243o \342\200\224 no README.\n' >> "$d/README.md"
log="$TRABALHO/travessao.log"
docker run --rm -v "$d:/src:ro,z" "$IMAGEM" python3 -I /src/tools/portoes/higiene_travessao.py /src > "$log" 2>&1
rc=$?
grep -q 'REPROVA' "$log" && m=sim || m=nao
registrar travessao "$rc" "$m" "$log"

# 4. Portão Flatpak contra o SDK 6.11: API só do Qt 6.12 (qSharedPointerStaticCast) numa cópia do teste.
d=$(copiar flatpak611) || { echo "copia flatpak611 falhou"; exit 1; }
printf '\n#include <QSharedPointer>\nstatic const auto kSabotagemQt612 = qSharedPointerStaticCast<QObject>(QSharedPointer<QObject>());\n' \
    >> "$d/tests/sanidade/tst_sanidade.cpp"
log="$TRABALHO/flatpak611.log"
rm -rf "$d/.fb-build" "$d/.fb-state" "$d/.fb-repo"
if [ "$MODO" = docker ]; then
    docker run --rm --privileged -v "$d:/src:z" -w /src "$FLATHUB_KDE" \
        flatpak-builder --disable-rofiles-fuse --force-clean --state-dir=/src/.fb-state --repo=/src/.fb-repo /src/.fb-build /src/flatpak/io.github.petrinhu.notalivre.yml > "$log" 2>&1
else
    flatpak run --user --filesystem="$TRABALHO" --filesystem=xdg-data/flatpak:ro \
        --env=FLATPAK_USER_DIR="$HOME/.local/share/flatpak" --command=flatpak-builder org.flatpak.Builder \
        --disable-rofiles-fuse --force-clean --state-dir="$d/.fb-state" --repo="$d/.fb-repo" "$d/.fb-build" "$d/flatpak/io.github.petrinhu.notalivre.yml" > "$log" 2>&1
fi
rc=$?
grep -q 'qSharedPointerStaticCast' "$log" && m=sim || m=nao
registrar flatpak611 "$rc" "$m" "$log"

if [ "$FALHAS" -eq 0 ]; then
    echo "VEREDITO: os quatro portões reprovaram a própria sabotagem" | tee -a "$RESUMO"
    exit 0
fi
echo "VEREDITO: $FALHAS portão(ões) NÃO reprovaram pelo motivo esperado (verde falso)" | tee -a "$RESUMO"
exit 1
