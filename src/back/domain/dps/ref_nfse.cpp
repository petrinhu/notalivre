// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de RefNfse (NL-1.4, DA-1.5): de 1 a 99 chaves (refNFSe maxOccurs 99).

#include <nl/back/dps/ref_nfse.h>

#include <cstddef>
#include <utility>

namespace nl::back {

std::expected<RefNfse, nl::base::Erro> RefNfse::de(std::vector<ChaveNfse> chaves)
{
    constexpr std::size_t kMaximo = 99;
    if (chaves.empty() || chaves.size() > kMaximo) {
        return std::unexpected(nl::base::Erro{"ref_nfse.quantidade", std::string{"gRefNFSe"},
                                              "gRefNFSe deve ter de 1 a 99 chaves de NFS-e referenciadas"});
    }
    return RefNfse{std::move(chaves)};
}

} // namespace nl::back
