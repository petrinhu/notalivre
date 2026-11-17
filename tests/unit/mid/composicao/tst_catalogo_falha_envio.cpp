// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo da falha de envio (NL-3.C, plano 1.5 e 4.4): texto e código das 10 classes. O teste nunca afirma a frase
// literal aprovada; afirma as propriedades léxicas da seção 1.5 (mesmas listas do awk de conferência do CTO e do CISO).
#include <nl/mid/composicao/catalogo_falha_envio.h>

#include <QRegularExpression>
#include <QStringList>
#include <QtTest>

#include <array>
#include <cstddef>
#include <set>
#include <string>

namespace cp = nl::mid::composicao;

namespace {

QString paraQString(std::string_view texto)
{
    return QString::fromUtf8(texto.data(), static_cast<qsizetype>(texto.size()));
}

QString textoDe(cp::ClasseFalhaEnvio classe)
{
    return paraQString(cp::textoDaClasse(classe).texto);
}

QString codigoDe(cp::ClasseFalhaEnvio classe)
{
    return paraQString(cp::textoDaClasse(classe).codigo);
}

bool naoVazia(cp::ClasseFalhaEnvio classe)
{
    return !textoDe(classe).isEmpty() && !codigoDe(classe).isEmpty();
}

// Trecho (sem caixa) dentro do texto da classe.
bool contem(cp::ClasseFalhaEnvio classe, const char* trecho)
{
    return textoDe(classe).toLower().contains(QString::fromUtf8(trecho));
}

// Regras "em toda classe" da seção 1.5: devolve a primeira regra violada, ou vazio se o texto passa.
QString violacaoGeral(const QString& texto)
{
    if (texto.isEmpty()) {
        return QStringLiteral("vazio");
    }
    if (texto.size() > 300) {
        return QStringLiteral("acima de 300 caracteres");
    }
    if (texto.contains(QChar(u'\n')) || texto.contains(QChar(u'\r'))) {
        return QStringLiteral("mais de uma linha");
    }
    for (const QChar c : texto) {
        if (c >= QChar(u'0') && c <= QChar(u'9')) {
            return QStringLiteral("digito");
        }
        if (c == QChar(u'"') || c == QChar(u'\\')) {
            return QStringLiteral("aspas duplas ou barra invertida");
        }
        if (c == QChar(0x2014) || c == QChar(0x2013)) {
            return QStringLiteral("travessao ou meia-risca");
        }
    }
    const QString minusculo = texto.toLower();
    const QStringList proibidas{
        QStringLiteral("http"), QStringLiteral("://"), QStringLiteral("www"), QStringLiteral(".br"),
        QStringLiteral("ssl"), QStringLiteral("tls"), QStringLiteral("openssl"), QStringLiteral("handshake"),
        QStringLiteral("recusad"), QStringLiteral("não foi emitida"), QStringLiteral("certificado recusado"),
        QStringLiteral("desculp"), QStringLiteral("ops"), QStringLiteral("por favor"), QStringLiteral("transitória"),
        QStringLiteral("indeterminad"), QStringLiteral("definitiva"), QStringLiteral("rejeitada"),
    };
    for (const QString& trecho : proibidas) {
        if (minusculo.contains(trecho)) {
            return QStringLiteral("proibido: ") + trecho;
        }
    }
    if (minusculo.contains(QRegularExpression(QStringLiteral("(^|[^a-z])qt([^a-z]|$)")))) {
        return QStringLiteral("proibido: qt");
    }
    return {};
}

// NL-3.C S5 (A2 da revisão): cada classe tem uma marca de texto que só ela tem. Trocar o texto de duas classes reprova aqui, sem
// afirmar a frase aprovada inteira (DA-3C.9). A marca é um trecho do texto aprovado, em minúsculas, na ordem de kTodasAsClasses.
struct MarcaDaClasse {
    cp::ClasseFalhaEnvio classe;
    const char* marca;
};

constexpr std::array<MarcaDaClasse, 10> kMarcas{{
    {cp::ClasseFalhaEnvio::CertificadoProvavel, "não tenha sido aceito"},
    {cp::ClasseFalhaEnvio::ServidorNaoConfirmado, "não confirmou que fala"},
    {cp::ClasseFalhaEnvio::RedeAntesDoEnvio, "antes de a nota sair do computador"},
    {cp::ClasseFalhaEnvio::EstadoIncerto, "não houve confirmação do sistema nacional da nfs-e. a nota"},
    {cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel, "provável problema no certificado"},
    {cp::ClasseFalhaEnvio::NaoConcluido, "não concluiu a emissão"},
    {cp::ClasseFalhaEnvio::FalhaAntesDoEnvio, "falhou ao preparar a nota"},
    {cp::ClasseFalhaEnvio::RespostaSemCodigo, "não conseguiu ler a resposta"},
    {cp::ClasseFalhaEnvio::ConfiguracaoDaConexao, "configuração da conexão segura está incompleta"},
    {cp::ClasseFalhaEnvio::ComparacaoIndisponivel, "conferir a nota que já existe"},
}};

// NL-3.C S5 (A2, gêmeo do código): cada classe devolve o próprio código estável, na ordem de kTodasAsClasses.
struct CodigoDaClasse {
    cp::ClasseFalhaEnvio classe;
    std::string_view codigo;
};

constexpr std::array<CodigoDaClasse, 10> kCodigosEsperados{{
    {cp::ClasseFalhaEnvio::CertificadoProvavel, cp::kCodigoCertificadoProvavel},
    {cp::ClasseFalhaEnvio::ServidorNaoConfirmado, cp::kCodigoServidorNaoConfirmado},
    {cp::ClasseFalhaEnvio::RedeAntesDoEnvio, cp::kCodigoRedeAntesDoEnvio},
    {cp::ClasseFalhaEnvio::EstadoIncerto, cp::kCodigoEstadoIncerto},
    {cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel, cp::kCodigoEstadoIncertoCertificadoProvavel},
    {cp::ClasseFalhaEnvio::NaoConcluido, cp::kCodigoNaoConcluido},
    {cp::ClasseFalhaEnvio::FalhaAntesDoEnvio, cp::kCodigoFalhaAntesDoEnvio},
    {cp::ClasseFalhaEnvio::RespostaSemCodigo, cp::kCodigoRespostaSemCodigo},
    {cp::ClasseFalhaEnvio::ConfiguracaoDaConexao, cp::kCodigoConfiguracaoDaConexao},
    {cp::ClasseFalhaEnvio::ComparacaoIndisponivel, cp::kCodigoComparacaoIndisponivel},
}};

} // namespace

