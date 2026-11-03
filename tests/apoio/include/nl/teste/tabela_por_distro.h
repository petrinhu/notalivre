// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor genérico de tabela por distro (NL-2.14, adendo 2 da DA-2.61). A 3ª coluna é o valor da expectativa; quem chama
// valida o conjunto de valores permitidos. O leitor do SHA-1 (tabela_sha1_distro.h) não muda de contrato.
#ifndef NL_TESTE_APOIO_TABELA_POR_DISTRO_H
#define NL_TESTE_APOIO_TABELA_POR_DISTRO_H

#include <QString>
#include <QStringList>

namespace nl::teste {

// Valor da 3ª coluna da linha de dados do par ID/VERSION_ID (a última que casa vence). Vazio se a tabela não abre ou
// se não há linha para o par.
QString valorDaDistro(const QString& caminhoTabela, const QString& id, const QString& versao);

// Valor da 3ª coluna para a distro atual (ID e VERSION_ID do /etc/os-release). Vazio se não há linha.
QString valorDaDistroAtual(const QString& caminhoTabela);

// Valores da 3ª coluna de todas as linhas de dados (com 3 campos, sem comentário), na ordem do arquivo. Vazia se a tabela
// não abre ou não tem linha de dados.
QStringList valoresDaTabela(const QString& caminhoTabela);

} // namespace nl::teste

#endif // NL_TESTE_APOIO_TABELA_POR_DISTRO_H
