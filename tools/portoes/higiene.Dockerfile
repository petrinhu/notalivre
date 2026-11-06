# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Imagem local dos portões de higiene do NotaLivre (NL-0.6).
# Base fedora:45 (lista fechada do plano, ponto L7) mais gitleaks (ponto L9), python3 e openssl.
# Os pacotes ficam DENTRO do contêiner; o host não é alterado.
FROM fedora@sha256:a7d72ba2745b100a3351087472e5ee7b2450af7cf331e7673007e3817612839c
RUN dnf -y --setopt=install_weak_deps=False install gitleaks python3 openssl \
 && dnf clean all
