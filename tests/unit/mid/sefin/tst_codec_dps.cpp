// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do codec XML da DPS (NL-1.6b): cada caso serializa uma Dps montada pelos tipos do Back e valida no XSD de
// Produção Restrita (ValidadorXsd). O oráculo xmllint roda sobre as mesmas fixtures no CTest (oraculo_xmllint_dps).

#include "casos_dps.h"

#include <nl/foundation/xml/erros_xsd.h>
#include <nl/foundation/xml/esquema.h>
#include <nl/foundation/xml/validador_xsd.h>
#include <nl/mid/sefin/leiaute/prodrest_20261020/codec_dps.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QtTest>

#include <optional>
#include <string_view>

#include "limites_leiaute.h"

namespace {

namespace prod = nl::mid::sefin::prodrest_20261020;
using nl::foundation::xml::ErrosXsd;
using nl::foundation::xml::Esquema;
using nl::foundation::xml::ValidadorXsd;

const QString kNamespaceNfse = QStringLiteral("http://www.sped.fazenda.gov.br/nfse");
const QString kFixturesDps = QStringLiteral(NL_FIXTURES_DPS);

/// Esquema de Produção Restrita, aberto uma única vez (o SHA-256 de cada arquivo é conferido na abertura).
const std::expected<Esquema, ErrosXsd>& esquemaProdRestrita()
{
    static const auto esquema = Esquema::abrir(QStringLiteral(NL_XSD_PRODREST_DIR), QStringLiteral(NL_XSD_PRODREST_MANIFESTO),
                                               QStringLiteral(NL_XSD_PRODREST_RAIZ));
    return esquema;
}

/// Mensagens de todos os erros concatenadas, para a descrição da falha.
QString mensagensDe(const ErrosXsd& erros)
{
    QStringList partes;
    for (const auto& erro : erros.erros()) {
        partes << QString::fromStdString(erro.mensagem());
    }
    return partes.join(QStringLiteral(" | "));
}

/// Veredito do ValidadorXsd sobre um XML. 'motivoInvalido' separa "o XML não confere" de "o esquema nem abriu".
struct Veredito
{
    bool valido = false;
    bool motivoInvalido = false;
    QString detalhe;
};

Veredito validar(const QByteArray& xml)
{
    const auto& esquema = esquemaProdRestrita();
    if (!esquema) {
        return Veredito{false, false, QStringLiteral("esquema não abriu: ") + mensagensDe(esquema.error())};
    }
    const auto resultado = ValidadorXsd::validar(xml, *esquema);
    if (resultado) {
        return Veredito{true, false, {}};
    }
    return Veredito{false, resultado.error().motivo() == ErrosXsd::Motivo::invalido, mensagensDe(resultado.error())};
}

QString comoQString(std::string_view texto)
{
    return QString::fromUtf8(texto.data(), static_cast<qsizetype>(texto.size()));
}

/// Texto do primeiro elemento com esse nome no XML (o nome é local; o namespace padrão não entra na comparação).
/// Vazio se o elemento não existe. O valor é lido do XML gerado, não do tipo do Back.
QString textoDoElemento(const QByteArray& xml, QStringView nome)
{
    QXmlStreamReader leitor{xml};
    while (!leitor.atEnd()) {
        if (leitor.readNext() == QXmlStreamReader::StartElement && leitor.name() == nome) {
            return leitor.readElementText();
        }
    }
    return {};
}

} // namespace

class TstCodecDps : public QObject
{
    Q_OBJECT

private slots:
    void dpsMeiMinimaValidaNoXsd();
    void dpsSimplesComIbsCbsValidaNoXsd();
    void dpsTomadorCpfSemEnderecoValida();
    void dpsTomadorCNaoNifValida();
    void dpsCompletaComOpcionaisValidaNoXsd();
    void cnpjAlfanumericoSaiComoTexto();
    void cnpjSerializaSempreMaiusculo();
    void dpsSemAssinaturaValidaNoXsd();
    void versaoUmPontoZeroUm();
    void xDescServ2000Aceita();
    void xDescServ2001Reprova();
    void xDescServContaPontosDeCodigo();
    void xDescServSoEspacosReprova();
    void cepComZeroAEsquerdaSai8Digitos();
    // Valores exatos do cabeçalho da DPS (b, revisão): literais escritos à mão, um slot por campo.
    void dhEmiValorExato();
    void nDpsValorExato();
    void dCompetValorExato();
    void serieValorExato();
    void tpEmitValorExato();
    void finNFSeValorExato();
    void cLocEmiValorExato();
    void vereditoDoValidadorBateComONomeDaFixture();
    void fixturesDpsIguaisAoCodec();
};

