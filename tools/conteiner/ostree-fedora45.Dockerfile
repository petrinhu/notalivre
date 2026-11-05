# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Imagem de conferência do repositório OSTree do Flatpak (NL-3.0g). Base Fedora 45 fixada por digest, o mesmo da matriz.
# `ostree` é ferramenta de linha de comando (L-51). O pacote já vinha como dependência na imagem antiga
# flatpak-builder-fedora44:nl-0.5 (medido pelo main com rpm -q); esta receita o torna explícito.
FROM fedora@sha256:a7d72ba2745b100a3351087472e5ee7b2450af7cf331e7673007e3817612839c

RUN dnf -y --setopt=install_weak_deps=False install ostree \
 && dnf clean all
