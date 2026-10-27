// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tomador do serviço (grupo infDPS/toma): identificação, nome e endereço opcionais pelo Anexo de 20/10 (NL-1.4, TCInfoPessoa, tiposComplexos_v1.01.xsd:1006).

#ifndef NL_BACK_DPS_TOMADOR_H
#define NL_BACK_DPS_TOMADOR_H

#include <optional>
#include <utility>

#include <nl/back/dps/endereco.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/telefone.h>


namespace nl::back {

/// Invariante: a identificação é obrigatória e é uma das quatro alternativas de IdentificacaoPessoa (sem default).
/// xNome é opcional no leiaute de 20/10 (toma/xNome, 0-1). Se a tela exige o nome é a linha 5 da INBOX, não uma regra daqui.
class Tomador
{
public:
    static Tomador montar(IdentificacaoPessoa identificacao, std::optional<InscricaoMunicipal> im, std::optional<NomeRazaoSocial> xNome,
                          std::optional<Endereco> endereco, std::optional<Telefone> fone, std::optional<EmailPessoa> email)
    {
        return Tomador{std::move(identificacao), std::move(im), std::move(xNome), std::move(endereco), std::move(fone), std::move(email)};
    }

    const IdentificacaoPessoa& identificacao() const noexcept { return identificacao_; }
    const std::optional<InscricaoMunicipal>& im() const noexcept { return im_; }
    const std::optional<NomeRazaoSocial>& xNome() const noexcept { return xNome_; }
    const std::optional<Endereco>& endereco() const noexcept { return endereco_; }
    const std::optional<Telefone>& fone() const noexcept { return fone_; }
    const std::optional<EmailPessoa>& email() const noexcept { return email_; }

    bool operator==(const Tomador& outro) const = default;

private:
    Tomador(IdentificacaoPessoa identificacao, std::optional<InscricaoMunicipal> im, std::optional<NomeRazaoSocial> xNome,
            std::optional<Endereco> endereco, std::optional<Telefone> fone, std::optional<EmailPessoa> email)
        : identificacao_{std::move(identificacao)}, im_{std::move(im)}, xNome_{std::move(xNome)},
          endereco_{std::move(endereco)}, fone_{std::move(fone)}, email_{std::move(email)}
    {
    }

    IdentificacaoPessoa identificacao_;
    std::optional<InscricaoMunicipal> im_;
    std::optional<NomeRazaoSocial> xNome_;
    std::optional<Endereco> endereco_;
    std::optional<Telefone> fone_;
    std::optional<EmailPessoa> email_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TOMADOR_H
