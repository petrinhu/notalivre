#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Mutação dos portões (definição de fechamento da Onda 0, passo 2; L-36 e L-27).
# Cada portão TEM de reprovar a própria sabotagem, pelo motivo esperado. Se um deles não reprovar,
# o portão é um verde falso e o job falha. Sabota só CÓPIAS extraídas do blob commitado (git archive).
# Os portões rodam dentro de contêiner (L-50); o flatpak roda pelo modo indicado abaixo.
#
# Uso: sabotar_portoes.sh REF TRABALHO MODO_FLATPAK
#   REF           commit (HEAD ou SHA): a árvore sai de `git archive` desse commit (A0-09, L-27).
#                 WORKTREE: árvore de trabalho ainda sem commit (a saída diz que não é commit).
#   TRABALHO      pasta de rascunho fora da árvore (ex.: $RUNNER_TEMP/mutacao).
#   MODO_FLATPAK  docker  = contêiner ghcr flathub kde-6.11 com --privileged (só no CI);
#                 flatpak = org.flatpak.Builder via flatpak run (execução local, sem --privileged).
# Variáveis:
#   HIGIENE_IMAGEM              imagem dos portões em contêiner (padrão notalivre-higiene:ci)
#   FLATHUB_KDE_IMAGEM          imagem do build Flatpak no modo docker (padrão: digest do índice kde-6.11)
#   NL_PORTOES                  portões a sabotar (padrão: a2 gitleaks travessao spdx flatpak611 hex)
#   MUTACAO_CONTROLE_HARNESS=1  controle do PRÓPRIO harness: só o a2, SEM sabotagem; o veredito tem de sair 1.
# Saída: "portao=<nome> rc=<rc> motivo=<ok|nao> sha=<sha>" por portão, e o veredito. Termina com exit 1
# se algum portão não reprovar pelo motivo esperado, ou se nenhum portão rodou (varredura vazia, L-36).
set -u

REF="${1:?uso: sabotar_portoes.sh REF TRABALHO MODO_FLATPAK}"
TRABALHO="${2:?uso: sabotar_portoes.sh REF TRABALHO MODO_FLATPAK}"
MODO="${3:?uso: sabotar_portoes.sh REF TRABALHO MODO_FLATPAK}"
IMAGEM="${HIGIENE_IMAGEM:-notalivre-higiene:ci}"
FLATHUB_KDE="${FLATHUB_KDE_IMAGEM:-ghcr.io/flathub-infra/flatpak-github-actions@sha256:b0a8a2acb9e420dd0e3e083ff552bea5a817e17e41532f1d2287ff49ef9aae0e}"
PORTOES="${NL_PORTOES:-a2 gitleaks travessao spdx flatpak611 hex}"
CONTROLE="${MUTACAO_CONTROLE_HARNESS:-}"
RESUMO="${GITHUB_STEP_SUMMARY:-/dev/null}"
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FALHAS=0
EXECUTADOS=0
SHA=""
FONTE=""

[ "$MODO" = docker ] || [ "$MODO" = flatpak ] || { echo "REPROVA: MODO_FLATPAK inválido: $MODO"; exit 1; }
mkdir -p "$TRABALHO" || { echo "REPROVA: sem pasta de trabalho em $TRABALHO"; exit 1; }

# Fonte da árvore: o blob commitado (git archive), ou a árvore de trabalho quando pedida explicitamente.
if [ "$REF" = WORKTREE ]; then
    SHA="$(git -C "$RAIZ" rev-parse --verify HEAD)" || { echo "REPROVA: sem HEAD"; exit 1; }
    FONTE="arvore-de-trabalho (NAO commitada; a prova no blob fica para o próximo commit)"
    tar --exclude=./.git --exclude='./build*' -C "$RAIZ" -cf "$TRABALHO/base.tar" . || { echo "REPROVA: tar da árvore de trabalho"; exit 1; }
else
    SHA="$(git -C "$RAIZ" rev-parse --verify "$REF^{commit}")" || { echo "REPROVA: REF não resolve: $REF"; exit 1; }
    FONTE="git-archive"
    git -C "$RAIZ" archive --format=tar "$SHA" > "$TRABALHO/base.tar" || { echo "REPROVA: git archive de $SHA falhou"; exit 1; }
