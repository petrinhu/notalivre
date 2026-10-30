// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre emitir` (NL-2.9). Ver comando_emitir.h. WIP: sem verificação.
#include "comando_emitir.h"

#include "argumentos_emitir.h"
#include "senha_terminal.h"

#include <nl/mid/composicao/politica_emissao.h>

#include <QTextStream>

namespace nl::cli {

int executarEmitir(const QStringList& argumentos)
{
    QTextStream erro{stderr};
    const auto lidos = lerArgumentosEmitir(argumentos);
    if (!lidos) {
        erro << lidos.error() << Qt::endl;
        return 2;
    }
    const auto ambiente = nl::mid::composicao::ambienteDeTexto(lidos->ambiente.toStdString());
    if (!ambiente) {
        erro << QString::fromStdString(ambiente.error().mensagem()) << Qt::endl;
        return 2;
    }
    // A configuração de Produção nasce falsa: a liberação chega com o go/no-go do líder (Onda 12), não por esta CLI.
    const auto liberado = nl::mid::composicao::liberarAmbiente(nl::mid::composicao::ConfigEmissao{*ambiente, false});
    if (!liberado) {
        erro << QString::fromStdString(liberado.error().mensagem()) << Qt::endl;
        return 1;
    }
    if (*ambiente == nl::mid::composicao::Ambiente::Producao && lidos->cnpjConfirmado.isEmpty()) {
        erro << QStringLiteral("Produção exige o CNPJ digitado como confirmação (--cnpj-confirmado)") << Qt::endl;
        return 1;
    }
    const auto senha = lerSenhaSemEco(QStringLiteral("senha do certificado: "));
    if (!senha) {
        erro << QStringLiteral("senha ausente: sem terminal de controle, a CLI não lê a senha") << Qt::endl;
        return 2;
    }
    erro << QStringLiteral("envio ainda não ligado nesta fatia (WIP)") << Qt::endl;
    return 3;
}

} // namespace nl::cli
