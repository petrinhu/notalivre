// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Configuração da CLI (NL-2.9, DA-2.44). Ver configuracao_cli.h. FATIA VERMELHA: ainda não lê nada.
#include "configuracao_cli.h"

namespace nl::cli {

QString caminhoConfiguracao()
{
    return QString();
}

std::expected<ConfiguracaoCli, QString> lerConfiguracao(const QString& caminho)
{
    static_cast<void>(caminho);
    return std::unexpected(QStringLiteral("configuração ainda não implementada"));
}

} // namespace nl::cli
