// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do manipulador de mensagens do Qt ligado ao Log (NL-2.9, ADR-0005 a): mensagem com CPF não sai pelo Log, e desligar o
// manipulador não devolve o padrão do Qt (que escreveria a mensagem crua na saída de erro).
#include <nl/foundation/log/log.h>
#include <nl/mid/composicao/guarda_manipulador_qt.h>
#include <nl/mid/composicao/manipulador_qt.h>

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QLoggingCategory>

#include <sstream>
#include <string>

#include <unistd.h>

namespace {

// Captura o que o processo escreve na saída de erro durante o trecho, redirecionando o descritor 2 para um canal.
std::string capturarStderr(void (*trecho)())
{
    int canal[2];
    if (pipe(canal) != 0) {
        return "pipe-falhou";
    }
    const int salvo = dup(STDERR_FILENO);
    dup2(canal[1], STDERR_FILENO);
    trecho();
    dup2(salvo, STDERR_FILENO);
    close(salvo);
    close(canal[1]);
    std::string saida;
    char buffer[256];
    ssize_t lidos = 0;
    while ((lidos = read(canal[0], buffer, sizeof buffer)) > 0) {
        saida.append(buffer, static_cast<std::size_t>(lidos));
    }
    close(canal[0]);
    return saida;
}

} // namespace

class TstManipuladorQt : public QObject
{
    Q_OBJECT

private slots:
    // qWarning com CPF não aparece no registro; o descarte fica contado.
    void qWarningComCpfNaoVaiAoLog();
    // Depois de desligar o manipulador, o Qt não escreve a mensagem na saída de erro: nem o padrão, nem o texto cru.
    void mensagemDoQtNaSaidaDoProcessoNaoUsaLogDestruido();
    // A guarda ligada ao Log descarta ao sair do escopo: depois dela, o Qt não escreve na saída de erro (NL-2.9, item 4).
    void guardaDescartaAoSairDoEscopo();
    // NL-2.10e (DA-2.82): a categoria do Qt sai num conjunto fechado (qt.network.ssl, qt.network.http2, qt.network.access, outro); o texto não.
    void categoriaDoQtFechadaETextoDescartado();
    // NL-2.10e (R2-B1, DA-2.82): categoria fora do conjunto fechado sai como "outro", nunca crua (mutação CAT1).
    void categoriaForaDoConjuntoViraOutro();
};

void TstManipuladorQt::categoriaDoQtFechadaETextoDescartado()
{
    std::ostringstream destino;
    nl::foundation::log::Log registro{destino, "notalivre-cli", {nl::foundation::log::DeclaracaoCampo{"categoria", nl::foundation::log::Regra::Cru}}, {"qt.mensagem"}};
    nl::mid::composicao::instalarManipuladorQt(&registro);
    const QLoggingCategory categoria("qt.network.ssl");
    qCWarning(categoria) << "texto do Qt com CNPJ 11222333000181";
    nl::mid::composicao::instalarManipuladorQt(nullptr);
    const std::string saida = destino.str();
    // Portão de PII (NL-2.10e, R-B2): a saída vai para a pasta que o portão varre; um texto com CNPJ nunca pode sair.
    const QString pasta = qEnvironmentVariable("NL_LOGS_PII_CLI_DIR");
    if (!pasta.isEmpty()) {
        QDir().mkpath(pasta);
        QFile arquivo(pasta + QLatin1String("/qt_mensagem.log"));
        if (arquivo.open(QIODevice::WriteOnly | QIODevice::Append)) {
            arquivo.write(saida.data(), static_cast<qint64>(saida.size()));
        }
    }
    QVERIFY2(saida.find("\"categoria\":\"qt.network.ssl\"") != std::string::npos, "a categoria do Qt tem de sair no conjunto fechado");
    QVERIFY2(saida.find("11222333000181") == std::string::npos, "o texto do Qt não pode sair no log");
}

// NL-2.10e (R2-B1, DA-2.82): uma categoria que não está no conjunto fechado não pode sair pelo nome; o valor vira "outro".
void TstManipuladorQt::categoriaForaDoConjuntoViraOutro()
{
    std::ostringstream destino;
    nl::foundation::log::Log registro{destino, "notalivre-cli", {nl::foundation::log::DeclaracaoCampo{"categoria", nl::foundation::log::Regra::Cru}}, {"qt.mensagem"}};
    nl::mid::composicao::instalarManipuladorQt(&registro);
    const QLoggingCategory desconhecida("nl.teste.desconhecida");
    qCWarning(desconhecida) << "mensagem de categoria desconhecida";
    nl::mid::composicao::instalarManipuladorQt(nullptr);
    const std::string saida = destino.str();
    QVERIFY2(saida.find("\"categoria\":\"outro\"") != std::string::npos, "categoria fora do conjunto fechado tem de sair como outro");
    QVERIFY2(saida.find("nl.teste.desconhecida") == std::string::npos, "o nome cru da categoria não pode sair no log");
}

void TstManipuladorQt::qWarningComCpfNaoVaiAoLog()
{
    std::ostringstream destino;
    // NL-2.10e (DA-2.82): categoria declarada, como em produção; só o texto fica sem declaração e é o único descartado.
    nl::foundation::log::Log registro{destino, "notalivre-cli", {nl::foundation::log::DeclaracaoCampo{"categoria", nl::foundation::log::Regra::Cru}}, {"qt.mensagem"}};
    nl::mid::composicao::instalarManipuladorQt(&registro);
    qWarning("CPF 12345678901 no aviso do Qt");
    nl::mid::composicao::instalarManipuladorQt(nullptr);
    QVERIFY2(destino.str().find("12345678901") == std::string::npos, "o CPF da mensagem do Qt não pode ir ao Log");
    QCOMPARE(registro.descartados(), std::size_t{1});
}

namespace {

void trechoDepoisDaGuarda()
{
    qWarning("CPF 12345678901 depois da guarda");
}

void trechoComManipuladorDesligado()
{
    nl::mid::composicao::instalarManipuladorQt(nullptr);
    qWarning("CPF 12345678901 depois de desligar o manipulador");
}

} // namespace

void TstManipuladorQt::mensagemDoQtNaSaidaDoProcessoNaoUsaLogDestruido()
{
    std::ostringstream destino;
    {
        nl::foundation::log::Log registro{destino, "notalivre-cli", {}, {"qt.mensagem"}};
        nl::mid::composicao::instalarManipuladorQt(&registro);
        nl::mid::composicao::instalarManipuladorQt(nullptr);
    }
    const std::string saida = capturarStderr(trechoComManipuladorDesligado);
    QVERIFY2(saida.find("12345678901") == std::string::npos, "o CPF não pode sair pela saída de erro do processo");
    QVERIFY2(saida.empty(), "com o manipulador desligado, o Qt não pode escrever na saída de erro (padrão do Qt)");
}

void TstManipuladorQt::guardaDescartaAoSairDoEscopo()
{
    std::ostringstream destino;
    nl::foundation::log::Log registro{destino, "notalivre-cli", {}, {"qt.mensagem"}};
    {
        nl::mid::composicao::GuardaManipuladorQt guarda(&registro);
        qWarning("CPF 12345678901 dentro da guarda");
    }
    const std::string saida = capturarStderr(trechoDepoisDaGuarda);
    QVERIFY2(saida.empty(), "depois de sair da guarda, o Qt não pode escrever na saída de erro");
    QVERIFY2(destino.str().find("12345678901") == std::string::npos, "o CPF da guarda não pode ir ao Log");
}

QTEST_APPLESS_MAIN(TstManipuladorQt)
#include "tst_manipulador_qt.moc"
