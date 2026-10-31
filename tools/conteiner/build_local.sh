#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Build e teste local do NotaLivre dentro de contêiner (L-50: nada de teste dinâmico na sessão do líder).
#
# Uso:
#   build_local.sh imagem <distro>        constrói a imagem de build da distro (pesado, sob flock)
#   build_local.sh qt                     instala Qt 6.12 no volume notalivre-qt-6120 (só se ainda não estiver)
#   build_local.sh preparar               imagem fedora44 + Qt (forma antiga da NL-0.2)
#   build_local.sh <distro> <preset>      configure + build + ctest do preset na distro (exige NL_FATIA)
#   build_local.sh <preset>               atalho: fedora44 <preset> (forma antiga, mantida; exige NL_FATIA)
#   distro: fedora44 | ubuntu26.04 | manjaro | mint22.3
#
# Variáveis de ambiente:
#   NL_FATIA          OBRIGATÓRIA nos builds: identificador da fatia (ex.: NL-2.0), letras, dígitos, . _ e -.
#                     Dá nome ao diretório de fonte e ao de build. Sem default: um default silencioso faria dois
#                     executores voltarem a compartilhar diretório (achado A1-07).
#   NL_SHA            commit a construir (padrão: HEAD do repositório NL_REPO). É resolvido para 40 hex antes de tudo.
#   NL_REPO           repositório git de onde sai o git archive (padrão: a raiz do repositório).
#   NL_FONTE          modo compatível para revisores com snapshot: copia esta pasta em vez de fazer o archive.
#                     O log grava "SHA: <sha>+copia", e o build PARA antes do contêiner se o manifesto da cópia
#                     diverge do commit (arquivo fora do commit). Sem NL_SHA, usa o HEAD da pasta, se ela for git.
#   NL_HIGIENE_IMAGEM imagem dos portões estáticos rodados antes do configure (padrão notalivre-higiene:ci; DA-2.55)
#   NL_TMP_BASE       base de temporários e de build no host (padrão: /var/tmp; o preset exige a variável)
#   NL_LOG            log do run (padrão: $NL_TMP_BASE/notalivre-logs/NL-0.4-<distro>-<preset>.log;
#                     imagem: $NL_TMP_BASE/notalivre-logs/NL-2.0-imagem-<distro>.log)
#   NL_BUILD_ROOT     diretório de build no host (padrão: $NL_TMP_BASE/notalivre-build-<distro>-<fatia>)
#   NL_XSD_ZIP_CACHE  diretório no host com o ZIP do XSD (padrão: $NL_TMP_BASE/notalivre-xsd-cache). É montado no
#                     contêiner em /xsd-cache e exportado como NL_XSD_ZIP_CACHE; o CMake do third_party lê a variável.
#
# Evidência por SHA (NL-2.0): a linha 1 do log é "SHA: <40 hex> ...", e NL-MANIFESTO-SHA256 traz o hash da fonte
# que o contêiner viu. tools/conteiner/evidencia_sha.sh verificar <log> confere o manifesto contra o commit.
#
# Regras aplicadas (L-11, L-36, L-45, L-49, L-53):
#   - um trabalho pesado por vez: cópia da fonte E docker run ficam DENTRO da mesma trava flock $TRAVA;
#   - cada fatia tem a própria fonte (notalivre-src-<SHA>-<fatia>) e o próprio build; nada compartilhado;
#   - cota de CPU e RAM do contêiner: NL_THREADS (padrão 7) e NL_MEMORIA (padrão 12g), por ordem do líder de 30/10/2026;
#   - build e temporários em /var/tmp (TMPDIR exportado abaixo), nunca em /tmp (tmpfs);
#   - o status é o do contêiner: o log é gravado por redirecionamento, nunca por pipe em tail;
#   - o contêiner roda com --user 1000:1000 e sem --privileged.
set -euo pipefail

