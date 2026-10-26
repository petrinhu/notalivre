// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Placeholder de destino ainda sem tela nesta fatia (as telas entram nas fatias de cada destino).

import QtQuick

Item {
    id: tela

    property string titulo: ""

    Column {
        width: parent.width
        spacing: 12

        Text {
            text: tela.titulo
            color: Theme.text1
            font.family: Theme.fonteTitulo
            font.pixelSize: Theme.tamH2
            font.bold: true
        }

        Text {
            text: "Esta tela ainda não está disponível nesta versão."
            color: Theme.text2
            font.family: Theme.fonteCorpo
            font.pixelSize: Theme.tamBody
        }
    }
}
