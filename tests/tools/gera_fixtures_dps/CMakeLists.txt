# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Gerador das fixtures da DPS (NL-1.6b, DA-2.61). Alvo sob demanda: EXCLUDE_FROM_ALL, fora do build padrão e do CTest.
# Uso: cmake --build <build> --target gera_fixtures_dps; depois <build>/tests/tools/gera_fixtures_dps/gera_fixtures_dps <pasta>.
add_executable(gera_fixtures_dps EXCLUDE_FROM_ALL gera_fixtures_dps.cpp)
target_include_directories(gera_fixtures_dps PRIVATE ${CMAKE_SOURCE_DIR}/tests/unit/mid/sefin
                           ${CMAKE_SOURCE_DIR}/src/mid/sefin/leiaute/prodrest_20261020)
target_link_libraries(gera_fixtures_dps PRIVATE Qt6::Core nl_mid nl_back nl_foundation nl_warnings)
