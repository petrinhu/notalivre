// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Grupo IBSCBS do TCDPS (infDPS/IBSCBS, NL-1.6b): cIndOp, tpOper, gRefNFSe, indDest e valores/trib.
#include "codigos.h"
#include "grupos.h"

#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/ref_nfse.h>
#include <nl/back/dps/tributacao_ibscbs.h>

namespace nl::mid::sefin::prodrest_20261020::grupos {

namespace nb = nl::back;

void escreveIbscbs(QXmlStreamWriter& w, const nb::Ibscbs& ibscbs)
{
    w.writeStartElement(QStringLiteral("IBSCBS"));
    if (ibscbs.cIndOp()) {
        w.writeTextElement(QStringLiteral("cIndOp"), ibscbs.cIndOp()->texto());
    }
    if (ibscbs.tpOper()) {
        w.writeTextElement(QStringLiteral("tpOper"), codigos::tipoOperacao(ibscbs.tpOper()->valor()));
    }
    if (ibscbs.gRefNFSe()) {
        w.writeStartElement(QStringLiteral("gRefNFSe"));
        for (const auto& chave : ibscbs.gRefNFSe()->chaves()) {
            w.writeTextElement(QStringLiteral("refNFSe"), chave.texto());
        }
        w.writeEndElement();
    }
    w.writeTextElement(QStringLiteral("indDest"), codigos::indDestinatario(ibscbs.indDest().valor()));
    w.writeStartElement(QStringLiteral("valores"));
    w.writeStartElement(QStringLiteral("trib"));
    w.writeTextElement(QStringLiteral("CST"), ibscbs.trib().cst().texto());
    w.writeTextElement(QStringLiteral("cClassTrib"), ibscbs.trib().cClassTrib().texto());
    w.writeEndElement();
    w.writeEndElement();
    w.writeEndElement();
}

} // namespace nl::mid::sefin::prodrest_20261020::grupos
