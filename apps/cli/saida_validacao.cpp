// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Formatação e escrita das linhas de erro e de aviso da validação (NL-3.M). Textos vêm do domínio, literais; nada de valor da DPS.
#include "saida_validacao.h"

#include <QTextStream>

#include <cstdio>

namespace nl::cli {

namespace {

void escreveNoErro(const QString& linha)
{
    QTextStream saida{stderr};
    saida << linha << Qt::endl;
}

} // namespace

QString linhaDeErro(const nl::base::Erro& erro)
{
    const QString codigo = QString::fromStdString(erro.codigo());
    const QString mensagem = QString::fromStdString(erro.mensagem());
    if (erro.campo()) {
        return QStringLiteral("erro %1 (%2): %3").arg(codigo, QString::fromStdString(*erro.campo()), mensagem);
    }
    return QStringLiteral("erro %1: %2").arg(codigo, mensagem);
}

QString linhaDeAviso(const nl::mid::composicao::AvisoDps& aviso)
{
    return QStringLiteral("aviso %1 (%2): %3")
        .arg(QString::fromStdString(aviso.codigo), QString::fromStdString(aviso.campo), QString::fromStdString(aviso.mensagem));
}

void mostrarErros(const std::vector<nl::base::Erro>& erros)
{
    for (const nl::base::Erro& erro : erros) {
        escreveNoErro(linhaDeErro(erro));
    }
}

void mostrarAvisos(const std::vector<nl::mid::composicao::AvisoDps>& avisos)
{
    for (const nl::mid::composicao::AvisoDps& aviso : avisos) {
        escreveNoErro(linhaDeAviso(aviso));
    }
}

} // namespace nl::cli
