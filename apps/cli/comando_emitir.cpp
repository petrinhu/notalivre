// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre emitir` (NL-2.9, DA-2.44, DA-2.45). Ver comando_emitir.h. WIP: sem verificação.
#include "comando_emitir.h"

#include "argumentos_emitir.h"
#include "configuracao_cli.h"

#include <optional>
#include <string>

#include <nl/mid/composicao/emissao_cli.h>
#include <nl/mid/composicao/codigo_saida.h>
#include <nl/mid/composicao/politica_emissao.h>
#include <nl/mid/composicao/registro_cli.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTextStream>


namespace nl::cli {

// Confere um campo inteiro da DPS contra o valor da linha de comando. Campo ausente recebe o valor; campo diferente é conflito.
std::expected<void, QString> conferirCampoDaDps(const QJsonObject& objeto, const QString& campo, qint64 valor)
{
    const QJsonValue atual = objeto.value(campo);
    if (!atual.isUndefined() && atual.toInteger() != valor) {
        return std::unexpected(QStringLiteral("%1 da DPS diferente da linha de comando: a DPS traz %2 e a linha de comando pede %3")
                                   .arg(campo)
                                   .arg(atual.toInteger())
                                   .arg(valor));
    }
    return {};
}

std::expected<QByteArray, QString> aplicarSerieENumero(const QByteArray& dpsJson, const ArgumentosEmitir& argumentos)
{
    QJsonParseError erroJson;
    const QJsonDocument documento = QJsonDocument::fromJson(dpsJson, &erroJson);
    if (erroJson.error != QJsonParseError::NoError || !documento.isObject()) {
        return std::unexpected(QStringLiteral("a DPS de entrada não é um objeto JSON"));
    }
    QJsonObject objeto = documento.object();
    const qint64 serie = argumentos.serie.toLongLong();
    const qint64 numero = argumentos.numero.toLongLong();
    // Conflito recusa a emissão (código 2): não se escolhe um dos dois valores (NL-2.10b, emenda 3).
    if (auto conferido = conferirCampoDaDps(objeto, QStringLiteral("serie"), serie); !conferido) {
        return std::unexpected(conferido.error());
    }
    if (auto conferido = conferirCampoDaDps(objeto, QStringLiteral("nDps"), numero); !conferido) {
        return std::unexpected(conferido.error());
    }
    objeto.insert(QStringLiteral("serie"), QJsonValue(serie));
    objeto.insert(QStringLiteral("nDps"), QJsonValue(numero));
    return QJsonDocument(objeto).toJson(QJsonDocument::Compact);
}

int executarEmitir(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro)
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
    const auto config = lerConfiguracao(caminhoConfiguracao(), QCoreApplication::applicationDirPath());
    if (!config) {
        // Configuração ou banco (NL-3.8, código 10): a tabela de códigos de saída classifica o código da configuração.
        erro << config.error().mensagem << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao(config.error().codigo);
    }
    // O endpoint decide o ambiente (NL-2.9, item 1 da triagem do CISO): a flag só pode confirmar, nunca trocar.
    const auto derivado = nl::mid::composicao::ambienteDoEndpoint(config->urlSefin);
    if (!derivado) {
        erro << QString::fromStdString(derivado.error().mensagem()) << Qt::endl;
        return 2;
    }
    // Âncoras definidas fora de Homologação são erro de configuração, código 10 (condição do CISO, 31/10/2026).
    const auto ancorasConferidas = conferirAncorasDeAmbiente(*config, *derivado == nl::mid::composicao::Ambiente::Homologacao);
    if (!ancorasConferidas) {
        erro << QString::fromStdString(ancorasConferidas.error().codigo) << ": " << ancorasConferidas.error().mensagem << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao(ancorasConferidas.error().codigo);
    }
    const auto conferido = nl::mid::composicao::conferirFlagDoAmbiente(*ambiente, *derivado);
    if (!conferido) {
        erro << QString::fromStdString(conferido.error().mensagem()) << Qt::endl;
        return 2;
    }
    const auto liberado = nl::mid::composicao::liberarAmbiente(nl::mid::composicao::ConfigEmissao{*derivado, config->producaoLiberada});
    if (!liberado) {
        erro << QString::fromStdString(liberado.error().mensagem()) << Qt::endl;
        return 1;
    }
    QFile arquivoDps(lidos->entrada);
    if (!arquivoDps.open(QIODevice::ReadOnly)) {
        erro << QStringLiteral("não foi possível ler a DPS de entrada: %1").arg(lidos->entrada) << Qt::endl;
        return 2;
    }
    // --serie e --numero entram na DPS (NL-2.10b, emenda 3); conflito com o que a DPS traz sai com 2, sem escolher um dos dois.
    const auto dpsAplicada = aplicarSerieENumero(arquivoDps.readAll(), *lidos);
    if (!dpsAplicada) {
        erro << dpsAplicada.error() << Qt::endl;
        return 2;
    }
    // Produção: o CPF ou CNPJ é digitado no terminal, antes da senha, e confere com o A1 e com o emitente (item 2).
    std::optional<QString> confirmacao;
    if (*derivado == nl::mid::composicao::Ambiente::Producao) {
        confirmacao = nl::mid::composicao::lerConfirmacaoPeloTerminal(QStringLiteral("confirme o CPF ou CNPJ do emissor: "));
        if (!confirmacao || confirmacao->isEmpty()) {
            erro << QStringLiteral("Produção exige o CPF ou CNPJ digitado no terminal como confirmação") << Qt::endl;
            return 1;
        }
    }
    // O .pfx vem da configuração (NL-2.10a, passo 5). Ilegível ou inexistente sai com 10 e a instrução do Flatpak; o caminho não
    // vai à saída nem ao log: só o evento certificado.caminho_configurado, sem campo.
    {
        QFile certificado(config->caminhoCertificado);
        if (!certificado.open(QIODevice::ReadOnly)) {
            erro << QStringLiteral("certificado.caminho_ilegivel: o arquivo do certificado não existe ou o app não consegue lê-lo. "
                                   "No Flatpak, o arquivo precisa estar acessível ao app: rode flatpak document-export e grave o "
                                   "caminho impresso em [certificado] caminho.")
                 << Qt::endl;
            return nl::mid::composicao::codigoDeSaidaDaEmissao("certificado.caminho_ilegivel");
        }
    }
    registro.registrarCertificadoConfigurado();
    const auto algoritmo = config->algoritmoAssinatura == AlgoritmoConfigurado::RsaSha256 ? nl::mid::composicao::AlgoritmoAssinatura::RsaSha256
                                                                                       : nl::mid::composicao::AlgoritmoAssinatura::RsaSha1;
    nl::mid::composicao::PedidoEmissao pedido{
        *dpsAplicada,
        nl::mid::composicao::LocalizacaoEsquemaDps{config->xsdDiretorio, config->xsdManifesto},
        config->caminhoCertificado,
        config->urlSefin,
        config->ancorasCertificado,
        confirmacao.value_or(QString()),
        algoritmo,
        config->prefixo,
        config->ancorasTls,
        *derivado,
        QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../share/notalivre/icp-brasil")),
    };
    // A senha só entra pelo terminal, sem eco, dentro da fachada: a CLI nunca a vê (DA-2.57b).
    const auto chave = nl::mid::composicao::emitirDpsPeloTerminal(pedido, registro, QStringLiteral("senha do certificado: "));
    if (!chave) {
        if (chave.error().codigo == "cofre.senha_ausente") {
            erro << QString::fromStdString(chave.error().mensagem) << Qt::endl;
            return 2;
        }
        erro << QStringLiteral("emissão recusada (%1): %2")
                    .arg(QString::fromStdString(chave.error().codigo), QString::fromStdString(chave.error().mensagem))
             << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao(chave.error().codigo);
    }
    saida << QStringLiteral("emitida: chave ") << *chave << Qt::endl;
    return 0;
}

} // namespace nl::cli
