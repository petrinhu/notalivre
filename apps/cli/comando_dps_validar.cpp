// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre dps validar` (NL-1.7; NL-3.M): lê a entrada, chama a fachada da Mid e grava o XML só se a DPS passar.
// Avisos vão ao stderr e não mudam o código de saída (DA-3M.3).
#include "comando_dps_validar.h"

#include "argumentos_dps_validar.h"
#include "saida_validacao.h"

#include <nl/mid/composicao/dps_validar.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QTextStream>

#include <cstdio>

namespace nl::cli {

namespace {

constexpr int kCodigoGravado = 0;
constexpr int kCodigoReprovado = 1;
constexpr int kCodigoUsoOuAmbiente = 2;

void mostrarTexto(const QString& texto)
{
    QTextStream saida{stderr};
    saida << texto << Qt::endl;
}

std::expected<QByteArray, QString> lerEntrada(const QString& caminho)
{
    QFile arquivo{caminho};
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(QStringLiteral("não consegui ler a entrada %1: %2").arg(caminho, arquivo.errorString()));
    }
    return arquivo.readAll();
}

bool gravarSaida(const QString& caminho, const QByteArray& xml, QString* motivo)
{
    QFile arquivo{caminho};
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *motivo = QStringLiteral("não consegui abrir a saída %1: %2").arg(caminho, arquivo.errorString());
        return false;
    }
    if (arquivo.write(xml) != static_cast<qint64>(xml.size())) {
        *motivo = QStringLiteral("não consegui gravar a saída %1: %2").arg(caminho, arquivo.errorString());
        return false;
    }
    return true;
}

} // namespace

int executarDpsValidar(const QStringList& argumentos)
{
    const auto lidos = lerArgumentosDpsValidar(argumentos);
    if (!lidos) {
        mostrarTexto(lidos.error());
        return kCodigoUsoOuAmbiente;
    }

    const auto entrada = lerEntrada(lidos->entrada);
    if (!entrada) {
        mostrarTexto(entrada.error());
        return kCodigoUsoOuAmbiente;
    }

    const nl::mid::composicao::LocalizacaoEsquemaDps esquema{lidos->diretorioXsd, lidos->manifesto};
    const nl::mid::composicao::LocalizacaoTabelas tabelas{lidos->tabelas};
    const auto validada = nl::mid::composicao::DpsValidar::executar(*entrada, esquema, tabelas);
    if (!validada) {
        mostrarAvisos(validada.error().avisos);
        mostrarErros(validada.error().erros);
        return validada.error().natureza == nl::mid::composicao::NaturezaFalha::ambiente ? kCodigoUsoOuAmbiente : kCodigoReprovado;
    }

    mostrarAvisos(validada->avisos);
    QString motivo;
    if (!gravarSaida(lidos->saida, validada->xml, &motivo)) {
        mostrarTexto(motivo);
        return kCodigoUsoOuAmbiente;
    }
    return kCodigoGravado;
}

} // namespace nl::cli
