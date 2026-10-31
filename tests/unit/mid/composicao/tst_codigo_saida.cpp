// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do código de saída da emissão (NL-2.9, DA-2.51 e DA-2.57c): cada código classificado sai com a sua classe, e um
// código fora da lista sai como 70, nunca como uma recusa comum.
#include <nl/mid/composicao/codigo_saida.h>

#include <QtTest>

class TstCodigoSaida : public QObject
{
    Q_OBJECT

private slots:
    // Documento reprovado no leiaute (entrada, XML assinado, identificação) sai com 4.
    void codigosDeDocumentoSaemComQuatro();
    // Ambiente do XSD (esquema ou libxml2) sai com 2: não é culpa do documento.
    void codigosDeAmbienteXsdSaemComDois();
    // Recusa de regra (credencial, assinatura, confirmação, Sefin) sai com 1.
    void codigosDeRecusaSaemComUm();
    // E0014 com infDPS diferente: DPS duplicada sai com 5 (DA-2.58).
    void dpsDuplicadaSaiComCinco();
    // E0014 sem conseguir comparar sai com 7 (DA-2.58).
    void comparacaoIndisponivelSaiComSete();
    // Código sem classificação, ou falha interna do Id, sai com 70.
    void codigoDesconhecidoSaiComSetenta();
};

using nl::mid::composicao::codigoDeSaidaDaEmissao;

void TstCodigoSaida::codigosDeDocumentoSaemComQuatro()
{
    // Todo código de documento que o validador de entrada, o XSD e a identificação emitem (levantado no código em 30/10/2026).
    for (const char* codigo : {"xsd.invalido", "xsd.pos_assinatura", "cpf.dv", "cpf.formato", "cpf.repetido", "cpf.tamanho",
                               "cnpj.dv", "cnpj.formato", "cnpj.tamanho", "cnpj.zerado", "dps.dcompet", "dps.invalida",
                               "dps.nao_suportado_v1", "entrada.campo_ausente", "entrada.campos_conflitantes", "entrada.json_invalido",
                               "entrada.tipo_invalido", "entrada.xml"}) {
        QCOMPARE(codigoDeSaidaDaEmissao(codigo), 4);
    }
}

void TstCodigoSaida::codigosDeAmbienteXsdSaemComDois()
{
    QCOMPARE(codigoDeSaidaDaEmissao("xsd.esquema"), 2);
    QCOMPARE(codigoDeSaidaDaEmissao("xsd.interno"), 2);
}

void TstCodigoSaida::codigosDeRecusaSaemComUm()
{
    for (const char* codigo : {"credencial.nao_carregou", "assinatura.algoritmo_bloqueado", "confirmacao.diferente_do_a1", "sefin.falha"}) {
        QCOMPARE(codigoDeSaidaDaEmissao(codigo), 1);
    }
}

void TstCodigoSaida::dpsDuplicadaSaiComCinco()
{
    QCOMPARE(codigoDeSaidaDaEmissao("sefin.dps_duplicada"), 5);
}

void TstCodigoSaida::comparacaoIndisponivelSaiComSete()
{
    QCOMPARE(codigoDeSaidaDaEmissao("sefin.comparacao_indisponivel"), 7);
}

void TstCodigoSaida::codigoDesconhecidoSaiComSetenta()
{
    QCOMPARE(codigoDeSaidaDaEmissao("emissao.id_dps_invalido"), 70);
    QCOMPARE(codigoDeSaidaDaEmissao("emissao.id_vazio"), 70);
    QCOMPARE(codigoDeSaidaDaEmissao("codigo_que_nao_existe"), 70);
}

QTEST_APPLESS_MAIN(TstCodigoSaida)
#include "tst_codigo_saida.moc"
