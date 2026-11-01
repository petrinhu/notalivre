// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Nomes de campo que o cliente Sefin lê de uma resposta (NL-2.10c, Emenda 3, MÉDIA-1). Fonte única: o cliente (cliente_sefin.cpp)
// e o leitor de erro (leitor_erro_sefin.cpp) leem por estas constantes, e a allowlist da resposta crua (composicao/resposta_crua.cpp)
// também. Não há lista paralela que possa divergir: um nome novo que o cliente passe a ler entra aqui e na allowlist ao mesmo tempo.
#ifndef NL_MID_SEFIN_CLIENTE_CAMPOS_LIDOS_SEFIN_H
#define NL_MID_SEFIN_CLIENTE_CAMPOS_LIDOS_SEFIN_H

#include <array>
#include <string_view>

namespace nl::mid::sefin::cliente {

inline constexpr std::string_view kCampoChaveAcesso = "chaveAcesso";
inline constexpr std::string_view kCampoNfseXmlGZipB64 = "nfseXmlGZipB64";
inline constexpr std::string_view kCampoErro = "erro";
inline constexpr std::string_view kCampoErros = "erros";
inline constexpr std::string_view kCampoCodigo = "Codigo";
inline constexpr std::string_view kCampoCodigoMinusculo = "codigo";

/// Todos os nomes que o cliente lê. Só estes saem em claro na resposta de análise.
inline constexpr std::array<std::string_view, 6> kCamposLidosPeloCliente{
    kCampoChaveAcesso, kCampoNfseXmlGZipB64, kCampoErro, kCampoErros, kCampoCodigo, kCampoCodigoMinusculo};

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_CAMPOS_LIDOS_SEFIN_H
