// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Interpretação dos argumentos de `notalivre dps validar` (NL-1.7; NL-3.M): posicionais, --xsd, --manifesto e --tabelas, em qualquer ordem.
#include "argumentos_dps_validar.h"

namespace nl::cli {

namespace {

const QString kUso = QStringLiteral(
    "uso: notalivre dps validar <entrada.json> <saida.xml> --xsd <diretorio> --manifesto <arquivo> --tabelas <diretorio>");

QString comDetalhe(const QString& detalhe)
{
    return kUso + QStringLiteral(" (") + detalhe + QStringLiteral(")");
}

bool ehOpcaoDeCaminho(const QString& token)
{
    return token == QLatin1String("--xsd") || token == QLatin1String("--manifesto") || token == QLatin1String("--tabelas");
}

void guardaValor(ArgumentosDpsValidar& lidos, const QString& opcao, const QString& valor)
{
    if (opcao == QLatin1String("--xsd")) {
        lidos.diretorioXsd = valor;
    } else if (opcao == QLatin1String("--manifesto")) {
        lidos.manifesto = valor;
    } else {
        lidos.tabelas = valor;
    }
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
        if (ehOpcaoDeCaminho(token)) {
            if (opcoesVistas.contains(token)) {
                return std::unexpected(comDetalhe(QStringLiteral("opção repetida: %1").arg(token)));
            }
            opcoesVistas.append(token);
            if (i + 1 >= argumentos.size()) {
                return std::unexpected(comDetalhe(QStringLiteral("falta o valor de %1").arg(token)));
            }
            i += 1;
            guardaValor(lidos, token, argumentos.at(i));
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
    if (lidos.diretorioXsd.isEmpty()) {
        return std::unexpected(comDetalhe(QStringLiteral("falta --xsd")));
    }
    if (lidos.manifesto.isEmpty()) {
        return std::unexpected(comDetalhe(QStringLiteral("falta --manifesto")));
    }
    if (lidos.tabelas.isEmpty()) {
        return std::unexpected(comDetalhe(QStringLiteral("falta --tabelas")));
    }
    lidos.entrada = posicionais.at(0);
    lidos.saida = posicionais.at(1);
    return lidos;
}

} // namespace nl::cli
