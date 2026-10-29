#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Ferramentas de build e bibliotecas de runtime do Qt para Ubuntu 26.04 (NL-0.4).
# Usado pelo Dockerfile (ubuntu26.04-build.Dockerfile) e pelo ci.yml: uma só fonte de verdade.
# Instala ferramentas pelo gerenciador de pacotes; nunca toca o host (roda dentro do contêiner).
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y --no-install-recommends \
    g++ cmake ninja-build pkg-config git ca-certificates \
    python3 python3-venv python3-pip openssl libssl-dev \
    libgl-dev libegl-dev libxkbcommon-dev libfontconfig-dev libfreetype-dev \
    libx11-dev libglib2.0-dev libdbus-1-dev libgssapi-krb5-2 libxml2-dev libxml2-utils \
    xmlsec1
apt-get clean
rm -rf /var/lib/apt/lists/*

# aqtinstall num venv próprio (PEP 668: o Python do sistema não aceita pip global).
python3 -m venv /opt/aqt
/opt/aqt/bin/pip install --no-cache-dir aqtinstall
