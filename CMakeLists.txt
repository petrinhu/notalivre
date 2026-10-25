# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# NotaLivre: raiz do CMake por camadas (NL-0.2, plano 1.1 e L-33).
# Cada alvo declara include dirs e links por alvo (PUBLIC/PRIVATE); nada de include_directories global.
cmake_minimum_required(VERSION 3.25)

project(NotaLivre VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

# Qt 6.12 com a lista de módulos do plano (NL-0.2). Não usar API exclusiva do 6.12 ainda (compatível com 6.11).
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Test Quick Qml Network)

# Avisos como erro, aplicados por alvo através deste INTERFACE (L-36: portão só conta depois de visto vermelho).
add_library(nl_warnings INTERFACE)
target_compile_options(nl_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)

enable_testing()

add_subdirectory(src/foundation/base)
add_subdirectory(src/foundation)
add_subdirectory(src/back)
add_subdirectory(src/mid)
add_subdirectory(apps/cli)
add_subdirectory(apps/gui)
add_subdirectory(tests)
