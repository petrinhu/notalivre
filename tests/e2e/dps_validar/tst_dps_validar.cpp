// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Arnês ponta a ponta de `notalivre dps validar` (NL-1.7). Roda a CLI em processo próprio (QProcess), como o usuário a roda,
// e confere código de saída, XML gravado e mensagem no stderr. Não inclui nenhuma fachada: só fala com o binário.
// Os XML de referência são as fixtures do codec (tests/fixtures/dps); a saída da CLI tem de sair byte a byte igual.

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QtTest>

namespace {

const QString kCli = QStringLiteral(NL_CLI_DPS);
const QString kEntradas = QStringLiteral(NL_ENTRADAS_DPS);
const QString kSaida = QStringLiteral(NL_SAIDA_DPS);
const QString kFixtures = QStringLiteral(NL_FIXTURES_DPS);
const QString kXsd = QStringLiteral(NL_XSD_PRODREST_DIR);
const QString kManifesto = QStringLiteral(NL_XSD_PRODREST_MANIFESTO);

constexpr int kTempoMaximoMs = 60000;

/// Resultado de uma execução da CLI. codigo é -1 se o processo não terminou normalmente (travou ou morreu por sinal).
struct Execucao
{
    bool terminou = false;
    int codigo = -1;
    QString erro;
};

/// Roda a CLI com os argumentos dados e espera o fim, com teto de tempo; estourado o teto, mata o processo.
Execucao executarCli(const QStringList& argumentos)
{
    QProcess processo;
    processo.start(kCli, argumentos);
    Execucao resultado;
    if (!processo.waitForStarted(kTempoMaximoMs)) {
        resultado.erro = QStringLiteral("a CLI não iniciou: %1").arg(processo.errorString());
        return resultado;
    }
    resultado.terminou = processo.waitForFinished(kTempoMaximoMs);
    if (!resultado.terminou) {
        processo.kill();
        processo.waitForFinished(kTempoMaximoMs);
        resultado.erro = QStringLiteral("a CLI estourou o tempo de %1 ms").arg(kTempoMaximoMs);
        return resultado;
    }
    resultado.codigo = processo.exitStatus() == QProcess::NormalExit ? processo.exitCode() : -1;
    resultado.erro = QString::fromUtf8(processo.readAllStandardError());
    return resultado;
}

/// Argumentos da subcomando `dps validar`: entrada, saída e a localização do XSD de Produção Restrita.
QStringList argumentosValidar(const QString& entrada, const QString& saida)
{
    return {QStringLiteral("dps"), QStringLiteral("validar"), entrada, saida,
            QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto};
}

QString caminhoEntrada(const QString& nome)
{
    return QStringLiteral("%1/%2.json").arg(kEntradas, nome);
}

/// Nome do XML gravado começa com valida_ para o oráculo xmllint (oraculo_xmllint.sh) poder conferi-lo depois.
QString caminhoSaida(const QString& nome)
{
    return QStringLiteral("%1/valida_%2.xml").arg(kSaida, nome);
}

QByteArray lerArquivo(const QString& caminho, bool* ok)
{
    QFile arquivo{caminho};
    *ok = arquivo.open(QIODevice::ReadOnly);
    return *ok ? arquivo.readAll() : QByteArray{};
}

/// Entrada que tem de reprovar com rc 1: o stderr traz o código, o campo e um trecho da mensagem, e nenhum XML fica gravado.
void verificaReprovaEntrada(const QString& nome, const QString& codigoErro, const QString& campoErro, const QString& trecho)
{
    const QString saida = caminhoSaida(nome);
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(nome), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 1,
             qPrintable(QStringLiteral("rc=%1, esperado 1 (reprovada). stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(codigoErro), qPrintable(QStringLiteral("código %1 ausente. stderr: %2").arg(codigoErro, execucao.erro)));
    if (!campoErro.isEmpty()) {
        QVERIFY2(execucao.erro.contains(campoErro), qPrintable(QStringLiteral("campo %1 ausente. stderr: %2").arg(campoErro, execucao.erro)));
    }
    QVERIFY2(execucao.erro.contains(trecho), qPrintable(QStringLiteral("trecho '%1' ausente. stderr: %2").arg(trecho, execucao.erro)));
    QVERIFY2(!QFile::exists(saida), "entrada reprovada não pode deixar XML gravado");
}

} // namespace

class TstDpsValidar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void referenciasGeramXmlIgualAoCodec_data();
    void referenciasGeramXmlIgualAoCodec();

    void cpfInvalidoReprovaComRegra();
    void campoObrigatorioAusenteReprova();

    // NL-2.10h (H1): cTribMun (código de tributação municipal, 3 dígitos, nunca 000, proibido para MEI).
    void cTribMunZeroReprova();
    void cTribMunDoisDigitosReprova();
    void cTribMunLetraReprova();
    void cTribMunEntraNoXmlEntreCTribNacECNbs();

    // NL-2.10j (J1): totTrib nas quatro formas do XSD, por regime, com as faixas e o conflito de formas.
    void totTribPTotTribSNMeEppAceitaNoXml();
    void totTribPTotTribMeEppAceitaNoXmlNaOrdem();
    void totTribPTotTribNaoOptanteAceitaNoXml();
    void totTribIndTotTribMeEppRecusaComE0712();
    void totTribPTotTribSNMeiRecusaComE0710();
    void totTribIndTotTribNaoOptanteRecusaComE0713();
    void totTribPercentualAcimaDe100RecusaComFaixa();
    void totTribValorAcimaDoServicoRecusaComFaixa();
    void totTribDuasFormasRecusamConflito();

    void jsonMalformadoReprova();
    void tipoInvalidoReprova();
    void serieForaDaFaixaReprova();
    void prestadorComCnpjECpfReprova();
    void totTribIndicadorEMonetarioReprova();
    void entradaInexistenteRetornaUso();
    void argumentoFaltandoRetornaUso();

    void dCompetInvalidaReprova();
    void xsdInvalidoReprova();
    void xsdAusenteRetornaAmbiente();
    void manifestoIlegivelRetornaAmbiente();
    void opcaoDesconhecidaRetornaUso();
    void opcaoRepetidaRetornaUso();
};

void TstDpsValidar::initTestCase()
{
    QVERIFY2(QDir{}.mkpath(kSaida), qPrintable(QStringLiteral("não consegui criar %1").arg(kSaida)));
}

void TstDpsValidar::referenciasGeramXmlIgualAoCodec_data()
{
    QTest::addColumn<QString>("entrada");
    QTest::addColumn<QString>("fixture");

    QTest::newRow("MEI minima") << QStringLiteral("mei_minima") << QStringLiteral("valida_dps_mei_minima.xml");
    QTest::newRow("Simples com IBS/CBS") << QStringLiteral("simples_ibscbs") << QStringLiteral("valida_dps_simples_ibscbs.xml");
    QTest::newRow("tomador CPF") << QStringLiteral("tomador_cpf") << QStringLiteral("valida_dps_tomador_cpf_sem_endereco.xml");
    QTest::newRow("CNPJ minusculo e mascarado")
        << QStringLiteral("cnpj_minusculo_mascarado") << QStringLiteral("valida_dps_cnpj_minusculo_normalizado.xml");
}

void TstDpsValidar::referenciasGeramXmlIgualAoCodec()
{
    QFETCH(QString, entrada);
    QFETCH(QString, fixture);

    const QString saida = caminhoSaida(entrada);
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(entrada), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1, stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(QFile::exists(saida), "a CLI saiu com rc 0 e não gravou o XML validado");

    bool lidoGerado = false;
    bool lidoEsperado = false;
    const QByteArray gerado = lerArquivo(saida, &lidoGerado);
    const QByteArray esperado = lerArquivo(QStringLiteral("%1/%2").arg(kFixtures, fixture), &lidoEsperado);
    QVERIFY2(lidoGerado, "não consegui ler o XML gravado pela CLI");
    QVERIFY2(lidoEsperado, "não consegui ler a fixture de referência");
    QCOMPARE(gerado, esperado);
}

void TstDpsValidar::cpfInvalidoReprovaComRegra()
{
    const QString saida = caminhoSaida(QStringLiteral("cpf_invalido"));
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(QStringLiteral("cpf_invalido")), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("CPF com dígito verificador errado tinha de reprovar com rc 1, saiu rc=%1").arg(execucao.codigo)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("cpf.dv")), qPrintable(execucao.erro));
    QVERIFY2(execucao.erro.contains(QStringLiteral("dígito verificador do CPF não confere")), qPrintable(execucao.erro));
    QVERIFY2(!QFile::exists(saida), "entrada reprovada não pode deixar XML gravado");
}

