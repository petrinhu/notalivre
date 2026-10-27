// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Grupo de NFS-e referenciadas (gRefNFSe): de 1 a 99 chaves (NL-1.4, DA-1.5, TCInfoRefNFSe, tiposComplexos_v1.01.xsd:2397-2403).

#ifndef NL_BACK_DPS_REF_NFSE_H
#define NL_BACK_DPS_REF_NFSE_H

#include <expected>
#include <utility>
#include <vector>

#include <nl/base/erro.h>
#include <nl/back/dps/chave_nfse.h>


namespace nl::back {

/// Invariante: entre 1 e 99 chaves (minOccurs 1, maxOccurs 99 em refNFSe). Só se constrói por RefNfse::de.
class RefNfse
{
public:
    /// Erro: "ref_nfse.quantidade" (0 ou mais de 99 chaves).
    static std::expected<RefNfse, nl::base::Erro> de(std::vector<ChaveNfse> chaves);

    const std::vector<ChaveNfse>& chaves() const noexcept { return chaves_; }

    bool operator==(const RefNfse& outro) const = default;

private:
    explicit RefNfse(std::vector<ChaveNfse> chaves) : chaves_{std::move(chaves)} {}

    std::vector<ChaveNfse> chaves_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_REF_NFSE_H
