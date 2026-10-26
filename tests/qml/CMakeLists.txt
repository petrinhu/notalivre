# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes QML da casca (NL-1.8): Qt Quick Test, offscreen (L-50). Roda só dentro do contêiner ou do CI.
# Os casos ficam em tst_*.qml; a função de teste começa com test_ (exigência do Qt Quick Test).
find_package(Qt6 6.11 REQUIRED COMPONENTS Quick QuickTest Qml)

qt_add_executable(tst_qml_gui tst_main.cpp)
target_compile_definitions(tst_qml_gui PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    NL_FONTES_DIR="${CMAKE_SOURCE_DIR}/apps/gui/recursos/fontes")
target_link_libraries(tst_qml_gui PRIVATE
    Qt6::Quick Qt6::QuickTest Qt6::Qml notalivre_gui_qml nl_warnings)

# O módulo NotaLivre.Gui é construído em apps/gui/qml (ver apps/gui/CMakeLists.txt). O caminho é repetido aqui
# porque a ordem de add_subdirectory não expõe variáveis entre irmãos.
set(NL_QML_IMPORTACAO "${CMAKE_BINARY_DIR}/apps/gui/qml")
set(NL_CAPTURAS "${CMAKE_BINARY_DIR}/capturas")
file(MAKE_DIRECTORY "${NL_CAPTURAS}")

add_test(NAME tst_qml_gui COMMAND tst_qml_gui)
set_tests_properties(tst_qml_gui PROPERTIES ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QML_XHR_ALLOW_FILE_READ=1;QML_IMPORT_PATH=${NL_QML_IMPORTACAO};NL_CAPTURAS_DIR=${NL_CAPTURAS}")
