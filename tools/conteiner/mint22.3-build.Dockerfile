# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Imagem de build do NotaLivre para Linux Mint 22.3 (NL-0.4). ATENÇÃO: imagem de TERCEIROS (linuxmintd, Docker Hub),
# fixada por digest (pull de 25/10/2026). DESVIO MEDIDO (fatos, sem conclusão; a decisão é do líder):
#   1. os-release diz Ubuntu 24.04.5 LTS (glibc 2.39); /etc/linuxmint não existe; o pacote mint-info não está instalado;
#   2. as fontes apt incluem o repositório da Mint (packages.linuxmint.com, suite zena, que é a de Mint 22.3);
#   3. há pacotes da Mint instalados (linuxmint-keyring 2022.06.21, mint-dev-tools 1.5.4).
# A base de pacotes é Ubuntu noble, com GCC 13.3. Registrado no ci_medicao.md.
# Ferramentas instaladas DENTRO da imagem pelo mesmo script que o ci.yml usa (instalar/mint22.3.sh).
# Nada roda na sessão do líder (L-50): tudo executa dentro de contêiner.
FROM linuxmintd/mint22.3-amd64@sha256:304bbf7ec2a61252962c5210467d231a141661490e8962f52fd4df7688370369

COPY instalar/mint22.3.sh /opt/nl/instalar.sh
RUN bash /opt/nl/instalar.sh && rm -rf /opt/nl

ENV PATH=/opt/aqt/bin:$PATH
