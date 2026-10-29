// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de contrato do gzip da Foundation (NL-2.4, DA-2.4). Valem para qualquer implementação: o formato é o RFC 1952,
// que é FATO (não é INFERÊNCIA). O que ainda não está decidido é a biblioteca de compressão, não o formato.
// Conferência independente: o CRC-32 é recalculado aqui, sem reuso de código de produção.

#include <nl/foundation/gzip/gzip.h>

#include <QtTest>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gz = nl::foundation::gzip;

namespace {

/// Carga de teste com bytes de 0x00 a 0xFF repetidos.
std::vector<std::byte> cargaDeTeste(std::size_t tamanho)
{
    std::vector<std::byte> carga;
    carga.reserve(tamanho);
    for (std::size_t indice = 0; indice < tamanho; ++indice) {
        carga.push_back(static_cast<std::byte>(indice % 256));
    }
    return carga;
}

/// CRC-32 do gzip (polinômio refletido 0xEDB88320, RFC 1952 seção 8), bit a bit. Conferência independente.
std::uint32_t crc32Gzip(const std::vector<std::byte>& dados)
{
    std::uint32_t crc = 0xFFFFFFFFu;
    for (const std::byte byte : dados) {
        crc ^= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(byte));
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1u) != 0 ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

/// Inteiro de 32 bits em little-endian a partir do índice dado.
std::uint32_t lerLittleEndian32(const std::vector<std::byte>& dados, std::size_t inicio)
{
    std::uint32_t valor = 0;
    for (std::size_t deslocamento = 0; deslocamento < 4; ++deslocamento) {
        const auto byte = static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(dados[inicio + deslocamento]));
        valor |= byte << (8 * deslocamento);
    }
    return valor;
}

/// Byte de posição dada como inteiro.
int byteEm(const std::vector<std::byte>& dados, std::size_t posicao)
{
    return static_cast<int>(std::to_integer<std::uint8_t>(dados[posicao]));
}

} // namespace

class TstGzip : public QObject
{
    Q_OBJECT

private slots:
    void gzipComCabecalhoRfc1952();
    void naoUsaQCompress();
};

void TstGzip::gzipComCabecalhoRfc1952()
{
    // RFC 1952 seções 2.3 e 2.3.1: ID1 = 0x1f, ID2 = 0x8b, CM = 8 (deflate). O rodapé traz CRC-32 e ISIZE (tamanho
    // original mod 2^32), os últimos 8 bytes, em little-endian.
    const std::vector<std::byte> entrada = cargaDeTeste(1000);
    const auto saida = gz::comprimir(entrada);
    QVERIFY2(saida.has_value(), "comprimir recusou uma entrada válida");
    const std::vector<std::byte>& gzip = *saida;

    QVERIFY2(gzip.size() >= 18, "gzip menor que o cabeçalho mais o rodapé");
    QCOMPARE(byteEm(gzip, 0), 0x1f);
    QCOMPARE(byteEm(gzip, 1), 0x8b);
    QCOMPARE(byteEm(gzip, 2), 0x08);

    const std::size_t inicioRodape = gzip.size() - 8;
    QCOMPARE(lerLittleEndian32(gzip, inicioRodape), crc32Gzip(entrada));
    QCOMPARE(lerLittleEndian32(gzip, inicioRodape + 4), static_cast<std::uint32_t>(entrada.size()));
}

void TstGzip::naoUsaQCompress()
{
    // qCompress do Qt grava 4 bytes com o tamanho original em big-endian, seguidos de zlib (RFC 1950). Não é gzip.
    // A saída tem de começar por 1f 8b e não pode começar pelo prefixo de tamanho nem pelo CMF de zlib (0x78).
    const std::vector<std::byte> entrada = cargaDeTeste(1000);
    const auto saida = gz::comprimir(entrada);
    QVERIFY2(saida.has_value(), "comprimir recusou uma entrada válida");
    const std::vector<std::byte>& gzip = *saida;

    const auto tamanho = static_cast<std::uint32_t>(entrada.size());
    const bool prefixoDoQCompress = byteEm(gzip, 0) == static_cast<int>((tamanho >> 24) & 0xFFu)
                                    && byteEm(gzip, 1) == static_cast<int>((tamanho >> 16) & 0xFFu)
                                    && byteEm(gzip, 2) == static_cast<int>((tamanho >> 8) & 0xFFu)
                                    && byteEm(gzip, 3) == static_cast<int>(tamanho & 0xFFu);
    QVERIFY2(!prefixoDoQCompress, "a saída começa com o prefixo de tamanho do qCompress");
    QVERIFY2(byteEm(gzip, 0) != 0x78, "a saída começa com o CMF de zlib cru");
    QCOMPARE(byteEm(gzip, 0), 0x1f);
    QCOMPARE(byteEm(gzip, 1), 0x8b);
}

QTEST_GUILESS_MAIN(TstGzip)
#include "tst_gzip.moc"
