// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Captura offscreen da casca nos dois temas, para o qa-engineer conferir contra a spec visual (L-13).
// Só grava quando o CTest define NL_CAPTURAS_DIR; sem a variável o caso é pulado, nunca falha em silêncio.

import QtQuick
import QtTest
import NotaLivre.Gui

TestCase {
    id: caso
    name: "tst_Captura"
    visible: true
    when: windowShown

    property bool gravado: false
    property bool salvouOk: false

    Component {
        id: cascaComponente
        Casca {
            width: 1000
            height: 640
        }
    }

    function salvarCaptura(item, caminho) {
        gravado = false
        salvouOk = false
        var iniciou = item.grabToImage(function (resultado) {
            caso.salvouOk = resultado.saveToFile(caminho)
            caso.gravado = true
        })
        verify(iniciou, "grabToImage não iniciou para " + caminho)
        tryVerify(function () { return caso.gravado }, 5000, "captura não concluiu: " + caminho)
        verify(salvouOk, "falha ao gravar " + caminho)
    }

    function test_capturaClaroEscuro() {
        if (typeof capturasDir === "undefined" || capturasDir === "")
            skip("NL_CAPTURAS_DIR ausente: captura pulada fora do contêiner de teste")

        var casca = createTemporaryObject(cascaComponente, caso)
        verify(casca !== null, "a casca não foi criada")
        casca.destinoAtual = "tabelas"
        waitForRendering(casca)

        Theme.escuro = false
        waitForRendering(casca)
        salvarCaptura(casca, capturasDir + "/casca-claro.png")

        Theme.escuro = true
        waitForRendering(casca)
        salvarCaptura(casca, capturasDir + "/casca-escuro.png")

        Theme.escuro = false
    }
}
