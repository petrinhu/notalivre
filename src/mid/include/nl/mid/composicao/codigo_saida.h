// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de saída da CLI para uma emissão recusada (NL-2.9, DA-2.51 e DA-2.57c). Mora na Mid, junto dos códigos que classifica,
// para ser testado sem a CLI. Classes: 4 documento reprovado no leiaute (entrada, XML assinado, identificação, e regra local
// E seguido de 4 dígitos sem prefixo, NL-3.M, DA-3M.8); 2 ambiente do
// XSD; 5 DPS duplicada na Sefin (E0014 com infDPS diferente); 1 recusa de regra (credencial, assinatura, confirmação, cofre,
// Sefin com código E); 10 configuração ou banco (chave ausente, arquivo ou caminho ilegível, algoritmo fora da lista,
// envio.configuracao_da_conexao; NL-3.8). NL-3.C (plano 1.6): falha do envio sem código E: 8 certificado provável; 12 servidor
// não confirmado; 6 rede antes do envio e não concluído; 7 estado incerto e comparação indisponível (sefin.comparacao_indisponivel);
// 70 falha antes do envio e resposta sem código que o app reconhece. NL-2.27: erro de entrada da senha (cofre.*, sem terminal, em segundo plano, descritor recusado,
// senha vazia) sai com 2, por entrada explícita; Produção sem autorização headless (headless.sem_autorizacao) sai com 11.
// Código fora dessas listas sai como 70 (EX_SOFTWARE): código novo sem classificação não passa calado.
#ifndef NL_MID_COMPOSICAO_CODIGO_SAIDA_H
#define NL_MID_COMPOSICAO_CODIGO_SAIDA_H

#include <string_view>

namespace nl::mid::composicao {

/// Código de saída da CLI para o código estável de uma emissão recusada.
int codigoDeSaidaDaEmissao(std::string_view codigo);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_CODIGO_SAIDA_H
