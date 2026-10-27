// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// GERADO por tools/tema/gerar_theme.py a partir da spec visual, seção 3.4. Não editar à mão.
// Único lugar do app com cor em hexadecimal (portão nenhum_hex_fora_do_theme).

pragma Singleton
import QtQuick

QtObject {
    property bool escuro: false

    readonly property color bg: escuro ? "#111A17" : "#EDF2EE"
    readonly property color surface: escuro ? "#18241F" : "#FFFFFF"
    readonly property color text1: escuro ? "#E6F0EB" : "#16241E"
    readonly property color text2: escuro ? "#A7BAB1" : "#4B5E56"
    readonly property color border: escuro ? "#6E8379" : "#6F8178"
    readonly property color borderSoft: escuro ? "#25352E" : "#D3DDD6"
    readonly property color brand: escuro ? "#8FD0B7" : "#2F5D50"
    readonly property color brandHover: escuro ? "#A5DEC9" : "#26493F"
    readonly property color brandPress: escuro ? "#7BC2A8" : "#1E3B33"
    readonly property color sobreBrand: escuro ? "#0E1714" : "#FFFFFF"
    readonly property color hover: escuro ? "#1F2F28" : "#E1EAE4"
    readonly property color sel: escuro ? "#21372E" : "#D3E4DB"
    readonly property color focus: escuro ? "#8FD0B7" : "#2F5D50"
    readonly property color ok: escuro ? "#8FD0B7" : "#2F6B3A"
    readonly property color warn: escuro ? "#E8B65B" : "#7A4A00"
    readonly property color danger: escuro ? "#FF9A9E" : "#A3262B"
    readonly property color faixaBg: escuro ? "#2E2614" : "#F6E7C8"
    readonly property color faixaFg: escuro ? "#F3D08C" : "#5C3600"
    readonly property color stamp: escuro ? "#E8B65B" : "#8A4B00"
    readonly property color sobreStamp: escuro ? "#111A17" : "#FFFFFF"
    readonly property color disBg: escuro ? "#223029" : "#DDE4E0"
    readonly property color disFg: escuro ? "#6F817A" : "#5E6E67"

    readonly property string fonteTitulo: "Bricolage Grotesque"
    readonly property string fonteCorpo: "Atkinson Hyperlegible"
    readonly property int tamDisplay: 26
    readonly property int tamH2: 21
    readonly property int tamH3: 17
    readonly property int tamMarca: 22
    readonly property int tamBody: 16
    readonly property int tamLabel: 15
    readonly property int tamHelp: 14
    readonly property int tamButton: 15
    readonly property int raioCampo: 10
    readonly property int raioCard: 12
    readonly property int raioJanela: 14
    readonly property int raioFaixa: 10
    readonly property int raioPilula: 999
    readonly property int raioCheckbox: 5
    readonly property var espaco: [4, 8, 12, 16, 24, 32]
}
