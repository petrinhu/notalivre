# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes de contrato do gzip da Foundation (NL-2.4, DA-2.4): cabeçalho e rodapé RFC 1952, e recusa do formato do qCompress.
add_executable(tst_gzip tst_gzip.cpp)
target_link_libraries(tst_gzip PRIVATE Qt6::Test nl_foundation_gzip nl_warnings)
# O seam interno (gzip_interno.h) é do motor: o teste o inclui pelo mesmo include PRIVATE, como tst_codec_dps faz.
target_include_directories(tst_gzip PRIVATE ${CMAKE_SOURCE_DIR}/src/foundation/gzip/privado)
nl_registra_teste_qt(NAME tst_gzip COMMAND tst_gzip)
