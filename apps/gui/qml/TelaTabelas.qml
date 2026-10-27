// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tela Tabelas (spec visual, seção 7.7). Área de conteúdo com o botão desabilitado e o motivo escrito ao lado
// (seção 3.3). O mesmo botão também fica no cabeçalho, em toda tela (decisão do líder, spec seção 6).

import QtQuick

Item {
    Column {
        width: parent.width
        spacing: 16

        Text {
            text: "Tabelas fiscais"
            color: Theme.text1
            font.family: Theme.fonteTitulo
            font.pixelSize: Theme.tamH2
            font.bold: true
        }

        Row {
            spacing: 12

            BotaoAtualizarTabelas {
                id: atualizar
            }

            Text {
                anchors.verticalCenter: atualizar.verticalCenter
                text: "Disponível quando a atualização das tabelas fiscais estiver pronta."
                color: Theme.text2
                font.family: Theme.fonteCorpo
                font.pixelSize: Theme.tamHelp
            }
        }
    }
}
