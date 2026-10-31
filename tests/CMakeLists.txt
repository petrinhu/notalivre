# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do NL-0.2. Cada suíte entra no CTest pelo próprio CMakeLists.
add_subdirectory(sanidade)

# Auxiliar único de teste (DA-2.61): leitor da tabela de SHA-1 por distro, antes dos consumidores em unit e e2e.
add_subdirectory(apoio)

# Ganchos da NL-1.0 (costura de build): e2e, qml, testes unitários por camada e ferramentas.
add_subdirectory(unit)
add_subdirectory(fixtures/certificados)
add_subdirectory(e2e)
add_subdirectory(qml)
add_subdirectory(tools)
