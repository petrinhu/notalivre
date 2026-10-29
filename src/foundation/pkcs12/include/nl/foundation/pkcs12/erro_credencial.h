// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Erros do carregador de A1 (NL-2.2, ADR-S1). Cada valor tem teste com fixture; o código é estável para log.
#ifndef NL_FOUNDATION_PKCS12_ERRO_CREDENCIAL_H
#define NL_FOUNDATION_PKCS12_ERRO_CREDENCIAL_H

#include <string_view>

namespace nl::foundation::pkcs12 {

/// Motivo pelo qual uma credencial A1 não é aceita.
enum class ErroCredencial : int {
    SenhaErrada,              ///< MAC do PKCS#12 não confere: senha incorreta.
    CifraLegadaIndisponivel,  ///< Cifra antiga (RC2, 3DES) que o provider legacy não consegue abrir.
    Vencido,                  ///< notAfter da folha anterior ao instante atual.
    AindaNaoValido,           ///< notBefore da folha posterior ao instante atual.
    CadeiaInvalida,           ///< A cadeia não chega a uma âncora fornecida pelo chamador.
    SemInscricao,             ///< Sem otherName de CNPJ (2.16.76.1.3.3) nem de CPF (2.16.76.1.3.1).
    SemAutenticacaoCliente,   ///< EKU presente e sem clientAuth.
    PfxMalformado,            ///< Não decodifica como PKCS#12, ou não traz chave e certificado.
    FalhaInterna,             ///< Alocação ou falha do OpenSSL fora dos casos acima.
};

/// Código estável para log e para o chamador. Não muda com o texto da mensagem.
[[nodiscard]] constexpr std::string_view codigo(ErroCredencial erro) noexcept {
    switch (erro) {
    case ErroCredencial::SenhaErrada: return "credencial.senha_errada";
    case ErroCredencial::CifraLegadaIndisponivel: return "credencial.cifra_legada_indisponivel";
    case ErroCredencial::Vencido: return "credencial.vencido";
    case ErroCredencial::AindaNaoValido: return "credencial.ainda_nao_valido";
    case ErroCredencial::CadeiaInvalida: return "credencial.cadeia_invalida";
    case ErroCredencial::SemInscricao: return "credencial.sem_inscricao";
    case ErroCredencial::SemAutenticacaoCliente: return "credencial.sem_autenticacao_cliente";
    case ErroCredencial::PfxMalformado: return "credencial.pfx_malformado";
    case ErroCredencial::FalhaInterna: return "credencial.falha_interna";
    }
    return "credencial.falha_interna";
}

/// Erro ao ler as âncoras de confiança (PEM).
enum class ErroAncora {
    PemSemCertificado,  ///< Nenhum certificado PEM foi lido do texto.
    FalhaInterna,       ///< Alocação ou falha do OpenSSL ao ler o PEM.
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_ERRO_CREDENCIAL_H
