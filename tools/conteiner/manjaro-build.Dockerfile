# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Imagem de build do NotaLivre para Manjaro (NL-0.4). ATENÇÃO: imagem de TERCEIROS (manjarolinux/base, Docker Hub),
# não é imagem oficial do Arch nem da Canonical. Fixada por digest (pull de 25/10/2026; os-release: Manjaro Linux, glibc 2.43).
# Ferramentas instaladas DENTRO da imagem pelo mesmo script que o ci.yml usa (instalar/manjaro.sh).
# Nada roda na sessão do líder (L-50): tudo executa dentro de contêiner.
FROM manjarolinux/base@sha256:bbf1f1d746f28e138eea610e140d2f28cbb5b7c5da2fbff034b883527aa604e9

COPY instalar/manjaro.sh /opt/nl/instalar.sh
RUN bash /opt/nl/instalar.sh && rm -rf /opt/nl

ENV PATH=/opt/aqt/bin:$PATH
