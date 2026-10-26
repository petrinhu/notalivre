// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Interpretação dos argumentos de `notalivre dps validar` (NL-1.7): posicionais, --xsd e --manifesto, em qualquer ordem.
#include "argumentos_dps_validar.h"

namespace nl::cli {

namespace {

const QString kUso = QStringLiteral(
    "uso: notalivre dps validar <entrada.json> <saida.xml> --xsd <diretorio> --manifesto <arquivo>");

QString comDetalhe(const QString& detalhe)
{
    return kUso + QStringLiteral(" (") + detalhe + QStringLiteral(")");
}

} // namespace

std::expected<ArgumentosDpsValidar, QString> lerArgumentosDpsValidar(const QStringList& argumentos)
{
    constexpr qsizetype kPrimeiroArgumento = 2;
    ArgumentosDpsValidar lidos;
    QStringList posicionais;
    QStringList opcoesVistas;

    for (qsizetype i = kPrimeiroArgumento; i < argumentos.size(); ++i) {
        const QString& token = argumentos.at(i);
        if (token == QLatin1String("--xsd") || token == QLatin1String("--manifesto")) {
            if (opcoesVistas.contains(token)) {
                return std::unexpected(comDetalhe(QStringLiteral("opção repetida: %1").arg(token)));
            }
            opcoesVistas.append(token);
            if (i + 1 >= argumentos.size()) {
                return std::unexpected(comDetalhe(QStringLiteral("falta o valor de %1").arg(token)));
            }
            i += 1;
            if (token == QLatin1String("--xsd")) {
                lidos.diretorioXsd = argumentos.at(i);
            } else {
                lidos.manifesto = argumentos.at(i);
            }
            continue;
        }
        if (token.startsWith(QLatin1String("--"))) {
            return std::unexpected(comDetalhe(QStringLiteral("opção desconhecida: %1").arg(token)));
        }
        posicionais.append(token);
    }

    if (posicionais.size() != 2) {
        return std::unexpected(comDetalhe(QStringLiteral("informe a entrada e a saída, nesta ordem")));
    }
    if (lidos.diretorioXsd.isEmpty() || lidos.manifesto.isEmpty()) {
        return std::unexpected(comDetalhe(QStringLiteral("informe --xsd e --manifesto")));
    }
    lidos.entrada = posicionais.at(0);
    lidos.saida = posicionais.at(1);
    return lidos;
}

} // namespace nl::cli