# Base de temporários e de build no HOST (A0-08). Padrão /var/tmp; trocável pela variável NL_TMP_BASE.
NL_TMP_BASE="${NL_TMP_BASE:-/var/tmp}"
export NL_TMP_BASE
export TMPDIR="$NL_TMP_BASE"

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="$RAIZ/tools/conteiner/build_local.sh"
NL_REPO="${NL_REPO:-$RAIZ}"
# Funções de evidência por SHA (extrair_do_sha, manifesto_de_dir, manifesto_do_sha) e a lista LISTA_FONTE.
. "$RAIZ/tools/conteiner/evidencia_sha.sh"
VOLUME_QT="notalivre-qt-6120"
QT_PREFIXO="/opt/qt/6.12.0/gcc_64"
# SELinux Enforcing: a fonte (pasta sincronizada com a nuvem) é unlabeled_t e o contêiner não a lê por bind mount.
# Por isso o contêiner recebe uma cópia montada de /var/tmp (rótulo do próprio usuário, :z), refeita a cada fatia.
# Não se relabela a árvore sincronizada do líder.
LOG_DIR="$NL_TMP_BASE/notalivre-logs"
TRAVA="$NL_TMP_BASE/notalivre-heavy.lock"
# Cota dividida com outra sessão desta máquina (ordem do líder de 30/10/2026): 1 núcleo e 4 GiB livres para o sistema.
THREADS="${NL_THREADS:-7}"
NL_MEMORIA="${NL_MEMORIA:-12g}"

# Distros aceitas: cada uma tem <distro>-build.Dockerfile e instalar/<distro>.sh (ver ci.yml).
checar_distro()
{
    case "$1" in
        fedora44|ubuntu26.04|manjaro|mint22.3) ;;
        *) echo "distro desconhecida: $1 (aceitas: fedora44 ubuntu26.04 manjaro mint22.3)" >&2; exit 2 ;;
    esac
}

# Tag NL-2.0: as imagens ganharam xmlsec1 (oráculo da DA-2.1). A tag nl-0.4 antiga fica no Docker, sem apagar.
imagem_de()
{
    echo "notalivre/build-$1:nl-2.0"
}

# Executa um comando pesado sob a trava compartilhada e com prioridade reduzida.
# Se o chamador já detém a trava (NL_TRAVA_DETIDA=1), não pede de novo: flock aninhado no mesmo arquivo
# trava o próprio processo (medido em 25/10/2026: um flock órfão esperou a própria trava por 18 min).
pesado()
{
    if [ "${NL_TRAVA_DETIDA:-}" = 1 ]; then
        nice -n 5 "$@"
    else
        flock "$TRAVA" nice -n 5 "$@"
    fi
}

# Reconstrói a imagem de uma distro. O log é gravado por redirecionamento (NL_LOG ou padrão).
# Exceção documentada à evidência por SHA: o contexto da imagem (tools/conteiner) vem da ÁRVORE DE TRABALHO, porque
# as mudanças de instalar/ ainda não estão commitadas. A linha 1 traz o SHA de HEAD, mas declara origem=arvore-de-trabalho
# e o log NÃO traz NL-MANIFESTO-SHA256: o verificador reprova de propósito (imagem não é evidência por SHA até o commit).
construir_imagem()
{
    local distro="$1" log rc=0 sha
    checar_distro "$distro"
    mkdir -p "$LOG_DIR"
    log="${NL_LOG:-$LOG_DIR/NL-2.0-imagem-$distro.log}"
    sha="$(resolver_sha)" || exit 2
    printf 'SHA: %s fatia=imagem distro=%s origem=arvore-de-trabalho data=%s\n' \
        "$sha" "$distro" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" > "$log"
    pesado docker build -t "$(imagem_de "$distro")" \
        -f "$RAIZ/tools/conteiner/$distro-build.Dockerfile" "$RAIZ/tools/conteiner" \
        >> "$log" 2>&1 || rc=$?
    echo "NL-RC=$rc" >> "$log"
    echo "log: $log rc=$rc"
    return "$rc"
}

