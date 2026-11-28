// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada de validação da DPS (NL-1.7; NL-3.M): JSON de entrada -> DadosDps -> Dps -> regras locais -> XML do leiaute -> XSD. É a única
// porta que a CLI usa; a CLI não inclui nl/back (NF-02), por isso os avisos têm tipo próprio aqui.
#ifndef NL_MID_COMPOSICAO_DPS_VALIDAR_H
#define NL_MID_COMPOSICAO_DPS_VALIDAR_H

#include <nl/base/erro.h>

#include <QByteArray>
#include <QByteArrayView>
#include <QString>

#include <expected>
#include <string>
#include <vector>

namespace nl::mid::composicao {

/// Onde estão o XSD de Produção Restrita já extraído e o manifesto de SHA-256 que confere cada arquivo (NL-1.6a).
/// A raiz do esquema (DPS_v1.01.xsd) é fixa do leiaute e fica dentro da fachada.
struct LocalizacaoEsquemaDps
{
    QString diretorio;
    QString manifesto;
};

/// Onde estão as tabelas do Anexo A (NL-3.M, DA-3M.4): o diretório com os três JSON e o SHA256SUMS. O pino do SHA256SUMS é
/// embutido no binário; o diretório vem da configuração ou do pacote.
struct LocalizacaoTabelas
{
    QString diretorio;
};

/// Aviso da validação (NL-3.M, DA-3M.3): não impede gravar nem emitir. Tipo próprio da fachada, porque nl::back::Aviso não sai da Mid.
struct AvisoDps
{
    std::string codigo;  ///< código D (por exemplo "D1")
    std::string campo;   ///< caminho do leiaute do campo, por exemplo "Dps.infDPS.toma.end"
    std::string mensagem;
};

/// Quem reprovou a DPS. reprovada: a entrada, as regras locais ou o XML não passaram (a CLI sai com rc 1).
/// ambiente: o esquema, o manifesto ou as tabelas não estão disponíveis, ou a libxml2 falhou (a CLI sai com rc 2). Não é culpa da DPS.
enum class NaturezaFalha { reprovada, ambiente };

struct FalhaDpsValidar
{
    NaturezaFalha natureza;
    std::vector<nl::base::Erro> erros; ///< sempre pelo menos um
    /// Avisos da etapa das regras locais, quando ela rodou antes da falha (NL-3.M, DA-3M.3). Vazio nas falhas de entrada e de carga.
    std::vector<AvisoDps> avisos = {};
};

/// DPS validada: o XML que vai à gravação ou à assinatura, e os avisos da etapa das regras locais (NL-3.M).
struct DpsValidada
{
    QByteArray xml;
    std::vector<AvisoDps> avisos;
};

/// Monta a DPS a partir de um JSON de entrada e devolve o XML já validado no XSD, depois das regras locais (NL-3.M, DA-3M.2).
/// Ordem das etapas, com parada na primeira que reprova: entrada, montagem, catálogo de localidades, regras locais, XML, XSD. Dentro
/// da etapa das regras saem todos os erros.
/// Entrada malformada ou campo obrigatório ausente vêm com código "entrada.*"; regra de domínio vem com o código do Back
/// (por exemplo "cpf.dv", "dps.dcompet"); regra local vem com o código E da regra (por exemplo "E0302"); reprovação do XSD vem com
/// "xsd.invalido"; esquema ausente com "xsd.esquema"; tabelas ausentes ou divergentes com "tabelas.*" (natureza ambiente).
class DpsValidar
{
public:
    DpsValidar() = delete;

    static std::expected<DpsValidada, FalhaDpsValidar> executar(QByteArrayView entradaJson, const LocalizacaoEsquemaDps& esquema,
                                                                const LocalizacaoTabelas& tabelas);

    /// Revalida um XML já pronto no XSD (NL-2.9, DA-2.51): o XML assinado é o que vai à Sefin, então é validado de novo antes do
    /// envio. Reprovação sai com NaturezaFalha::reprovada.
    static std::expected<void, FalhaDpsValidar> validarXml(QByteArrayView xml, const LocalizacaoEsquemaDps& localizacao);
};

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_DPS_VALIDAR_H
