// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Chave de acesso de NFS-e de 50 posições (TSChaveNFSe), usada em subst/chSubstda e gRefNFSe/refNFSe. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:206 (TSChaveNFSe). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CHAVE_NFSE_H
#define NL_BACK_DPS_CHAVE_NFSE_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Chave de acesso de NFS-e de 50 posições (TSChaveNFSe), usada em subst/chSubstda e gRefNFSe/refNFSe. Invariante: o texto casa o padrão do XSD. Só se constrói por ChaveNfse::de. O construtor é privado.
class ChaveNfse
{
public:
    /// Erro: "chave_nfse.formato" (texto fora do padrão do XSD).
    static std::expected<ChaveNfse, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const ChaveNfse& outro) const = default;

private:
    explicit ChaveNfse(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CHAVE_NFSE_H