fi
echo "fonte=$FONTE sha=$SHA portoes=$PORTOES controle=${CONTROLE:-nao}" | tee -a "$RESUMO"
if [ -n "$CONTROLE" ]; then
    PORTOES="a2"
    echo "CONTROLE do harness: o a2 roda SEM sabotagem; o veredito tem de sair 1 (verde falso)"
fi

echo "## Mutação dos portões (Onda 0, L-36, sha $SHA)" >> "$RESUMO"
echo "| Portão | rc da sabotagem | Reprovou pelo motivo esperado |" >> "$RESUMO"
echo "| --- | --- | --- |" >> "$RESUMO"

# Cópia limpa da árvore extraída do commit (ou da árvore de trabalho no modo WORKTREE). Uma extração por cópia.
copiar() {
    local destino="$TRABALHO/$1"
    rm -rf -- "$destino" && mkdir -p "$destino" && tar -C "$destino" -xf "$TRABALHO/base.tar" || return 1
    echo "$destino"
}

ativo() {
    case " $PORTOES " in *" $1 "*) return 0 ;; *) return 1 ;; esac
}

# Registra o resultado de um portão: rc != 0 (e não 126/127, que são falha de execução) E motivo presente.
registrar() {
    local nome="$1" rc="$2" motivo_ok="$3" log="$4" ok=nao
    EXECUTADOS=$((EXECUTADOS + 1))
    if [ "$rc" -ne 0 ] && [ "$rc" -ne 126 ] && [ "$rc" -ne 127 ] && [ "$motivo_ok" = sim ]; then
        ok=ok
    fi
    echo "portao=$nome rc=$rc motivo=$ok sha=$SHA"
    echo "| $nome | $rc | $ok |" >> "$RESUMO"
    if [ "$ok" != ok ]; then
        FALHAS=$((FALHAS + 1))
        echo "--- saída de $nome (não reprovou pelo motivo esperado):"
        tail -n 15 "$log"
    fi
}

# Contêiner de portão: sem rede, usuário do host (escreve só na cópia de rascunho).
em_conteiner() {
    docker run --rm --network none --user "$(id -u):$(id -g)" -e HOME=/var/tmp -e TMPDIR=/var/tmp "$@"
}

# 1. A2 de camadas: include de camada proibida (back incluindo mid). Sem sabotagem no controle do harness.
if ativo a2; then
    d=$(copiar a2) || { echo "copia a2 falhou"; exit 1; }
    [ -n "$CONTROLE" ] || echo '#include <nl/mid/camada.h>' >> "$d/src/back/camada.cpp"
    log="$TRABALHO/a2.log"
    em_conteiner -v "$d:/src:ro,z" "$IMAGEM" python3 -I /src/tools/portoes/camadas.py /src > "$log" 2>&1
    rc=$?
    grep -q 'VIOLACAO src/back/camada.cpp' "$log" && m=sim || m=nao
    registrar a2 "$rc" "$m" "$log"
fi

# 2. gitleaks: chave privada gerada DENTRO do contêiner, só na cópia; a chave é apagada logo depois.
if ativo gitleaks; then
    d=$(copiar gitleaks) || { echo "copia gitleaks falhou"; exit 1; }
    log="$TRABALHO/gitleaks.log"
    # ssh-keygen precisa de entrada no passwd: gera como root no contêiner e devolve a posse ao UID do host.
    docker run --rm --network none --user 0:0 -e HOME=/var/tmp -e TMPDIR=/var/tmp -v "$d:/src:z" "$IMAGEM" sh -c \
        'ssh-keygen -q -t ed25519 -N "" -C mutacao-nao-real -f /src/chave_de_mutacao && chown '"$(id -u):$(id -g)"' /src/chave_de_mutacao /src/chave_de_mutacao.pub && bash /src/tools/portoes/higiene_segredos.sh /src' \
        > "$log" 2>&1
    rc=$?
    rm -f -- "$d/chave_de_mutacao" "$d/chave_de_mutacao.pub"
    grep -q 'REPROVA: segredo detectado' "$log" && m=sim || m=nao
    registrar gitleaks "$rc" "$m" "$log"
fi

