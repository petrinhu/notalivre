# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# NotaLivre: raiz do CMake por camadas (NL-0.2, plano 1.1 e L-33).
# Cada alvo declara include dirs e links por alvo (PUBLIC/PRIVATE); nada de include_directories global.
cmake_minimum_required(VERSION 3.25)

project(NotaLivre VERSION 0.1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

# Build Flatpak (SDK KDE 6.11, sem xmlsec1): o manifesto liga esta opção. Só desliga os testes de ferramenta externa
# (tests/tools/servidor_falso); o build nativo fica com a opção desligada e com os find_program REQUIRED.
option(NL_FLATPAK_BUILD "Build dentro do sandbox Flatpak (SDK KDE 6.11): desliga os testes que pedem ferramenta fora do SDK" OFF)

# Qt 6.11 como mínimo (NL-0.5): o build Flatpak roda contra o SDK KDE 6.11, portão da Onda 9. O CI nativo usa Qt 6.12.
# API exclusiva do 6.12 quebra o build Flatpak (sabotagem no job mutacao do ci.yml).
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Test Quick Qml Network)

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
