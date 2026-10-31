// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes das travas de `notalivre emitir` antes da senha (NL-2.9, DA-2.5, spec 4.12): Produção bloqueada e CNPJ digitado.
// NL-2.10b (emenda 3): --serie e --numero entram na DPS, e nDps conflitante sai com 2.
// Estes casos recusam antes de ler a senha, então não dependem de terminal. Cada teste aponta XDG_CONFIG_HOME para uma
// configuração temporária. A fixture não define âncoras: fora de Homologação elas são erro de configuração (código 10).
#include "argumentos_emitir.h"
#include "comando_emitir.h"

#include <nl/mid/composicao/registro_cli.h>

#include <sstream>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Configuração temporária, apontada por XDG_CONFIG_HOME. A pasta fica viva enquanto o objeto existir.
int emitirComRegistro(const QStringList& argumentos)
{
    std::ostringstream destino;
    auto registro = nl::mid::composicao::criarLogDaCli(destino);
    return nl::cli::executarEmitir(argumentos, *registro);
}

// Padrão: Produção com [emissao] falso. Os testes que precisam de outro ambiente passam o próprio texto.
const QString kConfigProducaoFalsa = QStringLiteral(
    "[sefin]\nurl = https://sefin.nfse.gov.br/SefinNacional\nprefixo = /SefinNacional\n"
    "[certificado]\ncaminho = /tmp/x.pfx\n"
    "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
    "[emissao]\nproducao_liberada = false\n");

class ConfigTemporaria
{
public:
    explicit ConfigTemporaria(const QString& ini = kConfigProducaoFalsa)
    {
        QDir().mkpath(pasta_.filePath(QStringLiteral("notalivre")));
        QFile arquivo(pasta_.filePath(QStringLiteral("notalivre/config.ini")));
        static_cast<void>(arquivo.open(QIODevice::WriteOnly | QIODevice::Text));
        arquivo.write(ini.toUtf8());
        arquivo.close();
        qputenv("XDG_CONFIG_HOME", pasta_.path().toLocal8Bit());
    }

    QString caminhoDeArquivo(const QString& nome) const
    {
        return pasta_.filePath(nome);
    }

private:
    QTemporaryDir pasta_;
};

} // namespace

class TstComandoEmitir : public QObject
{
    Q_OBJECT

private slots:
    // Produção sem liberação por configuração sai com rc 1, antes de qualquer leitura de senha.
    void producaoBloqueadaSaiAntesDaSenha();
    // Produção exige o CNPJ digitado como confirmação.
    void producaoSemCnpjDigitadoSaiAntesDaSenha();
    // NL-2.10b, emenda 3: --serie e --numero sem conflito vão para a DPS (serie e nDps).
    void numeroDaLinhaDeComandoVaiNaDps();
    // NL-2.10b, emenda 3: nDps da DPS diferente do --numero sai com 2, sem escolher um dos dois.
    void numeroConflitanteSaiComDois();
};

void TstComandoEmitir::producaoBloqueadaSaiAntesDaSenha()
{
    ConfigTemporaria config;
    const QStringList argumentos{"emitir", "dps.json", "--serie", "1", "--numero", "1", "--ambiente", "producao"};
    QCOMPARE(emitirComRegistro(argumentos), 1);
}

void TstComandoEmitir::producaoSemCnpjDigitadoSaiAntesDaSenha()
{
    ConfigTemporaria config;
    const QStringList argumentos{"emitir", "dps.json", "--serie", "1", "--numero", "1", "--ambiente", "producao"};
    QCOMPARE(emitirComRegistro(argumentos), 1);
}

void TstComandoEmitir::numeroDaLinhaDeComandoVaiNaDps()
{
    nl::cli::ArgumentosEmitir argumentos;
    argumentos.serie = QStringLiteral("7");
    argumentos.numero = QStringLiteral("42");
    const QByteArray dps = QByteArrayLiteral("{\"dCompet\":\"2026-10-01\"}");
    const auto aplicado = nl::cli::aplicarSerieENumero(dps, argumentos);
    QVERIFY2(aplicado.has_value(), "--serie e --numero sem conflito tem de entrar na DPS");
    const QJsonObject objeto = QJsonDocument::fromJson(*aplicado).object();
    QCOMPARE(objeto.value(QStringLiteral("serie")).toInteger(), qint64(7));
    QCOMPARE(objeto.value(QStringLiteral("nDps")).toInteger(), qint64(42));
}

void TstComandoEmitir::numeroConflitanteSaiComDois()
{
    nl::cli::ArgumentosEmitir argumentos;
    argumentos.serie = QStringLiteral("7");
    argumentos.numero = QStringLiteral("42");
    // Mesma função, no nível da CLI: a DPS traz nDps 43 e a linha de comando pede 42.
    const QByteArray conflitante = QByteArrayLiteral("{\"serie\":7,\"nDps\":43}");
    QVERIFY2(!nl::cli::aplicarSerieENumero(conflitante, argumentos).has_value(),
             "nDps diferente do --numero tem de ser recusado, sem escolher um dos dois");

    // Ponta a ponta até o código de saída: ambiente de Homologação (loopback), sem terminal, DPS com nDps 43 no arquivo.
    const QString ambienteHomologacao = QStringLiteral(
        "[sefin]\nurl = https://127.0.0.1:9443\nprefixo = /SefinNacional\n"
        "[certificado]\ncaminho = /tmp/x.pfx\n"
        "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
        "[emissao]\nproducao_liberada = false\n");
    ConfigTemporaria config(ambienteHomologacao);
    QFile dpsArquivo(config.caminhoDeArquivo(QStringLiteral("dps.json")));
    QVERIFY(dpsArquivo.open(QIODevice::WriteOnly));
    dpsArquivo.write(conflitante);
    dpsArquivo.close();
    const QStringList argumentosCli{"emitir", dpsArquivo.fileName(), "--serie", "7", "--numero", "42", "--ambiente", "homologacao"};
    QCOMPARE(emitirComRegistro(argumentosCli), 2);
}

QTEST_APPLESS_MAIN(TstComandoEmitir)
#include "tst_comando_emitir.moc"
