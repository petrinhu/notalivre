// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre config` (NL-2.10a, passo 6): grava [certificado] caminho no config.ini, preservando as outras chaves.
#ifndef NL_CLI_COMANDO_CONFIG_H
#define NL_CLI_COMANDO_CONFIG_H

#include <QStringList>

namespace nl::cli {

/// `notalivre config certificado <caminho>`. Confere que o arquivo existe e é legível, e grava [certificado] caminho no config.ini
/// de caminhoConfiguracao(). Devolve o código de saída: 0 gravou; 2 uso; 10 arquivo ilegível (certificado.caminho_ilegivel).
int executarConfig(const QStringList& argumentos);

} // namespace nl::cli

#endif // NL_CLI_COMANDO_CONFIG_H
