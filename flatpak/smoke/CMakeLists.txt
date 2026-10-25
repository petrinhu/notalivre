# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Harness de medição da NL-0.5 (não é produto): exercita o Qt 6.11 do runtime com QML, sem janela.
cmake_minimum_required(VERSION 3.25)
project(NotaLivreQmlSmoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Qml Quick)
add_executable(notalivre-qml-smoke main.cpp)
target_link_libraries(notalivre-qml-smoke PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick)
install(TARGETS notalivre-qml-smoke RUNTIME DESTINATION bin)
