// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Casos de DPS montados só pelos tipos do Back (NL-1.6b). Usados pelo QtTest (tst_codec_dps) e pela geração das fixtures.
// Cada builder devolve std::nullopt se algum tipo de entrada falhar; o teste acusa com QVERIFY. Nada aqui lança exceção.
#ifndef NL_TESTE_MID_SEFIN_CASOS_DPS_H
#define NL_TESTE_MID_SEFIN_CASOS_DPS_H

#include <nl/base/erro.h>
#include <nl/back/dinheiro/valor.h>
#include <nl/back/dps/aliquota_issqn.h>
#include <nl/back/dps/chave_nfse.h>
#include <nl/back/dps/codigo_classificacao_tributaria.h>
#include <nl/back/dps/codigo_indicador_operacao.h>
#include <nl/back/dps/codigo_justificativa_substituicao.h>
#include <nl/back/dps/codigo_nbs.h>
#include <nl/back/dps/codigo_pais.h>
#include <nl/back/dps/codigo_situacao_tributaria.h>
#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/descricao_servico.h>
#include <nl/back/dps/dps.h>
#include <nl/back/dps/endereco.h>
#include <nl/back/dps/endereco_exterior.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/indicador_destinatario.h>
#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/motivo_nao_nif.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/percentual_tributo.h>
#include <nl/back/dps/prestador.h>
#include <nl/back/dps/reg_ap_trib_sn.h>
#include <nl/back/dps/reg_esp_trib.h>
#include <nl/back/dps/ref_nfse.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/serie.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/substituicao.h>
#include <nl/back/dps/telefone.h>
#include <nl/back/dps/tipo_ambiente.h>
#include <nl/back/dps/tipo_operacao.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/trib_issqn.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_ibscbs.h>
#include <nl/back/dps/tributacao_municipal.h>
#include <nl/back/dps/tributos_monetarios.h>
#include <nl/back/dps/tributos_percentuais.h>
#include <nl/back/dps/valores_servico.h>
#include <nl/back/identificacao/cep.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/cpf.h>
#include <nl/back/identificacao/inscricao.h>

#include <nl/mid/sefin/leiaute/prodrest_20261020/codec_dps.h>

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>
#include <QTimeZone>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace casos {

namespace nb = nl::back;

/// Valor do resultado, ou std::nullopt. Não usa value(), que lançaria dentro do slot do QtTest.
template <typename T>
std::optional<T> ok(const std::expected<T, nl::base::Erro>& resultado)
{
    if (resultado) {
        return *resultado;
    }
    return std::nullopt;
}

/// Data e hora de emissão com offset -03:00 (horário de Brasília).
inline QDateTime dhEmiPadrao()
{
    return QDateTime{QDate{2026, 10, 25}, QTime{10, 0}, QTimeZone::fromSecondsAheadOfUtc(-3 * 3600)};
}

/// Endereço nacional do prestador ou do tomador. O CEP entra com ou sem hífen; o XML sai com 8 dígitos.
inline std::optional<nb::Endereco> enderecoNacional(const QString& cep)
{
    const auto cepValor = ok(nb::Cep::de(cep));
    const auto cMun = ok(nb::CodigoMunicipio::de(u"4305108"));
    const auto xLgr = ok(nb::Logradouro::de(u"Rua das Flores", "xLgr"));
    const auto nro = ok(nb::NumeroEndereco::de(u"10", "nro"));
    const auto bairro = ok(nb::Bairro::de(u"Centro", "xBairro"));
    if (!cepValor || !cMun || !xLgr || !nro || !bairro) {
        return std::nullopt;
    }
    return nb::Endereco::montar(nb::LocalEndereco{nb::EnderecoNacional::montar(*cMun, *cepValor)}, *xLgr, *nro, std::nullopt, *bairro);
}

