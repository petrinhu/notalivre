# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Back: domínio e casos de uso. Só pode linkar para baixo, e só até nl_foundation_base (matriz 1.1, L-33).
add_library(nl_back STATIC camada.cpp)
target_include_directories(nl_back PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_back PUBLIC nl_foundation_base)
target_link_libraries(nl_back PRIVATE nl_warnings)

# Ganchos da NL-1.0 (costura de build): diretórios que as fatias da Onda 1 enchem.
add_subdirectory(domain)
add_subdirectory(ports)
