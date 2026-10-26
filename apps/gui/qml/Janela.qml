// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Janela principal da GUI (NL-1.8): hospeda a casca. Estilo Basic (spec visual, seção 9).

import QtQuick
import QtQuick.Controls.Basic

ApplicationWindow {
    width: 1040
    height: 680
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: "NotaLivre"
    color: Theme.bg

    Casca {
        anchors.fill: parent
    }
}
