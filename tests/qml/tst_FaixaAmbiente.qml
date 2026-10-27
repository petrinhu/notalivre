// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Faixa de ambiente (spec visual, seção 4): aparece em Produção Restrita, some em Produção, não pode ser fechada.

import QtQuick
import QtTest
import NotaLivre.Gui

TestCase {
    id: caso
    name: "tst_FaixaAmbiente"
    visible: true
    when: windowShown

    readonly property string textoRestrita: "Produção Restrita. As notas emitidas aqui não têm validade fiscal."

    Component {
        id: faixaComponente
        FaixaAmbiente {}
    }

    function contarBotoes(item) {
        var total = 0
        for (var i = 0; i < item.children.length; ++i) {
            var filho = item.children[i]
            if (filho.checkable !== undefined)
                total += 1
            total += contarBotoes(filho)
        }
        return total
    }

    function test_visivelEmProducaoRestrita() {
        // A casca cria a faixa e só depois define o ambiente: a ordem é a mesma aqui.
        var faixa = createTemporaryObject(faixaComponente, caso)
        verify(faixa !== null, "a faixa não foi criada")
        faixa.ambiente = "producaoRestrita"
        compare(faixa.visible, true, "em Produção Restrita a faixa tem de aparecer")
        verify(faixa.height > 0, "a faixa visível precisa ocupar altura, senão o fundo some e o conteúdo se sobrepõe")
        compare(faixa.texto, textoRestrita, "texto da faixa diferente da seção 4 da spec visual")
    }

    function test_ocultaEmProducao() {
        var faixa = createTemporaryObject(faixaComponente, caso, { ambiente: "producao" })
        verify(faixa !== null, "a faixa não foi criada")
        compare(faixa.visible, false, "em Produção a faixa não deve aparecer")
    }

    function test_naoPodeSerFechada() {
        var faixa = createTemporaryObject(faixaComponente, caso, { ambiente: "producaoRestrita" })
        verify(faixa !== null, "a faixa não foi criada")
        compare(typeof faixa.fechar, "undefined", "a faixa não pode expor ação de fechar")
        compare(contarBotoes(faixa), 0, "a faixa não pode conter nenhum botão")
        compare(faixa.visible, true, "a faixa continua visível em Produção Restrita")
    }
}
