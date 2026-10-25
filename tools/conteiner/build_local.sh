#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Build e teste local do NotaLivre dentro de contêiner (L-50: nada de teste dinâmico na sessão do líder).
#
# Uso:
#   build_local.sh imagem <distro>        constrói a imagem de build da distro (pesado, sob flock)
#   build_local.sh qt                     instala Qt 6.12 no volume notalivre-qt-6120 (só se ainda não estiver)
#   build_local.sh preparar               imagem fedora44 + Qt (forma antiga da NL-0.2)
#   build_local.sh <distro> <preset>      configure + build + ctest do preset na distro
#   build_local.sh <preset>               atalho: fedora44 <preset> (forma antiga, mantida)
#   distro: fedora44 | ubuntu26.04 | manjaro | mint22.3
#
# Variáveis de ambiente:
#   NL_TMP_BASE    base de temporários e de build no host (padrão: /var/tmp; o preset exige a variável)
#   NL_LOG         log do run (padrão: $NL_TMP_BASE/notalivre-logs/NL-0.4-<distro>-<preset>.log)
#   NL_FONTE       árvore de origem do build (padrão: a raiz do repositório)
#   NL_BUILD_ROOT  diretório de build no host (padrão: $NL_TMP_BASE/notalivre-build-<distro>)
#
# Regras aplicadas (L-11, L-45, L-49):
#   - um trabalho pesado por vez: cópia do rascunho E docker run ficam DENTRO da mesma trava flock $TRAVA;
#   - 13 threads e 20 GiB de RAM no contêiner (teto brando de 20G por workload, L-11);
#   - build e temporários em /var/tmp (TMPDIR exportado abaixo), nunca em /tmp (tmpfs);
#   - o status é o do contêiner: o log é gravado por redirecionamento, nunca por pipe em tail;
#   - cada distro tem seu diretório de build no host, montado no MESMO caminho do preset (/var/tmp/notalivre-build);
#   - o contêiner roda com --user 1000:1000 e sem --privileged.
set -euo pipefail

# Base de temporários e de build no HOST (A0-08). Padrão /var/tmp; trocável pela variável NL_TMP_BASE.
NL_TMP_BASE="${NL_TMP_BASE:-/var/tmp}"
export NL_TMP_BASE
export TMPDIR="$NL_TMP_BASE"

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="$RAIZ/tools/conteiner/build_local.sh"
VOLUME_QT="notalivre-qt-6120"
QT_PREFIXO="/opt/qt/6.12.0/gcc_64"
# SELinux Enforcing: a árvore de origem (pasta sincronizada com a nuvem) é unlabeled_t e o contêiner não a lê por bind mount.
# Por isso o contêiner recebe uma cópia de rascunho em /var/tmp (rótulo do próprio usuário, :z),
# refeita a cada build. Não se relabela a árvore sincronizada do líder.
SRC_COPIA="$NL_TMP_BASE/notalivre-src"
LOG_DIR="$NL_TMP_BASE/notalivre-logs"
TRAVA="$NL_TMP_BASE/notalivre-heavy.lock"
THREADS=13

# Distros aceitas: cada uma tem <distro>-build.Dockerfile e instalar/<distro>.sh (ver ci.yml).
checar_distro()
{
    case "$1" in
        fedora44|ubuntu26.04|manjaro|mint22.3) ;;
        *) echo "distro desconhecida: $1 (aceitas: fedora44 ubuntu26.04 manjaro mint22.3)" >&2; exit 2 ;;
    esac
}

