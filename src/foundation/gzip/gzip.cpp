// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Compactação gzip (RFC 1952) sobre a zlib do sistema (NL-2.4, autorizada pelo líder em 29/10/2026). A zlib entra só
// aqui. Com windowBits 15 + 16 a própria zlib escreve o cabeçalho e o rodapé gzip (CRC-32 e ISIZE).

#include <nl/foundation/gzip/gzip.h>
#include <nl/foundation/gzip/gzip_interno.h>

#include <nl/base/erro.h>

#include <zlib.h>

#include <cstddef>
#include <expected>
#include <limits>
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

// Os códigos das costuras são os da zlib: se algum divergir, a compilação para aqui.
static_assert(detalhe::kPassoOk == Z_OK);
static_assert(detalhe::kPassoFim == Z_STREAM_END);
static_assert(detalhe::kPassoErroFluxo == Z_STREAM_ERROR);
static_assert(detalhe::kPassoSemProgresso == Z_BUF_ERROR);

nl::base::Erro erroGzip(std::string codigo, std::string mensagem)
{
    return nl::base::Erro{std::move(codigo), std::nullopt, std::move(mensagem)};
}

} // namespace

namespace detalhe {

std::expected<std::vector<std::byte>, nl::base::Erro> acumularSaida(const PassoFn& passo)
{
    std::vector<std::byte> saida;
    std::vector<std::byte> bloco(kTamanhoBloco);
    for (;;) {
        const PassoCompressao resultado = passo(bloco);
        const bool conhecido = resultado.codigo == kPassoOk || resultado.codigo == kPassoFim
                               || resultado.codigo == kPassoSemProgresso;
        // Sem produção de bytes só é aceitável no fim. Ok sem bytes e Z_BUF_ERROR sem bytes são paradas sem progresso.
        const bool progrediu = resultado.produzido > 0 || resultado.codigo == kPassoFim;
        if (!conhecido || !progrediu || resultado.produzido > bloco.size()) {
            return std::unexpected(erroGzip("gzip.compactacao", "a zlib parou de compactar antes do fim"));
        }
        saida.insert(saida.end(), bloco.begin(), bloco.begin() + static_cast<std::ptrdiff_t>(resultado.produzido));
        if (resultado.codigo == kPassoFim) {
            return saida;
        }
    }
}

bool entradaCabeNaZlib(std::size_t tamanho) noexcept
{
    return tamanho <= static_cast<std::size_t>(std::numeric_limits<uInt>::max());
}

} // namespace detalhe

std::expected<std::vector<std::byte>, nl::base::Erro> comprimir(std::span<const std::byte> entrada)
{
    if (!detalhe::entradaCabeNaZlib(entrada.size())) {
        return std::unexpected(erroGzip("gzip.tamanho", "entrada acima do limite da zlib (uInt); não foi truncada"));
    }

    z_stream fluxo{};
    if (deflateInit2(&fluxo, Z_DEFAULT_COMPRESSION, Z_DEFLATED, kJanelaGzip, kMemoriaPadrao, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::unexpected(erroGzip("gzip.init", "a zlib não iniciou a compactação gzip"));
    }

    // A zlib não altera a entrada; a API declara next_in sem const, por isso o const_cast.
    fluxo.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(entrada.data()));
    fluxo.avail_in = static_cast<uInt>(entrada.size());

    const detalhe::PassoFn passo = [&fluxo](std::span<std::byte> bloco) -> detalhe::PassoCompressao {
        fluxo.next_out = reinterpret_cast<Bytef*>(bloco.data());
        fluxo.avail_out = static_cast<uInt>(bloco.size());
        const int codigo = deflate(&fluxo, Z_FINISH);
        return {codigo, bloco.size() - fluxo.avail_out};
    };
    auto saida = detalhe::acumularSaida(passo);
    deflateEnd(&fluxo);
    return saida;
}

} // namespace nl::foundation::gzip
