# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Imagem de build do NotaLivre para Ubuntu 26.04 (NL-0.4). Imagem oficial da Canonical (ubuntu:26.04),
# fixada por digest (pull de 25/10/2026; os-release: Ubuntu 26.04.1 LTS, glibc 2.43).
# Ferramentas instaladas DENTRO da imagem pelo mesmo script que o ci.yml usa (instalar/ubuntu26.04.sh).
# Nada roda na sessão do líder (L-50): tudo executa dentro de contêiner.
FROM ubuntu@sha256:c098726dd9b9733f3e885f8de0ef375206a311b0a317eadc40d347b1115fb944

COPY instalar/ubuntu26.04.sh /opt/nl/instalar.sh
RUN bash /opt/nl/instalar.sh && rm -rf /opt/nl

ENV PATH=/opt/aqt/bin:$PATH
