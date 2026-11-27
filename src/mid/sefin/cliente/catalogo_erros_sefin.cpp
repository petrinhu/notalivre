// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo de erros da Sefin (NL-2.8, ADR-0005 c; NL-3.L). FATO (DA-2.41, conferido de novo na NL-3.L no Anexo I de 24/11/2026):
// aba RN_RECEP_DPS, E1200 na linha Excel 3, E1203 a E1208 nas linhas 4 a 8 (E1207 na 7), E1209 na 9 e E1235 na 17; aba
// RN DPS_NFS-e 1.01, E0238 na 283, E0244 na 284, E0245 na 285, E0653 nas linhas 286 (CEP do tomador) e 555 (retenção pela
// exceção do município), E0908 na 299, E0910 na 300, E0920 a E0922 nas 310 a 312, E0924 na 313, E0901 na 605 e E0714 na 661.
// Os textos de E0714 e de E1203 a E1208 vêm da planilha. Os de E0238 em diante (NL-3.L) partem do texto da planilha e dizem o que
// corrigir (ponto 5 ratificado pelo líder, 27/11/2026: "Como rejeição ativa (Recomendado)"). E1204 não está na aba RN_RECEP_DPS de
// 24/11/2026; não se inventa um texto. E2404 é INFERÊNCIA (pesquisa g e texto do servidor falso): é código da API de consulta,
// fora do Anexo I. Código fora desta tabela não ganha texto inventado.
#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>

#include <array>
#include <utility>

namespace nl::mid::sefin::cliente {
namespace {

constexpr std::array<std::pair<std::string_view, std::string_view>, 21> kCatalogo{{
    {"E0238", "O município do endereço do tomador não existe na tabela de municípios do IBGE. Confira o código do município do tomador."},
    {"E0244", "O município do endereço do tomador não é o do cadastro do CNPJ dele na data de competência. Confira o município do "
              "tomador com o cadastro do CNPJ."},
    {"E0245", "O município do endereço do tomador não é o do cadastro do CPF dele na data de competência. Confira o município do "
              "tomador com o cadastro do CPF."},
    {"E0653", "A Sefin recusou o CEP do tomador (não existe ou não é do município informado) ou a retenção do ISSQN pela regra do "
              "município. Confira o CEP e o município do tomador e, se houver retenção, a regra do município de incidência."},
    {"E0714", "Assinatura inválida"},
    {"E0901", "O código indicador da operação (cIndOp) não vale para prestação de serviços. A tabela de cIndOp da Sefin mudou: "
              "atualize o NotaLivre e as tabelas e confira o código."},
    {"E0908", "O destinatário foi informado com indDest 0. Com indDest 0 o destinatário é o próprio tomador e não se informa à parte."},
    {"E0910", "Com indDest 1 o grupo do destinatário é obrigatório, e o app ainda não tem esse grupo. Use indDest 0 quando o "
              "destinatário é o próprio tomador."},
    {"E0920", "O município do endereço do destinatário não existe na tabela de municípios do IBGE. Confira o código do município do "
              "destinatário."},
    {"E0921", "O município do endereço do destinatário não é o do cadastro do CNPJ dele na data de competência. Confira o município "
              "do destinatário com o cadastro do CNPJ."},
    {"E0922", "O município do endereço do destinatário não é o do cadastro do CPF dele na data de competência. Confira o município "
              "do destinatário com o cadastro do CPF."},
    {"E0924", "O CEP do destinatário não existe ou não é do município informado. Confira o CEP e o município do destinatário."},
    {"E1200", "Certificado de Transmissão inválido. Use um certificado A1 de pessoa (não de autoridade certificadora), versão 3 e "
              "com uso de autenticação de cliente."},
    {"E1203", "Certificado de Transmissão expirado"},
    {"E1205", "Certificado de Transmissão - Erro na cadeia de certificação"},
    {"E1206", "Certificado de Transmissão - Erro de acesso à LCR"},
    {"E1207", "Certificado de Transmissão revogado"},
    {"E1208", "Certificado de Transmissão difere da ICP-Brasil"},
    {"E1209", "Certificado de Transmissão sem CNPJ ou CPF. Use um certificado A1 da ICP-Brasil com o CNPJ ou o CPF do emitente."},
    {"E1235", "A Sefin recusou o XML pelo esquema (leiaute) em vigor. O leiaute da Sefin mudou e esta versão do app não o acompanha: "
              "atualize o NotaLivre."},
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
