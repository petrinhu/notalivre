// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do Log da Foundation (NL-2.7, ADR-S4). Cada teste grava a saída em NL_LOGS_PII_DIR, que o portão
// de varredura lê. Os valores de referência vêm de tests/fixtures/pii/referencia.txt, o mesmo arquivo do portão.

#include <nl/foundation/log/log.h>
#include <nl/foundation/log/redacao.h>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

using nl::foundation::log::Campo;
using nl::foundation::log::DeclaracaoCampo;
using nl::foundation::log::Log;
using nl::foundation::log::Nivel;
using nl::foundation::log::Regra;

namespace {

constexpr std::size_t kTamanhoIdDps = 45;
constexpr std::string_view kMunicipio = "3550308";
constexpr std::string_view kSerie = "00001";
constexpr std::string_view kNumero = "000000000000001";

/// Allowlist de referência: o que o app declara. Qualquer outro nome é descartado.
std::vector<DeclaracaoCampo> allowlistDeReferencia()
{
    return {
        {"idDps", Regra::InscricaoMascarada},
        {"codigoErro", Regra::Cru},
        {"senha", Regra::Proibido},
        {"tokenAcesso", Regra::Proibido},
        {"envelopeAssinado", Regra::Proibido},
    };
}

/// Valor de referência da categoria, lido de NL_FIXTURES_PII/referencia.txt (formato categoria|valor). Vazio se ausente.
std::string referencia(const QString& categoria)
{
    QFile arquivo{QStringLiteral(NL_FIXTURES_PII) + QStringLiteral("/referencia.txt")};
    if (!arquivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QStringList linhas = QString::fromUtf8(arquivo.readAll()).split(QLatin1Char('\n'));
    for (const QString& linha : linhas) {
        const QStringList partes = linha.split(QLatin1Char('|'));
        if (partes.size() == 2 && partes.at(0) == categoria) {
            return partes.at(1).toStdString();
        }
    }
    return {};
}

/// Id da DPS no leiaute de 45 posições: DPS + município (7) + tipo (1) + inscrição (14) + série (5) + número (15).
std::string idDps(char tipo, std::string_view inscricao)
{
    std::string id{"DPS"};
    id += kMunicipio;
    id += tipo;
    id += inscricao;
    id += kSerie;
    id += kNumero;
    return id;
}

/// Grava a saída do teste em NL_LOGS_PII_DIR/<nome>.log: é o que o portão de varredura lê.
void publicarNoLogDeReferencia(const QString& nome, const std::string& conteudo)
{
    QFile arquivo{QStringLiteral(NL_LOGS_PII_DIR) + QLatin1Char('/') + nome + QStringLiteral(".log")};
    QVERIFY2(arquivo.open(QIODevice::WriteOnly | QIODevice::Truncate), "não abriu o log de referência para escrita");
    QVERIFY(arquivo.write(QByteArray::fromStdString(conteudo)) >= 0);
}

/// Log que escreve num ostringstream. destino vem antes de log: o Log guarda uma referência a ele.
struct Sessao
{
    std::ostringstream destino;
    Log log{destino, "notalivre", allowlistDeReferencia()};
};

std::vector<Campo> umCampo(std::string nome, std::string valor)
{
    return {Campo{std::move(nome), std::move(valor)}};
}

} // namespace

class TstLog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void senhaNuncaApareceNoLog();
    void cpfDoEmitenteMascaradoNoIdDps();
    void cnpjDoPrestadorMascaradoNoIdDps();
    void envelopeBase64NaoVaiAoLog();
    void campoSemDeclaracaoDescartadoEContado();
    void quebraDeLinhaNaoCriaLinhaFalsa();
    void varreduraDeLogVaziaReprova();
};

void TstLog::initTestCase()
{
    QDir pasta{QStringLiteral(NL_LOGS_PII_DIR)};
    if (pasta.exists()) {
        QVERIFY2(pasta.removeRecursively(), "não limpou a pasta de logs da execução anterior");
    }
    QVERIFY2(QDir().mkpath(QStringLiteral(NL_LOGS_PII_DIR)), "não criou a pasta de logs");
}

void TstLog::senhaNuncaApareceNoLog()
{
    const std::string senha = referencia(QStringLiteral("senha"));
    const std::string token = referencia(QStringLiteral("token"));
    QVERIFY(!senha.empty());
    QVERIFY(!token.empty());

    Sessao sessao;
    const std::vector<Campo> campos{Campo{"senha", senha}, Campo{"tokenAcesso", token}};
    QVERIFY(sessao.log.registrar(Nivel::Erro, "certificado.senha_incorreta", campos).has_value());
    const std::string saida = sessao.destino.str();
    publicarNoLogDeReferencia(QStringLiteral("senhaNuncaApareceNoLog"), saida);

    QVERIFY2(saida.find("certificado.senha_incorreta") != std::string::npos, "o evento precisa estar no log");
    QVERIFY2(saida.find(senha) == std::string::npos, "a senha apareceu no log");
    QVERIFY2(saida.find(token) == std::string::npos, "o token apareceu no log");
    QCOMPARE(sessao.log.descartados(), std::size_t{2});
}

