// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Captura da janela principal no tamanho padrão do app (Janela.qml, 1040x680), claro e escuro, para o
// qa-engineer conferir contra a spec visual (NL-1.8, L-13). Também confirma que as duas famílias de fonte
// foram carregadas, porque o fallback do offscreen é uma fonte padrão e não acusa erro sozinho.
// Só grava quando o CTest define NL_CAPTURAS_DIR; sem a variável o caso é pulado, nunca falha em silêncio.

import QtQuick
import QtTest
import NotaLivre.Gui

TestCase {
    id: caso
    name: "tst_CapturaJanela"
    visible: true
    when: windowShown

    property bool gravado: false
    property bool salvouOk: false

    Component {
        id: janelaComponente
        Janela {}
    }

    Component {
        id: cascaComponente
        Casca {
            // Mesma área da Janela (anchors.fill do contentItem), sem a moldura da janela do sistema.
            width: 1040
            height: 680
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

    function test_fontesCarregadas() {
        var familias = Qt.fontFamilies()
        console.info("tst_CapturaJanela: " + familias.length + " famílias de fonte visíveis")
        verify(familias.indexOf("Bricolage Grotesque") >= 0, "Bricolage Grotesque não foi carregada (addApplicationFont)")
        verify(familias.indexOf("Atkinson Hyperlegible") >= 0, "Atkinson Hyperlegible não foi carregada (addApplicationFont)")
    }

    function test_tamanhoPadraoDaJanela() {
        var janela = createTemporaryObject(janelaComponente, caso)
        verify(janela !== null, "a Janela não foi criada")
        compare(janela.width, 1040, "largura padrão da janela (Janela.qml)")
        compare(janela.height, 680, "altura padrão da janela (Janela.qml)")
        janela.visible = false
    }

    function test_capturaClaroEscuroNosDoisDestinos() {
        if (typeof capturasDir === "undefined" || capturasDir === "")
            skip("NL_CAPTURAS_DIR ausente: captura pulada fora do contêiner de teste")

        var casca = createTemporaryObject(cascaComponente, caso)
        verify(casca !== null, "a casca não foi criada")
        var destinos = ["emissao", "tabelas"]
        for (var d = 0; d < destinos.length; ++d) {
            casca.destinoAtual = destinos[d]
            waitForRendering(casca)

            Theme.escuro = false
            waitForRendering(casca)
            salvarCaptura(casca, capturasDir + "/janela-" + destinos[d] + "-claro.png")

            Theme.escuro = true
            waitForRendering(casca)
            salvarCaptura(casca, capturasDir + "/janela-" + destinos[d] + "-escuro.png")
        }
        Theme.escuro = false
    }
}
