# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mid: adaptadores e composição. Pode linkar nl_back e nl_foundation; nunca apps (matriz 1.1, L-33).
add_library(nl_mid STATIC camada.cpp)
target_include_directories(nl_mid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Back e Foundation ficam PRIVATE: quem linka nl_mid (Front) não herda os includes deles (matriz 1.1).
target_link_libraries(nl_mid PUBLIC nl_foundation_base)
target_link_libraries(nl_mid PRIVATE nl_back nl_foundation nl_foundation_pkcs12 nl_foundation_log
                      nl_foundation_transporte nl_foundation_gzip nl_foundation_xmldsig)
target_link_libraries(nl_mid PRIVATE nl_warnings)

# Ganchos da NL-1.0 (costura de build): diretórios que as fatias da Onda 1 enchem.
add_subdirectory(sefin)
add_subdirectory(composicao)
add_subdirectory(cofre)
