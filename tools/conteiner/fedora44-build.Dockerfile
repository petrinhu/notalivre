# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Imagem de build do NotaLivre para Fedora 44 (NL-0.4). Base fixada por digest (pull local de fedora:44).
# Ferramentas instaladas DENTRO da imagem pelo mesmo script que o ci.yml usa (instalar/fedora44.sh).
# Nada roda na sessão do líder (L-50): tudo executa dentro de contêiner.
FROM fedora@sha256:43b29f65a41eb9c35e1cd5323e3bdf3b655c2357a9f4f1ff2f9c2798e5045d80

COPY instalar/fedora44.sh /opt/nl/instalar.sh
RUN bash /opt/nl/instalar.sh && rm -rf /opt/nl

ENV PATH=/opt/aqt/bin:$PATH