class TesteCatalogoFalhaEnvio : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void todaClasseTemTextoECodigo();
    void codigosSaoUnicosEComPrefixoEnvio();
    void textoSemDadoProibido();
    void nadaFoiEnviadoSoOndeNadaSaiu();
    void semConviteARepetirOndeRepetirEProibido();
    void estadoIncertoMandaOMesmoNumeroEOMesmoArquivo();
    void provavelOndeACausaNaoEProvada();
    void ehFalhaDeEnvioReconheceSoOsCodigosDoCatalogo();
    void textoDeCadaClasseTemMarcaPropria();
    void codigoDeCadaClasseEOProprio();
};

void TesteCatalogoFalhaEnvio::initTestCase()
{
}

void TesteCatalogoFalhaEnvio::cleanupTestCase()
{
}

void TesteCatalogoFalhaEnvio::todaClasseTemTextoECodigo()
{
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
}

void TesteCatalogoFalhaEnvio::codigosSaoUnicosEComPrefixoEnvio()
{
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
    std::set<QString> vistos;
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        const QString codigo = codigoDe(classe);
        const bool novo = vistos.insert(codigo).second;
        QVERIFY2(novo, "codigo repetido entre classes");
        if (codigo != QStringLiteral("sefin.comparacao_indisponivel")) {
            QVERIFY2(codigo.startsWith(QStringLiteral("envio.")), "codigo sem prefixo envio.");
        }
    }
}

void TesteCatalogoFalhaEnvio::textoSemDadoProibido()
{
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QCOMPARE(violacaoGeral(textoDe(classe)), QString());
    }
}

