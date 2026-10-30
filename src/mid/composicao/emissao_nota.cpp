// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Raiz de composição da emissão (NL-2.9, DA-2.42). Ver emissao_nota.h.
// FATIA VERMELHA: a raiz ainda liga um placeholder que recusa. A ligação ao AssinadorDpsSefin vem na fatia verde.
#include <nl/mid/composicao/emissao_nota.h>

#include <optional>
#include <utility>

namespace nl::mid::composicao {
namespace {

class AssinadorNaoLigado final : public nl::back::AssinadorDps
{
public:
    std::expected<QByteArray, nl::base::Erro> assinar(QByteArrayView xml, QStringView idReferencia) override
    {
        static_cast<void>(xml);
        static_cast<void>(idReferencia);
        return std::unexpected(nl::base::Erro("composicao.nao_ligada", std::nullopt, "o adaptador de assinatura não está ligado"));
    }
};

} // namespace

EmissaoNota::EmissaoNota(const nl::foundation::pkcs12::CredencialX509& credencial)
    : assinador_(std::make_unique<AssinadorNaoLigado>()), emitir_(*assinador_)
{
    static_cast<void>(credencial);
}

EmissaoNota::~EmissaoNota() = default;

nl::back::EmitirNota& EmissaoNota::emitirNota() noexcept
{
    return emitir_;
}

} // namespace nl::mid::composicao
