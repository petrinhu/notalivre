# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/xml (NL-1.6a): validador XSD sobre libxml2. Acrescenta as fontes ao alvo nl_foundation (não cria alvo próprio).
# Os XSD ficam fora da árvore: o subdiretório third_party/nfse-xsd baixa, confere e extrai no build (ver lá).
find_package(LibXml2 REQUIRED)

target_sources(nl_foundation PRIVATE
    erros_xsd.cpp
    esquema.cpp
    validador_xsd.cpp
    libxml_util.cpp)

# Qt Core na interface pública (QByteArrayView, QString). libxml2 só no .cpp (nenhum tipo do libxml2 no cabeçalho público).
# libxml2 pelas variáveis do FindLibXml2, não pelo alvo importado LibXml2::LibXml2: o alvo nasce neste diretório e o
# nl_foundation (diretório pai) o linka; o AUTOMOC do pai não o enxerga ("dependency target ... does not exist", medido).
target_link_libraries(nl_foundation PUBLIC Qt6::Core)
target_include_directories(nl_foundation PRIVATE ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(nl_foundation PRIVATE ${LIBXML2_LIBRARIES} nl_warnings)

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/nfse-xsd/prodrest-20261020
                 ${CMAKE_BINARY_DIR}/third_party/nfse-xsd/prodrest-20261020)
