// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// NL-2.2, fase vermelha: as fábricas ainda recusam tudo.
#include <nl/foundation/pkcs12/inscricao.h>

namespace nl::foundation::pkcs12 {

std::optional<Inscricao> Inscricao::cnpj(QStringView) {
    return std::nullopt;
}

std::optional<Inscricao> Inscricao::cpf(QStringView) {
    return std::nullopt;
}

bool mesmaRaizCnpj(const Inscricao&, const Inscricao&) {
    return false;
}

} // namespace nl::foundation::pkcs12
