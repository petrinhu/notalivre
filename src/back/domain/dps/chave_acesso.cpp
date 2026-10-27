// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação da chave de acesso mínima (NL-1.3). Posições: 0-8 dígitos, 9-22 de [0-9A-Z], 23-49 dígitos (xsd:41, sem "NFS").

#include <nl/back/dps/chave_acesso.h>

#include <optional>
#include <string>
#include <utility>

namespace nl::back {
namespace {

constexpr qsizetype kTamanho = 50;
constexpr qsizetype kFimDigitosIniciais = 9;
constexpr qsizetype kFimAlfanumericos = 23;

bool digito(char16_t c)
{
    return c >= u'0' && c <= u'9';
}

bool digitoOuMaiuscula(char16_t c)
{
    return digito(c) || (c >= u'A' && c <= u'Z');
}

bool posicaoValida(qsizetype posicao, char16_t c)
{
    if (posicao < kFimDigitosIniciais) {
        return digito(c);
    }
    if (posicao < kFimAlfanumericos) {
        return digitoOuMaiuscula(c);
    }
    return digito(c);
}

} // namespace

std::expected<ChaveAcesso, nl::base::Erro> ChaveAcesso::de(QStringView entrada)
{
    if (entrada.size() != kTamanho) {
        return std::unexpected(nl::base::Erro{"chave.tamanho", std::nullopt, "chave de acesso deve ter 50 posições"});
    }
    for (qsizetype posicao = 0; posicao < kTamanho; ++posicao) {
        if (!posicaoValida(posicao, entrada[posicao].unicode())) {
            return std::unexpected(nl::base::Erro{"chave.formato", std::nullopt,
                                                  "chave de acesso fora do padrão na posição " + std::to_string(posicao + 1)});
        }
    }
    return ChaveAcesso{entrada.toString()};
}

ChaveAcesso::ChaveAcesso(QString texto)
    : texto_{std::move(texto)}
{
}

QString ChaveAcesso::texto() const
{
    return texto_;
}

} // namespace nl::back
