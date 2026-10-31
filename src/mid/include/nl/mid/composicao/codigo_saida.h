// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de saída da CLI para uma emissão recusada (NL-2.9, DA-2.51 e DA-2.57c). Mora na Mid, junto dos códigos que classifica,
// para ser testado sem a CLI. Classes: 4 documento reprovado no leiaute (entrada, XML assinado, identificação); 2 ambiente do
// XSD; 5 DPS duplicada na Sefin (E0014 com infDPS diferente); 7 comparação indisponível; 1 recusa de regra (credencial,
// assinatura, confirmação, cofre, Sefin). Código fora dessas listas sai como 70 (EX_SOFTWARE): código novo sem classificação
// não passa calado.
#ifndef NL_MID_COMPOSICAO_CODIGO_SAIDA_H
#define NL_MID_COMPOSICAO_CODIGO_SAIDA_H

#include <string_view>

namespace nl::mid::composicao {

/// Código de saída da CLI para o código estável de uma emissão recusada.
int codigoDeSaidaDaEmissao(std::string_view codigo);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_CODIGO_SAIDA_H
