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
    zlib xmlsec
pacman -Scc --noconfirm

# xmlsec (pacote do Arch/Manjaro) traz o binário xmlsec1, oráculo de linha de comando dos testes (NL-2.0, DA-2.1).
# Não entra no app nem no Flatpak.

# aqtinstall num venv próprio, fora do Python do sistema.
python -m venv /opt/aqt
/opt/aqt/bin/pip install --no-cache-dir aqtinstall
