// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor único da tabela de expectativa de SHA-1 por distro (DA-2.38, DA-2.40, DA-2.61). Usado por tst_assinatura_dps,
// tst_assinador_xml e tst_cli_emitir. Antes, cada um tinha a sua cópia (L-33, regra de 3).
#ifndef NL_TESTE_APOIO_TABELA_SHA1_DISTRO_H
#define NL_TESTE_APOIO_TABELA_SHA1_DISTRO_H

#include <QString>

namespace nl::teste {

// Campo de /etc/os-release, sem aspas. Sem VERSION_ID (ex.: manjaro, rolling) devolve "rolling". Sem o arquivo, ou sem o campo
// (fora VERSION_ID), devolve vazio.
QString campoDoOsRelease(const QString& campo);

// Expectativa de SHA-1 do par ID/VERSION_ID na tabela: "sha1" ou "algoritmo_bloqueado". Vazio quando a tabela não abre, a
// distro não tem linha, ou o valor da linha não é um dos dois. Quem chama reprova o teste com o vazio.
QString expectativaSha1(const QString& caminhoTabela, const QString& id, const QString& versao);

// Expectativa da distro atual: ID e VERSION_ID do /etc/os-release, lidos na tabela de caminhoTabela.
QString expectativaSha1DaDistroAtual(const QString& caminhoTabela);

} // namespace nl::teste

#endif // NL_TESTE_APOIO_TABELA_SHA1_DISTRO_H
