// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Prestador de serviços do grupo infDPS/prest: inscrição (CNPJ ou CPF), dados cadastrais e regime (NL-1.4, TCInfoPrestador, tiposComplexos_v1.01.xsd:890).

#ifndef NL_BACK_DPS_PRESTADOR_H
#define NL_BACK_DPS_PRESTADOR_H

#include <optional>
#include <utility>

#include <nl/back/dps/endereco.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/dps/telefone.h>
#include <nl/back/identificacao/inscricao.h>


namespace nl::back {

/// Invariante: a inscrição é CNPJ ou CPF (Inscricao, NL-1.1). A v1 não emite com prestador no exterior (NIF): fora da v1, INBOX.
class Prestador
{
public:
    static Prestador montar(Inscricao inscricao, std::optional<InscricaoMunicipal> im, std::optional<NomeRazaoSocial> xNome,
                            std::optional<Endereco> endereco, std::optional<Telefone> fone, std::optional<EmailPessoa> email,
                            RegimeTributacao regime)
    {
        return Prestador{std::move(inscricao), std::move(im), std::move(xNome), std::move(endereco), std::move(fone), std::move(email), std::move(regime)};
    }

    const Inscricao& inscricao() const noexcept { return inscricao_; }
    const std::optional<InscricaoMunicipal>& im() const noexcept { return im_; }
    const std::optional<NomeRazaoSocial>& xNome() const noexcept { return xNome_; }
    const std::optional<Endereco>& endereco() const noexcept { return endereco_; }
    const std::optional<Telefone>& fone() const noexcept { return fone_; }
    const std::optional<EmailPessoa>& email() const noexcept { return email_; }
    const RegimeTributacao& regime() const noexcept { return regime_; }

    bool operator==(const Prestador& outro) const = default;

private:
    Prestador(Inscricao inscricao, std::optional<InscricaoMunicipal> im, std::optional<NomeRazaoSocial> xNome,
              std::optional<Endereco> endereco, std::optional<Telefone> fone, std::optional<EmailPessoa> email, RegimeTributacao regime)
        : inscricao_{std::move(inscricao)}, im_{std::move(im)}, xNome_{std::move(xNome)}, endereco_{std::move(endereco)},
          fone_{std::move(fone)}, email_{std::move(email)}, regime_{std::move(regime)}
    {
    }

    Inscricao inscricao_;
    std::optional<InscricaoMunicipal> im_;
    std::optional<NomeRazaoSocial> xNome_;
    std::optional<Endereco> endereco_;
    std::optional<Telefone> fone_;
    std::optional<EmailPessoa> email_;
    RegimeTributacao regime_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_PRESTADOR_H
