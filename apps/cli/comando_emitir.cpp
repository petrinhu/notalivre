// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre emitir` (NL-2.9, DA-2.44, DA-2.45). Ver comando_emitir.h. WIP: sem verificação.
#include "comando_emitir.h"

#include "argumentos_emitir.h"
#include "configuracao_cli.h"
#include "senha_terminal.h"

#include <nl/mid/composicao/emissao_cli.h>
#include <nl/mid/composicao/politica_emissao.h>

#include <QFile>
#include <QIODevice>
#include <QTextStream>


namespace nl::cli {

int executarEmitir(const QStringList& argumentos)
{
    QTextStream erro{stderr};
    QTextStream saida{stdout};
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
    // A configuração de Produção vem do arquivo XDG e nasce falsa: a liberação chega com o go/no-go do líder (Onda 12).
    const auto config = lerConfiguracao(caminhoConfiguracao());
    if (!config) {
        erro << config.error() << Qt::endl;
        return 2;
    }
    const auto liberado = nl::mid::composicao::liberarAmbiente(nl::mid::composicao::ConfigEmissao{*ambiente, config->producaoLiberada});
    if (!liberado) {
        erro << QString::fromStdString(liberado.error().mensagem()) << Qt::endl;
        return 1;
    }
    if (*ambiente == nl::mid::composicao::Ambiente::Producao && lidos->cnpjConfirmado.isEmpty()) {
        erro << QStringLiteral("Produção exige o CNPJ digitado como confirmação (--cnpj-confirmado)") << Qt::endl;
        return 1;
    }
    if (lidos->certificado.isEmpty()) {
        erro << QStringLiteral("falta o certificado: informe --certificado <caminho do .pfx>") << Qt::endl;
        return 2;
    }
    QFile arquivoDps(lidos->entrada);
    if (!arquivoDps.open(QIODevice::ReadOnly)) {
        erro << QStringLiteral("não foi possível ler a DPS de entrada: %1").arg(lidos->entrada) << Qt::endl;
        return 2;
    }
    // A senha só entra pelo terminal, sem eco. Sem terminal de controle, a CLI não emite.
    const auto senhaTerminal = lerSenhaSemEco(QStringLiteral("senha do certificado: "));
    if (!senhaTerminal) {
        erro << QStringLiteral("senha ausente: sem terminal de controle, a CLI não lê a senha") << Qt::endl;
        return 2;
    }

    nl::mid::composicao::PedidoEmissao pedido{
        arquivoDps.readAll(),
        nl::mid::composicao::LocalizacaoEsquemaDps{config->xsdDiretorio, config->xsdManifesto},
        lidos->certificado,
        config->urlSefin,
        config->ancoras,
    };
    const auto chave = nl::mid::composicao::emitirDpsPelaCli(pedido, *senhaTerminal);
    if (!chave) {
        erro << QStringLiteral("emissão recusada (%1): %2")
                    .arg(QString::fromStdString(chave.error().codigo()), QString::fromStdString(chave.error().mensagem()))
             << Qt::endl;
        return 1;
    }
    saida << QStringLiteral("emitida: chave ") << *chave << Qt::endl;
    return 0;
}

} // namespace nl::cli
