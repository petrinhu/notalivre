#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Ferramentas de build e bibliotecas de runtime do Qt para Fedora 45 (NL-0.4).
# Usado pelo Dockerfile (fedora45-build.Dockerfile) e pelo ci.yml: uma só fonte de verdade.
# Instala ferramentas pelo gerenciador de pacotes; nunca toca o host (roda dentro do contêiner).
set -euo pipefail

dnf -y install --setopt=install_weak_deps=False \
    gcc-c++ libasan libubsan cmake ninja-build pkgconf-pkg-config python3-pip \
    git-core openssl openssl-devel \
    mesa-libGL-devel mesa-libEGL-devel libxkbcommon-devel fontconfig-devel \
    freetype-devel libX11-devel dbus-libs krb5-libs \
    libxml2-devel libxml2 \
    zlib-ng-compat-devel libsodium-devel \
    xmlsec1 xmlsec1-openssl
dnf clean all

# xmlsec1 (NL-2.0, DA-2.1): oráculo de linha de comando dos testes (xmlsec1 --verify). Não entra no app nem no Flatpak.

# Venv em /opt/aqt SEM aqtinstall (NL-3.0g, decisão do CTO de 05/11/2026). O aqt não entra aqui: as dependências dele
# (py7zr, pybcj, pyppmd, inflate64) não têm wheel para o Python 3.15 do Fedora 45, e a compilação da fonte precisa de
# Python.h, que não está na lista. O Qt vem do volume e do cache do CI, populados pela imagem ubuntu26.04
# (build_local.sh qt_instalar e o job qt-cache do ci.yml).
python3 -m venv /opt/aqt
