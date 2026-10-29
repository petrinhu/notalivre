// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Classificação dos erros do OpenSSL na decifragem do .pfx (NL-2.2, revisão 3, A7). Interno ao módulo: exposto
// só para teste. Classifica pela biblioteca e pelo motivo do código, nunca pela mensagem em texto.
#ifndef NL_FOUNDATION_PKCS12_CLASSIFICACAO_ERROS_H
#define NL_FOUNDATION_PKCS12_CLASSIFICACAO_ERROS_H

#include <vector>

namespace nl::foundation::pkcs12::interno {

/// Motivo de uma tentativa de decifrar o .pfx.
enum class Falha {
    SenhaErrada,
    Unsupported,
    Malformado,
    LegadoIndisponivel,
    Interno,
};

/// Função pura: classifica a partir dos códigos de erro, sem tocar na fila do OpenSSL.
[[nodiscard]] Falha classificarCodigos(const std::vector<unsigned long>& codigos);

/// Esvazia a fila de erros do OpenSSL e devolve os códigos que estavam nela.
[[nodiscard]] std::vector<unsigned long> colherErrosOpenssl();

} // namespace nl::foundation::pkcs12::interno

#endif // NL_FOUNDATION_PKCS12_CLASSIFICACAO_ERROS_H
