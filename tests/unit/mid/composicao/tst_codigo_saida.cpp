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
    // Configuração ou banco (NL-3.8, código 10): chave ausente, arquivo ilegível, caminho do certificado ilegível e algoritmo
    // de assinatura fora da lista (NL-2.10a, emenda 4). Nenhum deles sai como recusa comum.
    void configuracaoSaiComDez();
};

using nl::mid::composicao::codigoDeSaidaDaEmissao;

void TstCodigoSaida::codigosDeDocumentoSaemComQuatro()
{
    // Todo código de documento que o validador de entrada, o XSD e a identificação emitem (levantado no código em 30/10/2026).
    for (const char* codigo : {"xsd.invalido", "xsd.pos_assinatura", "cpf.dv", "cpf.formato", "cpf.repetido", "cpf.tamanho",
                               "cnpj.dv", "cnpj.formato", "cnpj.tamanho", "cnpj.zerado", "dps.dcompet", "dps.invalida",
                               "dps.nao_suportado_v1", "entrada.campo_ausente", "entrada.campos_conflitantes", "entrada.json_invalido",
                               "entrada.tipo_invalido", "entrada.xml", "entrada.numero_invalido",
                               // NL-2.21 (Emenda 2 do CTO): DOCTYPE no XML da DPS é recusa do documento, não falha interna.
                               "xml.com_dtd"}) {
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

void TstCodigoSaida::configuracaoSaiComDez()
{
    // NL-2.10b (condição do CISO, 31/10/2026, emenda 2): âncora definida fora de Homologação sai com 10 também.
    for (const char* codigo : {"configuracao.chave_ausente", "configuracao.arquivo_ilegivel", "certificado.caminho_ilegivel",
                               "assinatura.algoritmo_invalido", "configuracao.ancoras_a1_fora_de_homologacao",
                               "configuracao.ancoras_tls_fora_de_homologacao", "configuracao.raizes_diretorio_vazio",
                               "configuracao.xsd_manifesto_nao_fixado", "configuracao.xsd_fora_de_homologacao",
                               "configuracao.chave_duplicada", "configuracao.chave_vazia"}) {
        QCOMPARE(codigoDeSaidaDaEmissao(codigo), 10);
    }
}

void TstCodigoSaida::codigoDesconhecidoSaiComSetenta()
{
    QCOMPARE(codigoDeSaidaDaEmissao("emissao.id_dps_invalido"), 70);
    QCOMPARE(codigoDeSaidaDaEmissao("emissao.id_vazio"), 70);
    QCOMPARE(codigoDeSaidaDaEmissao("codigo_que_nao_existe"), 70);
    // NL-2.10b, MÉDIA-1: ambiente do pedido divergente do host sai com 70, por entrada explícita (interno, não recusa).
    QCOMPARE(codigoDeSaidaDaEmissao("emissao.ambiente_divergente"), 70);
}

QTEST_APPLESS_MAIN(TstCodigoSaida)
#include "tst_codigo_saida.moc"
