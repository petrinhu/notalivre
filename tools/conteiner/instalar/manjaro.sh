#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Ferramentas de build e bibliotecas de runtime do Qt para Manjaro (imagem manjarolinux/base) (NL-0.4).
# Usado pelo Dockerfile (manjaro-build.Dockerfile) e pelo ci.yml: uma só fonte de verdade.
set -euo pipefail

pacman-key --init
pacman-key --populate archlinux manjaro
pacman -Syu --noconfirm --needed \
    gcc cmake ninja pkgconf git python python-pip openssl \
    mesa libglvnd libxkbcommon libx11 fontconfig freetype2 dbus glib2 krb5 brotli libxml2 \
    zlib libsodium xmlsec mariadb-libs curl
pacman -Scc --noconfirm

# Cliente MariaDB (NL-3.1h, passo 6.3 e v3.18): o mariadb-libs da distro fica (como o libmariadb-dev no ubuntu e no mint), e o
# módulo compila o Connector/C 3.4.11 por cima, em /usr/local (decisão do líder de 19/11/2026, "3.4.11 também no Manjaro (Recomendado)").
MODULO=/opt/nl/modulo_mariadb.sh
[ -f "$MODULO" ] || MODULO="$(dirname "$0")/../modulo_mariadb.sh"
bash "$MODULO"

# xmlsec (pacote do Arch/Manjaro) traz o binário xmlsec1, oráculo de linha de comando dos testes (NL-2.0, DA-2.1).
# Não entra no app nem no Flatpak.

# aqtinstall num venv próprio, fora do Python do sistema.
python -m venv /opt/aqt
/opt/aqt/bin/pip install --no-cache-dir aqtinstall
