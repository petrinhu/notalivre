# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Front (GUI, QML): só pode linkar fachadas de nl_mid (matriz 1.1, L-33). Sem janela nesta fatia (L-50).
add_executable(notalivre-gui main.cpp)
target_link_libraries(notalivre-gui PRIVATE nl_mid nl_warnings)
