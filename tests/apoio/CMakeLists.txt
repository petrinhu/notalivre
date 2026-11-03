# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Auxiliar único de teste (DA-2.61, L-33): leitores de tabela por distro (SHA-1 e HTTP), usados pelos testes que as leem.
# Biblioteca só de teste: não entra no app nem na camada Front/Mid/Back/Foundation (o portão de camadas não varre tests/).
add_library(nl_teste_apoio STATIC tabela_sha1_distro.cpp tabela_por_distro.cpp)
target_include_directories(nl_teste_apoio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_teste_apoio PUBLIC Qt6::Core)
