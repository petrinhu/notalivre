# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Gancho da NL-1.0 (costura de build), sem fonte ainda.
# A fatia dona deste diretório acrescenta os arquivos ao alvo da camada com target_sources(nl_back PRIVATE ...); não cria alvo próprio.
# NL-1.2: Valor monetário em centavos inteiros (cabeçalho público em src/back/include/nl/back/dinheiro/valor.h).
target_sources(nl_back PRIVATE valor.cpp)
