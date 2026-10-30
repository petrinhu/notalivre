# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Front (CLI): só pode linkar fachadas de nl_mid (matriz 1.1, L-33). Qt6::Core entra para QFile e QTextStream, que são de Front.
# nl_foundation_log entra para o registro da CLI (ADR-0005 a e d).
add_executable(notalivre-cli
    main.cpp
    argumentos_dps_validar.cpp
    comando_dps_validar.cpp
    argumentos_emitir.cpp
    comando_emitir.cpp
    senha_terminal.cpp
    excecao_topo.cpp)
target_link_libraries(notalivre-cli PRIVATE nl_mid nl_foundation_log Qt6::Core nl_warnings)
