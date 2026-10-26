// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Barra lateral com os oito destinos (spec visual, seções 6 e 9): navegação completa por teclado
// e item ativo com marca além da cor.

import QtQuick
import QtTest
import NotaLivre.Gui

TestCase {
    id: caso
    name: "tst_Navegacao"
    visible: true
    when: windowShown

    readonly property var chavesEsperadas: [
        "emissao", "lote", "tomadores", "historico",
        "modelos", "emissores", "tabelas", "configuracoes"
    ]

    Component {
        id: cascaComponente
        Casca {
            width: 960
            height: 640
        }
    }

    function test_oitoDestinosPorTeclado() {
        var casca = createTemporaryObject(cascaComponente, caso)
        verify(casca !== null, "a casca não foi criada")
        var barra = casca.barra
        verify(barra !== null, "barra lateral ausente")
        compare(barra.count, 8, "a barra lateral tem de ter exatamente oito destinos")
        compare(FachadaSimulada.destinos.length, 8, "a fachada tem de expor oito destinos")
        compare(barra.activeFocusOnTab, true, "a barra lateral tem de receber foco pelo Tab")

        for (var i = 0; i < chavesEsperadas.length; ++i) {
            compare(FachadaSimulada.destinos[i].chave, chavesEsperadas[i], "destino " + i + " fora da ordem da spec")
        }

        barra.forceActiveFocus()
        for (var descer = 0; descer < 8; ++descer) {
            compare(barra.currentIndex, descer, "setas não percorrem os oito destinos na ordem")
            if (descer < 7)
                keyClick(Qt.Key_Down)
        }

        keyClick(Qt.Key_Return)
        compare(casca.destinoAtual, "configuracoes", "Enter não ativou o destino focado")

        keyClick(Qt.Key_Up)
        keyClick(Qt.Key_Space)
        compare(casca.destinoAtual, "tabelas", "Espaço não ativou o destino focado")
    }

    function test_itemAtivoTemMarcaAlemDaCor() {
        var casca = createTemporaryObject(cascaComponente, caso)
        verify(casca !== null, "a casca não foi criada")
        var barra = casca.barra
        verify(barra !== null, "barra lateral ausente")

        barra.forceActiveFocus()
        keyClick(Qt.Key_Return)
        var item = barra.itemAtIndex(barra.currentIndex)
        verify(item !== null, "item do destino ativo não está instanciado")
        compare(item.ativo, true, "o destino ativo não se marca como ativo")
        verify(item.marcaAtiva.visible, "o destino ativo precisa de marca visível, não só cor")
    }

    function test_botaoAtualizarTabelasNoCabecalhoEmTodaTela() {
        // Decisão do líder (spec visual, seção 6 e item 13 da seção 10): botão no cabeçalho, em toda tela.
        var casca = createTemporaryObject(cascaComponente, caso)
        verify(casca !== null, "a casca não foi criada")
        var botao = casca.botaoAtualizarCabecalho
        verify(!!botao, "botão Atualizar tabelas ausente do cabeçalho")
        compare(botao.text, "Atualizar tabelas", "texto do botão do cabeçalho")
        compare(botao.enabled, false, "o botão fica desabilitado até NL-5.7")
        verify((botao.focusPolicy & Qt.TabFocus) !== 0, "o botão precisa entrar na ordem de Tab (acessível por teclado)")

        for (var i = 0; i < FachadaSimulada.destinos.length; ++i) {
            casca.destinoAtual = FachadaSimulada.destinos[i].chave
            compare(botao.visible, true, "o botão do cabeçalho some no destino " + FachadaSimulada.destinos[i].chave)
        }
    }
}
