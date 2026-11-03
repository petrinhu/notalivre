# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/log (NL-2.7, ADR-S4): Log com allowlist e redação no ponto de escrita.
# Motor agnóstico de domínio: não conhece Dps nem Tomador, só regras de redação por nome de campo.
add_library(nl_foundation_log STATIC log.cpp redacao.cpp)
target_include_directories(nl_foundation_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_foundation_log PUBLIC nl_foundation_base)
target_link_libraries(nl_foundation_log PRIVATE nl_warnings)
