# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/gzip (NL-2.4, DA-2.4): compactação gzip RFC 1952. Motor agnóstico: depende só de base (DA-2.30).
# A implementação (zlib ou gzip próprio) entra quando o líder decidir a dependência (L-51); o contrato é o dos testes.
add_library(nl_foundation_gzip STATIC gzip.cpp)
target_include_directories(nl_foundation_gzip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_foundation_gzip PUBLIC nl_foundation_base)
target_link_libraries(nl_foundation_gzip PRIVATE nl_warnings)
