// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Confirmação forte da inscrição em Produção (NL-2.9, item 2 da triagem do CISO, DA-2.14): o CPF ou CNPJ digitado tem de bater,
// depois da normalização do Back, com a inscrição do A1 e com a do emitente da DPS. Função pura, sem I/O.
#ifndef NL_MID_COMPOSICAO_CONFIRMACAO_INSCRICAO_H
#define NL_MID_COMPOSICAO_CONFIRMACAO_INSCRICAO_H

#include <nl/base/erro.h>
#include <nl/foundation/pkcs12/inscricao.h>

#include <QString>
#include <QStringView>

#include <expected>

namespace nl::mid::composicao {

/// Forma de 14 posições da inscrição, a mesma do Id da DPS (DA-2.2): CNPJ como está; CPF com três zeros à esquerda.
/// Entrada que não é CNPJ nem CPF válido vem com "confirmacao.invalida".
std::expected<QString, nl::base::Erro> chaveDeInscricaoDigitada(QStringView digitado);

/// Forma de 14 posições da inscrição do A1, na mesma convenção de chaveDeInscricaoDigitada.
QString chaveDeInscricaoDoA1(const nl::foundation::pkcs12::Inscricao& a1);

/// Recusa "confirmacao.diferente_do_a1" se o digitado não bate com o A1, e "confirmacao.diferente_do_emitente" se bate com o
/// A1 mas não com a chave de 14 posições do emitente da DPS (o Id da DPS, posições 12 a 25).
std::expected<void, nl::base::Erro> conferirConfirmacao(QStringView digitado, const nl::foundation::pkcs12::Inscricao& a1,
                                                        QStringView chaveDoEmitente);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_CONFIRMACAO_INSCRICAO_H