void TesteCatalogoFalhaEnvio::nadaFoiEnviadoSoOndeNadaSaiu()
{
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
    // Só onde a regra de nada ter saído vale (zero bytes no primeiro envio, ou tradução de causa de configuração ou de preparo).
    const cp::ClasseFalhaEnvio comNada[] = {cp::ClasseFalhaEnvio::CertificadoProvavel, cp::ClasseFalhaEnvio::ServidorNaoConfirmado,
                                            cp::ClasseFalhaEnvio::RedeAntesDoEnvio, cp::ClasseFalhaEnvio::FalhaAntesDoEnvio,
                                            cp::ClasseFalhaEnvio::ConfiguracaoDaConexao};
    const cp::ClasseFalhaEnvio semNada[] = {cp::ClasseFalhaEnvio::EstadoIncerto, cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel,
                                            cp::ClasseFalhaEnvio::NaoConcluido, cp::ClasseFalhaEnvio::RespostaSemCodigo,
                                            cp::ClasseFalhaEnvio::ComparacaoIndisponivel};
    for (cp::ClasseFalhaEnvio classe : comNada) {
        QVERIFY2(contem(classe, "nada foi enviado"), "falta: nada foi enviado");
    }
    for (cp::ClasseFalhaEnvio classe : semNada) {
        QVERIFY2(!contem(classe, "nada foi enviado"), "proibido aqui: nada foi enviado");
    }
}

void TesteCatalogoFalhaEnvio::semConviteARepetirOndeRepetirEProibido()
{
    const cp::ClasseFalhaEnvio aoRepetir[] = {cp::ClasseFalhaEnvio::CertificadoProvavel, cp::ClasseFalhaEnvio::ServidorNaoConfirmado,
                                              cp::ClasseFalhaEnvio::EstadoIncerto, cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel};
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
    const QStringList convites{QStringLiteral("de novo"), QStringLiteral("novamente"), QStringLiteral("tente")};
    for (cp::ClasseFalhaEnvio classe : aoRepetir) {
        for (const QString& convite : convites) {
            QVERIFY2(!contem(classe, convite.toUtf8().constData()), "convite a repetir proibido");
        }
    }
    const QStringList deServidor{QStringLiteral("deslig"), QStringLiteral("desativ"), QStringLiteral("ignor"),
                                 QStringLiteral("instal"), QStringLiteral("aceit"), QStringLiteral("exceç"),
                                 QStringLiteral("confie"), QStringLiteral("confiar")};
    for (const QString& palavra : deServidor) {
        QVERIFY2(!contem(cp::ClasseFalhaEnvio::ServidorNaoConfirmado, palavra.toUtf8().constData()),
                 "C2 sugere desligar verificacao ou instalar");
    }
    const QStringList deConfiguracao{QStringLiteral("deslig"), QStringLiteral("desativ"), QStringLiteral("ignor"),
                                     QStringLiteral("aceit"), QStringLiteral("exceç"), QStringLiteral("confie"),
                                     QStringLiteral("confiar")};
    for (const QString& palavra : deConfiguracao) {
        QVERIFY2(!contem(cp::ClasseFalhaEnvio::ConfiguracaoDaConexao, palavra.toUtf8().constData()),
                 "C8 sugere desligar verificacao");
    }
}