/// Endereço no exterior (endExt do tomador).
inline std::optional<nb::Endereco> enderecoExterior()
{
    const auto cPais = ok(nb::CodigoPais::de(u"US"));
    const auto cEndPost = ok(nb::CodigoEndPostal::de(u"33101", "cEndPost"));
    const auto xCidade = ok(nb::Cidade::de(u"Miami", "xCidade"));
    const auto xEstProvReg = ok(nb::EstadoProvRegiao::de(u"FL", "xEstProvReg"));
    const auto xLgr = ok(nb::Logradouro::de(u"Brickell Avenue", "xLgr"));
    const auto nro = ok(nb::NumeroEndereco::de(u"1", "nro"));
    const auto bairro = ok(nb::Bairro::de(u"Downtown", "xBairro"));
    if (!cPais || !cEndPost || !xCidade || !xEstProvReg || !xLgr || !nro || !bairro) {
        return std::nullopt;
    }
    return nb::Endereco::montar(nb::LocalEndereco{nb::EnderecoExterior::montar(*cPais, *cEndPost, *xCidade, *xEstProvReg)}, *xLgr, *nro,
                                std::nullopt, *bairro);
}

/// Dados de um MEI (opSimpNac 2), prestador com CNPJ informado e serviço com a descrição dada. Sem tomador e sem IBS/CBS.
inline std::optional<nb::DadosDps> dadosMei(const QString& cnpj, const QString& xDescServ)
{
    const auto tpAmb = ok(nb::TipoAmbiente::de(u"2"));
    const auto verAplic = ok(nb::VerAplic::de(u"NotaLivre 0.1", "verAplic"));
    const auto serie = ok(nb::Serie::de(1));
    const auto nDps = ok(nb::NumeroDps::de(1));
    const auto cLocEmi = ok(nb::CodigoMunicipio::de(u"4305108"));
    const auto cnpjPrest = ok(nb::Cnpj::de(cnpj));
    const auto opSimpNac = ok(nb::OpSimpNac::de(u"2"));
    const auto regEspTrib = ok(nb::RegEspTrib::de(u"0"));
    const auto cLocPrestacao = ok(nb::CodigoMunicipio::de(u"4305108"));
    const auto cTribNac = ok(nb::CodigoTribNacional::de(u"170201"));
    const auto descricao = ok(nb::DescricaoServico::de(xDescServ));
    const auto vServ = ok(nb::Valor::de(10000));
    const auto issqn = ok(nb::TribIssqn::de(u"1"));
    const auto retencao = ok(nb::RetencaoIssqn::de(u"1"));
    const auto indTot = ok(nb::IndicadorTotalTributos::de(u"0"));
    if (!tpAmb || !verAplic || !serie || !nDps || !cLocEmi || !cnpjPrest || !opSimpNac || !regEspTrib || !cLocPrestacao || !cTribNac
        || !descricao || !vServ || !issqn || !retencao || !indTot) {
        return std::nullopt;
    }
    const auto regime = ok(nb::RegimeTributacao::montar(*opSimpNac, std::nullopt, *regEspTrib));
    const auto municipal = ok(nb::TributacaoMunicipal::montar(*issqn, *retencao, std::nullopt));
    if (!regime || !municipal) {
        return std::nullopt;
    }
    return nb::DadosDps{
        .tpAmb = *tpAmb,
        .dhEmi = dhEmiPadrao(),
        .verAplic = *verAplic,
        .serie = *serie,
        .nDps = *nDps,
        .dCompet = QDate{2026, 10, 1},
        .cLocEmi = *cLocEmi,
        .subst = std::nullopt,
        .prest = nb::Prestador::montar(nb::Inscricao{*cnpjPrest}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, *regime),
        .toma = std::nullopt,
        .serv = nb::Servico::montar(*cLocPrestacao, *cTribNac, *descricao, std::nullopt),
        .valores = nb::ValoresServico::montar(*vServ, nb::Tributacao::montar(*municipal, nb::TotalTributos{*indTot})),
        .ibscbs = std::nullopt,
    };
}

/// Dps montada a partir de dados já prontos. std::nullopt se os dados não foram montados ou se a Dps recusar.
inline std::optional<nb::Dps> montarDps(const std::optional<nb::DadosDps>& dados)
{
    if (!dados) {
        return std::nullopt;
    }
    return ok(nb::Dps::montar(*dados));
}

/// MEI com CNPJ numérico e descrição de 'Consultoria em tecnologia'. Caso mínimo do leiaute.
inline std::optional<nb::Dps> dpsMei()
{
    return montarDps(dadosMei(QStringLiteral("11222333000181"), QStringLiteral("Consultoria em tecnologia")));
}

