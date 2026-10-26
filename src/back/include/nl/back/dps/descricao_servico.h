// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Descrição do serviço (xDescServ, NL-1.4, DA-1.2). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_DESCRICAO_SERVICO_H
#define NL_BACK_DPS_DESCRICAO_SERVICO_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Descrição do serviço: não vazia e com pelo menos um caractere que não seja espaço, tab, CR ou LF
/// (o whitespace do XML Schema, tiposSimples_v1.01.xsd:1439 TSDesc2000, pattern na linha 1443).
/// Sem limite de tamanho aqui: o limite é da versão do leiaute (NL-1.6). Invariante: só se constrói por DescricaoServico::de.
class DescricaoServico
{
public:
    /// Erros: "servico.xDescServ.vazia" (sem caractere), "servico.xDescServ.sem_texto" (só whitespace do XSD).
    static std::expected<DescricaoServico, nl::base::Erro> de(QStringView entrada);

    QString texto() const;

    bool operator==(const DescricaoServico& outro) const = default;

private:
    explicit DescricaoServico(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_DESCRICAO_SERVICO_H
