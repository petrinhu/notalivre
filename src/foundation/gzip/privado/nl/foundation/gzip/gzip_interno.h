// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Costuras internas do motor gzip (NL-2.4, rodada 2 da revisão). Não é API pública: só o gzip.cpp e os testes usam.
// Não inclui nada da zlib: os códigos abaixo têm os mesmos valores da zlib, conferidos por static_assert no gzip.cpp.

#ifndef NL_FOUNDATION_GZIP_GZIP_INTERNO_H
#define NL_FOUNDATION_GZIP_GZIP_INTERNO_H

#include <nl/base/erro.h>

#include <cstddef>
#include <expected>
#include <functional>
#include <span>
#include <vector>

namespace nl::foundation::gzip::detalhe {

inline constexpr int kPassoOk = 0;               ///< Z_OK: mais saída virá
inline constexpr int kPassoFim = 1;              ///< Z_STREAM_END: a compactação terminou
inline constexpr int kPassoErroFluxo = -2;       ///< Z_STREAM_ERROR: a zlib recusou o fluxo
inline constexpr int kPassoSemProgresso = -5;    ///< Z_BUF_ERROR: nenhum progresso possível

/// Resultado de um passo de compactação: o código e quantos bytes ele escreveu no bloco de saída.
struct PassoCompressao
{
    int codigo;
    std::size_t produzido;
};

/// Um passo sobre o bloco de saída. A produção chama a zlib; os testes trocam por uma função falsa.
using PassoFn = std::function<PassoCompressao(std::span<std::byte> bloco)>;

/// Chama o passo até o fim e junta os blocos na ordem. Erro (gzip.compactacao) se o passo recusar o fluxo ou parar
/// sem progresso antes do fim.
[[nodiscard]] std::expected<std::vector<std::byte>, nl::base::Erro> acumularSaida(const PassoFn& passo);

/// A entrada cabe no avail_in da zlib (uInt)? Acima disso comprimir recusa, em vez de truncar o tamanho.
[[nodiscard]] bool entradaCabeNaZlib(std::size_t tamanho) noexcept;

/// comprimir com limite explícito: entrada acima de `limite` recusa com gzip.tamanho, antes de tocar na zlib. A produção
/// passa o maior uInt; o teste passa um limite pequeno, para exercitar a fiação sem alocar 4 GiB (revisão O1 da NL-2.4).
[[nodiscard]] std::expected<std::vector<std::byte>, nl::base::Erro> comprimirComLimite(std::span<const std::byte> entrada,
                                                                                       std::size_t limite);

} // namespace nl::foundation::gzip::detalhe

#endif // NL_FOUNDATION_GZIP_GZIP_INTERNO_H
