// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do corpo de erro da Sefin (NL-2.8, pesquisa g: forma INFERÊNCIA até a NL-2.10). Aceita "erro" ou "erros" (lista)
// e "Codigo" ou "codigo". Devolve só o código E, nunca a descrição (ADR-0005 c).
#ifndef NL_MID_SEFIN_CLIENTE_LEITOR_ERRO_SEFIN_H
#define NL_MID_SEFIN_CLIENTE_LEITOR_ERRO_SEFIN_H

#include <QByteArray>

#include <optional>
#include <string>

namespace nl::mid::sefin::cliente {

/// Código E do corpo, ou nullopt quando o corpo não traz código lido.
[[nodiscard]] std::optional<std::string> codigoDoErro(const QByteArray& corpo);

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_LEITOR_ERRO_SEFIN_H