void TstCodecDps::dpsMeiMinimaValidaNoXsd()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(!xml.isEmpty(), "codec devolveu XML vazio");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::dpsSimplesComIbsCbsValidaNoXsd()
{
    const auto dps = casos::dpsSimplesIbsCbs();
    QVERIFY2(dps.has_value(), "builder do Simples com IBS/CBS recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(xml.contains("<opSimpNac>3</opSimpNac>"), "regime do Simples não serializado");
    QVERIFY2(xml.contains("<regApTribSN>1</regApTribSN>"), "regApTribSN não serializado");
    QVERIFY2(xml.contains("<CST>000</CST>"), "grupo IBS/CBS não serializado");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::dpsTomadorCpfSemEnderecoValida()
{
    const auto dps = casos::dpsTomadorCpfSemEndereco();
    QVERIFY2(dps.has_value(), "builder do tomador CPF recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(xml.contains("<CPF>52998224725</CPF>"), "CPF do tomador não serializado sem máscara");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::dpsTomadorCNaoNifValida()
{
    const auto dps = casos::dpsTomadorCNaoNif();
    QVERIFY2(dps.has_value(), "builder do tomador cNaoNIF recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(xml.contains("<cNaoNIF>2</cNaoNIF>"), "cNaoNIF do tomador não serializado");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::dpsCompletaComOpcionaisValidaNoXsd()
{
    const auto dps = casos::dpsCompletaComOpcionais();
    QVERIFY2(dps.has_value(), "builder do caso completo recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(xml.contains("<NIF>ABC123</NIF>"), "NIF do tomador não serializado");
    QVERIFY2(xml.contains("<gRefNFSe>"), "gRefNFSe não serializado");
    QVERIFY2(xml.contains("<subst>"), "substituição não serializada");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::cnpjAlfanumericoSaiComoTexto()
{
    // CNPJ alfanumérico do exemplo da Receita (Q.17): 12ABC34501DE35, DV válido segundo o Back (NL-1.1).
    const auto dps = casos::montarDps(casos::dadosMei(QStringLiteral("12ABC34501DE35"), QStringLiteral("Consultoria em tecnologia")));
    QVERIFY2(dps.has_value(), "builder com CNPJ alfanumérico recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(xml.contains("<CNPJ>12ABC34501DE35</CNPJ>"), "CNPJ alfanumérico não saiu como texto");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::cnpjSerializaSempreMaiusculo()
{
    // Entrada em minúsculas e com máscara: o Back normaliza (DA-1.1), o codec serializa só o texto em maiúsculas.
    const auto dps = casos::montarDps(casos::dadosMei(QStringLiteral("12.abc.345/01de-35"), QStringLiteral("Consultoria em tecnologia")));
    QVERIFY2(dps.has_value(), "builder com CNPJ minúsculo recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(xml.contains("<CNPJ>12ABC34501DE35</CNPJ>"), "CNPJ não saiu normalizado em maiúsculas");
    QVERIFY2(!xml.contains("abc"), "CNPJ minúsculo vazou para o XML");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::dpsSemAssinaturaValidaNoXsd()
{
    // ds:Signature é minOccurs=0 no TCDPS (DA-1.4): a Dps sem assinatura valida, e o codec não escreve assinatura.
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(!xml.contains("Signature"), "codec escreveu assinatura que a v1 não produz");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::versaoUmPontoZeroUm()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QXmlStreamReader leitor{xml};
    QVERIFY2(leitor.readNextStartElement(), "XML sem elemento raiz");
    QCOMPARE(leitor.name().toString(), QStringLiteral("DPS"));
    QCOMPARE(leitor.namespaceUri().toString(), kNamespaceNfse);
    const QString versao = leitor.attributes().value(QStringLiteral("versao")).toString();
    QCOMPARE(versao, QStringLiteral("1.01"));
    QCOMPARE(comoQString(prod::kVersaoLeiaute), versao);
}

void TstCodecDps::xDescServ2000Aceita()
{
    QCOMPARE(prod::kLimiteXDescServ, 2000);
    const auto dps = casos::montarDps(casos::dadosMei(QStringLiteral("11222333000181"),
                                                      QString{static_cast<qsizetype>(prod::kLimiteXDescServ), QLatin1Char{'a'}}));
    QVERIFY2(dps.has_value(), "builder com 2000 caracteres recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

void TstCodecDps::xDescServ2001Reprova()
{
    // O Back não limita o tamanho (DescricaoServico, NL-1.4): o limite é do XSD, então o XML tem de reprovar.
    const auto dps = casos::montarDps(casos::dadosMei(QStringLiteral("11222333000181"),
                                                      QString{static_cast<qsizetype>(prod::kLimiteXDescServ) + 1, QLatin1Char{'a'}}));
    QVERIFY2(dps.has_value(), "builder com 2001 caracteres recusou a entrada (o Back não tem limite)");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    const Veredito veredito = validar(xml);
    QVERIFY2(!veredito.valido, "XSD aceitou xDescServ com 2001 caracteres");
    QVERIFY2(veredito.motivoInvalido, qPrintable(veredito.detalhe));
}

void TstCodecDps::xDescServContaPontosDeCodigo()
{
    // 2000 pontos de código fora do plano básico (U+20000) ocupam 4000 unidades UTF-16: o XSD conta pontos de código.
    const QString doisMil = casos::textoDePontosDeCodigo(prod::kLimiteXDescServ, U'\U00020000');
    QCOMPARE(doisMil.size(), static_cast<qsizetype>(2 * prod::kLimiteXDescServ));
    const auto valido = casos::montarDps(casos::dadosMei(QStringLiteral("11222333000181"), doisMil));
    QVERIFY2(valido.has_value(), "builder com 2000 pontos de código recusou a entrada");
    const Veredito aceito = validar(prod::CodecDps::serializar(*valido));
    QVERIFY2(aceito.valido, qPrintable(aceito.detalhe));

    const auto excedente = casos::montarDps(casos::dadosMei(
        QStringLiteral("11222333000181"), casos::textoDePontosDeCodigo(prod::kLimiteXDescServ + 1, U'\U00020000')));
    QVERIFY2(excedente.has_value(), "builder com 2001 pontos de código recusou a entrada");
    const Veredito reprovado = validar(prod::CodecDps::serializar(*excedente));
    QVERIFY2(!reprovado.valido, "XSD aceitou 2001 pontos de código");
    QVERIFY2(reprovado.motivoInvalido, qPrintable(reprovado.detalhe));
}

void TstCodecDps::xDescServSoEspacosReprova()
{
    // O Back já recusa a descrição só de espaços (DescricaoServico), então o caso prova a recusa do XSD sobre o XML.
    QVERIFY(!nl::back::DescricaoServico::de(u"   ").has_value());
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    const QByteArray semTexto = casos::comDescricaoSoEspacos(xml);
    QVERIFY2(semTexto != xml, "a troca da descrição por espaços não aconteceu no XML");
    const Veredito veredito = validar(semTexto);
    QVERIFY2(!veredito.valido, "XSD aceitou xDescServ só de espaços");
    QVERIFY2(veredito.motivoInvalido, qPrintable(veredito.detalhe));
}

void TstCodecDps::cepComZeroAEsquerdaSai8Digitos()
{
    const auto dps = casos::dpsCepComZeroAEsquerda();
    QVERIFY2(dps.has_value(), "builder do CEP com zero à esquerda recusou a entrada");
    const QByteArray xml = prod::CodecDps::serializar(*dps);
    QVERIFY2(xml.contains("<CEP>01001000</CEP>"), "CEP com zero à esquerda não saiu com 8 dígitos");
    const Veredito veredito = validar(xml);
    QVERIFY2(veredito.valido, qPrintable(veredito.detalhe));
}

// Os valores abaixo são os da MEI mínima (casos::dpsMei), escritos à mão. Não derivam do Back nem do codec: se o codec
// trocar um deles, a asserção reprova mesmo que o XML continue válido no XSD (o caso dos zeros à esquerda da série).

void TstCodecDps::dhEmiValorExato()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    QCOMPARE(textoDoElemento(prod::CodecDps::serializar(*dps), u"dhEmi"), QStringLiteral("2026-10-25T10:00:00-03:00"));
}

void TstCodecDps::nDpsValorExato()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    QCOMPARE(textoDoElemento(prod::CodecDps::serializar(*dps), u"nDPS"), QStringLiteral("1"));
}

void TstCodecDps::dCompetValorExato()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    QCOMPARE(textoDoElemento(prod::CodecDps::serializar(*dps), u"dCompet"), QStringLiteral("2026-10-01"));
}

void TstCodecDps::serieValorExato()
{
    // TSSerieDPS aceita "00001" pelo segundo ramo do pattern; o valor esperado é "1" (sem zeros à esquerda).
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    QCOMPARE(textoDoElemento(prod::CodecDps::serializar(*dps), u"serie"), QStringLiteral("1"));
}

void TstCodecDps::tpEmitValorExato()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    QCOMPARE(textoDoElemento(prod::CodecDps::serializar(*dps), u"tpEmit"), QStringLiteral("1"));
}

void TstCodecDps::finNFSeValorExato()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    QCOMPARE(textoDoElemento(prod::CodecDps::serializar(*dps), u"finNFSe"), QStringLiteral("0"));
}

void TstCodecDps::cLocEmiValorExato()
{
    const auto dps = casos::dpsMei();
    QVERIFY2(dps.has_value(), "builder da MEI mínima recusou a entrada");
    QCOMPARE(textoDoElemento(prod::CodecDps::serializar(*dps), u"cLocEmi"), QStringLiteral("4305108"));
}

void TstCodecDps::vereditoDoValidadorBateComONomeDaFixture()
{
    // Régua por nome, igual à do oráculo xmllint: valida_* tem de validar, invalida_* tem de reprovar.
    const auto fixtures = casos::casosFixture();
    QVERIFY2(fixtures.has_value(), "builder de alguma fixture recusou a entrada");
    QVERIFY(!fixtures->empty());
    for (const auto& caso : *fixtures) {
        const Veredito veredito = validar(caso.xml);
        const bool deveValidar = caso.nome.startsWith(QStringLiteral("valida_"));
        QVERIFY2(veredito.valido == deveValidar, qPrintable(caso.nome + QStringLiteral(": ") + veredito.detalhe));
    }
}

void TstCodecDps::fixturesDpsIguaisAoCodec()
{
    // A fixture da árvore é a saída do codec, byte a byte: se o codec mudar, a fixture tem de ser regerada e revisada.
    const auto fixtures = casos::casosFixture();
    QVERIFY2(fixtures.has_value(), "builder de alguma fixture recusou a entrada");
    for (const auto& caso : *fixtures) {
        QFile arquivo{QDir{kFixturesDps}.filePath(caso.nome)};
        QVERIFY2(arquivo.open(QIODevice::ReadOnly), qPrintable(QStringLiteral("fixture ausente: ") + caso.nome));
        const QByteArray gravado = arquivo.readAll();
        QVERIFY2(gravado == caso.xml, qPrintable(QStringLiteral("fixture divergente do codec: ") + caso.nome));
    }
}

// A geração das fixtures saiu deste executável de teste (DA-2.61): é a ferramenta tests/tools/gera_fixtures_dps, fora do CTest.

QTEST_GUILESS_MAIN(TstCodecDps)
#include "tst_codec_dps.moc"