void TstDpsValidar::campoObrigatorioAusenteReprova()
{
    const QString saida = caminhoSaida(QStringLiteral("sem_campo_obrigatorio"));
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(QStringLiteral("sem_campo_obrigatorio")), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("entrada sem servico.cTribNac tinha de reprovar com rc 1, saiu rc=%1").arg(execucao.codigo)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("entrada.campo_ausente")), qPrintable(execucao.erro));
    QVERIFY2(execucao.erro.contains(QStringLiteral("servico.cTribNac")), qPrintable(execucao.erro));
    QVERIFY2(!QFile::exists(saida), "entrada reprovada não pode deixar XML gravado");
}

void TstDpsValidar::cTribMunZeroReprova()
{
    // NL-2.10h: 000 é recusado no leitor, com o código do tipo e a regra do Anexo I (E0315, linha Excel 342). Sem XML.
    verificaReprovaEntrada(QStringLiteral("ctribmun_000"), QStringLiteral("cod_trib_municipal.zero"), QString{},
                           QStringLiteral("E0315"));
}

void TstDpsValidar::cTribMunDoisDigitosReprova()
{
    verificaReprovaEntrada(QStringLiteral("ctribmun_dois_digitos"), QStringLiteral("cod_trib_municipal.formato"), QString{},
                           QStringLiteral("3 dígitos"));
}

