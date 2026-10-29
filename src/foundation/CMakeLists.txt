# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation: motores agnósticos de domínio. Só pode linkar para baixo (nl_foundation_base).
add_library(nl_foundation STATIC camada.cpp)
target_include_directories(nl_foundation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_foundation PUBLIC nl_foundation_base)
target_link_libraries(nl_foundation PRIVATE nl_warnings)

# Ganchos da NL-1.0 (costura de build): diretórios que as fatias da Onda 1 enchem.
add_subdirectory(xml)
add_subdirectory(log)
add_subdirectory(pkcs12)
add_subdirectory(gzip)
add_subdirectory(xmldsig)
