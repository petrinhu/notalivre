// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Forma canônica da senha (NL-3.2b, ADR-0003, entrada canônica: senha em UTF-8 na forma NFC).
// Só ASCII nunca passa por QString. Byte não ASCII: UTF-8 estrito conferido antes, e só então QString com normalização NFC
// numa versão Unicode fixa, para a chave não mudar com a atualização do Qt (ciso, item 10).

#include <nl/foundation/cripto/senha_nfc.h>

#include "nl/foundation/cripto/erros.h"
#include "nl/foundation/cripto/senha_nfc_interno.h"

#include <QChar>
#include <QString>

#include <cstddef>

namespace nl::foundation::cripto {

namespace {

// Versão Unicode fixada para a normalização NFC. Trocar esta constante muda a chave derivada da senha: é formato gravado.
constexpr QChar::UnicodeVersion kVersaoUnicodeNormalizacao = QChar::Unicode_15_1;

bool soAscii(QByteArrayView utf8)
{
    for (const char byte : utf8) {
        if (static_cast<unsigned char>(byte) >= 0x80) {
            return false;
        }
    }
    return true;
}

// UTF-8 estrito (RFC 3629): rejeita sequência truncada, forma longa, substituto U+D800..U+DFFF e acima de U+10FFFF.
bool utf8Estrito(QByteArrayView utf8)
{
    const auto tamanho = static_cast<std::size_t>(utf8.size());
    const auto* bytes = reinterpret_cast<const unsigned char*>(utf8.data());
    std::size_t posicao = 0;
    while (posicao < tamanho) {
        const unsigned char primeiro = bytes[posicao];
        if (primeiro < 0x80) {
            ++posicao;
            continue;
        }
        std::size_t comprimento = 0;
        unsigned char minimoSegundo = 0x80;
        unsigned char maximoSegundo = 0xBF;
        if (primeiro >= 0xC2 && primeiro <= 0xDF) {
            comprimento = 2;
        } else if (primeiro >= 0xE0 && primeiro <= 0xEF) {
            comprimento = 3;
            if (primeiro == 0xE0) {
                minimoSegundo = 0xA0; // sem forma longa
            } else if (primeiro == 0xED) {
                maximoSegundo = 0x9F; // sem substitutos
            }
        } else if (primeiro >= 0xF0 && primeiro <= 0xF4) {
            comprimento = 4;
            if (primeiro == 0xF0) {
                minimoSegundo = 0x90; // sem forma longa
            } else if (primeiro == 0xF4) {
                maximoSegundo = 0x8F; // no máximo U+10FFFF
            }
        } else {
            return false;
        }
        if (posicao + comprimento > tamanho) {
            return false;
        }
        const unsigned char segundo = bytes[posicao + 1];
        if (segundo < minimoSegundo || segundo > maximoSegundo) {
            return false;
        }
        for (std::size_t indice = 2; indice < comprimento; ++indice) {
            if (bytes[posicao + indice] < 0x80 || bytes[posicao + indice] > 0xBF) {
                return false;
            }
        }
        posicao += comprimento;
    }
    return true;
}

std::expected<int, nl::base::Erro> copiarParaDestino(const QByteArray& bytes, std::span<unsigned char> destino)
{
    if (static_cast<std::size_t>(bytes.size()) > destino.size()) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "área de destino pequena para a senha"));
    }
    for (qsizetype indice = 0; indice < bytes.size(); ++indice) {
        destino[static_cast<std::size_t>(indice)] = static_cast<unsigned char>(bytes[indice]);
    }
    return static_cast<int>(bytes.size());
}

} // namespace

std::expected<int, nl::base::Erro> senhaEmNfc(QByteArrayView utf8, std::span<unsigned char> destino)
{
    detalhe::RelatorioNfc relatorio;
    return detalhe::senhaEmNfcComRelatorio(utf8, destino, relatorio);
}

namespace detalhe {

std::expected<int, nl::base::Erro> senhaEmNfcComRelatorio(QByteArrayView utf8, std::span<unsigned char> destino,
                                                          RelatorioNfc& relatorio)
{
    relatorio = RelatorioNfc{};
    if (soAscii(utf8)) {
        return copiarParaDestino(QByteArray(utf8.data(), utf8.size()), destino);
    }
    if (!utf8Estrito(utf8)) {
        return std::unexpected(erroCripto("cripto.utf8_invalido", "senha com UTF-8 inválido: recusada sem conversão"));
    }
    relatorio.passouPorQString = true;
    const QString texto = QString::fromUtf8(utf8);
    const QString nfc = texto.normalized(QString::NormalizationForm_C, kVersaoUnicodeNormalizacao);
    return copiarParaDestino(nfc.toUtf8(), destino);
}

} // namespace detalhe

} // namespace nl::foundation::cripto
