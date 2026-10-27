// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Casa o texto inteiro com um xs:pattern do XSD (NL-1.4). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CASA_PADRAO_H
#define NL_BACK_DPS_CASA_PADRAO_H

#include <QRegularExpression>
#include <QString>
#include <QStringView>

namespace nl::back::detalhe {

/// xs:pattern casa o valor inteiro (ancorado). anchoredPattern reproduz essa ancoragem com \A e \z.
inline bool casaPadrao(QStringView texto, const char* padrao)
{
    const QRegularExpression regex{QRegularExpression::anchoredPattern(QString::fromUtf8(padrao))};
    return regex.matchView(texto).hasMatch();
}

} // namespace nl::back::detalhe

#endif // NL_BACK_DPS_CASA_PADRAO_H
