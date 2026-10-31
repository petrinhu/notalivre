# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do NL-0.2. Cada suíte entra no CTest pelo próprio CMakeLists.
# Registro dos Qt Test (NL-2.14, DA-2.61 adendo 3): junitxml por teste, pela função de registro única.
# A pasta junit sai e volta na configuração: um xml de execução antiga não mascara um teste que deixou de gravar, e a
# pasta tem de existir para o -o do QTest (sem ela, todo Qt Test falha ao abrir o arquivo).
file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/junit")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/junit")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/nl_registra_teste.cmake)
add_subdirectory(sanidade)

# Auxiliar único de teste (DA-2.61): leitor da tabela de SHA-1 por distro, antes dos consumidores em unit e e2e.
add_subdirectory(apoio)

# Ganchos da NL-1.0 (costura de build): e2e, qml, testes unitários por camada e ferramentas.
add_subdirectory(unit)
add_subdirectory(fixtures/certificados)
add_subdirectory(e2e)
add_subdirectory(qml)
add_subdirectory(tools)
