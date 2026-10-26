// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada SIMULADA da casca (NL-1.8): dados fixos, sem SQL, cripto, rede ou regra fiscal.
// Será trocada pela fachada real de src/mid/composicao em NL-4.x, sem mudar a casca.

pragma Singleton
import QtQuick

QtObject {
    readonly property string ambiente: "producaoRestrita"
    readonly property string emissorAtual: "Ateliê Cacto Verde MEI"

    readonly property var destinos: [
        { grupo: "Notas", chave: "emissao", nome: "Emissão" },
        { grupo: "Notas", chave: "lote", nome: "Lote" },
        { grupo: "Notas", chave: "tomadores", nome: "Tomadores" },
        { grupo: "Notas", chave: "historico", nome: "Histórico" },
        { grupo: "Cadastros", chave: "modelos", nome: "Modelos" },
        { grupo: "Cadastros", chave: "emissores", nome: "Emissores" },
        { grupo: "Cadastros", chave: "tabelas", nome: "Tabelas" },
        { grupo: "Cadastros", chave: "configuracoes", nome: "Configurações" }
    ]

    function nomeDoDestino(chave) {
        for (var i = 0; i < destinos.length; ++i) {
            if (destinos[i].chave === chave)
                return destinos[i].nome
        }
        return ""
    }
}
