// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Confirmação forte da inscrição (NL-2.9, item 2). Ver confirmacao_inscricao.h. A normalização é a do Back (Cnpj::de e Cpf::de).
#include <nl/mid/composicao/confirmacao_inscricao.h>

#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>

#include <optional>

namespace nl::mid::composicao {
namespace {

// Os três zeros à esquerda que o Id da DPS usa para o CPF (DA-2.2): a inscrição tem 14 posições nos dois tipos.
QString comZerosDoCpf(const QString& cpf)
{
    return QStringLiteral("000") + cpf;
}

} // namespace

std::expected<QString, nl::base::Erro> chaveDeInscricaoDigitada(QStringView digitado)
{
    if (auto cnpj = nl::back::Cnpj::de(digitado)) {
        return cnpj->texto();
    }
    if (auto cpf = nl::back::Cpf::de(digitado)) {
        return comZerosDoCpf(cpf->texto());
    }
    return std::unexpected(nl::base::Erro("confirmacao.invalida", std::nullopt, "a confirmação não é um CNPJ nem um CPF válido"));
}

QString chaveDeInscricaoDoA1(const nl::foundation::pkcs12::Inscricao& a1)
{
    if (a1.tipo() == nl::foundation::pkcs12::TipoInscricao::Cpf) {
        return comZerosDoCpf(a1.valor());
    }
    return a1.valor();
}

std::expected<void, nl::base::Erro> conferirConfirmacao(QStringView digitado, const nl::foundation::pkcs12::Inscricao& a1,
                                                        QStringView chaveDoEmitente)
{
    auto chave = chaveDeInscricaoDigitada(digitado);
    if (!chave) {
        return std::unexpected(chave.error());
    }
    if (*chave != chaveDeInscricaoDoA1(a1)) {
        return std::unexpected(nl::base::Erro("confirmacao.diferente_do_a1", std::nullopt,
                                              "a confirmação não bate com a inscrição do certificado A1"));
    }
    if (*chave != chaveDoEmitente.toString()) {
        return std::unexpected(nl::base::Erro("confirmacao.diferente_do_emitente", std::nullopt,
                                              "a confirmação não bate com o emitente da DPS"));
    }
    return {};
}

} // namespace nl::mid::composicao
