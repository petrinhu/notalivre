// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Inscrição do titular (NL-2.2). Validação de formato na fábrica; o DV não é verificado aqui (é do domínio, Back).
#include <nl/foundation/pkcs12/inscricao.h>

#include <utility>

namespace nl::foundation::pkcs12 {

namespace {

bool digito(char16_t unidade) {
    return unidade >= u'0' && unidade <= u'9';
}

bool caractereDeCnpj(char16_t unidade) {
    return digito(unidade) || (unidade >= u'A' && unidade <= u'Z');
}

} // namespace

std::optional<Inscricao> Inscricao::cnpj(QStringView texto) {
    if (texto.size() != 14) {
        return std::nullopt;
    }
    for (qsizetype i = 0; i < 12; ++i) {
        if (!caractereDeCnpj(texto[i].unicode())) {
            return std::nullopt;
        }
    }
    for (qsizetype i = 12; i < 14; ++i) {
        if (!digito(texto[i].unicode())) {
            return std::nullopt;
        }
    }
    return Inscricao(TipoInscricao::Cnpj, texto.toString());
}

std::optional<Inscricao> Inscricao::cpf(QStringView texto) {
    if (texto.size() != 11) {
        return std::nullopt;
    }
    for (qsizetype i = 0; i < 11; ++i) {
        if (!digito(texto[i].unicode())) {
            return std::nullopt;
        }
    }
    return Inscricao(TipoInscricao::Cpf, texto.toString());
}

Inscricao::Inscricao(TipoInscricao tipo, QString valor)
    : tipo_(tipo), valor_(std::move(valor)) {}

bool mesmaRaizCnpj(const Inscricao& primeiro, const Inscricao& segundo) {
    if (primeiro.tipo() != TipoInscricao::Cnpj || segundo.tipo() != TipoInscricao::Cnpj) {
        return false;
    }
    // Oito primeiros caracteres como texto: o CNPJ alfanumérico não é número.
    // Oito primeiros caracteres como texto: o CNPJ alfanumérico não é número.
    return QStringView(primeiro.valor()).first(8) == QStringView(segundo.valor()).first(8);
}

} // namespace nl::foundation::pkcs12
