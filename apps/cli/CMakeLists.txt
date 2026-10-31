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
    comando_config.cpp
    configuracao_cli.cpp)
target_link_libraries(notalivre-cli PRIVATE nl_mid Qt6::Core nl_warnings)

# Instalação (NL-2.10b, emenda 1 do CTO): o bundle leva a CLI em bin, pelo cmake --install do manifesto.
install(TARGETS notalivre-cli RUNTIME DESTINATION bin)
