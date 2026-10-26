// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de MotivoNaoNif (NL-1.4). Fonte: tiposSimples_v1.01.xsd:394-402 (TSCodNaoNIF).

#include <nl/back/dps/motivo_nao_nif.h>

#include <optional>

namespace nl::back {

std::expected<MotivoNaoNif, nl::base::Erro> MotivoNaoNif::de(QStringView entrada)
{
    if (entrada == u"0") {
        return MotivoNaoNif{MotivoNaoNif::Valor::naoInformadoNaOrigem};
    }
    if (entrada == u"1") {
        return MotivoNaoNif{MotivoNaoNif::Valor::dispensadoDoNif};
    }
    if (entrada == u"2") {
        return MotivoNaoNif{MotivoNaoNif::Valor::naoExigenciaDoNif};
    }
    return std::unexpected(nl::base::Erro{"nif.motivo", std::nullopt, "MotivoNaoNif: valor fora do conjunto do XSD"});
}

} // namespace nl::back