imagem_de()
{
    echo "notalivre/build-$1:nl-0.4"
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

construir_imagem()
{
    local distro="$1"
    checar_distro "$distro"
    mkdir -p "$LOG_DIR"
    pesado docker build -t "$(imagem_de "$distro")" \
        -f "$RAIZ/tools/conteiner/$distro-build.Dockerfile" "$RAIZ/tools/conteiner" \
        > "$LOG_DIR/NL-0.4-imagem-$distro.log" 2>&1
}

# Instala Qt 6.12 no volume, uma única vez. Se o volume já tem o Qt, não reinstala (aqt reinstalaria tudo).
qt_instalar()
{
    if pesado docker run --rm --user 1000:1000 -v "$VOLUME_QT:/opt/qt:ro" "$(imagem_de fedora44)" \
        test -f "$QT_PREFIXO/lib/cmake/Qt6/Qt6Config.cmake"; then
        echo "Qt 6.12 já presente no volume $VOLUME_QT: nada a instalar"
        return 0
    fi
    docker volume create "$VOLUME_QT" > /dev/null
    # O volume nasce com dono root; ajusta uma única vez para o usuário 1000 (dono do host).
    pesado docker run --rm -u 0:0 -v "$VOLUME_QT:/opt/qt" "$(imagem_de fedora44)" chown 1000:1000 /opt/qt
    # HOME em /tmp/home (gravável pelo uid 1000 no contêiner; /var/tmp da imagem é do root e aqt falha ali).
    # aqt pode terminar com status 0 mesmo após erro: a conferência final mede o artefato no volume.
    pesado docker run --rm --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -e NL_TMP_BASE=/var/tmp \
        -v "$VOLUME_QT:/opt/qt" "$(imagem_de fedora44)" \
        sh -c 'mkdir -p "$HOME" && cd "$HOME" && aqt install-qt -O /opt/qt linux desktop 6.12.0 linux_gcc_64 -m qtnetworkauth qthttpserver qtimageformats qtshadertools qtwebsockets && test -f /opt/qt/6.12.0/gcc_64/lib/cmake/Qt6/Qt6Config.cmake'
}

# Copia só o que o build usa (CMake, presets, src, apps, tests e os scripts do contêiner) para o rascunho.
copiar_fonte()
{
    local fonte="$1"
    rm -rf -- "$SRC_COPIA"
    mkdir -p "$SRC_COPIA/tools"
    cp -a "$fonte/CMakeLists.txt" "$fonte/CMakePresets.json" "$fonte/src" "$fonte/apps" "$fonte/tests" "$SRC_COPIA/"
    cp -a "$fonte/tools/conteiner" "$SRC_COPIA/tools/"
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

# Corpo do build. Adquire a trava por conta própria e só então copia o rascunho e roda o contêiner.
executar()
{
    local distro="$1" preset="$2"
    local fonte="${NL_FONTE:-$RAIZ}"

    adquirir_trava
    local build_host="${NL_BUILD_ROOT:-$NL_TMP_BASE/notalivre-build-$distro}"

    local imagem
    checar_distro "$distro"
    imagem="$(imagem_de "$distro")"
    copiar_fonte "$fonte"
    mkdir -p "$build_host"
    # Etapa 1, como root e só de leitura do Qt: tabela de versões e pré-cheque (o índice do gerenciador exige root).
    if ! docker run --rm --user 0:0 -e QT_PREFIXO="$QT_PREFIXO" \
        -v "$SRC_COPIA:/src:ro,z" -v "$VOLUME_QT:/opt/qt:ro" \
        -w /src "$imagem" bash tools/conteiner/executar_no_conteiner.sh diagnostico; then
        echo "PARADA: diagnóstico do contêiner reprovou (pré-cheque do Qt ou ambiente); build não iniciado"
        return 3
    fi
    # Etapa 2, como 1000:1000: configure, build e ctest. Arquivos do build ficam com o dono do host.
    docker run --rm --cpus "$THREADS" --memory 20g --memory-swap 20g \
        --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -e NL_TMP_BASE=/var/tmp \
        -e CMAKE_PREFIX_PATH="$QT_PREFIXO" -e CMAKE_BUILD_PARALLEL_LEVEL="$THREADS" \
        -v "$SRC_COPIA:/src:ro,z" -v "$build_host:/var/tmp/notalivre-build:z" -v "$VOLUME_QT:/opt/qt:ro" \
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
    # A trava é adquirida dentro de executar (cópia do rascunho e docker run ficam na mesma seção crítica).
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
        echo "distro: fedora44 ubuntu26.04 manjaro mint22.3" >&2
        exit 2
        ;;
esac
