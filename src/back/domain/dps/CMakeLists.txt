# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Id da DPS, série, número e chave de acesso mínima (NL-1.3). Fontes no alvo nl_back; cabeçalhos em src/back/include/nl/back/dps/.
target_sources(nl_back PRIVATE
    serie.cpp
    numero_dps.cpp
    id_dps.cpp
    chave_acesso.cpp
)