# Instala Qt 6.12 no volume, uma única vez. Se o volume já tem o Qt, não reinstala (aqt reinstalaria tudo).
qt_instalar()
{
    if pesado docker run --rm --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --user 1000:1000 -v "$VOLUME_QT:/opt/qt:ro" "$(imagem_de fedora44)" \
        test -f "$QT_PREFIXO/lib/cmake/Qt6/Qt6Config.cmake"; then
        echo "Qt 6.12 já presente no volume $VOLUME_QT: nada a instalar"
        return 0
    fi
    docker volume create "$VOLUME_QT" > /dev/null
    # O volume nasce com dono root; ajusta uma única vez para o usuário 1000 (dono do host).
    pesado docker run --rm --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" -u 0:0 -v "$VOLUME_QT:/opt/qt" "$(imagem_de fedora44)" chown 1000:1000 /opt/qt
    # HOME em /tmp/home (gravável pelo uid 1000 no contêiner; /var/tmp da imagem é do root e aqt falha ali).
    # aqt pode terminar com status 0 mesmo após erro: a conferência final mede o artefato no volume.
    pesado docker run --rm --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -e NL_TMP_BASE=/var/tmp \
        -v "$VOLUME_QT:/opt/qt" "$(imagem_de fedora44)" \
        sh -c 'mkdir -p "$HOME" && cd "$HOME" && aqt install-qt -O /opt/qt linux desktop 6.12.0 linux_gcc_64 -m qtnetworkauth qthttpserver qtimageformats qtshadertools qtwebsockets && test -f /opt/qt/6.12.0/gcc_64/lib/cmake/Qt6/Qt6Config.cmake'
}

# Resolve o SHA a construir para 40 hex, sem efeito colateral. Falha com PARADA (rc 2) se não for commit.
resolver_sha()
{
    local rev="${NL_SHA:-}" sha
    if [ -z "$rev" ]; then
        if [ -n "${NL_FONTE:-}" ]; then
            rev="$(git -C "$NL_FONTE" rev-parse --verify --quiet HEAD 2>/dev/null || true)"
            if [ -z "$rev" ]; then
                echo "PARADA: NL_FONTE não é repositório git: defina NL_SHA com o commit do snapshot" >&2
                return 2
            fi
        else
            rev=HEAD
        fi
    fi
    sha="$(git -C "$NL_REPO" rev-parse --verify --quiet "$rev^{commit}" 2>/dev/null || true)"
    if [ -z "$sha" ]; then
        echo "PARADA: '$rev' não é commit de $NL_REPO" >&2
        return 2
    fi
    printf '%s\n' "$sha"
}

# Valida NL_FATIA (obrigatória). Ela entra em nome de diretório, então aceita só [A-Za-z0-9._-] e sem ponto inicial.
checar_fatia()
{
    local fatia="${NL_FATIA:-}"
    if [ -z "$fatia" ]; then
        echo "PARADA: NL_FATIA obrigatória (ex.: NL_FATIA=NL-2.0): o nome da fatia dá o diretório de fonte e de build" >&2
        return 2
    fi
    case "$fatia" in
        .*|*..*|*[!A-Za-z0-9._-]*)
            echo "PARADA: NL_FATIA '$fatia' com caractere ou formato fora de [A-Za-z0-9._-]" >&2
            return 2
            ;;
    esac
    printf '%s\n' "$fatia"
}

# Monta a fonte do build no diretório $2, que é só desta fatia (criado aqui; o rm -rf atinge só o caminho que o
# próprio script montou com SHA hex e fatia validada, dentro de NL_TMP_BASE).
# Modo archive (padrão): git archive do SHA $1 em NL_REPO, sem nenhum arquivo fora do commit.
# Modo cópia (NL_FONTE): cópia dos mesmos caminhos de LISTA_FONTE, sem --preserve=timestamps (ver a verificação de SHA).
preparar_fonte()
{
    local sha="$1" destino="$2"
    rm -rf -- "$destino"
    mkdir -p -- "$destino"
    if [ -n "${NL_FONTE:-}" ]; then
        (cd "$NL_FONTE" && cp -a --parents --no-preserve=timestamps -- "${LISTA_FONTE[@]}" "$destino/")
    else
        extrair_do_sha "$NL_REPO" "$sha" "$destino"
    fi
}

