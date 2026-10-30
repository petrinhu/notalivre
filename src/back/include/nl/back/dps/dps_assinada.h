// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// DPS já assinada, pronta para envio à Sefin (NL-2.8). O XML é o assinado pelo motor XMLDSIG (NL-2.3); o Id é o da DPS.
// Tipo de valor, sem I/O.
#ifndef NL_BACK_DPS_DPS_ASSINADA_H
#define NL_BACK_DPS_DPS_ASSINADA_H

#include <nl/back/dps/id_dps.h>

#include <QByteArray>

namespace nl::back {

struct DpsAssinada {
    IdDps id;
    QByteArray xml;
};

} // namespace nl::back

#endif // NL_BACK_DPS_DPS_ASSINADA_H
