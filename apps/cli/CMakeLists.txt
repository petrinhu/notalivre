# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Front (CLI): só pode linkar fachadas de nl_mid (matriz 1.1, L-33).
add_executable(notalivre-cli main.cpp)
target_link_libraries(notalivre-cli PRIVATE nl_mid nl_warnings)