# Trava própria do build: o subcomando "executar" a adquire aqui, antes de tocar em qualquer coisa.
# Assim, mesmo chamado direto (sem construir), ele não roda fora da trava (L-11: um trabalho pesado por vez).
# Se o chamador já detém a trava (NL_TRAVA_DETIDA=1), não pega de novo (ver pesado).
adquirir_trava()
{
    if [ "${NL_TRAVA_DETIDA:-}" = 1 ]; then
        return 0
    fi
    exec 9> "$TRAVA"
    flock 9
    # Quem adquire exporta a marca: nenhum filho readquire o mesmo arquivo.
    NL_TRAVA_DETIDA=1
    export NL_TRAVA_DETIDA
}

# Corpo do build. A linha 1 do log é o SHA (escrita antes de qualquer trava, para que um log interrompido ainda
# diga de qual commit era). O trailer NL-RC é gravado ao sair, com o status real do script.
executar()
{
    local distro="$1" preset="$2"
    local sha fatia origem sufixo fonte build_host xsd_host imagem manifesto esperado

    checar_distro "$distro"
    sha="$(resolver_sha)" || exit 2
    fatia="$(checar_fatia)" || exit 2
    origem=archive
    sufixo=""
    if [ -n "${NL_FONTE:-}" ]; then
        origem=copia
        sufixo="+copia"
    fi
    printf 'SHA: %s%s fatia=%s distro=%s preset=%s origem=%s data=%s\n' \
        "$sha" "$sufixo" "$fatia" "$distro" "$preset" "$origem" "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    trap 'echo "NL-RC=$?"' EXIT

    fonte="$NL_TMP_BASE/notalivre-src-$sha-$fatia"
    build_host="${NL_BUILD_ROOT:-$NL_TMP_BASE/notalivre-build-$distro-$fatia}"
    xsd_host="${NL_XSD_ZIP_CACHE:-$NL_TMP_BASE/notalivre-xsd-cache}"
    imagem="$(imagem_de "$distro")"

    adquirir_trava
    preparar_fonte "$sha" "$fonte"
    manifesto="$(manifesto_de_dir "$fonte")"
    printf 'NL-SRC=%s\nNL-BUILD=%s\nNL-XSD-CACHE=%s\nNL-MANIFESTO-SHA256=%s\n' \
        "$fonte" "$build_host" "$xsd_host" "$manifesto"
    if [ -n "${NL_FONTE:-}" ]; then
        if ! esperado="$(manifesto_do_sha "$NL_REPO" "$sha")"; then
            echo "PARADA: não consegui extrair $sha de $NL_REPO para conferir a cópia de NL_FONTE"
            return 4
        fi
        if [ "$esperado" != "$manifesto" ]; then
            echo "PARADA: a cópia de NL_FONTE diverge do commit $sha (manifesto $manifesto, commit $esperado): arquivo fora do commit"
            return 4
        fi
    fi
    mkdir -p "$build_host" "$xsd_host"

    # Autotestes de estreia (NL-2.11, DA-2.61; L-36): rodam no contêiner de build, sobre o mesmo SHA, antes dos portões.
    # Um trava nova só vale com autoteste que roda. O autoteste de limites (autoteste_build_limites.sh) NÃO entra aqui:
    # ele chama o próprio build_local (recursão) e usa um docker de mentira no PATH, que não existe no contêiner. Roda
    # no job autotestes do ci.yml.
    local autoteste
    for autoteste in tools/conteiner/autoteste_piso_portoes.sh flatpak/autoteste_suite_flatpak.sh tools/portoes/autoteste_pulos_testes.sh; do
        if ! docker run --rm --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --network none \
            --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -v "$fonte:/src:ro,z" -w /src "$imagem" \
            sh -c "mkdir -p \"\$HOME\" && bash $autoteste"; then
            echo "PARADA: autoteste $autoteste reprovou (L-36); build não iniciado"
            return 6
        fi
    done

    # Portões estáticos (DA-2.55, NL-2.11): os mesmos que o CI roda nos jobs higiene e camadas, sobre o mesmo SHA, antes do
    # configure (L-36). Pasta própria do SHA e da fatia; o rm -rf atinge só o caminho que este script monta.
    portoes="$NL_TMP_BASE/notalivre-portoes-$sha-$fatia"
    rm -rf -- "$portoes"
    if ! bash "$RAIZ/tools/conteiner/portoes_estaticos.sh" "$NL_REPO" "$sha" "$portoes"; then
        echo "PARADA: portões estáticos reprovaram (DA-2.55); build não iniciado"
        return 5
    fi

    # Etapa 1, como root e só de leitura do Qt: tabela de versões e pré-cheque (o índice do gerenciador exige root).
    if ! docker run --rm --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --user 0:0 -e QT_PREFIXO="$QT_PREFIXO" \
        -v "$fonte:/src:ro,z" -v "$VOLUME_QT:/opt/qt:ro" \
        -w /src "$imagem" bash tools/conteiner/executar_no_conteiner.sh diagnostico; then
        echo "PARADA: diagnóstico do contêiner reprovou (pré-cheque do Qt ou ambiente); build não iniciado"
        return 3
    fi
    # Etapa 2, como 1000:1000: configure, build e ctest. Arquivos do build ficam com o dono do host.
    # NL_XSD_ZIP_CACHE (gêmeo da INBOX): o ZIP do XSD vem do cache do host, como no CI, e não do gov.br.
    # NL-2.14 (adendo 5, item 2): o diretório git comum do repositório entra só para leitura, para o resumo de revisão
    # (tools/portoes/resumo_listas.sh) imprimir o diff das listas contra o merge-base com a onda-2. Sem git, o resumo diz que
    # o diff está indisponível; o build não para por isso.
    gitopts=()
    gitcomum="$(git -C "$NL_REPO" rev-parse --path-format=absolute --git-common-dir 2>/dev/null || true)"
    if [ -n "$gitcomum" ] && [ -d "$gitcomum" ]; then
        gitopts=(-v "$gitcomum:/gitrepo:ro,z" -e GIT_DIR=/gitrepo -e NL_SHA="$sha")
    fi
    docker run --rm --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" \
        --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -e NL_TMP_BASE=/var/tmp -e NL_DISTRO="$distro" \
        -e CMAKE_PREFIX_PATH="$QT_PREFIXO" -e CMAKE_BUILD_PARALLEL_LEVEL="$THREADS" \
        -e NL_XSD_ZIP_CACHE=/xsd-cache \
        "${gitopts[@]}" \
        -v "$fonte:/src:ro,z" -v "$build_host:/var/tmp/notalivre-build:z" -v "$VOLUME_QT:/opt/qt:ro" \
        -v "$xsd_host:/xsd-cache:ro,z" \
        -w /src "$imagem" \
        sh -c "mkdir -p \"\$HOME\" && bash tools/conteiner/executar_no_conteiner.sh $preset"
}

