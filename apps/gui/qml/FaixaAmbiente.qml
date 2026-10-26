// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Faixa de ambiente (spec visual, seção 4): visível em Produção Restrita, oculta em Produção.
// Não tem ação de fechar: a faixa só some quando o ambiente muda, nunca pelo usuário.

import QtQuick

Rectangle {
    id: faixa

    property string ambiente: ""
    readonly property string texto: "Produção Restrita. As notas emitidas aqui não têm validade fiscal."

    // Estado explícito, não a visibilidade efetiva: os dois bindings leem o mesmo booleano.
    readonly property bool exibir: ambiente === "producaoRestrita"

    visible: exibir
    height: exibir ? 40 : 0
    color: Theme.faixaBg
    radius: Theme.raioFaixa

    Accessible.role: Accessible.StaticText
    Accessible.name: texto

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Rectangle {
            id: icone
            width: 18
            height: 18
            radius: 9
            color: Theme.stamp
            anchors.verticalCenter: parent.verticalCenter
            Accessible.ignored: true

            Text {
                anchors.centerIn: parent
                text: "!"
                color: Theme.sobreStamp
                font.family: Theme.fonteCorpo
                font.pixelSize: 12
                font.bold: true
            }
        }

        Text {
            text: faixa.texto
            color: Theme.faixaFg
            font.family: Theme.fonteCorpo
            font.pixelSize: Theme.tamLabel
            font.bold: true
        }
    }
}