void TstDpsValidar::cTribMunLetraReprova()
{
    verificaReprovaEntrada(QStringLiteral("ctribmun_letra"), QStringLiteral("cod_trib_municipal.formato"), QString{},
                           QStringLiteral("3 dígitos"));
}

void TstDpsValidar::cTribMunEntraNoXmlEntreCTribNacECNbs()
{
    // Entrada de ME/EPP com cTribMun 501: o XML validado sai com o elemento entre cTribNac e cNBS (ordem do XSD, tiposComplexos_v1.01.xsd:1478).
    const QString saida = caminhoSaida(QStringLiteral("ctribmun_501"));
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(QStringLiteral("ctribmun_501")), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1, stderr: %2").arg(execucao.codigo).arg(execucao.erro)));

    bool lido = false;
    const QByteArray gerado = lerArquivo(saida, &lido);
    QVERIFY2(lido, "não consegui ler o XML gravado pela CLI");
    const qsizetype posNac = gerado.indexOf("<cTribNac>040101</cTribNac>");
    const qsizetype posMun = gerado.indexOf("<cTribMun>501</cTribMun>");
    const qsizetype posNbs = gerado.indexOf("<cNBS>123456789</cNBS>");
    QVERIFY2(posMun >= 0, "o XML tem de trazer <cTribMun>501</cTribMun>");
    QVERIFY2(posNac >= 0 && posNac < posMun, "cTribMun tem de vir depois de cTribNac");
    QVERIFY2(posMun < posNbs, "cTribMun tem de vir antes de cNBS");
}

// NL-2.10j (J1): cada forma de totTrib entra no XML validado; a recusa por regime e por faixa sai antes da assinatura (dps.*).
void TstDpsValidar::totTribPTotTribSNMeEppAceitaNoXml()
{
    const QString saida = caminhoSaida(QStringLiteral("totTrib_pTotTribSN_me"));
    QFile::remove(saida);
    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(QStringLiteral("totTrib_pTotTribSN_me")), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1, stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    bool lido = false;
    const QByteArray gerado = lerArquivo(saida, &lido);
    QVERIFY2(lido, "não consegui ler o XML gravado pela CLI");
    QVERIFY2(gerado.contains("<pTotTribSN>4.50</pTotTribSN>"), "o XML tem de trazer <pTotTribSN>4.50</pTotTribSN>");
    QVERIFY2(!gerado.contains("<indTotTrib>"), "pTotTribSN não pode vir com indTotTrib");
}