construir()
{
    local distro="$1" preset="$2"
    local log="${NL_LOG:-$LOG_DIR/NL-0.4-$distro-$preset.log}"
    local rc=0

    checar_distro "$distro"
    mkdir -p "$LOG_DIR"
    # A trava é adquirida dentro de executar (cópia da fonte e docker run ficam na mesma seção crítica).
    nice -n 5 "$SCRIPT" executar "$distro" "$preset" > "$log" 2>&1 || rc=$?
    echo "log: $log rc=$rc"
    return "$rc"
}

case "${1:-}" in
    imagem)
        construir_imagem "${2:?uso: build_local.sh imagem <distro>}"
        ;;
    qt)
        qt_instalar
        ;;
    preparar)
        construir_imagem fedora44
        qt_instalar
        ;;
    executar)
        executar "${2:?distro}" "${3:?preset}"
        ;;
    fedora44|ubuntu26.04|manjaro|mint22.3)
        construir "$1" "${2:?uso: build_local.sh <distro> <preset>}"
        ;;
    debug|release|asan-ubsan)
        # Forma antiga (NL-0.2): atalho para fedora44.
        construir fedora44 "$1"
        ;;
    *)
        echo "uso: $0 imagem <distro> | qt | preparar | <distro> <preset> | <preset>" >&2
        echo "distro: fedora44 ubuntu26.04 manjaro mint22.3 (todos os builds exigem NL_FATIA)" >&2
        exit 2
        ;;
esac
