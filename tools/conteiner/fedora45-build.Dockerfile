# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Imagem de build do NotaLivre para Fedora 45 (NL-0.4). Base fixada por digest (pull local de fedora:45).
# Ferramentas instaladas DENTRO da imagem pelo mesmo script que o ci.yml usa (instalar/fedora45.sh).
# Nada roda na sessão do líder (L-50): tudo executa dentro de contêiner.
FROM fedora@sha256:a7d72ba2745b100a3351087472e5ee7b2450af7cf331e7673007e3817612839c

COPY instalar/fedora45.sh /opt/nl/instalar.sh
RUN bash /opt/nl/instalar.sh && rm -rf /opt/nl

ENV PATH=/opt/aqt/bin:$PATH
