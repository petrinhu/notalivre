# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Ports da Back (NL-1.3): só a interface RepositorioNumeracao; cabeçalho em src/back/include/nl/back/ports/.
target_sources(nl_back PRIVATE
    repositorio_numeracao.cpp
)
