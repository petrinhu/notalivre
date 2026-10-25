#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Build e teste local do NotaLivre dentro de contêiner (L-50: nada de teste dinâmico na sessão do líder).
#
# Uso:
#   build_local.sh preparar                    imagem fedora:44 + ferramentas + Qt 6.12 no volume (uma vez)
#   build_local.sh debug|release|asan-ubsan    configure + build + ctest do preset
#
# Regras aplicadas (L-11, L-45, L-49):
#   - um trabalho pesado por vez: cópia do rascunho E docker run ficam DENTRO da mesma trava flock $TRAVA;
#   - 13 threads e 20 GiB de RAM no contêiner (teto brando de 20G por workload, L-11);
#   - build e temporários em /var/tmp (TMPDIR exportado abaixo), nunca em /tmp (tmpfs);
#   - o status é o do contêiner: o log é gravado por redirecionamento, nunca por pipe em tail.
# Log: $NL_LOG, ou /var/tmp/notalivre-logs/NL-0.2-<preset>.log por padrão.
set -euo pipefail

export TMPDIR=/var/tmp

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="$RAIZ/tools/conteiner/build_local.sh"
IMAGEM="notalivre/build-fedora44:nl-0.2"
VOLUME_QT="notalivre-qt-6120"
QT_PREFIXO="/opt/qt/6.12.0/gcc_64"
BUILD_DIR="/var/tmp/notalivre-build"
# SELinux Enforcing: a árvore do IDrive é unlabeled_t e o contêiner não a lê por bind mount.
# Por isso o contêiner recebe uma cópia de rascunho em /var/tmp (rótulo do próprio usuário, :z),
# refeita a cada build. Não se relabela a árvore sincronizada do líder.
SRC_COPIA="/var/tmp/notalivre-src"
LOG_DIR="/var/tmp/notalivre-logs"
TRAVA="/var/tmp/notalivre-heavy.lock"
THREADS=13

# Executa um comando pesado sob a trava compartilhada e com prioridade reduzida.
pesado()
{
    flock "$TRAVA" nice -n 5 "$@"
}

preparar()
{
    mkdir -p "$BUILD_DIR" "$LOG_DIR"
    pesado docker build -t "$IMAGEM" \
        -f "$RAIZ/tools/conteiner/fedora44-build.Dockerfile" "$RAIZ/tools/conteiner"
    docker volume create "$VOLUME_QT" > /dev/null
    # O volume nasce com dono root; ajusta uma única vez para o usuário 1000 (dono do host).
    pesado docker run --rm -u 0:0 -v "$VOLUME_QT:/opt/qt" "$IMAGEM" chown 1000:1000 /opt/qt
    # HOME em /tmp/home (gravável pelo uid 1000 no contêiner; /var/tmp da imagem é do root e aqt falha ali).
    # aqt pode terminar com status 0 mesmo após erro: a conferência final mede o artefato no volume.
    pesado docker run --rm --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp \
        -v "$VOLUME_QT:/opt/qt" "$IMAGEM" \
        sh -c 'mkdir -p "$HOME" && cd "$HOME" && aqt install-qt -O /opt/qt linux desktop 6.12.0 linux_gcc_64 -m qtnetworkauth qthttpserver qtimageformats qtshadertools qtwebsockets && test -f /opt/qt/6.12.0/gcc_64/lib/cmake/Qt6/Qt6Config.cmake'
}

# Copia só as fontes do build (CMake, presets, src, apps, tests) para o rascunho; nada mais entra no contêiner.
copiar_fonte()
{
    rm -rf -- "$SRC_COPIA"
    mkdir -p "$SRC_COPIA"
    cp -a "$RAIZ/CMakeLists.txt" "$RAIZ/CMakePresets.json" "$RAIZ/src" "$RAIZ/apps" "$RAIZ/tests" "$SRC_COPIA/"
}

# Corpo do build. Só roda DENTRO da trava (subcomando interno "executar", chamado por construir).
executar()
{
    local preset="$1"

    copiar_fonte
    docker run --rm --cpus "$THREADS" --memory 20g --memory-swap 20g \
        --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp \
        -e CMAKE_PREFIX_PATH="$QT_PREFIXO" -e CMAKE_BUILD_PARALLEL_LEVEL="$THREADS" \
        -v "$SRC_COPIA:/src:ro,z" -v "$BUILD_DIR:$BUILD_DIR:z" -v "$VOLUME_QT:/opt/qt:ro" \
        -w /src "$IMAGEM" \
        sh -c "mkdir -p \"\$HOME\" && cmake --preset $preset && cmake --build --preset $preset && ctest --preset $preset"
}

construir()
{
    local preset="$1"
    local log="${NL_LOG:-$LOG_DIR/NL-0.2-$preset.log}"
    local rc=0

    mkdir -p "$BUILD_DIR" "$LOG_DIR"
    # Cópia do rascunho e docker run na MESMA seção crítica: nenhum outro build mexe em $SRC_COPIA no meio.
    flock "$TRAVA" nice -n 5 "$SCRIPT" executar "$preset" > "$log" 2>&1 || rc=$?
    echo "log: $log rc=$rc"
    return "$rc"
}

case "${1:-}" in
    preparar)
        preparar
        ;;
    executar)
        executar "$2"
        ;;
    debug|release|asan-ubsan)
        construir "$1"
        ;;
    *)
        echo "uso: $0 preparar|debug|release|asan-ubsan" >&2
        exit 2
        ;;
esac
