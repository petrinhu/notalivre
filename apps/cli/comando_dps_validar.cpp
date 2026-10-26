// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre dps validar` (NL-1.7): o XML só vai para o disco depois que a fachada devolve a DPS validada.
#include "comando_dps_validar.h"

#include "argumentos_dps_validar.h"

#include <nl/base/erro.h>
#include <nl/mid/composicao/dps_validar.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QTextStream>

#include <cstdio>
#include <vector>

namespace nl::cli {

namespace {

constexpr int kCodigoGravado = 0;
constexpr int kCodigoReprovado = 1;
constexpr int kCodigoUsoOuAmbiente = 2;

/// Uma linha por erro: "erro <codigo> (<campo>): <mensagem>". O campo é omitido quando o erro não tem um.
QString linhaDeErro(const nl::base::Erro& erro)
{
    const QString codigo = QString::fromStdString(erro.codigo());
    const QString mensagem = QString::fromStdString(erro.mensagem());
    if (erro.campo()) {
        return QStringLiteral("erro %1 (%2): %3").arg(codigo, QString::fromStdString(*erro.campo()), mensagem);
    }
    return QStringLiteral("erro %1: %2").arg(codigo, mensagem);
}

void mostrarErros(const std::vector<nl::base::Erro>& erros)
{
    QTextStream saida{stderr};
    for (const nl::base::Erro& erro : erros) {
        saida << linhaDeErro(erro) << Qt::endl;
    }
}

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
    const auto xml = nl::mid::composicao::DpsValidar::executar(*entrada, esquema);
    if (!xml) {
        mostrarErros(xml.error().erros);
        return xml.error().natureza == nl::mid::composicao::NaturezaFalha::ambiente ? kCodigoUsoOuAmbiente : kCodigoReprovado;
    }

    QString motivo;
    if (!gravarSaida(lidos->saida, *xml, &motivo)) {
        mostrarTexto(motivo);
        return kCodigoUsoOuAmbiente;
    }
    return kCodigoGravado;
}

} // namespace nl::cli
