// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada de validação da DPS (NL-1.7): JSON de entrada -> DadosDps -> XML do leiaute -> XSD. É a única porta que a CLI usa.
#ifndef NL_MID_COMPOSICAO_DPS_VALIDAR_H
#define NL_MID_COMPOSICAO_DPS_VALIDAR_H

#include <nl/base/erro.h>

#include <QByteArray>
#include <QByteArrayView>
#include <QString>

#include <expected>
#include <vector>

namespace nl::mid::composicao {

/// Onde estão o XSD de Produção Restrita já extraído e o manifesto de SHA-256 que confere cada arquivo (NL-1.6a).
/// A raiz do esquema (DPS_v1.01.xsd) é fixa do leiaute e fica dentro da fachada.
struct LocalizacaoEsquemaDps
{
    QString diretorio;
    QString manifesto;
};

/// Quem reprovou a DPS. reprovada: a entrada ou o XML não passaram (a CLI sai com rc 1).
/// ambiente: o esquema ou o manifesto não estão disponíveis, ou a libxml2 falhou (a CLI sai com rc 2). Não é culpa da DPS.
enum class NaturezaFalha { reprovada, ambiente };

struct FalhaDpsValidar
{
    NaturezaFalha natureza;
    std::vector<nl::base::Erro> erros; ///< sempre pelo menos um
};

/// Monta a DPS a partir de um JSON de entrada e devolve o XML já validado no XSD.
/// Entrada malformada ou campo obrigatório ausente vêm com código "entrada.*"; regra de domínio vem com o código do Back
/// (por exemplo "cpf.dv", "dps.dcompet"); reprovação do XSD vem com "xsd.invalido"; esquema ausente com "xsd.esquema".
class DpsValidar
{
public:
    DpsValidar() = delete;

    static std::expected<QByteArray, FalhaDpsValidar> executar(QByteArrayView entradaJson, const LocalizacaoEsquemaDps& localizacao);

    /// Revalida um XML já pronto no XSD (NL-2.9, DA-2.51): o XML assinado é o que vai à Sefin, então é validado de novo antes do
    /// envio. Reprovação sai com NaturezaFalha::reprovada. Stub de teste: o corpo real vem no commit de código.
    static std::expected<void, FalhaDpsValidar> validarXml(QByteArrayView xml, const LocalizacaoEsquemaDps& localizacao);
};

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_DPS_VALIDAR_H