/// Simples Nacional (opSimpNac 3, regApTribSN 1), com endereço, IM, cNBS, alíquota do ISSQN e grupo IBS/CBS.
inline std::optional<nb::DadosDps> dadosSimplesIbsCbs()
{
    auto dados = dadosMei(QStringLiteral("11222333000181"), QStringLiteral("Consultoria em tecnologia"));
    const auto opSimpNac = ok(nb::OpSimpNac::de(u"3"));
    const auto regApTribSN = ok(nb::RegApTribSN::de(u"1"));
    const auto regEspTrib = ok(nb::RegEspTrib::de(u"0"));
    const auto issqn = ok(nb::TribIssqn::de(u"1"));
    const auto retencao = ok(nb::RetencaoIssqn::de(u"1"));
    const auto aliquota = ok(nb::AliquotaIssqn::de(u"5.00"));
    const auto cNbs = ok(nb::CodigoNbs::de(u"123456789"));
    const auto zero = ok(nb::Valor::de(0));
    const auto vServ = ok(nb::Valor::de(10000));
    const auto indOp = ok(nb::CodigoIndicadorOperacao::de(u"000001"));
    const auto tpOper = ok(nb::TipoOperacao::de(u"1"));
    const auto indDest = ok(nb::IndicadorDestinatario::de(u"0"));
    const auto cst = ok(nb::CodigoSituacaoTributaria::de(u"000"));
    const auto cClassTrib = ok(nb::CodigoClassificacaoTributaria::de(u"000001"));
    const auto im = ok(nb::InscricaoMunicipal::de(u"123456", "IM"));
    const auto xNome = ok(nb::NomeRazaoSocial::de(u"Empresa Simples Teste", "xNome"));
    const auto endereco = enderecoNacional(QStringLiteral("01001-000"));
    if (!dados || !opSimpNac || !regApTribSN || !regEspTrib || !issqn || !retencao || !aliquota || !cNbs || !zero || !vServ || !indOp
        || !tpOper || !indDest || !cst || !cClassTrib || !im || !xNome || !endereco) {
        return std::nullopt;
    }
    const auto regime = ok(nb::RegimeTributacao::montar(*opSimpNac, *regApTribSN, *regEspTrib));
    const auto municipal = ok(nb::TributacaoMunicipal::montar(*issqn, *retencao, *aliquota));
    const auto trib = nb::TributacaoIbscbs::montar(*cst, *cClassTrib);
    const auto ibs = ok(nb::Ibscbs::montar(*indOp, *tpOper, std::nullopt, *indDest, trib));
    const auto monetarios = nb::TributosMonetarios::montar(*zero, *zero, *zero);
    if (!regime || !municipal || !ibs) {
        return std::nullopt;
    }
    dados->prest = nb::Prestador::montar(dados->prest.inscricao(), *im, *xNome, *endereco, std::nullopt, std::nullopt, *regime);
    dados->serv = nb::Servico::montar(dados->serv.cLocPrestacao(), dados->serv.cTribNac(), dados->serv.xDescServ(), *cNbs);
    dados->valores = nb::ValoresServico::montar(*vServ, nb::Tributacao::montar(*municipal, nb::TotalTributos{monetarios}));
    dados->ibscbs = *ibs;
    return dados;
}

inline std::optional<nb::Dps> dpsSimplesIbsCbs()
{
    return montarDps(dadosSimplesIbsCbs());
}

/// MEI com tomador pessoa física sem endereço (o endereço do tomador é opcional no XSD).
inline std::optional<nb::Dps> dpsTomadorCpfSemEndereco()
{
    auto dados = dadosMei(QStringLiteral("11222333000181"), QStringLiteral("Consultoria em tecnologia"));
    const auto cpf = ok(nb::Cpf::de(u"52998224725"));
    const auto xNome = ok(nb::NomeRazaoSocial::de(u"Cliente Pessoa Fisica", "xNome"));
    if (!dados || !cpf || !xNome) {
        return std::nullopt;
    }
    dados->toma = nb::Tomador::montar(nb::IdentificacaoPessoa{*cpf}, std::nullopt, *xNome, std::nullopt, std::nullopt, std::nullopt);
    return montarDps(dados);
}

