// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta crua da Sefin para análise local (NL-2.9, DA-2.59 item 4). A fachada grava o corpo da resposta de sucesso em
// arquivo 0600, com a PII mascarada: o XML da NFS-e sai como marcador e a chave sai com # no lugar dos caracteres. Nome do
// arquivo é o hash do Id, sem o Id nem a chave. Nenhuma senha passa por aqui.
#ifndef NL_MID_COMPOSICAO_RESPOSTA_CRUA_H
#define NL_MID_COMPOSICAO_RESPOSTA_CRUA_H

#include <nl/base/erro.h>

#include <QByteArray>
#include <QString>

#include <expected>
#include <string_view>

namespace nl::mid::composicao {

/// Corpo com a PII mascarada: nfseXmlGZipB64 vira marcador, chaveAcesso vira #. Corpo que não é JSON vira um aviso.
QByteArray mascararRespostaCrua(const QByteArray& corpo);

/// Grava o corpo mascarado em <pastaBase>/respostas/<hash do Id>.json com permissão 0600. Devolve o caminho gravado.
std::expected<QString, nl::base::Erro> gravarRespostaCrua(const QByteArray& corpo, const QString& pastaBase, std::string_view idDps);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_RESPOSTA_CRUA_H