void TstDpsValidar::totTribPTotTribMeEppAceitaNoXmlNaOrdem()
{
    const QString saida = caminhoSaida(QStringLiteral("totTrib_pTotTrib_me"));
    QFile::remove(saida);
    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(QStringLiteral("totTrib_pTotTrib_me")), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1, stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    bool lido = false;
    const QByteArray gerado = lerArquivo(saida, &lido);
    QVERIFY2(lido, "não consegui ler o XML gravado pela CLI");
    const qsizetype fed = gerado.indexOf("<pTotTribFed>1.00</pTotTribFed>");
    const qsizetype est = gerado.indexOf("<pTotTribEst>0.00</pTotTribEst>");
    const qsizetype mun = gerado.indexOf("<pTotTribMun>0.00</pTotTribMun>");
    QVERIFY2(fed >= 0 && est >= 0 && mun >= 0, "o XML tem de trazer os três percentuais de pTotTrib");
    QVERIFY2(fed < est && est < mun, "os percentuais de pTotTrib têm de sair na ordem Fed, Est, Mun do XSD");
    QVERIFY2(!gerado.contains("<vTotTrib>"), "pTotTrib não pode vir com vTotTrib");
}

void TstDpsValidar::totTribPTotTribNaoOptanteAceitaNoXml()
{
    const QString saida = caminhoSaida(QStringLiteral("totTrib_pTotTrib_nao_optante"));
    QFile::remove(saida);
    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(QStringLiteral("totTrib_pTotTrib_nao_optante")), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1, stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    bool lido = false;
    const QByteArray gerado = lerArquivo(saida, &lido);
    QVERIFY2(lido, "não consegui ler o XML gravado pela CLI");
    QVERIFY2(gerado.contains("<pTotTribFed>1.00</pTotTribFed>"), "não optante aceita pTotTrib");
}

void TstDpsValidar::totTribIndTotTribMeEppRecusaComE0712()
{
    verificaReprovaEntrada(QStringLiteral("totTrib_indTotTrib_me"), QStringLiteral("dps.tottrib_regime"), QString{}, QStringLiteral("E0712"));
}

void TstDpsValidar::totTribPTotTribSNMeiRecusaComE0710()
{
    verificaReprovaEntrada(QStringLiteral("totTrib_pTotTribSN_mei"), QStringLiteral("dps.tottrib_regime"), QString{}, QStringLiteral("E0710"));
}

void TstDpsValidar::totTribIndTotTribNaoOptanteRecusaComE0713()
{
    verificaReprovaEntrada(QStringLiteral("totTrib_indTotTrib_nao_optante"), QStringLiteral("dps.tottrib_regime"), QString{}, QStringLiteral("E0713"));
}

void TstDpsValidar::totTribPercentualAcimaDe100RecusaComFaixa()
{
    verificaReprovaEntrada(QStringLiteral("totTrib_pTotTribFed_acima"), QStringLiteral("dps.tottrib_faixa"), QString{}, QStringLiteral("E0706"));
}

void TstDpsValidar::totTribValorAcimaDoServicoRecusaComFaixa()
{
    verificaReprovaEntrada(QStringLiteral("totTrib_vTotTrib_acima"), QStringLiteral("dps.tottrib_faixa"), QString{}, QStringLiteral("E0702"));
}

void TstDpsValidar::totTribDuasFormasRecusamConflito()
{
    verificaReprovaEntrada(QStringLiteral("totTrib_duas_formas"), QStringLiteral("entrada.campos_conflitantes"), QStringLiteral("valores.totTrib"),
                           QStringLiteral("não os dois"));
}

void TstDpsValidar::jsonMalformadoReprova()
{
    // Sem campo do leiaute: o erro de sintaxe é do documento inteiro. O trecho prova que veio do parser.
    verificaReprovaEntrada(QStringLiteral("json_malformado"), QStringLiteral("entrada.json_invalido"), QString{},
                           QStringLiteral("JSON malformado"));
}

void TstDpsValidar::tipoInvalidoReprova()
{
    verificaReprovaEntrada(QStringLiteral("tipo_invalido_tpamb"), QStringLiteral("entrada.tipo_invalido"), QStringLiteral("tpAmb"),
                           QStringLiteral("deve ser texto"));
}

void TstDpsValidar::serieForaDaFaixaReprova()
{
    verificaReprovaEntrada(QStringLiteral("serie_fora_da_faixa"), QStringLiteral("entrada.tipo_invalido"), QStringLiteral("serie"),
                           QStringLiteral("fora da faixa de inteiro"));
}