# 3. Travessão: U+2014 (bytes E2 80 94) escrito em octal, para este arquivo não conter o caractere.
if ativo travessao; then
    d=$(copiar travessao) || { echo "copia travessao falhou"; exit 1; }
    printf 'Texto de mutacao com travess\303\243o \342\200\224 no README.\n' >> "$d/README.md"
    log="$TRABALHO/travessao.log"
    em_conteiner -v "$d:/src:ro,z" "$IMAGEM" python3 -I /src/tools/portoes/higiene_travessao.py /src > "$log" 2>&1
    rc=$?
    grep -q 'VIOLACAO README.md' "$log" && m=sim || m=nao
    registrar travessao "$rc" "$m" "$log"
fi

# 4. SPDX (A0-06): arquivo de código sem cabeçalho na árvore do commit.
if ativo spdx; then
    d=$(copiar spdx) || { echo "copia spdx falhou"; exit 1; }
    printf 'int sem_cabecalho = 0;\n' > "$d/src/back/sem_cabecalho.cpp"
    log="$TRABALHO/spdx.log"
    em_conteiner -v "$d:/src:ro,z" "$IMAGEM" python3 -I /src/tools/portoes/spdx.py /src > "$log" 2>&1
    rc=$?
    grep -q 'VIOLACAO src/back/sem_cabecalho.cpp' "$log" && m=sim || m=nao
    registrar spdx "$rc" "$m" "$log"
fi

# 5. Flatpak contra o SDK 6.11: API só do Qt 6.12 (qSharedPointerStaticCast) numa cópia do teste.
if ativo flatpak611; then
    d=$(copiar flatpak611) || { echo "copia flatpak611 falhou"; exit 1; }
    printf '\n#include <QSharedPointer>\nstatic const auto kSabotagemQt612 = qSharedPointerStaticCast<QObject>(QSharedPointer<QObject>());\n' \
        >> "$d/tests/sanidade/tst_sanidade.cpp"
    log="$TRABALHO/flatpak611.log"
    rm -rf "$d/.fb-build" "$d/.fb-state" "$d/.fb-repo"
    if [ "$MODO" = docker ]; then
        docker run --rm --privileged -v "$d:/src:z" -w /src "$FLATHUB_KDE" \
            flatpak-builder --disable-rofiles-fuse --force-clean --state-dir=/src/.fb-state --repo=/src/.fb-repo \
            /src/.fb-build /src/flatpak/io.github.petrinhu.notalivre.yml > "$log" 2>&1
    else
        flatpak run --user --filesystem="$TRABALHO" --filesystem=xdg-data/flatpak:ro \
            --env=FLATPAK_USER_DIR="$HOME/.local/share/flatpak" --command=flatpak-builder org.flatpak.Builder \
            --disable-rofiles-fuse --force-clean --state-dir="$d/.fb-state" --repo="$d/.fb-repo" \
            "$d/.fb-build" "$d/flatpak/io.github.petrinhu.notalivre.yml" > "$log" 2>&1
    fi
    rc=$?
    grep -q 'qSharedPointerStaticCast' "$log" && m=sim || m=nao
    registrar flatpak611 "$rc" "$m" "$log"
fi

# 6. Hexadecimal fora do Theme (NL-1.8, L-36): um #RRGGBB num .qml de interface que não é Theme.qml.
if ativo hex; then
    d=$(copiar hex) || { echo "copia hex falhou"; exit 1; }
    mkdir -p "$d/apps/gui/qml"
    printf 'import QtQuick\n\nRectangle { color: "#FF0000" }\n' > "$d/apps/gui/qml/SabotagemHex.qml"
    log="$TRABALHO/hex.log"
    em_conteiner -v "$d:/src:ro,z" "$IMAGEM" python3 -I /src/tools/portoes/nenhum_hex_fora_do_theme.py /src > "$log" 2>&1
    rc=$?
    grep -q 'violacao: apps/gui/qml/SabotagemHex.qml:' "$log" && m=sim || m=nao
    registrar hex "$rc" "$m" "$log"
fi

if [ "$EXECUTADOS" -eq 0 ]; then
    echo "VEREDITO: nenhum portão foi sabotado (varredura vazia, L-36)" | tee -a "$RESUMO"
    exit 1
fi
if [ "$FALHAS" -eq 0 ]; then
    echo "VEREDITO: $EXECUTADOS portão(ões) reprovaram a própria sabotagem pelo motivo esperado (sha $SHA)" | tee -a "$RESUMO"
    exit 0
fi
echo "VEREDITO: $FALHAS de $EXECUTADOS portão(ões) NÃO reprovaram pelo motivo esperado (verde falso)" | tee -a "$RESUMO"
exit 1
