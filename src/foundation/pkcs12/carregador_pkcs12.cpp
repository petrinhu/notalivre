// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Orquestração do carregamento de A1 (NL-2.2). Ordem fixa: decifrar, vencimento, cadeia, EKU, inscrição.
// Cada critério devolve o próprio ErroCredencial; nenhum depende da mensagem de erro do OpenSSL.
#include <nl/foundation/pkcs12/carregador_pkcs12.h>

#include "decifrar_pfx.h"
#include "extrator_inscricao.h"
#include "verificador_cadeia.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <cstdint>
#include <limits>
#include <utility>

namespace nl::foundation::pkcs12 {

namespace {

// Evento de auditoria do certificado sem EKU. Só o nome do evento: nenhum CNPJ, CPF ou nome entra no registro.
constexpr const char* kEventoEkuAusente = "credencial.eku_ausente";

// X509_get_extended_key_usage devolve UINT32_MAX quando o certificado não traz a extensão EKU.
constexpr std::uint32_t kSemEku = std::numeric_limits<std::uint32_t>::max();

bool vencido(x509_st* certificado) {
    // X509_cmp_current_time < 0 quando notAfter é anterior ao instante atual.
    return X509_cmp_current_time(X509_get0_notAfter(certificado)) < 0;
}

} // namespace

CarregadorPkcs12::CarregadorPkcs12(nl::foundation::log::Log& registro) noexcept
    : registro_(registro) {}

std::expected<CredencialX509, ErroCredencial> CarregadorPkcs12::carregar(QByteArrayView pfx, SenhaSegura senha,
                                                                         const Ancoras& ancoras) {
    auto partes = decifrarPfx(pfx, senha);
    // A senha sai de uso logo após o parse, com sucesso ou falha.
    senha.zerar();
    if (!partes) {
        return std::unexpected(partes.error());
    }

    x509_st* folha = partes->certificado.get();
    if (vencido(folha)) {
        return std::unexpected(ErroCredencial::Vencido);
    }

    if (auto cadeia = verificarCadeia(folha, partes->intermediarios, ancoras); !cadeia) {
        return std::unexpected(cadeia.error());
    }

    const std::uint32_t eku = X509_get_extended_key_usage(folha);
    const bool ekuAusente = eku == kSemEku;
    if (!ekuAusente && (eku & XKU_SSL_CLIENT) == 0) {
        return std::unexpected(ErroCredencial::SemAutenticacaoCliente);
    }

    auto inscricao = extrairInscricao(folha);
    if (!inscricao) {
        return std::unexpected(ErroCredencial::SemInscricao);
    }

    if (ekuAusente) {
        auto gravado = registro_.registrar(nl::foundation::log::Nivel::Aviso, kEventoEkuAusente, {});
        if (!gravado) {
            // Sem o registro exigido, a credencial sem EKU não é aceita (falha fechada).
            return std::unexpected(ErroCredencial::FalhaInterna);
        }
    }

    return CredencialX509(std::move(partes->certificado), std::move(partes->chave), std::move(*inscricao),
                          ekuAusente);
}

} // namespace nl::foundation::pkcs12
