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

// Maior valor que cada campo do leiaute aceita (NL-2.10b, emenda 3): série tem cinco dígitos e número tem quinze.
constexpr double kMaiorSerieDoLeiaute = 99999.0;
constexpr double kMaiorNumeroDoLeiaute = 999999999999999.0;

// Inteiro exato de um campo da DPS (NL-2.10b, BAIXA-6). Ausente devolve vazio. Número JSON sem fração e dentro do limite do campo é
// aceito; fração, texto, booleano, objeto ou valor acima do limite do campo é inválido, com saída 4 e código entrada.numero_invalido.
std::expected<std::optional<qint64>, ErroNumero> inteiroDoCampo(const QJsonObject& objeto, const QString& campo, double limite)
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
    if (std::trunc(numero) != numero || std::fabs(numero) > limite) {
        return std::unexpected(ErroNumero{4, QStringLiteral("entrada.numero_invalido"),
                                          QStringLiteral("campo %1 da DPS fora de inteiro exato").arg(campo)});
    }
    return std::optional<qint64>{static_cast<qint64>(numero)};
}

// Confere um campo inteiro da DPS contra o valor da linha de comando. Ausente recebe o valor; igual passa; diferente é conflito,
// saída 2: a CLI não escolhe um dos dois valores.
std::expected<void, ErroNumero> conferirCampoDaDps(const QJsonObject& objeto, const QString& campo, qint64 valor, double limite)
{
    auto atual = inteiroDoCampo(objeto, campo, limite);
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

// Conferência da entrada da senha antes de ler a configuração (NL-2.27, DA-2.68): com descritor, só o número é conferido, sem ler
// nenhum byte; sem descritor, o terminal é conferido. Terminal em segundo plano sai aqui, antes de qualquer tcsetattr.
std::expected<void, nl::mid::composicao::FalhaEmissao> conferirEntradaDaSenha(const ArgumentosEmitir& lidos)
{
    if (lidos.descritorSenha) {
        return nl::mid::composicao::conferirEntradaDaSenhaPeloDescritor(*lidos.descritorSenha);
    }
    return nl::mid::composicao::conferirEntradaDaSenhaPeloTerminal();
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
    if (auto conferido = conferirCampoDaDps(objeto, QStringLiteral("serie"), serie, kMaiorSerieDoLeiaute); !conferido) {
        return std::unexpected(conferido.error());
    }
    if (auto conferido = conferirCampoDaDps(objeto, QStringLiteral("nDps"), numero, kMaiorNumeroDoLeiaute); !conferido) {
        return std::unexpected(conferido.error());
    }
    objeto.insert(QStringLiteral("serie"), QJsonValue(serie));
    objeto.insert(QStringLiteral("nDps"), QJsonValue(numero));
    return QJsonDocument(objeto).toJson(QJsonDocument::Compact);
}

int executarEmitir(const QStringList& argumentos, nl::mid::composicao::RegistroCli& registro,
                    const nl::mid::composicao::TomadaDaSenha* tomada)
{
    QTextStream erro{stderr};
    QTextStream saida{stdout};
    const auto lidos = lerArgumentosEmitir(argumentos);
    if (!lidos) {
        erro << lidos.error() << Qt::endl;
        return 2;
    }
    // NL-2.27: a entrada da senha é conferida antes de qualquer leitura de configuração (ordem da DA-2.68).
    if (const auto entrada = conferirEntradaDaSenha(*lidos); !entrada) {
        erro << QString::fromStdString(entrada.error().codigo) << ": " << QString::fromStdString(entrada.error().mensagem) << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao(entrada.error().codigo);
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
    // [xsd] definido fora de Homologação é erro de configuração, código 10 (condição do CISO; BAIXA-4; R9 do qa).
    const auto xsdConferido = conferirXsdDeAmbiente(*config, homologacao);
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
    // NL-2.27 (DA-2.68): Produção com senha por descritor sai com 11 antes de ler o descritor, de abrir o A1 e de qualquer envio.
    // Depois da trava da NL-2.9, que já saiu com 1 se Produção não estava liberada.
    if (lidos->descritorSenha && *derivado == nl::mid::composicao::Ambiente::Producao) {
        erro << QStringLiteral("headless.sem_autorizacao: Produção não aceita senha por descritor nesta versão; a autorização headless chega na NL-3.9")
             << Qt::endl;
        return nl::mid::composicao::codigoDeSaidaDaEmissao("headless.sem_autorizacao");
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
    // NL-2.27b (DA-2.72): na leitura, só vale o descritor tomado na entrada (mesmo dispositivo e inode).
    if (lidos->descritorSenha && tomada != nullptr) {
        if (const auto conferida = nl::mid::composicao::conferirTomadaDaSenha(*tomada); !conferida) {
            erro << QString::fromStdString(conferida.error().codigo) << ": " << QString::fromStdString(conferida.error().mensagem) << Qt::endl;
            return nl::mid::composicao::codigoDeSaidaDaEmissao(conferida.error().codigo);
        }
    }
    // A senha entra pela fachada, do terminal sem eco ou do descritor dado por --senha-certificado-fd: a CLI nunca a vê (DA-2.57b).
    const auto chave = lidos->descritorSenha
                           ? nl::mid::composicao::emitirDpsPeloDescritor(pedido, registro, *lidos->descritorSenha)
                           : nl::mid::composicao::emitirDpsPeloTerminal(pedido, registro, QStringLiteral("senha do certificado: "));
    if (!chave) {
        if (chave.error().codigo.rfind("cofre.", 0) == 0) {
            erro << QString::fromStdString(chave.error().codigo) << ": " << QString::fromStdString(chave.error().mensagem) << Qt::endl;
            return nl::mid::composicao::codigoDeSaidaDaEmissao(chave.error().codigo);
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

namespace nl::cli {

std::expected<std::optional<nl::mid::composicao::TomadaDaSenha>, nl::mid::composicao::FalhaEmissao> tomarEntradaDaSenhaNaLinha(
    QStringList& argumentos)
{
    using nl::mid::composicao::TomadaDaSenha;
    if (argumentos.size() < 2 || argumentos.at(0) != QLatin1String("emitir")) {
        return std::optional<TomadaDaSenha>{};
    }
    for (qsizetype i = 1; i + 1 < argumentos.size(); ++i) {
        if (argumentos.at(i) != QLatin1String("--senha-certificado-fd")) {
            continue;
        }
        // Número com dígitos só, como no analisador. Fora disso, o analisador recusa com uso (código 2), sem tomar nada.
        const QString valor = argumentos.at(i + 1);
        bool digitos = !valor.isEmpty() && valor.size() <= 6;
        for (const QChar caractere : valor) {
            if (caractere.unicode() < u'0' || caractere.unicode() > u'9') {
                digitos = false;
            }
        }
        if (!digitos) {
            return std::optional<TomadaDaSenha>{};
        }
        auto tomada = nl::mid::composicao::tomarEntradaDaSenhaPeloDescritor(valor.toInt());
        if (!tomada) {
            return std::unexpected(tomada.error());
        }
        argumentos[i + 1] = QString::number(tomada->fd);
        return std::optional<TomadaDaSenha>(*tomada);
    }
    return std::optional<TomadaDaSenha>{};
}

} // namespace nl::cli
