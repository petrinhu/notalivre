// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo de erros da Sefin (NL-2.8, ADR-0005 c): código E para mensagem do app. A mensagem sai daqui, nunca do texto livre
// da API. Códigos da tabela: os do Anexo I de 20/10/2026 (aba RN, FATO, por texto da planilha) e os de INFERÊNCIA marcados.
#ifndef NL_MID_SEFIN_CLIENTE_CATALOGO_ERROS_SEFIN_H
#define NL_MID_SEFIN_CLIENTE_CATALOGO_ERROS_SEFIN_H

#include <string_view>

namespace nl::mid::sefin::cliente {

class CatalogoErrosSefin {
public:
    /// Mensagem do código E. Código fora do catálogo: mensagem genérica (sem o texto livre da API).
    [[nodiscard]] static std::string_view mensagem(std::string_view codigoE) noexcept;

    /// Verdadeiro se o código está no catálogo.
    [[nodiscard]] static bool conhece(std::string_view codigoE) noexcept;
};

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_CATALOGO_ERROS_SEFIN_H
