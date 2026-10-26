// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor da inscrição de pessoa (NL-1.7): CNPJ ou CPF da chave do objeto pai. Os dois juntos reprovam.
#include "campos_json.h"
#include "leitores_comuns.h"

#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::Inscricao, nl::base::Erro> lerInscricao(const QJsonObject& pai, std::string_view caminhoPai)
{
    const auto cnpjTexto = campos::textoOpcional(pai, caminhoPai, "cnpj");
    if (!cnpjTexto) {
        return std::unexpected(cnpjTexto.error());
    }
    const auto cpfTexto = campos::textoOpcional(pai, caminhoPai, "cpf");
    if (!cpfTexto) {
        return std::unexpected(cpfTexto.error());
    }
    if (cnpjTexto->has_value() == cpfTexto->has_value()) {
        const std::string caminho = campos::caminhoDe(caminhoPai, "cnpj ou cpf");
        if (cnpjTexto->has_value()) {
            return std::unexpected(nl::base::Erro{"entrada.campos_conflitantes", caminho,
                                                  "informe só um: cnpj ou cpf, não os dois (" + caminho + ")"});
        }
        return std::unexpected(nl::base::Erro{"entrada.campo_ausente", caminho,
                                              "campo obrigatório ausente: cnpj ou cpf (" + caminho + ")"});
    }
    if (cnpjTexto->has_value()) {
        const auto cnpj = nb::Cnpj::de(**cnpjTexto);
        if (!cnpj) {
            return std::unexpected(cnpj.error());
        }
        return nb::Inscricao{*cnpj};
    }
    const auto cpf = nb::Cpf::de(**cpfTexto);
    if (!cpf) {
        return std::unexpected(cpf.error());
    }
    return nb::Inscricao{*cpf};
}

} // namespace nl::mid::composicao::leitores
