// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Confirmação forte da inscrição (NL-2.9, item 2). Ver confirmacao_inscricao.h. Stub de teste: o corpo real vem no código.
#include <nl/mid/composicao/confirmacao_inscricao.h>

namespace nl::mid::composicao {

std::expected<QString, nl::base::Erro> chaveDeInscricaoDigitada(QStringView digitado)
{
    static_cast<void>(digitado);
    return QString();
}

QString chaveDeInscricaoDoA1(const nl::foundation::pkcs12::Inscricao& a1)
{
    static_cast<void>(a1);
    return QString();
}

std::expected<void, nl::base::Erro> conferirConfirmacao(QStringView digitado, const nl::foundation::pkcs12::Inscricao& a1,
                                                        QStringView chaveDoEmitente)
{
    static_cast<void>(digitado);
    static_cast<void>(a1);
    static_cast<void>(chaveDoEmitente);
    return {};
}

} // namespace nl::mid::composicao