void TesteCatalogoFalhaEnvio::estadoIncertoMandaOMesmoNumeroEOMesmoArquivo()
{
    const cp::ClasseFalhaEnvio mesmoNumero[] = {cp::ClasseFalhaEnvio::CertificadoProvavel, cp::ClasseFalhaEnvio::ServidorNaoConfirmado,
                                                cp::ClasseFalhaEnvio::RedeAntesDoEnvio, cp::ClasseFalhaEnvio::EstadoIncerto,
                                                cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel, cp::ClasseFalhaEnvio::NaoConcluido,
                                                cp::ClasseFalhaEnvio::RespostaSemCodigo};
    const cp::ClasseFalhaEnvio mesmoArquivo[] = {cp::ClasseFalhaEnvio::EstadoIncerto,
                                                 cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel,
                                                 cp::ClasseFalhaEnvio::RespostaSemCodigo};
    const cp::ClasseFalhaEnvio podeTerSidoEmitida[] = {cp::ClasseFalhaEnvio::EstadoIncerto,
                                                       cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel};
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
    for (cp::ClasseFalhaEnvio classe : mesmoNumero) {
        QVERIFY2(contem(classe, "mesmo número"), "falta: mesmo número");
    }
    for (cp::ClasseFalhaEnvio classe : mesmoArquivo) {
        QVERIFY2(contem(classe, "mesmo arquivo"), "falta: mesmo arquivo");
    }
    for (cp::ClasseFalhaEnvio classe : podeTerSidoEmitida) {
        QVERIFY2(contem(classe, "pode ter sido emitida"), "falta: pode ter sido emitida");
    }
}

void TesteCatalogoFalhaEnvio::provavelOndeACausaNaoEProvada()
{
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
    const cp::ClasseFalhaEnvio provavel[] = {cp::ClasseFalhaEnvio::CertificadoProvavel,
                                             cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel};
    for (cp::ClasseFalhaEnvio classe : provavel) {
        QVERIFY2(contem(classe, "provável"), "falta: provável");
    }
    QVERIFY2(contem(cp::ClasseFalhaEnvio::CertificadoProvavel, "válid"), "falta: válid na classe de certificado");
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        const bool e1 = classe == cp::ClasseFalhaEnvio::CertificadoProvavel;
        const bool e5 = classe == cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel;
        if (!e1 && !e5) {
            QVERIFY2(!contem(classe, "recus"), "recus só cabe em C1 e C5");
        }
    }
}

void TesteCatalogoFalhaEnvio::ehFalhaDeEnvioReconheceSoOsCodigosDoCatalogo()
{
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        QVERIFY2(naoVazia(classe), "classe sem texto ou sem codigo");
    }
    for (cp::ClasseFalhaEnvio classe : cp::kTodasAsClasses) {
        const std::string codigo = codigoDe(classe).toStdString();
        QVERIFY2(cp::ehFalhaDeEnvio(codigo), "codigo do catalogo nao reconhecido");
    }
    for (const char* recusado : {"sefin.E1207", "sefin.dps_duplicada", "credencial.nao_carregou", "envio.desconhecido", ""}) {
        QVERIFY2(!cp::ehFalhaDeEnvio(recusado), recusado);
    }
}

void TesteCatalogoFalhaEnvio::textoDeCadaClasseTemMarcaPropria()
{
    QVERIFY2(kMarcas.size() == cp::kTodasAsClasses.size(), "tabela de marcas sem todas as classes");
    for (std::size_t i = 0; i < kMarcas.size(); ++i) {
        QVERIFY2(kMarcas[i].classe == cp::kTodasAsClasses[i], "tabela de marcas fora da ordem das classes");
        QVERIFY2(contem(kMarcas[i].classe, kMarcas[i].marca), kMarcas[i].marca);
        int donos = 0;
        for (cp::ClasseFalhaEnvio outra : cp::kTodasAsClasses) {
            if (contem(outra, kMarcas[i].marca)) {
                ++donos;
            }
        }
        QVERIFY2(donos == 1, kMarcas[i].marca);
    }
}

void TesteCatalogoFalhaEnvio::codigoDeCadaClasseEOProprio()
{
    QVERIFY2(kCodigosEsperados.size() == cp::kTodasAsClasses.size(), "tabela de codigos sem todas as classes");
    for (std::size_t i = 0; i < kCodigosEsperados.size(); ++i) {
        QVERIFY2(kCodigosEsperados[i].classe == cp::kTodasAsClasses[i], "tabela de codigos fora da ordem das classes");
        QVERIFY2(cp::textoDaClasse(kCodigosEsperados[i].classe).codigo == kCodigosEsperados[i].codigo,
                 kCodigosEsperados[i].codigo.data());
    }
}

QTEST_APPLESS_MAIN(TesteCatalogoFalhaEnvio)
#include "tst_catalogo_falha_envio.moc"
