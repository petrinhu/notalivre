// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura da linha de comando de `notalivre emitir` (NL-2.9). Ver argumentos_emitir.h. WIP: sem verificação.
#include "argumentos_emitir.h"

namespace nl::cli {
namespace {

const QString kUsoEmitir = QStringLiteral(
    "uso: notalivre emitir <entrada.json> --serie <S> --numero <N> --ambiente <homologacao|producao-restrita|producao> "
    "");

bool apenasDigitos(const QString& texto, int maximo)
{
    if (texto.isEmpty() || texto.size() > maximo) {
        return false;
    }
    for (const QChar caractere : texto) {
        if (!caractere.isDigit()) {
            return false;
        }
    }
    return true;
}

} // namespace

std::expected<ArgumentosEmitir, QString> lerArgumentosEmitir(const QStringList& argumentos)
{
    if (argumentos.size() < 2 || argumentos.at(0) != QLatin1String("emitir")) {
        return std::unexpected(kUsoEmitir);
    }
    ArgumentosEmitir lidos;
    lidos.entrada = argumentos.at(1);
    for (qsizetype i = 2; i < argumentos.size(); ++i) {
        const QString opcao = argumentos.at(i);
        if (opcao.contains(QLatin1String("senha"), Qt::CaseInsensitive)) {
            return std::unexpected(QStringLiteral("a senha não vai pela linha de comando: ela é pedida no terminal, sem eco"));
        }
        if (i + 1 >= argumentos.size()) {
            return std::unexpected(kUsoEmitir);
        }
        const QString valor = argumentos.at(++i);
        if (opcao == QLatin1String("--serie")) {
            lidos.serie = valor;
        } else if (opcao == QLatin1String("--numero")) {
            lidos.numero = valor;
        } else if (opcao == QLatin1String("--ambiente")) {
            lidos.ambiente = valor;
        } else if (opcao == QLatin1String("--certificado")) {
            lidos.certificado = valor;
        } else {
            return std::unexpected(kUsoEmitir);
        }
    }
    if (!apenasDigitos(lidos.serie, 5) || !apenasDigitos(lidos.numero, 15) || lidos.ambiente.isEmpty()) {
        return std::unexpected(kUsoEmitir);
    }
    return lidos;
}

} // namespace nl::cli
