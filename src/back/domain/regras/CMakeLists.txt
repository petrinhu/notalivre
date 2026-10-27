# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Regras locais do Back (NL-1.5): uma classe por regra E (código no nome) e os avisos do site, no alvo nl_back.
target_sources(nl_back PRIVATE
    regra_local.cpp
    regra_aviso.cpp
    validador_local.cpp
    aviso_d1.cpp
    regra_e0202.cpp
    regra_e0204.cpp
    regra_e0237.cpp
    regra_e0241.cpp
    regra_e0246.cpp
    regra_e0302.cpp
    regra_e0390.cpp
    regra_e0532.cpp
    regra_e0583.cpp
    regra_e0710.cpp
    regra_e0850.cpp
    regra_e0903.cpp
)
