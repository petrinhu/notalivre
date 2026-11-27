#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Qt 6.12 do cache do CI no volume notalivre-qt-6120 (NL-3.1v, seção 1.3 do plano do CTO), onde o build_local.sh e o harness
# do banco o esperam (/opt/qt/6.12.0/gcc_64). O cache é o do job qt-cache; o volume fica com dono 1000, como no caso local
# (build_local.sh qt). Dois docker run com a cota do átomo tools/conteiner/cota_conteiner.sh e --pull=never, na imagem de
# build da distro, já construída: um copia como root e acerta o dono, outro confere como 1000:1000.
# Uso: volume_qt.sh DISTRO ORIGEM    (ORIGEM: a pasta do cache, com 6.12.0/gcc_64 dentro)
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
VOLUME_QT=notalivre-qt-6120
CONFIG_QT=6.12.0/gcc_64/lib/cmake/Qt6/Qt6Config.cmake
. "$RAIZ/tools/conteiner/cota_conteiner.sh"

# nl_volume_qt_copiar ORIGEM IMAGEM: cria o volume e copia ORIGEM para ele, com dono 1000:1000, sem rede.
nl_volume_qt_copiar() {
    docker volume create "$VOLUME_QT" >/dev/null || return 1
    docker run --pull=never --rm "${NL_COTAS_DOCKER[@]}" --network none --user 0:0 \
        -v "$1:/origem:ro" -v "$VOLUME_QT:/opt/qt" "$2" \
        sh -c 'cp -a /origem/. /opt/qt/ && chown -R 1000:1000 /opt/qt'
}

# nl_volume_qt_conferir IMAGEM: rc 0 só se o uid 1000 lê o Qt6Config.cmake no volume, só para leitura.
nl_volume_qt_conferir() {
    docker run --pull=never --rm "${NL_COTAS_DOCKER[@]}" --network none --user 1000:1000 \
        -v "$VOLUME_QT:/opt/qt:ro" "$1" test -r "/opt/qt/$CONFIG_QT"
}

DISTRO="${1:-}"
ORIGEM="${2:-}"
[ -n "$DISTRO" ] && [ -n "$ORIGEM" ] || { echo "PARADA: uso: volume_qt.sh DISTRO ORIGEM"; exit 2; }
[ -f "$ORIGEM/$CONFIG_QT" ] || { echo "PARADA: Qt do cache ausente em $ORIGEM ($CONFIG_QT)"; exit 1; }
nl_cotas_docker || { echo "PARADA: teto de contêiner inválido"; exit 2; }
IMAGEM="notalivre/build-$DISTRO:nl-2.0"
nl_volume_qt_copiar "$ORIGEM" "$IMAGEM" || { echo "PARADA: cópia do Qt para o volume $VOLUME_QT falhou"; exit 1; }
nl_volume_qt_conferir "$IMAGEM" || { echo "PARADA: o uid 1000 não lê o Qt no volume $VOLUME_QT"; exit 1; }
echo "NL-VOLUME-QT volume=$VOLUME_QT origem=$ORIGEM conferido=sim"
