// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tipo de operação com entes governamentais ou sobre bens imóveis (tpOper). (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1719-1736 (TSRTCTpOper). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_TIPO_OPERACAO_H
#define NL_BACK_DPS_TIPO_OPERACAO_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Tipo de operação com entes governamentais ou sobre bens imóveis (tpOper).
/// Só se constrói por TipoOperacao::de, que aceita somente os valores do XSD. O construtor é privado.
class TipoOperacao
{
public:
    enum class Valor
    {
        fornecimentoComPagamentoPosterior, // XML "1": 1 - Fornecimento com pagamento posterior
        recebimentoComFornecimentoRealizado, // XML "2": 2 - Recebimento do pagamento com fornecimento já realizado
        fornecimentoComPagamentoRealizado, // XML "3": 3 - Fornecimento com pagamento já realizado
        recebimentoComFornecimentoPosterior, // XML "4": 4 - Recebimento do pagamento com fornecimento posterior
        fornecimentoERecebimentoConcomitantes, // XML "5": 5 - Fornecimento e recebimento do pagamento concomitantes
    };

    /// Erro: "tipooperacao.valor" (valor fora do conjunto do XSD).
    static std::expected<TipoOperacao, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const TipoOperacao& outro) const noexcept = default;

private:
    explicit TipoOperacao(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TIPO_OPERACAO_H
