# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/gzip (NL-2.4, DA-2.4): compactação gzip RFC 1952 sobre a zlib do sistema, autorizada pelo líder em 29/10/2026.
# Motor agnóstico: depende de base (DA-2.30) e da zlib. A zlib fica privada ao .cpp (nenhum tipo dela no cabeçalho).
find_package(ZLIB REQUIRED)
add_library(nl_foundation_gzip STATIC gzip.cpp)
target_include_directories(nl_foundation_gzip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Costura interna (gzip_interno.h) fica num include PRIVATE: só o próprio motor a enxerga (revisão O6 da NL-2.4).
target_include_directories(nl_foundation_gzip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/privado)
target_link_libraries(nl_foundation_gzip PUBLIC nl_foundation_base)
target_link_libraries(nl_foundation_gzip PRIVATE ZLIB::ZLIB nl_warnings)
