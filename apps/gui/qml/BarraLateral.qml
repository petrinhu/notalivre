// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Barra lateral (spec visual, seção 6): oito destinos em dois grupos, uma só lista para o teclado
// atravessar os grupos com as setas. O item ativo tem marca de 3 px e negrito, não só cor.

import QtQuick

ListView {
    id: barra

    property string atual: ""
    signal selecionado(string chave)

    width: 184
    activeFocusOnTab: true
    clip: true
    currentIndex: 0
    boundsBehavior: Flickable.StopAtBounds
    section.property: "grupo"
    section.criteria: ViewSection.FullString

    function ativar(indice) {
        if (indice < 0 || indice >= count)
            return
        barra.selecionado(model[indice].chave)
    }

    Keys.onReturnPressed: ativar(currentIndex)
    Keys.onEnterPressed: ativar(currentIndex)
    Keys.onSpacePressed: ativar(currentIndex)

    section.delegate: Item {
        required property string section
        width: barra.width
        height: 32

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            text: parent.section
            color: Theme.text2
            font.family: Theme.fonteCorpo
            font.pixelSize: Theme.tamHelp
            font.bold: true
        }
    }

    delegate: Item {
        id: linha

        required property var modelData
        required property int index

        readonly property bool ativo: modelData.chave === barra.atual
        readonly property alias marcaAtiva: marca

        width: barra.width
        height: 44

        Rectangle {
            id: fundo
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            radius: Theme.raioCampo
            color: linha.ativo ? Theme.hover : "transparent"
        }

        Rectangle {
            id: marca
            visible: linha.ativo
            width: 3
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            color: Theme.brand
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: linha.modelData.nome
            color: linha.ativo ? Theme.brand : Theme.text1
            font.family: Theme.fonteCorpo
            font.pixelSize: Theme.tamLabel
            font.bold: linha.ativo
        }

        Rectangle {
            id: anelFoco
            visible: barra.activeFocus && linha.ListView.isCurrentItem
            anchors.fill: parent
            anchors.margins: -4
            radius: Theme.raioCampo
            color: "transparent"
            border.width: 2
            border.color: Theme.focus
        }

        Accessible.role: Accessible.PageTab
        Accessible.name: linha.modelData.nome
        Accessible.checkable: true
        Accessible.checked: linha.ativo

        MouseArea {
            anchors.fill: parent
            onClicked: {
                barra.forceActiveFocus()
                barra.currentIndex = linha.index
                barra.ativar(linha.index)
            }
        }
    }
}
