# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Front (GUI, QML), NL-1.8: casca em QML sobre fachada simulada. Só fala com fachadas (matriz 1.1, L-33).
# Nada de SQL, cripto, rede ou regra fiscal aqui; a fachada real entra em NL-4.x.
find_package(Qt6 6.11 REQUIRED COMPONENTS Quick QuickControls2 Qml)

# Propriedades de singleton precisam estar definidas ANTES de qt_add_qml_module (senão o qmldir sai sem "singleton").
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/FachadaSimulada.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# O módulo sai em apps/gui/qml/NotaLivre/Gui (o destino deve terminar no caminho do módulo). O executável
# aponta o motor para apps/gui/qml, por isso a importação NotaLivre.Gui resolve sem caminho de máquina.
qt_add_qml_module(notalivre_gui_qml
    URI NotaLivre.Gui
    VERSION 1.0
    SHARED
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml/NotaLivre/Gui"
    QML_FILES
        qml/Theme.qml
        qml/FachadaSimulada.qml
        qml/FaixaAmbiente.qml
        qml/BarraLateral.qml
        qml/BotaoAtualizarTabelas.qml
        qml/TelaTabelas.qml
        qml/TelaEmBreve.qml
        qml/Casca.qml
        qml/Janela.qml
)

qt_add_executable(notalivre-gui main.cpp)
qt_add_resources(notalivre-gui notalivre_fontes
    PREFIX "/fontes"
    BASE "recursos/fontes"
    FILES
        recursos/fontes/BricolageGrotesque-VF.ttf
        recursos/fontes/AtkinsonHyperlegible-Regular.ttf
        recursos/fontes/AtkinsonHyperlegible-Bold.ttf
)
target_link_libraries(notalivre-gui PRIVATE nl_mid nl_warnings notalivre_gui_qml Qt6::Quick)
