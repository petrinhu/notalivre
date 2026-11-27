// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Impressões do certificado para o modo fixado (NL-3.1h, C4'): a correta, dada pelo harness, e uma errada de propósito.
#ifndef NL_TESTE_IMPRESSAO_BANCO_H
#define NL_TESTE_IMPRESSAO_BANCO_H

#include "ambiente_banco.h"

#include <QByteArray>
#include <QString>

namespace {

// Impressão que o modo fixado usa: a dada pelo harness (64 hexadecimais), ou 64 zeros quando não há (nenhuma).
QByteArray impressaoParaFixar(const DadosBanco &dados)
{
    if (dados.impressao == QStringLiteral("nenhuma")) {
        return QByteArray(64, '0');
    }
    return dados.impressao.toLatin1();
}

// Impressão errada de propósito: o primeiro dígito muda (0 vira 1; qualquer outro vira 0). O comprimento fica.
[[maybe_unused]] QByteArray impressaoTrocada(const QByteArray &impressao)
{
    QByteArray errada = impressao;
    if (!errada.isEmpty()) {
        errada[0] = errada[0] == '0' ? '1' : '0';
    }
    return errada;
}

} // namespace

#endif // NL_TESTE_IMPRESSAO_BANCO_H
