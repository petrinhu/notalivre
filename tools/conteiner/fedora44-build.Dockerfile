# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Imagem de build local do NotaLivre (NL-0.2). Base fixa: fedora:44, já presente no Docker local.
# Ferramentas de build instaladas DENTRO da imagem (lista autorizada: "ferramentas de build dentro das imagens").
# Nada roda na sessão do líder (L-50): tudo executa dentro de contêiner.
FROM fedora:44

RUN dnf -y install --setopt=install_weak_deps=False \
        gcc-c++ libasan libubsan cmake ninja-build pkgconf-pkg-config python3-pip \
        mesa-libGL-devel libxkbcommon-devel fontconfig-devel \
    && dnf clean all \
    && python3 -m venv /opt/aqt \
    && /opt/aqt/bin/pip install --no-cache-dir aqtinstall

ENV PATH=/opt/aqt/bin:$PATH
