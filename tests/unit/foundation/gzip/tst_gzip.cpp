// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do gzip da Foundation (NL-2.4, DA-2.4). O formato é o RFC 1952, que é FATO (não é INFERÊNCIA). A biblioteca é a
// zlib, autorizada pelo líder em 29/10/2026. Os bytes compactados não se comparam entre distros (o zlib-ng do Fedora
// gera outros bytes); o que se confere é a descompressão: o gzip da própria imagem tem de desfazer a saída.
// Conferências independentes: o CRC-32 é recalculado aqui, e a descompressão usa o binário gzip do sistema.

#include <nl/foundation/gzip/gzip.h>

#include <QByteArray>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
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

/// Resultado do oráculo gzip -dc: se o binário existe, se a descompressão deu certo e o que ela devolveu.
struct Descompressao
{
    bool encontrou = false;
    bool sucesso = false;
    QByteArray saida;
};

/// Passa o gzip ao binário gzip do sistema (gzip -dc). Não usa a zlib do app: é a conferência independente.
Descompressao descomprimirComGzipDoSistema(const std::vector<std::byte>& gzip)
{
    Descompressao resultado;
    const QString binario = QStandardPaths::findExecutable(QStringLiteral("gzip"));
    if (binario.isEmpty()) {
        return resultado;
    }
    resultado.encontrou = true;

    QProcess processo;
    processo.start(binario, {QStringLiteral("-dc")});
    if (!processo.waitForStarted(10000)) {
        return resultado;
    }
    processo.write(QByteArray(reinterpret_cast<const char*>(gzip.data()), static_cast<qsizetype>(gzip.size())));
    processo.closeWriteChannel();
    if (!processo.waitForFinished(30000)) {
        return resultado;
    }
    resultado.sucesso = processo.exitStatus() == QProcess::NormalExit && processo.exitCode() == 0;
    resultado.saida = processo.readAllStandardOutput();
    return resultado;
}

} // namespace

class TstGzip : public QObject
{
    Q_OBJECT

private slots:
    void gzipComCabecalhoRfc1952();
    void naoUsaQCompress();
    void oraculoGzipDescomprimeIgual();
    void gzipDeEntradaVaziaDescomprimeVazio();
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

void TstGzip::oraculoGzipDescomprimeIgual()
{
    // O gzip do sistema, na própria imagem, desfaz a saída de comprimir e devolve a entrada original byte a byte.
    const std::vector<std::byte> entrada = cargaDeTeste(1000);
    const auto saida = gz::comprimir(entrada);
    QVERIFY2(saida.has_value(), "comprimir recusou uma entrada válida");

    const Descompressao volta = descomprimirComGzipDoSistema(*saida);
    QVERIFY2(volta.encontrou, "binário gzip não achado no PATH: o oráculo não rodou");
    QVERIFY2(volta.sucesso, "gzip -dc recusou a saída de comprimir");
    QCOMPARE(volta.saida, QByteArray(reinterpret_cast<const char*>(entrada.data()), static_cast<qsizetype>(entrada.size())));
}

void TstGzip::gzipDeEntradaVaziaDescomprimeVazio()
{
    // Entrada de zero bytes é um gzip válido (RFC 1952). O oráculo tem de aceitá-lo e devolver zero bytes.
    const std::vector<std::byte> entrada;
    const auto saida = gz::comprimir(entrada);
    QVERIFY2(saida.has_value(), "comprimir recusou entrada vazia");
    QVERIFY2(saida->size() >= 18, "gzip de entrada vazia menor que cabeçalho mais rodapé");

    const Descompressao volta = descomprimirComGzipDoSistema(*saida);
    QVERIFY2(volta.encontrou, "binário gzip não achado no PATH: o oráculo não rodou");
    QVERIFY2(volta.sucesso, "gzip -dc recusou o gzip de entrada vazia");
    QVERIFY(volta.saida.isEmpty());
}

QTEST_GUILESS_MAIN(TstGzip)
#include "tst_gzip.moc"
