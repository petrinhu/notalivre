# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/base: primitivas sem dependência interna (plano 1.1). Não linka nenhum outro alvo do projeto.
add_library(nl_foundation_base STATIC camada.cpp)
target_include_directories(nl_foundation_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_foundation_base PRIVATE nl_warnings)
