// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Argon2id da Foundation de cripto (NL-3.2b, ADR-0006, versão 1.3, lanes 1). Motor agnóstico.
// Parâmetros lidos do registro são conferidos contra piso e teto ANTES de derivar: o parâmetro é usado antes de a tag
// poder falhar (ADR-0006, item 5), então um valor adulterado não pode alocar memória sem limite.

#ifndef NL_FOUNDATION_CRIPTO_ARGON2ID_H
#define NL_FOUNDATION_CRIPTO_ARGON2ID_H

#include <nl/base/erro.h>

#include <QByteArray>
#include <QByteArrayView>
#include <QtGlobal>

#include <expected>
#include <span>

namespace nl::foundation::cripto {

/// Parâmetros gravados junto do dado: memória em bytes (memlimit), passes (opslimit) e salt de 16 bytes.
struct ParametrosArgon2id {
    qint64 memlimit;
    qint64 opslimit;
    QByteArray salt;
};

// ADR-0006, item 2: 128 MiB por decisão do líder de 05/11/2026; 6 passes por decisão do CISO de 06/11/2026.
inline constexpr qint64 kMemlimitCriacao = 134217728;
inline constexpr qint64 kOpslimitCriacao = 6;
// ADR-0006, item 5: piso de aceitação ao ler (OWASP: 19 MiB e 2 passes).
inline constexpr qint64 kMemlimitPiso = 19 * 1024 * 1024;
inline constexpr qint64 kOpslimitPiso = 2;
// ADR-0006, item 5 com o ajuste do CISO: teto ao ler, 256 MiB e 16 passes. Sobe por versão do app.
inline constexpr qint64 kMemlimitTeto = 268435456;
inline constexpr qint64 kOpslimitTeto = 16;

/// Deriva a área de saída (qualquer tamanho de 16 bytes ou mais) a partir da entrada, com os parâmetros recebidos.
/// Salt diferente de 16 bytes: cripto.tamanho. Abaixo do piso: cripto.parametro_abaixo_do_piso. Acima do teto:
/// cripto.parametro_acima_do_teto, sem alocar. Falha da biblioteca: cripto.biblioteca.
[[nodiscard]] std::expected<void, nl::base::Erro> argon2id(QByteArrayView entrada, const ParametrosArgon2id& parametros,
                                                            std::span<unsigned char> saida);

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_ARGON2ID_H
