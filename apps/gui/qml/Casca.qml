// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Casca da janela (NL-1.8, spec visual seções 4 e 6): cabeçalho de 44 px, faixa de ambiente fixa,
// barra lateral de 184 px e a área de conteúdo do destino ativo.

import QtQuick

Rectangle {
    id: casca

    property string destinoAtual: "emissao"
    property alias barra: barraLateral
    property alias botaoAtualizarCabecalho: botaoAtualizar
    readonly property string destinoNome: FachadaSimulada.nomeDoDestino(destinoAtual)

    color: Theme.bg

    Rectangle {
        id: cabecalho
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 44
        color: Theme.surface

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.borderSoft
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "NotaLivre"
            color: Theme.brand
            font.family: Theme.fonteTitulo
            font.pixelSize: Theme.tamMarca
            font.bold: true
        }

        BotaoAtualizarTabelas {
            id: botaoAtualizar
            anchors.right: emissor.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: emissor
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "Emissor: " + FachadaSimulada.emissorAtual
            color: Theme.text2
            font.family: Theme.fonteCorpo
            font.pixelSize: Theme.tamLabel
        }
    }

    FaixaAmbiente {
        id: faixa
        anchors.top: cabecalho.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 0
        ambiente: FachadaSimulada.ambiente
    }

    // Fundo e divisória da barra lateral (mockup: surface com borda direita de 1 px em border-soft).
    Rectangle {
        anchors.top: faixa.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: barraLateral.width
        color: Theme.surface

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.borderSoft
        }
    }

    BarraLateral {
        id: barraLateral
        anchors.top: faixa.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        model: FachadaSimulada.destinos
        atual: casca.destinoAtual
        onSelecionado: function (chave) { casca.destinoAtual = chave }
    }

    Loader {
        anchors.top: faixa.bottom
        anchors.bottom: parent.bottom
        anchors.left: barraLateral.right
        anchors.right: parent.right
        anchors.margins: 24
        sourceComponent: casca.destinoAtual === "tabelas" ? compTabelas : compEmBreve
    }

    Component {
        id: compTabelas
        TelaTabelas {}
    }

    Component {
        id: compEmBreve
        TelaEmBreve { titulo: casca.destinoNome }
    }
}