/// MEI com tomador no exterior identificado por cNaoNIF (código 2, não exigência do NIF).
inline std::optional<nb::Dps> dpsTomadorCNaoNif()
{
    auto dados = dadosMei(QStringLiteral("11222333000181"), QStringLiteral("Consultoria em tecnologia"));
    const auto motivo = ok(nb::MotivoNaoNif::de(u"2"));
    const auto xNome = ok(nb::NomeRazaoSocial::de(u"Cliente Exterior", "xNome"));
    if (!dados || !motivo || !xNome) {
        return std::nullopt;
    }
    dados->toma = nb::Tomador::montar(nb::IdentificacaoPessoa{*motivo}, std::nullopt, *xNome, std::nullopt, std::nullopt, std::nullopt);
    return montarDps(dados);
}

/// MEI com CEP que começa com zero: o XML tem de sair com os 8 dígitos (CEP sem hífen).
inline std::optional<nb::Dps> dpsCepComZeroAEsquerda()
{
    auto dados = dadosMei(QStringLiteral("11222333000181"), QStringLiteral("Consultoria em tecnologia"));
    const auto endereco = enderecoNacional(QStringLiteral("01001-000"));
    if (!dados || !endereco) {
        return std::nullopt;
    }
    dados->prest = nb::Prestador::montar(dados->prest.inscricao(), std::nullopt, std::nullopt, *endereco, std::nullopt, std::nullopt,
                                         dados->prest.regime());
    return montarDps(dados);
}

/// Caso com todos os grupos opcionais que o leiaute da v1 usa: substituição, tomador com NIF, endereço no exterior,
/// telefone e e-mail, IM e xNome do prestador, endereço, cNBS, IBS/CBS com gRefNFSe e tributos percentuais.
inline std::optional<nb::Dps> dpsCompletaComOpcionais()
{
    auto dados = dadosSimplesIbsCbs();
    const auto chaveSubstituida = ok(nb::ChaveNfse::de(u"123456" u"12ABC34501DE35" u"123456789012345678901234567890"));
    const auto cMotivo = ok(nb::CodigoJustificativaSubstituicao::de(u"01"));
    const auto xMotivo = ok(nb::MotivoSubstituicao::de(u"Erro de digitacao no valor", "xMotivo"));
    const auto nif = ok(nb::Nif::de(u"ABC123", "NIF"));
    const auto xNomeToma = ok(nb::NomeRazaoSocial::de(u"Tomador Exterior", "xNome"));
    const auto enderecoToma = enderecoExterior();
    const auto emailToma = ok(nb::EmailPessoa::de(u"contato@exemplo.com", "email"));
    const auto foneTomaNif = ok(nb::Telefone::de(u"11988887777"));
    const auto percentual = ok(nb::PercentualTributo::de(u"1.00"));
    const auto chaveRef = ok(nb::ChaveNfse::de(u"123456" u"12ABC34501DE35" u"123456789012345678901234567890"));
    const auto tpOperRef = ok(nb::TipoOperacao::de(u"2"));
    const auto indDestRef = ok(nb::IndicadorDestinatario::de(u"1"));
    if (!dados || !chaveSubstituida || !cMotivo || !xMotivo || !nif || !xNomeToma || !enderecoToma || !emailToma
        || !foneTomaNif || !percentual || !chaveRef || !tpOperRef || !indDestRef || !dados->ibscbs) {
        return std::nullopt;
    }
    const auto substituicao = nb::Substituicao::montar(*chaveSubstituida, *cMotivo, *xMotivo);
    const auto referencias = ok(nb::RefNfse::de(std::vector<nb::ChaveNfse>{*chaveRef}));
    if (!referencias) {
        return std::nullopt;
    }
    const auto ibs = ok(nb::Ibscbs::montar(dados->ibscbs->cIndOp(), *tpOperRef, *referencias, *indDestRef, dados->ibscbs->trib()));
    if (!ibs) {
        return std::nullopt;
    }
    const auto percentuais = nb::TributosPercentuais::montar(*percentual, *percentual, *percentual);
    dados->subst = substituicao;
    dados->toma = nb::Tomador::montar(nb::IdentificacaoPessoa{*nif}, std::nullopt, *xNomeToma, *enderecoToma, *foneTomaNif, *emailToma);
    dados->ibscbs = *ibs;
    dados->valores = nb::ValoresServico::montar(dados->valores.vServ(),
                                                nb::Tributacao::montar(dados->valores.trib().tribMun(), nb::TotalTributos{percentuais}));
    return montarDps(dados);
}

