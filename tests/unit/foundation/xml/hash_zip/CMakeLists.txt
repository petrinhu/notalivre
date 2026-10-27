# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Projeto mínimo do teste de hash do ZIP (NL-1.6a, mutação M3): só inclui third_party/nfse-xsd. Não é subdiretório do
# projeto principal: o teste_hash_zip.sh o configura com -S, uma vez por caso, sem rede (NL_XSD_URL=file://...).
cmake_minimum_required(VERSION 3.25)
project(NlTesteHashZip NONE)
add_subdirectory("${NL_FONTE}/third_party/nfse-xsd/prodrest-20261020" "${CMAKE_BINARY_DIR}/xsd")
