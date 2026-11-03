# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/transporte (NL-2.5): HTTPS com mTLS sobre QtNetwork (terceiro, DA-2.30). Não depende do motor pkcs12.
add_library(nl_foundation_transporte STATIC transporte_https.cpp classificacao_erro.cpp)
target_include_directories(nl_foundation_transporte PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_foundation_transporte PUBLIC nl_foundation_base Qt6::Core Qt6::Network)
target_link_libraries(nl_foundation_transporte PRIVATE nl_warnings)
