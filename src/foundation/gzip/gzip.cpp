// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Compactação gzip (RFC 1952) sobre a zlib do sistema (NL-2.4, autorizada pelo líder em 29/10/2026). A zlib entra só
// aqui. Com windowBits 15 + 16 a própria zlib escreve o cabeçalho e o rodapé gzip (CRC-32 e ISIZE).

#include <nl/foundation/gzip/gzip.h>
#include <nl/foundation/gzip/gzip_interno.h>

#include <nl/base/erro.h>

#include <zlib.h>

#include <array>
#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace nl::foundation::gzip {
namespace {

constexpr int kJanelaGzip = 15 + 16;  // 15 bits de janela mais 16: formato gzip, não zlib cru (RFC 1952)
constexpr int kMemoriaPadrao = 8;     // memLevel padrão da zlib
constexpr std::size_t kTamanhoBloco = 16384;

nl::base::Erro erroGzip(std::string codigo, std::string mensagem)
{
    return nl::base::Erro{std::move(codigo), std::nullopt, std::move(mensagem)};
}

} // namespace

std::expected<std::vector<std::byte>, nl::base::Erro> comprimir(std::span<const std::byte> entrada)
{
    z_stream fluxo{};
    if (deflateInit2(&fluxo, Z_DEFAULT_COMPRESSION, Z_DEFLATED, kJanelaGzip, kMemoriaPadrao, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::unexpected(erroGzip("gzip.init", "a zlib não iniciou a compactação gzip"));
    }

    // A zlib não altera a entrada; a API declara next_in sem const, por isso o const_cast.
    fluxo.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(entrada.data()));
    fluxo.avail_in = static_cast<uInt>(entrada.size());

    std::vector<std::byte> saida;
    std::array<Bytef, kTamanhoBloco> bloco{};
    int resultado = Z_OK;
    do {
        fluxo.next_out = bloco.data();
        fluxo.avail_out = static_cast<uInt>(bloco.size());
        resultado = deflate(&fluxo, Z_FINISH);
        const std::size_t produzido = bloco.size() - fluxo.avail_out;
        if (resultado == Z_STREAM_ERROR || (resultado == Z_BUF_ERROR && produzido == 0)) {
            deflateEnd(&fluxo);
            return std::unexpected(erroGzip("gzip.compactacao", "a zlib parou de compactar antes do fim"));
        }
        const auto* inicio = reinterpret_cast<const std::byte*>(bloco.data());
        saida.insert(saida.end(), inicio, inicio + produzido);
    } while (resultado != Z_STREAM_END);

    deflateEnd(&fluxo);
    return saida;
}

namespace detalhe {

// STUB VERMELHO (rodada 2 da revisão): as costuras ainda não fazem nada. Os testes de tst_gzip têm de falhar aqui.
std::expected<std::vector<std::byte>, nl::base::Erro> acumularSaida(const PassoFn&)
{
    return std::unexpected(erroGzip("gzip.nao_implementado", "acumularSaida ainda não implementado"));
}

bool entradaCabeNaZlib(std::size_t) noexcept
{
    return false;
}

} // namespace detalhe

} // namespace nl::foundation::gzip
