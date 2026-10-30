// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo de erros da Sefin (NL-2.8, ADR-0005 c). FATO (DA-2.41): os códigos E1203 a E1208 vêm do Anexo I de 20/10/2026,
// aba RN_RECEP_DPS, linhas 4 a 8 da planilha (E1207 na linha 7); os textos de E1207 vêm dos sharedStrings da planilha.
// E1204 ainda não tem texto no catálogo: falta conferir a linha dele no Anexo I. Não se inventa um texto.
// E2404 e E0714 são INFERÊNCIA (pesquisa g
// e texto do servidor falso), a confirmar na NL-2.10. Código fora desta tabela não ganha texto inventado.
#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>

#include <array>
#include <utility>

namespace nl::mid::sefin::cliente {
namespace {

constexpr std::array<std::pair<std::string_view, std::string_view>, 7> kCatalogo{{
    {"E0714", "Assinatura inválida"},
    {"E1203", "Certificado de Transmissão expirado"},
    {"E1205", "Certificado de Transmissão - Erro na cadeia de certificação"},
    {"E1206", "Certificado de Transmissão - Erro de acesso à LCR"},
    {"E1207", "Certificado de Transmissão revogado"},
    {"E1208", "Certificado de Transmissão difere da ICP-Brasil"},
    {"E2404", "Não foi gerada uma NFS-e com o identificador de DPS informado"},
}};

constexpr std::string_view kGenerica = "A Sefin recusou o envio sem um código conhecido pelo app.";

} // namespace

std::string_view CatalogoErrosSefin::mensagem(std::string_view codigoE) noexcept
{
    for (const auto& [codigo, texto] : kCatalogo) {
        if (codigo == codigoE) {
            return texto;
        }
    }
    return kGenerica;
}

bool CatalogoErrosSefin::conhece(std::string_view codigoE) noexcept
{
    for (const auto& entrada : kCatalogo) {
        if (entrada.first == codigoE) {
            return true;
        }
    }
    return false;
}

} // namespace nl::mid::sefin::cliente
