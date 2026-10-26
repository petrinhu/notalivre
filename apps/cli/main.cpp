// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ponto de entrada da CLI notalivre (NL-1.7): despacha `dps validar`; qualquer outra linha de comando mostra o uso.
#include "comando_dps_validar.h"

#include <QString>
#include <QStringList>
#include <QTextStream>

#include <cstdio>

int main(int argc, char* argv[])
{
    QStringList argumentos;
    for (int i = 1; i < argc; ++i) {
        argumentos.append(QString::fromLocal8Bit(argv[i]));
    }

    if (argumentos.size() >= 2 && argumentos.at(0) == QLatin1String("dps") && argumentos.at(1) == QLatin1String("validar")) {
        return nl::cli::executarDpsValidar(argumentos);
    }

    QTextStream erro{stderr};
    erro << QStringLiteral("uso: notalivre dps validar <entrada.json> <saida.xml> --xsd <diretorio> --manifesto <arquivo>") << Qt::endl;
    return 2;
}
