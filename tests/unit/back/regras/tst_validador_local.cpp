// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ValidadorLocal (NL-1.5): aplica todas as regras e todos os avisos sem parar na primeira violação,
// só os erros bloqueiam, e o padrão contém cada regra do conjunto (enumeração fechada, L-17). TDD.

#include <algorithm>
#include <vector>

#include <QtTest>

#include <nl/back/regras/validador_local.h>

#include "catalogo_falso.h"
#include "fabrica_dps.h"

using nl::back::ValidadorLocal;

namespace {

/// Uma DPS que viola a regra do código: a chave é o código E (ou o D) que o padrão tem de devolver.
struct CasoDeViolacao
{
    const char* codigo;
    teste::Opcoes opcoes;
};

/// Enumeração fechada do conjunto de ValidadorLocal::padrao (NL-1.5, DA-1.22 retirou a E1302).
/// Regras com dependência (DA-1.23) aparecem junto com as que disparam na mesma DPS; por isso a checagem é de presença.
std::vector<CasoDeViolacao> casosDeViolacao()
{
    std::vector<CasoDeViolacao> casos;
    {
        teste::Opcoes o;
        o.cnpjTomador = o.cnpjPrestador;
        casos.push_back({"E0202", o});
    }
    {
        teste::Opcoes o;
        o.tpRetISSQN = "2";
        o.tomador = teste::TipoTomador::semTomador;
        casos.push_back({"E0204", o});
    }
    {
        teste::Opcoes o;
        o.tpRetISSQN = "2";
        o.tomador = teste::TipoTomador::cnpjSemEndereco;
        casos.push_back({"E0237", o});
    }
    {
        teste::Opcoes o;
        o.tpRetISSQN = "2";
        o.tomador = teste::TipoTomador::cpfSemEndereco;
        casos.push_back({"E0241", o});
    }
    {
        teste::Opcoes o;
        o.tomador = teste::TipoTomador::exteriorComPais;
        o.cPais = "ZZ";
        casos.push_back({"E0246", o});
    }
    {
        teste::Opcoes o;
        o.cLocPrestacao = "9999999";
        casos.push_back({"E0302", o});
    }
    {
        teste::Opcoes o;
        o.cTribNac = "120101";
        casos.push_back({"E0390", o});
    }
    {
        teste::Opcoes o;
        o.cTribNac = "990101";
        o.tribISSQN = "1";
        casos.push_back({"E0532", o});
    }
    {
        teste::Opcoes o;
        o.opSimpNac = "2";
        o.tpRetISSQN = "2";
        casos.push_back({"E0583", o});
    }
    {
        teste::Opcoes o;
        o.opSimpNac = "2";
        o.totTribSn = true;
        casos.push_back({"E0710", o});
    }
    {
        teste::Opcoes o;
        o.ibscbs = true;
        o.dCompet = QDate{2025, 12, 31};
        casos.push_back({"E0850", o});
    }
    {
        teste::Opcoes o;
        o.cTribNac = "250501";
        o.ibscbs = true;
        o.tpOper = false;
        casos.push_back({"E0903", o});
    }
    return casos;
}

std::vector<QString> codigosDoResultado(const nl::back::ResultadoValidacao& resultado)
{
    std::vector<QString> codigos;
    for (const auto& erro : resultado.erros) {
        codigos.push_back(QString::fromStdString(erro.codigo()));
    }
    return codigos;
}

bool contem(const std::vector<QString>& codigos, const char* codigo)
{
    return std::find(codigos.begin(), codigos.end(), QString::fromUtf8(codigo)) != codigos.end();
}

} // namespace

class TstValidadorLocal : public QObject
{
    Q_OBJECT

private slots:
    void dpsLimpaNaoTemErroNemAviso();
    void acumulaTodosOsErrosDaDps();
    void avisoSozinhoNaoBloqueia();
    void retencaoSemTomadorDisparaTresRegrasJuntas();
    void padraoDetectaCadaCodigoDoConjunto();
};

void TstValidadorLocal::dpsLimpaNaoTemErroNemAviso()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    const auto resultado = ValidadorLocal::padrao(catalogo).validar(*dps);
    QVERIFY(resultado.erros.empty());
    QVERIFY(resultado.avisos.empty());
    QVERIFY(resultado.podeEmitir());
}

void TstValidadorLocal::acumulaTodosOsErrosDaDps()
{
    teste::Opcoes opcoes;
    opcoes.cnpjTomador = opcoes.cnpjPrestador;
    opcoes.cTribNac = "990101";
    opcoes.tribISSQN = "1";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    const auto resultado = ValidadorLocal::padrao(catalogo).validar(*dps);
    QCOMPARE(resultado.erros.size(), std::size_t{2});
    const auto codigos = codigosDoResultado(resultado);
    QVERIFY(contem(codigos, "E0202"));
    QVERIFY(contem(codigos, "E0532"));
    QVERIFY(!resultado.podeEmitir());
}

void TstValidadorLocal::avisoSozinhoNaoBloqueia()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::cnpjSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    const auto resultado = ValidadorLocal::padrao(catalogo).validar(*dps);
    QVERIFY(resultado.erros.empty());
    QCOMPARE(resultado.avisos.size(), std::size_t{1});
    QVERIFY(resultado.podeEmitir());
}

void TstValidadorLocal::retencaoSemTomadorDisparaTresRegrasJuntas()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::semTomador;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    const auto codigos = codigosDoResultado(ValidadorLocal::padrao(catalogo).validar(*dps));
    QCOMPARE(codigos.size(), std::size_t{3});
    QVERIFY(contem(codigos, "E0204"));
    QVERIFY(contem(codigos, "E0237"));
    QVERIFY(contem(codigos, "E0241"));
}

void TstValidadorLocal::padraoDetectaCadaCodigoDoConjunto()
{
    const teste::CatalogoFalso catalogo;
    const auto validador = ValidadorLocal::padrao(catalogo);
    const auto casos = casosDeViolacao();
    QCOMPARE(casos.size(), std::size_t{12});
    for (const auto& caso : casos) {
        const auto dps = teste::dpsCom(caso.opcoes);
        QVERIFY2(dps.has_value(), caso.codigo);
        const auto codigos = codigosDoResultado(validador.validar(*dps));
        QVERIFY2(contem(codigos, caso.codigo), caso.codigo);
    }
}

QTEST_GUILESS_MAIN(TstValidadorLocal)
#include "tst_validador_local.moc"
