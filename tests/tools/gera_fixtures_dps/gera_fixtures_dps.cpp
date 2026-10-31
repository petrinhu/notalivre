// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Gerador das fixtures da DPS (NL-1.6b, DA-2.61). Fora do CTest: não é teste, e não entra na contagem da suíte.
// Grava os XML do codec byte a byte na pasta que receber, para revisar o diff e copiar para tests/fixtures/dps.
// Uso: gera_fixtures_dps <pasta de saída>. Sai com 0 só se gravar todas as fixtures.
#include "casos_dps.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QString>

#include <cstdio>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::fprintf(stderr, "uso: gera_fixtures_dps <pasta de saída>\n");
        return 2;
    }
    const QString saida = QString::fromLocal8Bit(argv[1]);
    if (!QDir().mkpath(saida)) {
        std::fprintf(stderr, "não consegui criar a pasta de saída: %s\n", argv[1]);
        return 1;
    }
    const auto fixtures = casos::casosFixture();
    if (!fixtures) {
        std::fprintf(stderr, "builder de alguma fixture recusou a entrada\n");
        return 1;
    }
    for (const auto& caso : *fixtures) {
        QFile arquivo{QDir{saida}.filePath(caso.nome)};
        if (!arquivo.open(QIODevice::WriteOnly)) {
            std::fprintf(stderr, "não consegui gravar: %s\n", qPrintable(caso.nome));
            return 1;
        }
        if (arquivo.write(caso.xml) != static_cast<qint64>(caso.xml.size())) {
            std::fprintf(stderr, "gravação incompleta: %s\n", qPrintable(caso.nome));
            return 1;
        }
    }
    return 0;
}
