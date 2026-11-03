# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Agrupa os testes das ferramentas de build (tools/).
add_subdirectory(tabelas)
add_subdirectory(servidor_falso)
add_subdirectory(portoes)
# Gerador das fixtures da DPS (DA-2.61): alvo sob demanda, fora do CTest e fora do build padrão.
add_subdirectory(gera_fixtures_dps)
