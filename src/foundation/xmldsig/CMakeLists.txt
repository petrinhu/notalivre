# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/xmldsig (NL-2.3, DA-2.1): assinador XMLDSIG enveloped sobre libxml2 (C14N) e OpenSSL 3 (digest e RSA).
# Motor irmão do pkcs12 e não depende dele (DA-2.30): o material chega por ponteiro emprestado do chamador.
# OpenSSL só com OSSL_LIB_CTX próprio. O app nunca lê nem define a variável de SHA-1 (portão sha1_env).
find_package(OpenSSL 3.0 REQUIRED)
find_package(LibXml2 REQUIRED)

add_library(nl_foundation_xmldsig STATIC assinador_xml.cpp parse_xml_privado.cpp)
target_include_directories(nl_foundation_xmldsig PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_foundation_xmldsig PUBLIC nl_foundation_base Qt6::Core)
target_include_directories(nl_foundation_xmldsig PRIVATE ${LIBXML2_INCLUDE_DIR})
target_link_libraries(nl_foundation_xmldsig PRIVATE OpenSSL::Crypto ${LIBXML2_LIBRARY} nl_warnings)
