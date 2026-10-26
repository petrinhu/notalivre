// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fábrica de DPS válida para os testes de regras locais (NL-1.5). Cada caso altera só o campo que a regra lê.
// Falha de montagem devolve nullopt (o teste acusa com QVERIFY); nunca vira "sem tomador" em silêncio.
#ifndef NL_TESTE_BACK_REGRAS_FABRICA_DPS_H
#define NL_TESTE_BACK_REGRAS_FABRICA_DPS_H

#include <expected>
#include <optional>
#include <string>

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>
#include <QTimeZone>

#include <nl/base/erro.h>
#include <nl/back/dinheiro/valor.h>
#include <nl/back/dps/aliquota_issqn.h>
#include <nl/back/dps/codigo_classificacao_tributaria.h>
#include <nl/back/dps/codigo_pais.h>
#include <nl/back/dps/codigo_situacao_tributaria.h>
#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/descricao_servico.h>
#include <nl/back/dps/dps.h>
#include <nl/back/dps/endereco.h>
#include <nl/back/dps/endereco_exterior.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/indicador_destinatario.h>
#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/percentual_tributo_sn.h>
#include <nl/back/dps/prestador.h>
#include <nl/back/dps/reg_esp_trib.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/tipo_ambiente.h>
#include <nl/back/dps/tipo_operacao.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/trib_issqn.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_ibscbs.h>
#include <nl/back/dps/tributacao_municipal.h>
#include <nl/back/dps/valores_servico.h>
#include <nl/back/identificacao/cep.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/cpf.h>
#include <nl/back/identificacao/inscricao.h>

