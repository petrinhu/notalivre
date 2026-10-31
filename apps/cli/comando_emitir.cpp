// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre emitir` (NL-2.9, DA-2.44, DA-2.45). Ver comando_emitir.h. WIP: sem verificação.
#include "comando_emitir.h"

#include "argumentos_emitir.h"
#include "configuracao_cli.h"

#include <cmath>
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

namespace {

// Maior inteiro exato que o leiaute aceita para série (5 dígitos) e número (15 dígitos): acima disso o campo é inválido.
constexpr double kMaiorInteiroDoCampo = 999999999999999.0;

// Inteiro exato de um campo da DPS (NL-2.10b, BAIXA-6). Ausente devolve vazio. Número JSON sem fração e dentro da faixa é aceito;
// fração, texto, booleano, objeto ou valor fora da faixa é campo inválido, com saída 4 e código entrada.numero_invalido.
std::expected<std::optional<qint64>, ErroNumero> inteiroDoCampo(const QJsonObject& objeto, const QString& campo)
{
    const QJsonValue valor = objeto.value(campo);
    if (valor.isUndefined()) {
        return std::optional<qint64>{};
    }
    if (!valor.isDouble()) {
        return std::unexpected(ErroNumero{4, QStringLiteral("entrada.numero_invalido"),
                                          QStringLiteral("campo %1 da DPS não é um número inteiro").arg(campo)});
    }
    const double numero = valor.toDouble();
    if (std::trunc(numero) != numero || std::fabs(numero) > kMaiorInteiroDoCampo) {
        return std::unexpected(ErroNumero{4, QStringLiteral("entrada.numero_invalido"),
                                          QStringLiteral("campo %1 da DPS fora de inteiro exato").arg(campo)});
    }
    return std::optional<qint64>{static_cast<qint64>(numero)};
}

// Confere um campo inteiro da DPS contra o valor da linha de comando. Ausente recebe o valor; igual passa; diferente é conflito,
// saída 2: a CLI não escolhe um dos dois valores.
std::expected<void, ErroNumero> conferirCampoDaDps(const QJsonObject& objeto, const QString& campo, qint64 valor)
{
    auto atual = inteiroDoCampo(objeto, campo);
    if (!atual) {
        return std::unexpected(atual.error());
    }
    if (atual->has_value() && **atual != valor) {
        return std::unexpected(ErroNumero{2, QStringLiteral("emissao.numero_conflitante"),
                                          QStringLiteral("%1 da DPS diferente da linha de comando: a DPS traz %2 e a linha de comando pede %3")
                                              .arg(campo)
                                              .arg(**atual)
                                              .arg(valor)});
    }
    return {};
}

} // namespace

std::expected<QByteArray, ErroNumero> aplicarSerieENumero(const QByteArray& dpsJson, const ArgumentosEmitir& argumentos)
{
    QJsonParseError erroJson;
    const QJsonDocument documento = QJsonDocument::fromJson(dpsJson, &erroJson);
    if (erroJson.error != QJsonParseError::NoError || !documento.isObject()) {
        return std::unexpected(ErroNumero{4, QStringLiteral("entrada.numero_invalido"), QStringLiteral("a DPS de entrada não é um objeto JSON")});
    }
    bool serieValida = false;
    bool numeroValido = false;
    const qint64 serie = argumentos.serie.toLongLong(&serieValida);
    const qint64 numero = argumentos.numero.toLongLong(&numeroValido);
    if (!serieValida || !numeroValido) {
        return std::unexpected(ErroNumero{2, QStringLiteral("argumento.numero_invalido"), QStringLiteral("série ou número da linha de comando inválidos")});
    }
    QJsonObject objeto = documento.object();
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
    const bool homologacao = *derivado == nl::mid::composicao::Ambiente::Homologacao;
    // Âncoras e [xsd] definidos fora de Homologação são erro de configuração, código 10 (condição do CISO, 31/10/2026; BAIXA-4).
    // A guarda olha a presença da chave: a leitura já recusou o valor vazio, então não há como contornar por ele (BAIXA-5).
    const auto ancorasConferidas = conferirAncorasDeAmbiente(*config, homologacao);
    if (!ancorasConferidas) {
        erro << QString::fromStdString(ancorasConferidas.error().codigo) << ": " << ancorasConferidas.error().mensagem << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao(ancorasConferidas.error().codigo);
    }
    // STUB NL-2.10b (R9, vermelho por asserção): a conferência de [xsd] fora de Homologação ainda não é aplicada na emissão.
    const auto xsdConferido = conferirXsdDeAmbiente(*config, true);
    if (!xsdConferido) {
        erro << QString::fromStdString(xsdConferido.error().codigo) << ": " << xsdConferido.error().mensagem << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao(xsdConferido.error().codigo);
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
    // --serie e --numero entram na DPS (NL-2.10b, emenda 3). Conflito sai com 2; número inválido no JSON sai com 4.
    const auto dpsAplicada = aplicarSerieENumero(arquivoDps.readAll(), *lidos);
    if (!dpsAplicada) {
        erro << dpsAplicada.error().codigo << ": " << dpsAplicada.error().mensagem << Qt::endl;
        return dpsAplicada.error().saida;
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
        diretorioPadraoDasRaizesIcp(QCoreApplication::applicationDirPath()),
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
