# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Prova que o executor de testes (CTest + QtTest) está ligado (plano NL-0.2).
add_executable(tst_sanidade tst_sanidade.cpp)
target_link_libraries(tst_sanidade PRIVATE Qt6::Test nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME tst_sanidade COMMAND tst_sanidade)

# Canário do sanitizer (NL-3.0h): executável solto, sem add_test, de propósito. Não entra na contagem de testes nem nas listas
# fechadas, e é o portão tools/portoes/sanitizer_ativo.py quem o roda. Fora do Flatpak: binário com defeito de propósito não
# tem o que fazer naquele build. O registro não depende de -fsanitize, senão sumiria junto com o sanitizer.
if(NOT NL_FLATPAK_BUILD)
    find_package(Threads REQUIRED)
    add_executable(nl_canario_sanitizer canario_sanitizer.cpp)
    set_target_properties(nl_canario_sanitizer PROPERTIES AUTOMOC OFF)
    target_link_libraries(nl_canario_sanitizer PRIVATE Threads::Threads nl_warnings)
endif()