/// Texto com 'quantidade' pontos de código iguais. Fora do plano básico, cada ponto ocupa duas unidades UTF-16.
inline QString textoDePontosDeCodigo(std::int64_t quantidade, char32_t ponto)
{
    const std::u32string ucs4(static_cast<std::size_t>(quantidade), ponto);
    return QString::fromUcs4(ucs4.data(), static_cast<qsizetype>(ucs4.size()));
}

/// Mesmo XML, com a descrição trocada por espaços: o Back já recusa, então o caso prova só a recusa do XSD.
inline QByteArray comDescricaoSoEspacos(QByteArray xml)
{
    xml.replace("<xDescServ>Consultoria em tecnologia</xDescServ>", "<xDescServ>   </xDescServ>");
    return xml;
}

/// Caso de fixture: nome do arquivo (valida_* ou invalida_*) e XML gerado pelo codec.
struct CasoFixture
{
    QString nome;
    QByteArray xml;
};

/// Todas as fixtures de tests/fixtures/dps. std::nullopt se algum builder falhar.
inline std::optional<std::vector<CasoFixture>> casosFixture()
{
    struct Entrada
    {
        QString nome;
        std::optional<nb::Dps> dps;
    };
    const auto mei = dpsMei();
    const std::vector<Entrada> entradas = {
        {QStringLiteral("valida_dps_mei_minima.xml"), mei},
        {QStringLiteral("valida_dps_cnpj_alfanumerico.xml"),
         montarDps(dadosMei(QStringLiteral("12ABC34501DE35"), QStringLiteral("Consultoria em tecnologia")))},
        {QStringLiteral("valida_dps_cnpj_minusculo_normalizado.xml"),
         montarDps(dadosMei(QStringLiteral("12abc34501de35"), QStringLiteral("Consultoria em tecnologia")))},
        {QStringLiteral("valida_dps_tomador_cpf_sem_endereco.xml"), dpsTomadorCpfSemEndereco()},
        {QStringLiteral("valida_dps_tomador_cnaonif.xml"), dpsTomadorCNaoNif()},
        {QStringLiteral("valida_dps_cep_zero_a_esquerda.xml"), dpsCepComZeroAEsquerda()},
        {QStringLiteral("valida_dps_simples_ibscbs.xml"), dpsSimplesIbsCbs()},
        {QStringLiteral("valida_dps_completa.xml"), dpsCompletaComOpcionais()},
        {QStringLiteral("valida_dps_xdescserv_2000.xml"),
         montarDps(dadosMei(QStringLiteral("11222333000181"),
                            QString{2000, QLatin1Char{'a'}}))},
        {QStringLiteral("valida_dps_xdescserv_2000_pontos_codigo.xml"),
         montarDps(dadosMei(QStringLiteral("11222333000181"), textoDePontosDeCodigo(2000, U'\U00020000')))},
        {QStringLiteral("invalida_dps_xdescserv_2001.xml"),
         montarDps(dadosMei(QStringLiteral("11222333000181"), QString{2001, QLatin1Char{'a'}}))},
        {QStringLiteral("invalida_dps_xdescserv_2001_pontos_codigo.xml"),
         montarDps(dadosMei(QStringLiteral("11222333000181"), textoDePontosDeCodigo(2001, U'\U00020000')))},
    };
    std::vector<CasoFixture> casos;
    for (const auto& entrada : entradas) {
        if (!entrada.dps) {
            return std::nullopt;
        }
        casos.push_back(CasoFixture{entrada.nome, nl::mid::sefin::prodrest_20261020::CodecDps::serializar(*entrada.dps)});
    }
    if (!mei) {
        return std::nullopt;
    }
    casos.push_back(CasoFixture{QStringLiteral("invalida_dps_xdescserv_so_espacos.xml"),
                                comDescricaoSoEspacos(nl::mid::sefin::prodrest_20261020::CodecDps::serializar(*mei))});
    return casos;
}

} // namespace casos

#endif // NL_TESTE_MID_SEFIN_CASOS_DPS_H
