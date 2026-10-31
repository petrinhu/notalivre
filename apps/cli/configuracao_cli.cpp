// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Configuração da CLI (NL-2.9, DA-2.44). Ver configuracao_cli.h. Formato INI mínimo: [secao], chave = valor, # comentário.
// NL-2.10b: [xsd] tem padrão do bundle; [certificado] ancoras e [sefin] ancoras_tls são opcionais e só valem em Homologação.
#include "configuracao_cli.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QMap>
#include <QStringList>

namespace nl::cli {
namespace {

// Valor de uma chave obrigatória, ou o erro com o código configuracao.chave_ausente e o nome completo secao.chave.
std::expected<QString, ErroConfiguracao> obrigatoria(const QMap<QString, QString>& valores, const QString& chave)
{
    const QString valor = valores.value(chave);
    if (valor.isEmpty()) {
        return std::unexpected(ErroConfiguracao{"configuracao.chave_ausente",
                                                 QStringLiteral("configuração sem a chave obrigatória %1").arg(chave)});
    }
    return valor;
}

// Diretório share/notalivre do bundle, a partir do diretório do executável: <dir>/../share/notalivre (NL-2.10b).
QString shareDoBundle(const QString& dirExecutavel)
{
    return QDir::cleanPath(dirExecutavel + QStringLiteral("/../share/notalivre"));
}

} // namespace

QString caminhoConfiguracao()
{
    const QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
    const QString base = xdg.isEmpty() ? QDir::homePath() + QStringLiteral("/.config") : xdg;
    return base + QStringLiteral("/notalivre/config.ini");
}

std::expected<void, ErroConfiguracao> conferirXsdDeAmbiente(const ConfiguracaoCli& cfg, bool homologacao)
{
    if (!homologacao && cfg.xsdDefinido) {
        return std::unexpected(ErroConfiguracao{"configuracao.xsd_fora_de_homologacao",
                                                 QStringLiteral("[xsd] não se define fora de Homologação: o XSD vem do bundle, com o pin embutido")});
    }
    return {};
}

QString diretorioPadraoDasRaizesIcp(const QString& dirExecutavel)
{
    return QDir::cleanPath(shareDoBundle(dirExecutavel) + QStringLiteral("/icp-brasil"));
}

std::expected<void, ErroConfiguracao> conferirAncorasDeAmbiente(const ConfiguracaoCli& cfg, bool homologacao)
{
    if (homologacao) {
        return {};
    }
    // Fora de Homologação, qualquer âncora definida é erro de configuração: falha fechada, código 10 (condição do CISO).
    if (!cfg.ancorasCertificado.isEmpty()) {
        return std::unexpected(ErroConfiguracao{"configuracao.ancoras_a1_fora_de_homologacao",
                                                 QStringLiteral("[certificado] ancoras não se define fora de Homologação: as raízes ICP do bundle valem em Produção e em Produção Restrita")});
    }
    if (!cfg.ancorasTls.isEmpty()) {
        return std::unexpected(ErroConfiguracao{"configuracao.ancoras_tls_fora_de_homologacao",
                                                 QStringLiteral("[sefin] ancoras_tls não se define fora de Homologação: o servidor usa o repositório de CAs do sistema")});
    }
    return {};
}

std::expected<ConfiguracaoCli, ErroConfiguracao> lerConfiguracao(const QString& caminho, const QString& dirExecutavel)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::unexpected(ErroConfiguracao{"configuracao.arquivo_ilegivel",
                                                 QStringLiteral("não foi possível ler a configuração em %1").arg(caminho)});
    }
    QMap<QString, QString> valores;  // chave "secao.nome" -> valor
    QString secao;
    const QStringList linhas = QString::fromUtf8(arquivo.readAll()).split(QLatin1Char('\n'));
    for (const QString& bruta : linhas) {
        const QString linha = bruta.trimmed();
        if (linha.isEmpty() || linha.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (linha.startsWith(QLatin1Char('[')) && linha.endsWith(QLatin1Char(']'))) {
            secao = linha.mid(1, linha.size() - 2).trimmed();
            continue;
        }
        const qsizetype igual = linha.indexOf(QLatin1Char('='));
        if (igual <= 0) {
            continue;
        }
        const QString chave = secao + QLatin1Char('.') + linha.left(igual).trimmed();
        const QString valor = linha.mid(igual + 1).trimmed();
        // NL-2.10b, BAIXA-5: chave repetida não escolhe uma das ocorrências, e chave sem valor não contorna a guarda de ambiente.
        if (valores.contains(chave)) {
            return std::unexpected(ErroConfiguracao{"configuracao.chave_duplicada", QStringLiteral("chave repetida na configuração: %1").arg(chave)});
        }
        if (valor.isEmpty()) {
            return std::unexpected(ErroConfiguracao{"configuracao.chave_vazia", QStringLiteral("chave sem valor na configuração: %1").arg(chave)});
        }
        valores.insert(chave, valor);
    }

    ConfiguracaoCli cfg;
    auto url = obrigatoria(valores, QStringLiteral("sefin.url"));
    if (!url) {
        return std::unexpected(url.error());
    }
    auto prefixo = obrigatoria(valores, QStringLiteral("sefin.prefixo"));
    if (!prefixo) {
        return std::unexpected(prefixo.error());
    }
    auto caminhoCertificado = obrigatoria(valores, QStringLiteral("certificado.caminho"));
    if (!caminhoCertificado) {
        return std::unexpected(caminhoCertificado.error());
    }
    cfg.urlSefin = *url;
    cfg.prefixo = *prefixo;
    cfg.caminhoCertificado = *caminhoCertificado;
    // Opcionais (NL-2.10b): sem a chave, vale o padrão do bundle (XSD) ou o conjunto de produção (âncoras, em Produção).
    cfg.ancorasTls = valores.value(QStringLiteral("sefin.ancoras_tls"));
    cfg.ancorasCertificado = valores.value(QStringLiteral("certificado.ancoras"));
    const QString diretorio = valores.value(QStringLiteral("xsd.diretorio"));
    const QString manifesto = valores.value(QStringLiteral("xsd.manifesto"));
    cfg.xsdDiretorio = diretorio.isEmpty() ? shareDoBundle(dirExecutavel) + QStringLiteral("/xsd") : diretorio;
    cfg.xsdManifesto = manifesto.isEmpty() ? cfg.xsdDiretorio + QStringLiteral("/SHA256SUMS") : manifesto;
    cfg.xsdDefinido = valores.contains(QStringLiteral("xsd.diretorio")) || valores.contains(QStringLiteral("xsd.manifesto"));
    // Produção só liga com a chave em "true" (go/no-go do líder). Qualquer outro valor, ou ausência, fica bloqueada.
    cfg.producaoLiberada = valores.value(QStringLiteral("emissao.producao_liberada")) == QStringLiteral("true");
    // [assinatura] algoritmo: ausente ou rsa-sha1 é o padrão (DA-2.2); rsa-sha256 é a alternativa. Qualquer outro valor sai com 10.
    const QString algoritmo = valores.value(QStringLiteral("assinatura.algoritmo"));
    if (algoritmo.isEmpty() || algoritmo == QStringLiteral("rsa-sha1")) {
        cfg.algoritmoAssinatura = AlgoritmoConfigurado::RsaSha1;
    } else if (algoritmo == QStringLiteral("rsa-sha256")) {
        cfg.algoritmoAssinatura = AlgoritmoConfigurado::RsaSha256;
    } else {
        return std::unexpected(ErroConfiguracao{"assinatura.algoritmo_invalido",
                                                 QStringLiteral("algoritmo de assinatura fora da lista (rsa-sha1 ou rsa-sha256): %1").arg(algoritmo)});
    }
    return cfg;
}

} // namespace nl::cli
