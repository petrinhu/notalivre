// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tokens do tema (spec visual, seção 3.4 e seção 5). Os valores de referência são lidos do arquivo
// dados/tokens.json (cópia verbatim do bloco JSON da spec), e os pares de contraste de dados/pares_contraste.json.

import QtQuick
import QtTest
import NotaLivre.Gui

TestCase {
    id: caso
    name: "tst_Tema"

    property var tokens: null
    property var paresContraste: null

    Rectangle {
        id: sonda
        width: 1
        height: 1
        color: Theme.bg
    }

    function carregarJson(relativo, aoCarregar) {
        var xhr = new XMLHttpRequest()
        xhr.onreadystatechange = function () {
            if (xhr.readyState === XMLHttpRequest.DONE && xhr.responseText.length > 0)
                aoCarregar(xhr.responseText)
        }
        xhr.open("GET", Qt.resolvedUrl(relativo))
        xhr.send()
    }

    function tokensCarregados() {
        if (tokens === null) {
            carregarJson("dados/tokens.json", function (texto) { tokens = JSON.parse(texto) })
            tryVerify(function () { return tokens !== null }, 5000, "dados/tokens.json não carregou")
        }
        return tokens
    }

    function paresCarregados() {
        if (paresContraste === null) {
            carregarJson("dados/pares_contraste.json", function (texto) { paresContraste = JSON.parse(texto) })
            tryVerify(function () { return paresContraste !== null }, 5000, "dados/pares_contraste.json não carregou")
        }
        return paresContraste
    }

    function capitalizar(texto) {
        return texto.charAt(0).toUpperCase() + texto.slice(1)
    }

    // Mesmo mapeamento de tools/tema/gerar_theme.py: on<Maiúscula> vira sobre<Maiúscula> (QML lê on* como handler).
    function nomeQmlDaCor(nome) {
        if (nome.length > 2 && nome.indexOf("on") === 0 && nome.charAt(2) === nome.charAt(2).toUpperCase()
                && nome.charAt(2) !== nome.charAt(2).toLowerCase())
            return "sobre" + nome.slice(2)
        return nome
    }

    function luminancia(cor) {
        function canal(v) {
            return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4)
        }
        return 0.2126 * canal(cor.r) + 0.7152 * canal(cor.g) + 0.0722 * canal(cor.b)
    }

    function razaoDeContraste(corA, corB) {
        var la = luminancia(corA)
        var lb = luminancia(corB)
        return (Math.max(la, lb) + 0.05) / (Math.min(la, lb) + 0.05)
    }

    function test_tokensVemDoArquivo() {
        var t = tokensCarregados()
        var escuroOriginal = Theme.escuro
        var modos = [
            { escuro: false, bloco: t.claro },
            { escuro: true, bloco: t.escuro }
        ]
        for (var m = 0; m < modos.length; ++m) {
            Theme.escuro = modos[m].escuro
            var chaves = Object.keys(modos[m].bloco)
            verify(chaves.length > 0, "bloco de cores vazio no arquivo de tokens")
            for (var i = 0; i < chaves.length; ++i) {
                var chave = chaves[i]
                var nomeQml = nomeQmlDaCor(chave)
                verify(Theme[nomeQml] !== undefined, "token " + chave + " (" + nomeQml + ") ausente no Theme")
                verify(Qt.colorEqual(Theme[nomeQml], modos[m].bloco[chave]),
                       "cor " + chave + " (escuro=" + modos[m].escuro + ") diverge do arquivo de tokens")
            }
        }
        Theme.escuro = escuroOriginal

        compare(Theme.fonteTitulo, t.fontes.titulo, "fonte de título diverge do arquivo")
        compare(Theme.fonteCorpo, t.fontes.corpo, "fonte de corpo diverge do arquivo")
        var escalas = Object.keys(t.escala)
        for (var e = 0; e < escalas.length; ++e)
            compare(Theme["tam" + capitalizar(escalas[e])], t.escala[escalas[e]],
                    "escala " + escalas[e] + " diverge do arquivo")
        var raios = Object.keys(t.raio)
        for (var r = 0; r < raios.length; ++r)
            compare(Theme["raio" + capitalizar(raios[r])], t.raio[raios[r]],
                    "raio " + raios[r] + " diverge do arquivo")
        compare(JSON.stringify(Theme.espaco), JSON.stringify(t.espaco), "escala de espaçamento diverge")
    }

    function test_trocaClaroEscuroSemReinicio() {
        var t = tokensCarregados()
        var escuroOriginal = Theme.escuro

        Theme.escuro = false
        tryVerify(function () { return Qt.colorEqual(sonda.color, t.claro.bg) },
                  1000, "fundo do tema claro não aplicado")

        Theme.escuro = true
        tryVerify(function () { return Qt.colorEqual(sonda.color, t.escuro.bg) },
                  1000, "fundo do tema escuro não aplicado sem reiniciar")

        Theme.escuro = false
        tryVerify(function () { return Qt.colorEqual(sonda.color, t.claro.bg) },
                  1000, "volta ao tema claro não aplicada sem reiniciar")

        Theme.escuro = escuroOriginal
    }

    function test_contrasteDos50Pares() {
        tokensCarregados()
        var dados = paresCarregados()
        var escuroOriginal = Theme.escuro
        compare(dados.pares.length, 50, "a spec pede 50 pares medidos (seção 5)")

        var reprovas = []
        for (var i = 0; i < dados.pares.length; ++i) {
            var par = dados.pares[i]
            Theme.escuro = (par.tema === "escuro")
            var texto = Theme[nomeQmlDaCor(par.texto)]
            var fundo = Theme[nomeQmlDaCor(par.fundo)]
            verify(texto !== undefined && fundo !== undefined,
                   "par sem cor no tema: " + par.tema + " " + par.texto + " / " + par.fundo)
            var razao = razaoDeContraste(texto, fundo)
            if (!(razao >= par.minimo))
                reprovas.push(par.tema + " " + par.texto + "/" + par.fundo + " = " + razao.toFixed(2))
            if (par.publicada !== undefined) {
                var arredondada = Math.round(razao * 100) / 100
                compare(arredondada, par.publicada,
                        "razão publicada diverge da fórmula do WCAG: " + par.tema + " " + par.texto + "/" + par.fundo)
            }
        }
        Theme.escuro = escuroOriginal
        compare(reprovas.length, 0, "pares reprovados: " + reprovas.join("; "))
    }
}