void TstLog::cpfDoEmitenteMascaradoNoIdDps()
{
    const std::string cpf = referencia(QStringLiteral("cpf"));
    QVERIFY(cpf.size() == 11);
    const std::string inscricao = "000" + cpf;  // tipo 1 (CPF): 14 posições, com 000 à esquerda
    const std::string id = idDps('1', inscricao);
    QCOMPARE(id.size(), kTamanhoIdDps);
    QVERIFY(id.substr(11, 14) == inscricao);  // a inscrição ocupa as posições 12 a 25 (1-based)

    Sessao sessao;
    QVERIFY(sessao.log.registrar(Nivel::Info, "dps.gerada", umCampo("idDps", id)).has_value());
    const std::string saida = sessao.destino.str();
    publicarNoLogDeReferencia(QStringLiteral("cpfDoEmitenteMascaradoNoIdDps"), saida);

    const std::string esperado = idDps('1', std::string(14, '#'));
    QVERIFY2(saida.find(esperado) != std::string::npos,
             "o Id da DPS precisa sair com as 14 posições da inscrição mascaradas (12 a 25)");
    QVERIFY2(saida.find(cpf) == std::string::npos, "o CPF do emitente apareceu no log");
}

void TstLog::cnpjDoPrestadorMascaradoNoIdDps()
{
    const std::string cnpj = referencia(QStringLiteral("cnpj"));
    QVERIFY(cnpj.size() == 14);
    const std::string id = idDps('2', cnpj);  // tipo 2 (CNPJ): mascara do mesmo jeito que o CPF
    QCOMPARE(id.size(), kTamanhoIdDps);

    Sessao sessao;
    QVERIFY(sessao.log.registrar(Nivel::Info, "dps.gerada", umCampo("idDps", id)).has_value());
    const std::string saida = sessao.destino.str();
    publicarNoLogDeReferencia(QStringLiteral("cnpjDoPrestadorMascaradoNoIdDps"), saida);

    QVERIFY2(saida.find(idDps('2', std::string(14, '#'))) != std::string::npos,
             "o Id da DPS com CNPJ precisa sair mascarado nas posições 12 a 25");
    QVERIFY2(saida.find(cnpj) == std::string::npos, "o CNPJ do prestador apareceu no log");
}

void TstLog::envelopeBase64NaoVaiAoLog()
{
    const std::string envelope = referencia(QStringLiteral("envelope"));
    QVERIFY(!envelope.empty());

    Sessao sessao;
    QVERIFY(sessao.log.registrar(Nivel::Info, "dps.assinada", umCampo("envelopeAssinado", envelope)).has_value());
    const std::string saida = sessao.destino.str();
    publicarNoLogDeReferencia(QStringLiteral("envelopeBase64NaoVaiAoLog"), saida);

    QVERIFY2(saida.find("dps.assinada") != std::string::npos, "o evento precisa estar no log");
    QVERIFY2(saida.find(envelope) == std::string::npos, "o envelope em base64 apareceu no log");
    QCOMPARE(sessao.log.descartados(), std::size_t{1});
}

void TstLog::campoSemDeclaracaoDescartadoEContado()
{
    const std::string valor = "valor-nao-declarado-NL27";

    Sessao sessao;
    QVERIFY(sessao.log.registrar(Nivel::Aviso, "campo.novo", umCampo("campoNaoDeclarado", valor)).has_value());
    const std::string saida = sessao.destino.str();
    publicarNoLogDeReferencia(QStringLiteral("campoSemDeclaracaoDescartadoEContado"), saida);

    QVERIFY2(saida.find(valor) == std::string::npos, "campo fora da allowlist apareceu no log");
    QCOMPARE(sessao.log.descartados(), std::size_t{1});
}

void TstLog::quebraDeLinhaNaoCriaLinhaFalsa()
{
    const std::string injetado = "E0188\n{\"level\":\"erro\"}\r\nLINHA-FALSA";

    Sessao sessao;
    QVERIFY(sessao.log.registrar(Nivel::Erro, "api.erro", umCampo("codigoErro", injetado)).has_value());
    const std::string saida = sessao.destino.str();
    publicarNoLogDeReferencia(QStringLiteral("quebraDeLinhaNaoCriaLinhaFalsa"), saida);

    // Uma chamada de registrar gera exatamente uma linha: o terminador. Quebra injetada não pode criar outra.
    QCOMPARE(static_cast<std::size_t>(std::count(saida.begin(), saida.end(), '\n')), std::size_t{1});
    QVERIFY2(saida.find("\nLINHA-FALSA") == std::string::npos, "a quebra injetada virou linha no log");
}

void TstLog::varreduraDeLogVaziaReprova()
{
    QTemporaryDir vazio;
    QVERIFY(vazio.isValid());

    QProcess processo;
    processo.start(QStringLiteral(NL_PYTHON3),
                   {QStringLiteral("-I"), QStringLiteral(NL_VARREDURA_LOGS), QStringLiteral("--dir"), vazio.path(),
                    QStringLiteral("--termos"), QStringLiteral(NL_FIXTURES_PII) + QStringLiteral("/referencia.txt")});
    QVERIFY2(processo.waitForStarted(10000), "não iniciou o python da varredura");
    QVERIFY2(processo.waitForFinished(30000), "a varredura não terminou");

    const QString saida = QString::fromUtf8(processo.readAllStandardOutput());
    QVERIFY2(processo.exitStatus() == QProcess::NormalExit, "a varredura morreu por sinal");
    QCOMPARE(processo.exitCode(), 1);
    QVERIFY2(saida.contains(QStringLiteral("analisados=0")), "a varredura vazia precisa contar zero analisados");
    QVERIFY2(saida.contains(QStringLiteral("REPROVA")), "varredura vazia precisa reprovar");
}

QTEST_GUILESS_MAIN(TstLog)
#include "tst_log.moc"