namespace teste {

/// Tipo de tomador dos casos. "SemEndereco" omite o grupo end; "Exterior" usa endereço no exterior.
enum class TipoTomador { cnpjComEndereco, cnpjSemEndereco, cpfComEndereco, cpfSemEndereco, nifComEndereco, exteriorComPais, semTomador };

/// Campos variáveis da DPS. Padrão: não optante (opSimpNac 1), sem retenção, tomador CNPJ com endereço, competência 2026.
struct Opcoes
{
    std::string opSimpNac = "1";
    std::string tpRetISSQN = "1";
    std::string tribISSQN = "1";
    std::string cTribNac = "170201";
    std::string cLocPrestacao = "4305108";
    std::string cnpjPrestador = "11222333000181";
    std::string cnpjTomador = "11444777000161";
    TipoTomador tomador = TipoTomador::cnpjComEndereco;
    std::string cPais = "US";
    bool totTribSn = false;
    bool pAliq = false;
    bool ibscbs = false;
    bool tpOper = false;
    QDate dCompet{2026, 10, 1};
};

/// Valor do resultado, ou nullopt. Não lança dentro do slot do QtTest.
template <typename T>
std::optional<T> valor(const std::expected<T, nl::base::Erro>& resultado)
{
    if (resultado) {
        return *resultado;
    }
    return std::nullopt;
}

/// Código do erro devolvido por uma regra, ou QString vazia quando não há erro.
inline QString codigoDe(const std::optional<nl::base::Erro>& erro)
{
    if (!erro) {
        return QString{};
    }
    return QString::fromStdString(erro->codigo());
}

inline std::optional<nl::back::RegimeTributacao> regimeCom(const Opcoes& o)
{
    using namespace nl::back;
    const auto opSimpNac = valor(OpSimpNac::de(QString::fromStdString(o.opSimpNac)));
    const auto regEspTrib = valor(RegEspTrib::de(u"0"));
    if (!opSimpNac || !regEspTrib) {
        return std::nullopt;
    }
    return valor(RegimeTributacao::montar(*opSimpNac, std::nullopt, *regEspTrib));
}

inline std::optional<nl::back::Prestador> prestadorCom(const Opcoes& o)
{
    using namespace nl::back;
    const auto cnpj = valor(Cnpj::de(QString::fromStdString(o.cnpjPrestador)));
    const auto regime = regimeCom(o);
    if (!cnpj || !regime) {
        return std::nullopt;
    }
    return Prestador::montar(Inscricao{*cnpj}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, *regime);
}

inline std::optional<nl::back::Endereco> enderecoNacional()
{
    using namespace nl::back;
    const auto cMun = valor(CodigoMunicipio::de(u"4305108"));
    const auto cep = valor(Cep::de(u"95010000"));
    const auto xLgr = valor(Logradouro::de(u"Rua das Flores", "xLgr"));
    const auto nro = valor(NumeroEndereco::de(u"100", "nro"));
    const auto bairro = valor(Bairro::de(u"Centro", "xBairro"));
    if (!cMun || !cep || !xLgr || !nro || !bairro) {
        return std::nullopt;
    }
    return Endereco::montar(LocalEndereco{EnderecoNacional::montar(*cMun, *cep)}, *xLgr, *nro, std::nullopt, *bairro);
}

inline std::optional<nl::back::Endereco> enderecoExterior(const std::string& cPais)
{
    using namespace nl::back;
    const auto pais = valor(CodigoPais::de(QString::fromStdString(cPais)));
    const auto cEndPost = valor(CodigoEndPostal::de(u"12345", "cEndPost"));
    const auto xCidade = valor(Cidade::de(u"Miami", "xCidade"));
    const auto xEstProvReg = valor(EstadoProvRegiao::de(u"FL", "xEstProvReg"));
    const auto xLgr = valor(Logradouro::de(u"Ocean Drive", "xLgr"));
    const auto nro = valor(NumeroEndereco::de(u"1", "nro"));
    const auto bairro = valor(Bairro::de(u"Centro", "xBairro"));
    if (!pais || !cEndPost || !xCidade || !xEstProvReg || !xLgr || !nro || !bairro) {
        return std::nullopt;
    }
    const auto exterior = EnderecoExterior::montar(*pais, *cEndPost, *xCidade, *xEstProvReg);
    return Endereco::montar(LocalEndereco{exterior}, *xLgr, *nro, std::nullopt, *bairro);
}

/// Identificação do tomador conforme o caso. nullopt = falha de montagem.
inline std::optional<nl::back::IdentificacaoPessoa> identificacaoDoTomador(const Opcoes& o)
{
    using namespace nl::back;
    switch (o.tomador) {
    case TipoTomador::cnpjComEndereco:
    case TipoTomador::cnpjSemEndereco:
        if (const auto cnpj = valor(Cnpj::de(QString::fromStdString(o.cnpjTomador)))) {
            return IdentificacaoPessoa{*cnpj};
        }
        return std::nullopt;
    case TipoTomador::cpfComEndereco:
    case TipoTomador::cpfSemEndereco:
        if (const auto cpf = valor(Cpf::de(u"12345678909"))) {
            return IdentificacaoPessoa{*cpf};
        }
        return std::nullopt;
    case TipoTomador::nifComEndereco:
    case TipoTomador::exteriorComPais:
        if (const auto nif = valor(Nif::de(u"ABC123", "NIF"))) {
            return IdentificacaoPessoa{*nif};
        }
        return std::nullopt;
    case TipoTomador::semTomador:
        return std::nullopt;
    }
    return std::nullopt;
}

/// Endereço do tomador conforme o caso. Os casos "SemEndereco" devolvem nullopt de propósito; falha de montagem também.
/// Use tomadorTemEndereco para distinguir.
inline std::optional<nl::back::Endereco> enderecoDoTomador(const Opcoes& o)
{
    switch (o.tomador) {
    case TipoTomador::cnpjComEndereco:
    case TipoTomador::cpfComEndereco:
    case TipoTomador::nifComEndereco:
        return enderecoNacional();
    case TipoTomador::exteriorComPais:
        return enderecoExterior(o.cPais);
    case TipoTomador::cnpjSemEndereco:
    case TipoTomador::cpfSemEndereco:
    case TipoTomador::semTomador:
        return std::nullopt;
    }
    return std::nullopt;
}

inline bool tomadorTemEndereco(const Opcoes& o)
{
    return o.tomador == TipoTomador::cnpjComEndereco || o.tomador == TipoTomador::cpfComEndereco
           || o.tomador == TipoTomador::nifComEndereco || o.tomador == TipoTomador::exteriorComPais;
}

/// Tomador da DPS. Resultado externo: nullopt = falha de montagem; resultado interno vazio = sem tomador (caso intencional).
inline std::optional<std::optional<nl::back::Tomador>> tomadorCom(const Opcoes& o)
{
    using namespace nl::back;
    if (o.tomador == TipoTomador::semTomador) {
        return std::optional<Tomador>{};
    }
    const auto identificacao = identificacaoDoTomador(o);
    if (!identificacao) {
        return std::nullopt;
    }
    std::optional<Endereco> endereco;
    if (tomadorTemEndereco(o)) {
        endereco = enderecoDoTomador(o);
        if (!endereco) {
            return std::nullopt;
        }
    }
    return std::optional<Tomador>{Tomador::montar(*identificacao, std::nullopt, std::nullopt, endereco, std::nullopt, std::nullopt)};
}

inline std::optional<nl::back::Tributacao> tributacaoCom(const Opcoes& o)
{
    using namespace nl::back;
    const auto issqn = valor(TribIssqn::de(QString::fromStdString(o.tribISSQN)));
    const auto retencao = valor(RetencaoIssqn::de(QString::fromStdString(o.tpRetISSQN)));
    if (!issqn || !retencao) {
        return std::nullopt;
    }
    std::optional<AliquotaIssqn> pAliq;
    if (o.pAliq) {
        pAliq = valor(AliquotaIssqn::de(u"2.00"));
        if (!pAliq) {
            return std::nullopt;
        }
    }
    const auto municipal = valor(TributacaoMunicipal::montar(*issqn, *retencao, pAliq));
    if (!municipal) {
        return std::nullopt;
    }
    if (o.totTribSn) {
        const auto pSn = valor(PercentualTributoSn::de(u"6.00"));
        if (!pSn) {
            return std::nullopt;
        }
        return Tributacao::montar(*municipal, TotalTributos{*pSn});
    }
    const auto indicador = valor(IndicadorTotalTributos::de(u"0"));
    if (!indicador) {
        return std::nullopt;
    }
    return Tributacao::montar(*municipal, TotalTributos{*indicador});
}

/// Grupo IBS/CBS conforme o caso. Resultado externo: nullopt = falha; interno vazio = grupo ausente (intencional).
inline std::optional<std::optional<nl::back::Ibscbs>> ibscbsCom(const Opcoes& o)
{
    using namespace nl::back;
    if (!o.ibscbs) {
        return std::optional<Ibscbs>{};
    }
    const auto cst = valor(CodigoSituacaoTributaria::de(u"000"));
    const auto cClassTrib = valor(CodigoClassificacaoTributaria::de(u"000001"));
    const auto indDest = valor(IndicadorDestinatario::de(u"0"));
    if (!cst || !cClassTrib || !indDest) {
        return std::nullopt;
    }
    std::optional<TipoOperacao> tpOper;
    if (o.tpOper) {
        tpOper = valor(TipoOperacao::de(u"1"));
        if (!tpOper) {
            return std::nullopt;
        }
    }
    const auto trib = TributacaoIbscbs::montar(*cst, *cClassTrib);
    const auto ibs = valor(Ibscbs::montar(std::nullopt, tpOper, std::nullopt, *indDest, trib));
    if (!ibs) {
        return std::nullopt;
    }
    return std::optional<Ibscbs>{*ibs};
}

/// DPS montada a partir das opções. nullopt = falha de montagem (o teste acusa com QVERIFY).
inline std::optional<nl::back::Dps> dpsCom(const Opcoes& o)
{
    using namespace nl::back;
    const auto tpAmb = valor(TipoAmbiente::de(u"2"));
    const auto verAplic = valor(VerAplic::de(u"NotaLivre 0.1", "verAplic"));
    const auto serie = valor(Serie::de(1));
    const auto nDps = valor(NumeroDps::de(1));
    const auto cLocEmi = valor(CodigoMunicipio::de(u"4305108"));
    const auto cLocPrestacao = valor(CodigoMunicipio::de(QString::fromStdString(o.cLocPrestacao)));
    const auto cTribNac = valor(CodigoTribNacional::de(QString::fromStdString(o.cTribNac)));
    const auto xDescServ = valor(DescricaoServico::de(u"Consultoria em tecnologia"));
    const auto vServ = valor(Valor::de(10000));
    const auto prest = prestadorCom(o);
    const auto toma = tomadorCom(o);
    const auto tributacao = tributacaoCom(o);
    const auto ibs = ibscbsCom(o);
    if (!tpAmb || !verAplic || !serie || !nDps || !cLocEmi || !cLocPrestacao || !cTribNac || !xDescServ || !vServ || !prest
        || !toma || !tributacao || !ibs) {
        return std::nullopt;
    }
    const auto servico = Servico::montar(*cLocPrestacao, *cTribNac, *xDescServ, std::nullopt);
    const auto valores = ValoresServico::montar(*vServ, *tributacao);
    DadosDps dados{
        .tpAmb = *tpAmb,
        .dhEmi = QDateTime{QDate{2026, 10, 25}, QTime{10, 0}, QTimeZone::utc()},
        .verAplic = *verAplic,
        .serie = *serie,
        .nDps = *nDps,
        .dCompet = o.dCompet,
        .cLocEmi = *cLocEmi,
        .subst = std::nullopt,
        .prest = *prest,
        .toma = *toma,
        .serv = servico,
        .valores = valores,
        .ibscbs = *ibs,
    };
    return valor(Dps::montar(std::move(dados)));
}

} // namespace teste

#endif // NL_TESTE_BACK_REGRAS_FABRICA_DPS_H