void TstDpsValidar::prestadorComCnpjECpfReprova()
{
    verificaReprovaEntrada(QStringLiteral("campos_conflitantes_prestador"), QStringLiteral("entrada.campos_conflitantes"),
                           QStringLiteral("prestador"), QStringLiteral("não os dois"));
}

void TstDpsValidar::totTribIndicadorEMonetarioReprova()
{
    verificaReprovaEntrada(QStringLiteral("campos_conflitantes_tot_trib"), QStringLiteral("entrada.campos_conflitantes"),
                           QStringLiteral("valores.totTrib"), QStringLiteral("não os dois"));
}

void TstDpsValidar::entradaInexistenteRetornaUso()
{
    const QString saida = caminhoSaida(QStringLiteral("entrada_inexistente"));
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(QStringLiteral("nao_existe")), saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 2, qPrintable(QStringLiteral("rc=%1, esperado 2 (uso ou arquivo). stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("não consegui ler a entrada")), qPrintable(execucao.erro));
    QVERIFY2(!QFile::exists(saida), "sem entrada não pode haver XML gravado");
}

void TstDpsValidar::argumentoFaltandoRetornaUso()
{
    const QString saida = caminhoSaida(QStringLiteral("argumento_faltando"));
    QFile::remove(saida);

    // Falta --manifesto: a linha de comando é incompleta e a CLI tem de recusar antes de ler qualquer arquivo.
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd};
    const Execucao execucao = executarCli(argumentos);
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 2, qPrintable(QStringLiteral("rc=%1, esperado 2 (uso). stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("uso:")), qPrintable(execucao.erro));
    QVERIFY2(execucao.erro.contains(QStringLiteral("--manifesto")), qPrintable(execucao.erro));
    QVERIFY2(!QFile::exists(saida), "linha de comando recusada não pode gravar XML");
}

/// Linha de comando recusada antes de gravar: rc 2, trecho no stderr e nenhum XML. Vale para uso e para falha de ambiente.
void verificaRecusaRc2(const QStringList& argumentos, const QString& saida, const QString& trecho)
{
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentos);
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 2,
             qPrintable(QStringLiteral("rc=%1, esperado 2. stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(trecho), qPrintable(QStringLiteral("trecho '%1' ausente. stderr: %2").arg(trecho, execucao.erro)));
    QVERIFY2(!QFile::exists(saida), "recusa com rc 2 não pode gravar XML");
}

void TstDpsValidar::dCompetInvalidaReprova()
{
    verificaReprovaEntrada(QStringLiteral("dcompet_invalida"), QStringLiteral("dps.dcompet"), QStringLiteral("Dps.infDPS.dCompet"),
                           QStringLiteral("data de competência inválida"));
}

void TstDpsValidar::xsdInvalidoReprova()
{
    // Passa no Back (xDescServ de 2001 caracteres é só texto para o domínio) e reprova no XSD, que tem o limite de 2000.
    verificaReprovaEntrada(QStringLiteral("xdescserv_2001"), QStringLiteral("xsd.invalido"), QStringLiteral("xDescServ"),
                           QStringLiteral("maximum length"));
}

void TstDpsValidar::xsdAusenteRetornaAmbiente()
{
    const QString saida = caminhoSaida(QStringLiteral("xsd_ausente"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), QStringLiteral("/nao_existe_xsd_dps"), QStringLiteral("--manifesto"), kManifesto};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("xsd.esquema"));
}

void TstDpsValidar::manifestoIlegivelRetornaAmbiente()
{
    const QString saida = caminhoSaida(QStringLiteral("manifesto_ilegivel"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), QStringLiteral("/nao_existe_manifesto_dps")};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("manifesto"));
}

void TstDpsValidar::opcaoDesconhecidaRetornaUso()
{
    const QString saida = caminhoSaida(QStringLiteral("opcao_desconhecida"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto, QStringLiteral("--foo")};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("opção desconhecida: --foo"));
}

void TstDpsValidar::opcaoRepetidaRetornaUso()
{
    const QString saida = caminhoSaida(QStringLiteral("opcao_repetida"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("opção repetida: --xsd"));
}

QTEST_GUILESS_MAIN(TstDpsValidar)

#include "tst_dps_validar.moc"
