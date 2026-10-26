// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de DescricaoServico (NL-1.4, DA-1.2). Whitespace do XSD: espaço, tab, CR e LF.

#include <nl/back/dps/descricao_servico.h>

#include <utility>

namespace nl::back {
namespace {

bool espacoDoXsd(QChar caractere)
{
    const char16_t c = caractere.unicode();
    return c == u' ' || c == u'\t' || c == u'\r' || c == u'\n';
}

} // namespace

std::expected<DescricaoServico, nl::base::Erro> DescricaoServico::de(QStringView entrada)
{
    if (entrada.isEmpty()) {
        return std::unexpected(nl::base::Erro{"servico.xDescServ.vazia", std::string{"serv/cServ/xDescServ"},
                                              "descrição do serviço não pode ser vazia"});
    }
    for (const QChar caractere : entrada) {
        if (!espacoDoXsd(caractere)) {
            return DescricaoServico{entrada.toString()};
        }
    }
    return std::unexpected(nl::base::Erro{"servico.xDescServ.sem_texto", std::string{"serv/cServ/xDescServ"},
                                          "descrição do serviço só pode ter espaços"});
}

QString DescricaoServico::texto() const
{
    return texto_;
}

DescricaoServico::DescricaoServico(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
