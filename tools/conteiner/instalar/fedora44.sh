#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Ferramentas de build e bibliotecas de runtime do Qt para Fedora 44 (NL-0.4).
# Usado pelo Dockerfile (fedora44-build.Dockerfile) e pelo ci.yml: uma só fonte de verdade.
# Instala ferramentas pelo gerenciador de pacotes; nunca toca o host (roda dentro do contêiner).
set -euo pipefail

dnf -y install --setopt=install_weak_deps=False \
    gcc-c++ libasan libubsan cmake ninja-build pkgconf-pkg-config python3-pip \
    git-core openssl openssl-devel \
    mesa-libGL-devel mesa-libEGL-devel libxkbcommon-devel fontconfig-devel \
    freetype-devel libX11-devel dbus-libs krb5-libs \
    libxml2-devel libxml2 \
    xmlsec1 xmlsec1-openssl
dnf clean all

# xmlsec1 (NL-2.0, DA-2.1): oráculo de linha de comando dos testes (xmlsec1 --verify). Não entra no app nem no Flatpak.

# aqtinstall num venv próprio: só é usado para instalar Qt no volume (build_local.sh qt) e no cache do CI.
python3 -m venv /opt/aqt
/opt/aqt/bin/pip install --no-cache-dir aqtinstall
