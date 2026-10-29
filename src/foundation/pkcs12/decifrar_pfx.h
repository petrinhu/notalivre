// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Decifra o .pfx (NL-2.2, ADR-S1). Interno ao módulo.
#ifndef NL_FOUNDATION_PKCS12_DECIFRAR_PFX_H
#define NL_FOUNDATION_PKCS12_DECIFRAR_PFX_H

#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/ponteiros_ossl.h>
#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArrayView>

#include <expected>
#include <vector>

namespace nl::foundation::pkcs12 {

/// Partes do .pfx já reimportadas para o contexto default (nenhum ponteiro preso a contexto próprio).
struct PartesPfx {
    CertificadoX509 certificado;
    ChavePrivada chave;
    std::vector<CertificadoX509> intermediarios;
};

/// Tenta com o provider default; só se o motivo for "unsupported" tenta de novo com o legacy no contexto próprio.
[[nodiscard]] std::expected<PartesPfx, ErroCredencial> decifrarPfx(QByteArrayView pfx, const SenhaSegura& senha);

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_DECIFRAR_PFX_H
