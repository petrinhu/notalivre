// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Botão "Atualizar tabelas" (spec visual, seção 6 e seção 7.7). Único controle para o cabeçalho e para a tela
// Tabelas. Desabilitado até NL-5.7; o motivo fica escrito ao lado na tela Tabelas e na descrição de acessibilidade.

import QtQuick
import QtQuick.Controls.Basic

Button {
    id: botao

    text: "Atualizar tabelas"
    enabled: false
    height: 40
    leftPadding: 18
    rightPadding: 18
    font.family: Theme.fonteCorpo
    font.pixelSize: Theme.tamButton
    font.bold: true

    Accessible.description: "Disponível quando a atualização das tabelas fiscais estiver pronta."

    background: Rectangle {
        radius: Theme.raioPilula
        color: botao.enabled ? Theme.brand : Theme.disBg
    }

    contentItem: Text {
        text: botao.text
        font: botao.font
        color: botao.enabled ? Theme.sobreBrand : Theme.disFg
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
