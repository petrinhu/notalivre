// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do gzip da Foundation (NL-2.4, DA-2.4). O formato é o RFC 1952, que é FATO (não é INFERÊNCIA). A biblioteca é a
// zlib, autorizada pelo líder em 29/10/2026. Os bytes compactados não se comparam entre distros (o zlib-ng do Fedora
// gera outros bytes); o que se confere é a descompressão: o gzip da própria imagem tem de desfazer a saída.
// Conferências independentes: o CRC-32 é recalculado aqui, e a descompressão usa o binário gzip do sistema.

#include <nl/foundation/gzip/gzip.h>
#include <nl/foundation/gzip/gzip_interno.h>

#include <QByteArray>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QtTest>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace gz = nl::foundation::gzip;
namespace detalhe = nl::foundation::gzip::detalhe;

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
    void gzipMaisDeUmBlocoDeSaida();
    void acumulaVariosBlocosEmOrdem();
    void erroDoFluxoZlibViraErro();
    void bufferSemProgressoViraErro();
    void entradaAcimaDoLimiteDaZlibRecusada();
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

void TstGzip::gzipMaisDeUmBlocoDeSaida()
{
    // C1 (revisão): 64 KiB de bytes pouco compressíveis (LCG determinístico). A saída passa de 16384 bytes, ou seja,
    // mais de um bloco de saída. Com o laço removido (mutante m5) a saída teria 16384 bytes e o teste cai.
    std::vector<std::byte> entrada;
    entrada.reserve(65536);
    std::uint32_t semente = 12345u;
    for (int indice = 0; indice < 65536; ++indice) {
        semente = semente * 1664525u + 1013904223u;
        entrada.push_back(static_cast<std::byte>(semente >> 24));
    }

    const auto saida = gz::comprimir(entrada);
    QVERIFY2(saida.has_value(), "comprimir recusou 64 KiB válidos");
    QVERIFY2(saida->size() > 16384, "a saída coube num bloco: o laço de blocos não foi exercitado");
    QCOMPARE(lerLittleEndian32(*saida, saida->size() - 8), crc32Gzip(entrada));
    QCOMPARE(lerLittleEndian32(*saida, saida->size() - 4), static_cast<std::uint32_t>(entrada.size()));

    const Descompressao volta = descomprimirComGzipDoSistema(*saida);
    QVERIFY2(volta.encontrou, "binário gzip não achado no PATH: o oráculo não rodou");
    QVERIFY2(volta.sucesso, "gzip -dc recusou a saída de mais de um bloco");
    QCOMPARE(volta.saida, QByteArray(reinterpret_cast<const char*>(entrada.data()), static_cast<qsizetype>(entrada.size())));
}

void TstGzip::acumulaVariosBlocosEmOrdem()
{
    // Três passos: um bloco cheio de 1, um cheio de 2 e um parcial de 5 bytes de 3, depois o fim. A saída tem 32773
    // bytes na ordem dos passos. Com um só passo (mutante m5) sairia 16384 bytes e o teste cai.
    int chamada = 0;
    const detalhe::PassoFn passo = [&chamada](std::span<std::byte> bloco) -> detalhe::PassoCompressao {
        ++chamada;
        if (chamada == 1 || chamada == 2) {
            std::fill(bloco.begin(), bloco.end(), static_cast<std::byte>(chamada));
            return {detalhe::kPassoOk, bloco.size()};
        }
        if (chamada == 3) {
            std::fill(bloco.begin(), bloco.begin() + 5, static_cast<std::byte>(3));
            return {detalhe::kPassoOk, 5};
        }
        return {detalhe::kPassoFim, 0};
    };

    const auto saida = detalhe::acumularSaida(passo);
    QVERIFY2(saida.has_value(), "acumularSaida recusou três blocos válidos");
    QCOMPARE(saida->size(), std::size_t{32773});
    QCOMPARE(byteEm(*saida, 0), 1);
    QCOMPARE(byteEm(*saida, 16383), 1);
    QCOMPARE(byteEm(*saida, 16384), 2);
    QCOMPARE(byteEm(*saida, 32767), 2);
    QCOMPARE(byteEm(*saida, 32768), 3);
    QCOMPARE(byteEm(*saida, 32772), 3);
}

void TstGzip::erroDoFluxoZlibViraErro()
{
    // C2 (revisão): Z_STREAM_ERROR tem de virar erro, nunca saída. O segundo passo é o fim, para que um mutante sem a
    // checagem (m8) termine e caia por asserção, sem laço infinito.
    int chamada = 0;
    const detalhe::PassoFn passo = [&chamada](std::span<std::byte>) -> detalhe::PassoCompressao {
        ++chamada;
        return chamada == 1 ? detalhe::PassoCompressao{detalhe::kPassoErroFluxo, 0}
                            : detalhe::PassoCompressao{detalhe::kPassoFim, 0};
    };
    const auto saida = detalhe::acumularSaida(passo);
    QVERIFY2(!saida.has_value(), "erro de fluxo da zlib virou saída");
    QCOMPARE(QString::fromStdString(saida.error().codigo()), QStringLiteral("gzip.compactacao"));
}

void TstGzip::bufferSemProgressoViraErro()
{
    // C2 (revisão): Z_BUF_ERROR sem produção de bytes é erro, não repetição. Mesmo desenho: o segundo passo é o fim.
    int chamada = 0;
    const detalhe::PassoFn passo = [&chamada](std::span<std::byte>) -> detalhe::PassoCompressao {
        ++chamada;
        return chamada == 1 ? detalhe::PassoCompressao{detalhe::kPassoSemProgresso, 0}
                            : detalhe::PassoCompressao{detalhe::kPassoFim, 0};
    };
    const auto saida = detalhe::acumularSaida(passo);
    QVERIFY2(!saida.has_value(), "buffer sem progresso da zlib virou saída");
    QCOMPARE(QString::fromStdString(saida.error().codigo()), QStringLiteral("gzip.compactacao"));
}

void TstGzip::entradaAcimaDoLimiteDaZlibRecusada()
{
    // O1 (revisão): a zlib recebe avail_in em uInt. Acima do maior uInt, comprimir recusa em vez de truncar. O teste
    // cobre o limite pelo predicado que comprimir usa, sem alocar 4 GiB.
    const auto limite = static_cast<std::size_t>(std::numeric_limits<unsigned int>::max());
    QVERIFY2(detalhe::entradaCabeNaZlib(limite), "o maior uInt devia caber");
    QVERIFY2(!detalhe::entradaCabeNaZlib(limite + 1), "acima de uInt devia ser recusado");
}

QTEST_GUILESS_MAIN(TstGzip)
#include "tst_gzip.moc"
