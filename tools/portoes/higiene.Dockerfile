# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Imagem local dos portões de higiene do NotaLivre (NL-0.6).
# Base fedora:44 (lista fechada do plano, ponto L7) mais gitleaks (ponto L9), python3 e openssl.
# Os pacotes ficam DENTRO do contêiner; o host não é alterado.
FROM fedora:44
RUN dnf -y --setopt=install_weak_deps=False install gitleaks python3 openssl \
 && dnf clean all
