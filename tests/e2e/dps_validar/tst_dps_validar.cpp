// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Arnês ponta a ponta de `notalivre dps validar` (NL-1.7). Roda a CLI em processo próprio (QProcess), como o usuário a roda,
// e confere código de saída, XML gravado e mensagem no stderr. Não inclui nenhuma fachada: só fala com o binário.
// Os XML de referência são as fixtures do codec (tests/fixtures/dps); a saída da CLI tem de sair byte a byte igual.

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QtTest>

#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

namespace {

const QString kCli = QStringLiteral(NL_CLI_DPS);
const QString kEntradas = QStringLiteral(NL_ENTRADAS_DPS);
const QString kSaida = QStringLiteral(NL_SAIDA_DPS);
const QString kFixtures = QStringLiteral(NL_FIXTURES_DPS);
const QString kXsd = QStringLiteral(NL_XSD_PRODREST_DIR);
const QString kManifesto = QStringLiteral(NL_XSD_PRODREST_MANIFESTO);
const QString kTabelas = QStringLiteral(NL_TABELAS_DIR);

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

/// Argumentos da subcomando `dps validar`: entrada, saída, o XSD de Produção Restrita, o manifesto e as tabelas do Anexo A (NL-3.M).
QStringList argumentosValidar(const QString& entrada, const QString& saida)
{
    return {QStringLiteral("dps"), QStringLiteral("validar"), entrada, saida,
            QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto,
            QStringLiteral("--tabelas"), kTabelas};
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

// NL-3.M (passo 4, seção 4.2): cada variante abaixo dispara SÓ a regra local da linha, a partir de uma entrada base. Vão para a
// pasta de saída (nunca para entradas/, que pede exceção de SPDX por arquivo).

/// Texto JSON. QT_NO_CAST_FROM_ASCII está ligado no projeto: o literal passa por fromUtf8.
QJsonValue txt(const char* valor)
{
    return QJsonValue(QString::fromUtf8(valor));
}

/// Objeto JSON a partir de pares chave-valor de literais.
QJsonObject objetoDe(std::initializer_list<std::pair<const char*, QJsonValue>> pares)
{
    QJsonObject objeto;
    for (const auto& par : pares) {
        objeto.insert(QString::fromUtf8(par.first), par.second);
    }
    return objeto;
}

/// Endereço nacional com os cinco campos obrigatórios da entrada e o código do município dado.
QJsonObject enderecoNacional(const char* cMun)
{
    return objetoDe({{"cMun", txt(cMun)}, {"cep", txt("01001-000")}, {"xLgr", txt("Rua das Flores")}, {"nro", txt("10")},
                     {"xBairro", txt("Centro")}});
}

/// Troca um campo de um grupo de primeiro nível. QJsonObject é por valor: copia o grupo, altera e reinsere.
void defineNoGrupo(QJsonObject& raiz, const char* grupo, const char* campo, const QJsonValue& valor)
{
    const QString chave = QString::fromUtf8(grupo);
    QJsonObject alvo = raiz.value(chave).toObject();
    alvo.insert(QString::fromUtf8(campo), valor);
    raiz.insert(chave, alvo);
}

/// Troca um campo de valores.tribMun.
void defineTribMun(QJsonObject& raiz, const char* campo, const QJsonValue& valor)
{
    QJsonObject valores = raiz.value(QStringLiteral("valores")).toObject();
    QJsonObject tribMun = valores.value(QStringLiteral("tribMun")).toObject();
    tribMun.insert(QString::fromUtf8(campo), valor);
    valores.insert(QStringLiteral("tribMun"), tribMun);
    raiz.insert(QStringLiteral("valores"), valores);
}

/// Grava na pasta de saída a entrada base com a mutação aplicada, como regra_<nome>.json. Devolve o caminho, ou vazio se falhar.
QString gravarVariante(const QString& base, const QString& nome, void (*mutar)(QJsonObject&))
{
    bool lido = false;
    const QByteArray conteudo = lerArquivo(caminhoEntrada(base), &lido);
    if (!lido) {
        return {};
    }
    QJsonObject raiz = QJsonDocument::fromJson(conteudo).object();
    mutar(raiz);

    const QString caminho = QStringLiteral("%1/regra_%2.json").arg(kSaida, nome);
    QFile arquivo{caminho};
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    arquivo.write(QJsonDocument{raiz}.toJson());
    return caminho;
}

/// Pares "código|campo" de cada linha "erro <código> (<campo>): ..." do stderr, ordenados. Linha de erro sem campo vira "código|".
QStringList paresDeErro(const QString& stderrTexto)
{
    static const QRegularExpression linhaDeErro{QStringLiteral(R"re(^erro (\S+)(?: \(([^)]*)\))?: )re")};
    QStringList pares;
    for (const QString& linha : stderrTexto.split(QLatin1Char('\n'))) {
        const QRegularExpressionMatch casou = linhaDeErro.match(linha);
        if (casou.hasMatch()) {
            pares.append(casou.captured(1) + QLatin1Char('|') + casou.captured(2));
        }
    }
    pares.sort();
    return pares;
}

/// Um código e o campo em que ele tem de aparecer na linha de erro.
struct ParEsperado
{
    const char* codigo;
    const char* campo;
};

/// Uma linha da tabela de NL-3.M (seção 4.2): nome da linha, entrada base, mutação e o conjunto exato de código|campo esperado.
struct VarianteRegraLocal
{
    const char* nome;
    const char* base;
    void (*mutar)(QJsonObject&);
    std::vector<ParEsperado> esperados;
};

std::vector<VarianteRegraLocal> variantesRegraLocal()
{
    return {
        {"e0202_cnpj_do_tomador_igual_ao_prestador", "tomador_cpf",
         [](QJsonObject& r) {
             r.insert(QStringLiteral("tomador"),
                      objetoDe({{"cnpj", txt("11222333000181")}, {"xNome", txt("Cliente Empresa")}, {"endereco", enderecoNacional("4305108")}}));
         },
         {{"E0202", "Dps.infDPS.toma.CNPJ"}}},
        {"e0204_e0237_e0241_retencao_sem_tomador", "simples_ibscbs",
         [](QJsonObject& r) { defineTribMun(r, "tpRetISSQN", txt("2")); },
         {{"E0204", "Dps.infDPS.toma"}, {"E0237", "Dps.infDPS.toma.end.endNac"}, {"E0241", "Dps.infDPS.toma"}}},
        {"e0237_e0241_retencao_tomador_cpf_sem_endereco", "simples_ibscbs",
         [](QJsonObject& r) {
             defineTribMun(r, "tpRetISSQN", txt("2"));
             r.insert(QStringLiteral("tomador"), objetoDe({{"cpf", txt("52998224725")}, {"xNome", txt("Cliente Pessoa Fisica")}}));
         },
         {{"E0237", "Dps.infDPS.toma.end.endNac"}, {"E0241", "Dps.infDPS.toma"}}},
        {"e0238_municipio_do_tomador_inexistente", "tomador_cpf",
         [](QJsonObject& r) { defineNoGrupo(r, "tomador", "endereco", enderecoNacional("9999999")); },
         {{"E0238", "Dps.infDPS.toma.end.endNac.cMun"}}},
        {"e0302_local_da_prestacao_inexistente", "mei_minima",
         [](QJsonObject& r) { defineNoGrupo(r, "servico", "cLocPrestacao", txt("9999999")); },
         {{"E0302", "Dps.infDPS.serv.locPrest.cLocPrestacao"}}},
        {"e0313_mei_com_ctribmun", "mei_minima",
         [](QJsonObject& r) { defineNoGrupo(r, "servico", "cTribMun", txt("501")); },
         {{"E0313", "Dps.infDPS.serv.cServ.cTribMun"}}},
        {"e0390_ctribnac_item_12_sem_atvevento", "mei_minima",
         [](QJsonObject& r) { defineNoGrupo(r, "servico", "cTribNac", txt("120101")); },
         {{"E0390", "Dps.infDPS.serv.atvEvento"}}},
        {"e0390_ctribnac_subitem_17_10_sem_atvevento", "mei_minima",
         [](QJsonObject& r) { defineNoGrupo(r, "servico", "cTribNac", txt("171001")); },
         {{"E0390", "Dps.infDPS.serv.atvEvento"}}},
        {"e0532_ctribnac_99_com_issqn_1", "mei_minima",
         [](QJsonObject& r) {
             defineNoGrupo(r, "servico", "cTribNac", txt("990101"));
             defineTribMun(r, "tribISSQN", txt("1"));
         },
         {{"E0532", "Dps.infDPS.valores.trib.tribMun.tribISSQN"}}},
        {"e0583_mei_com_retencao_pelo_intermediario", "mei_minima",
         [](QJsonObject& r) { defineTribMun(r, "tpRetISSQN", txt("3")); },
         {{"E0583", "Dps.infDPS.valores.trib.tribMun.tpRetISSQN"}}},
        {"e0850_ibscbs_com_competencia_de_2025", "simples_ibscbs",
         [](QJsonObject& r) { r.insert(QStringLiteral("dCompet"), txt("2025-12-31")); },
         {{"E0850", "Dps.infDPS.IBSCBS"}}},
        {"e0903_ctribnac_2505_sem_tpoper", "mei_minima",
         [](QJsonObject& r) { defineNoGrupo(r, "servico", "cTribNac", txt("250501")); },
         {{"E0903", "Dps.infDPS.IBSCBS.tpOper"}}},
        {"e0910_indest_1_sem_grupo_dest", "tomador_cpf",
         [](QJsonObject& r) { r.insert(QStringLiteral("indDest"), txt("1")); },
         {{"E0910", "Dps.infDPS.dest"}}},
    };
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

    // NL-3.L: indDest na raiz da entrada (leiaute de 24/11/2026), opcional com padrão 0; o lugar antigo é recusado.
    void indDestNaRaizDoJson();
    void indDestDentroDeIbscbsRecusado();
    void semIndDestUsaZero();
    // NL-3.L (B2): indDest 2 é recusado pelo leitor. Variante de tomador_cpf gravada na pasta de saída.
    void indDestDoisNaRaizRecusado();

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

    // NL-3.M (passo 4): cada regra local reprova com o código e o campo da tabela; o aviso D1 não impede a gravação.
    void regraLocalReprovaComCodigoECampo_data();
    void regraLocalReprovaComCodigoECampo();
    void avisoD1NaoImpedeAGravacao();
    // NL-3.M: --tabelas é obrigatório como --xsd; diretório ausente é ambiente (rc 2).
    void tabelasAusentesRetornaAmbiente();
    void tabelasFaltandoRetornaUso();
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

/// XML gerado pela CLI para a entrada dada; vazio se a CLI não saiu com rc 0 ou não gravou (o QVERIFY do chamador acusa).
QByteArray xmlValidadoDe(const QString& nome)
{
    const QString saida = caminhoSaida(nome);
    QFile::remove(saida);
    const Execucao execucao = executarCli(argumentosValidar(caminhoEntrada(nome), saida));
    if (!(execucao.terminou && execucao.codigo == 0)) {
        qWarning("rc=%d stderr=%s", execucao.codigo, qPrintable(execucao.erro));
        return {};
    }
    bool lido = false;
    const QByteArray xml = lerArquivo(saida, &lido);
    return lido ? xml : QByteArray{};
}

/// Grava na pasta de saída a entrada tomador_cpf com "indDest" valor na raiz. A variante fica fora de entradas/, porque cada JSON
/// de lá pede exceção no portão de SPDX. Devolve o caminho, ou vazio se não conseguiu gravar.
QString gravarVarianteIndDest(const QString& valor)
{
    bool lido = false;
    const QByteArray base = lerArquivo(caminhoEntrada(QStringLiteral("tomador_cpf")), &lido);
    if (!lido) {
        return {};
    }
    QJsonObject raiz = QJsonDocument::fromJson(base).object();
    raiz.insert(QStringLiteral("indDest"), valor);

    const QString caminho = QStringLiteral("%1/indDest_%2.json").arg(kSaida, valor);
    QFile arquivo{caminho};
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    arquivo.write(QJsonDocument{raiz}.toJson());
    return caminho;
}

void TstDpsValidar::indDestNaRaizDoJson()
{
    // simples_ibscbs.json traz "indDest" na raiz e o grupo ibscbs sem ele: o XML sai com indDest entre prest e serv, fora de IBSCBS.
    const QByteArray gerado = xmlValidadoDe(QStringLiteral("simples_ibscbs"));
    QVERIFY2(!gerado.isEmpty(), "a CLI tinha de validar a entrada com indDest na raiz");
    const qsizetype posPrest = gerado.indexOf("</prest>");
    const qsizetype posIndDest = gerado.indexOf("<indDest>0</indDest>");
    const qsizetype posServ = gerado.indexOf("<serv>");
    const qsizetype posIbscbs = gerado.indexOf("<IBSCBS>");
    QVERIFY2(posIndDest >= 0, "o XML tem de trazer <indDest>0</indDest>");
    QVERIFY2(posPrest < posIndDest && posIndDest < posServ, "indDest tem de sair entre prest e serv, na raiz de infDPS");
    QVERIFY2(posIbscbs > posServ && gerado.indexOf("<indDest>", posIbscbs) < 0, "IBSCBS não pode trazer indDest");
}

void TstDpsValidar::indDestDentroDeIbscbsRecusado()
{
    // Entrada no formato antigo (indDest dentro de ibscbs): recusada com o campo e o lugar novo na mensagem, sem XML.
    verificaReprovaEntrada(QStringLiteral("inddest_dentro_de_ibscbs"), QStringLiteral("entrada.campo_fora_do_lugar"),
                           QStringLiteral("ibscbs.indDest"), QStringLiteral("raiz da entrada"));
}

void TstDpsValidar::semIndDestUsaZero()
{
    // mei_minima.json não traz indDest: vale o padrão 0 (decisão do líder, 27/11/2026, "Raiz, padrão 0 (Recomendado)").
    const QByteArray gerado = xmlValidadoDe(QStringLiteral("mei_minima"));
    QVERIFY2(!gerado.isEmpty(), "a CLI tinha de validar a MEI mínima sem indDest");
    QVERIFY2(gerado.contains("<indDest>0</indDest>"), "sem indDest na entrada, o XML tem de sair com indDest 0");
}

void TstDpsValidar::indDestDoisNaRaizRecusado()
{
    // Arrange: indDest "2" está fora do conjunto do XSD. O leitor recusa; a mensagem da CLI traz o código do valor, sem XML.
    const QString entrada = gravarVarianteIndDest(QStringLiteral("2"));
    QVERIFY2(!entrada.isEmpty(), "não consegui gravar a variante com indDest 2");
    const QString saida = caminhoSaida(QStringLiteral("indDest_2"));
    QFile::remove(saida);

    // Act
    const Execucao execucao = executarCli(argumentosValidar(entrada, saida));

    // Assert
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 1, qPrintable(QStringLiteral("rc=%1, esperado 1 (reprovada). stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("indicadordestinatario.valor")), qPrintable(execucao.erro));
    QVERIFY2(execucao.erro.contains(QStringLiteral("(indDest)")), qPrintable(QStringLiteral("campo indDest ausente. stderr: %1").arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("valor fora do conjunto do XSD")), qPrintable(execucao.erro));
    QVERIFY2(!QFile::exists(saida), "entrada reprovada não pode deixar XML gravado");
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

    // Falta só --manifesto (as tabelas vêm): a linha de comando é incompleta e a CLI tem de recusar antes de ler qualquer arquivo.
    // O detalhe tem de nomear a opção que falta; a linha de uso cita --manifesto sempre, então não serve de prova.
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--tabelas"), kTabelas};
    const Execucao execucao = executarCli(argumentos);
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 2, qPrintable(QStringLiteral("rc=%1, esperado 2 (uso). stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("uso:")), qPrintable(execucao.erro));
    QVERIFY2(execucao.erro.contains(QStringLiteral("falta --manifesto")), qPrintable(execucao.erro));
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
                                 QStringLiteral("--xsd"), QStringLiteral("/nao_existe_xsd_dps"), QStringLiteral("--manifesto"), kManifesto,
                                 QStringLiteral("--tabelas"), kTabelas};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("xsd.esquema"));
}

void TstDpsValidar::manifestoIlegivelRetornaAmbiente()
{
    // O manifesto não abre: a abertura do esquema recusa com xsd.esquema e a mensagem "manifesto ilegível". O trecho é essa frase
    // inteira; "manifesto" sozinho também casa com a linha de uso e passaria por acaso.
    const QString saida = caminhoSaida(QStringLiteral("manifesto_ilegivel"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), QStringLiteral("/nao_existe_manifesto_dps"),
                                 QStringLiteral("--tabelas"), kTabelas};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("manifesto ilegível"));
}

void TstDpsValidar::opcaoDesconhecidaRetornaUso()
{
    const QString saida = caminhoSaida(QStringLiteral("opcao_desconhecida"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto, QStringLiteral("--tabelas"),
                                 kTabelas, QStringLiteral("--foo")};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("opção desconhecida: --foo"));
}

void TstDpsValidar::opcaoRepetidaRetornaUso()
{
    const QString saida = caminhoSaida(QStringLiteral("opcao_repetida"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto,
                                 QStringLiteral("--tabelas"), kTabelas};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("opção repetida: --xsd"));
}

void TstDpsValidar::regraLocalReprovaComCodigoECampo_data()
{
    QTest::addColumn<int>("indice");
    const std::vector<VarianteRegraLocal> variantes = variantesRegraLocal();
    for (std::size_t i = 0; i < variantes.size(); ++i) {
        QTest::newRow(variantes[i].nome) << static_cast<int>(i);
    }
}

void TstDpsValidar::regraLocalReprovaComCodigoECampo()
{
    // Arrange: a variante da linha. Act: a CLI com o catálogo real. Assert: rc 1, sem XML, e o conjunto exato de código|campo.
    QFETCH(int, indice);
    const VarianteRegraLocal variante = variantesRegraLocal().at(static_cast<std::size_t>(indice));
    const QString entrada = gravarVariante(QString::fromUtf8(variante.base), QString::fromUtf8(variante.nome), variante.mutar);
    QVERIFY2(!entrada.isEmpty(), "não consegui gravar a variante da regra local");
    const QString saida = caminhoSaida(QString::fromUtf8(variante.nome));
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(entrada, saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 1,
             qPrintable(QStringLiteral("rc=%1, esperado 1 (reprovada por regra local). stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(!QFile::exists(saida), "DPS reprovada por regra local não pode deixar XML gravado");

    QStringList esperados;
    for (const ParEsperado& par : variante.esperados) {
        esperados.append(QString::fromUtf8(par.codigo) + QLatin1Char('|') + QString::fromUtf8(par.campo));
    }
    esperados.sort();
    QCOMPARE(paresDeErro(execucao.erro), esperados);
}

void TstDpsValidar::avisoD1NaoImpedeAGravacao()
{
    // Tomador por CNPJ válido, diferente do prestador, sem endereço: o aviso D1 sai no stderr, sem linha de erro, e a DPS é gravada.
    const QString entrada = gravarVariante(QStringLiteral("tomador_cpf"), QStringLiteral("aviso_d1"), [](QJsonObject& r) {
        r.insert(QStringLiteral("tomador"), objetoDe({{"cnpj", txt("11444777000161")}, {"xNome", txt("Cliente Empresa")}}));
    });
    QVERIFY2(!entrada.isEmpty(), "não consegui gravar a variante do aviso D1");
    const QString saida = caminhoSaida(QStringLiteral("aviso_d1"));
    QFile::remove(saida);

    const Execucao execucao = executarCli(argumentosValidar(entrada, saida));
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 0, qPrintable(QStringLiteral("rc=%1, stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("aviso D1 (Dps.infDPS.toma.end):")), qPrintable(execucao.erro));
    QVERIFY2(!execucao.erro.contains(QStringLiteral("erro ")), qPrintable(execucao.erro));
    QVERIFY2(QFile::exists(saida), "aviso não impede a gravação: o XML tem de sair com rc 0");
}

void TstDpsValidar::tabelasAusentesRetornaAmbiente()
{
    const QString saida = caminhoSaida(QStringLiteral("tabelas_ausentes"));
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto, QStringLiteral("--tabelas"),
                                 QStringLiteral("/nao_existe_tabelas_dps")};
    verificaRecusaRc2(argumentos, saida, QStringLiteral("tabelas.ausente"));
}

void TstDpsValidar::tabelasFaltandoRetornaUso()
{
    // Sem --tabelas a linha é incompleta: rc 2, o detalhe nomeia a opção que falta e a linha de uso a cita.
    const QString saida = caminhoSaida(QStringLiteral("tabelas_faltando"));
    QFile::remove(saida);
    const QStringList argumentos{QStringLiteral("dps"), QStringLiteral("validar"), caminhoEntrada(QStringLiteral("mei_minima")), saida,
                                 QStringLiteral("--xsd"), kXsd, QStringLiteral("--manifesto"), kManifesto};
    const Execucao execucao = executarCli(argumentos);
    QVERIFY2(execucao.terminou, qPrintable(execucao.erro));
    QVERIFY2(execucao.codigo == 2, qPrintable(QStringLiteral("rc=%1, esperado 2 (uso). stderr: %2").arg(execucao.codigo).arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("uso:")) && execucao.erro.contains(QStringLiteral("--tabelas <diretorio>")),
             qPrintable(QStringLiteral("a linha de uso tem de citar --tabelas. stderr: %1").arg(execucao.erro)));
    QVERIFY2(execucao.erro.contains(QStringLiteral("falta --tabelas")), qPrintable(execucao.erro));
    QVERIFY2(!QFile::exists(saida), "linha de comando recusada não pode gravar XML");
}

QTEST_GUILESS_MAIN(TstDpsValidar)

#include "tst_dps_validar.moc"
