// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da resposta crua para análise local (NL-2.9, DA-2.59 item 4): a mascaragem troca o XML da NFS-e e a chave, e a
// gravação sai em 0600 com nome que não contém o Id. Fixtures sintéticas, sem dado real.
#include <nl/mid/composicao/resposta_crua.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Chave sintética de 44 caracteres (não é chave real de NFS-e).
const QByteArray kChaveSintetica = "12345678901234567890123456789012345678901234";
const QByteArray kXmlSintetico = "H4sIAAAAAAAAAPNIzcnJVyjPL8pKUQQAjAqUFgUAAAA";

// Dono lê e escreve; grupo e outros sem nenhum bit (0600). O Qt devolve o bit de usuário junto do de dono, então a
// igualdade exata com ReadOwner|WriteOwner nunca bate; por isso a conferência é por máscara.
bool permissaoSoDoDono(const QString& caminho)
{
    const QFileDevice::Permissions p = QFile::permissions(caminho);
    const QFileDevice::Permissions dono = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    const QFileDevice::Permissions outros = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                            QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
    return (p & dono) == dono && (p & outros) == 0;
}

} // namespace

class TstRespostaCrua : public QObject
{
    Q_OBJECT

private slots:
    // O XML da NFS-e (GZip em Base64) e a chave não aparecem na resposta mascarada; a chave vira # no mesmo tamanho.
    void mascaraTrocaXmlEChave();
    // Corpo que não é JSON não passa cru: vira um aviso, sem nenhum conteúdo do corpo.
    void corpoQueNaoEJsonViraAviso();
    // A gravação sai com permissão 0600 e o nome do arquivo não contém o Id da DPS.
    void gravacaoSai0600ComNomeSemId();
    // NL-2.10a, passo 7 (CISO N-03): só a allowlist grava valor; um campo fora dela sai como <tipo:tamanho>, nunca com o valor.
    void respostaCruaGravadaPorAllowlist();
    // NL-2.10a, passo 7 (CISO N-03): um campo novo com o CPF da fixture não põe o CPF no arquivo gravado (varredura com contagem).
    void campoNovoComCpfSomeDoArquivo();
    // NL-2.10a, rodada 2 (CISO, pedido de 31/10): o CPF dentro de objeto aninhado não chega ao arquivo.
    void cpfAninhadoSomeDoArquivo();
    // NL-2.10a, rodada 2 (CISO): o CPF dentro de array não chega ao arquivo.
    void cpfEmArraySomeDoArquivo();
    // NL-2.10a, rodada 2 (CISO): o CPF como número JSON (não string) não chega ao arquivo.
    void cpfNumericoSomeDoArquivo();
    // NL-2.10c, passo 2 (DA-2.66, emenda 1): a recusa da Sefin também vai ao arquivo, pela allowlist, nas duas formas do
    // relato secundário ({"Codigo","Descricao"} e {"erros":[{"codigo","descricao"}]}). O texto livre não sai em nenhuma.
    void respostaCruaDaRecusaGravadaPorAllowlist();
    // NL-2.10c, passo 2: um campo novo com o CPF da fixture numa recusa não põe o CPF no arquivo (varredura com contagem).
    void campoNovoComCpfNaRecusaSomeDoArquivo();
    // NL-2.10c, emenda 1 item a condição do CISO (31/10/2026, 22:07:35): o código E sai sem máscara só nos campos Codigo e
    // codigo, e só quando o valor inteiro casa com ^E\d{3,4}$. Com texto junto, ou num campo de outro nome, sai mascarado.
    void codigoEValidoApareceECodigoForaDoFormatoNao();
    // NL-2.10c, condição do CISO: a allowlist desce em arrays; o CPF como string dentro de objeto de array não sai.
    void cpfComoStringEmObjetoDeArrayMascarado();
    // NL-2.10c, condição do CISO: o CPF como número dentro de array não sai; o número vira marcador de tipo e tamanho.
    void cpfComoNumeroDentroDeArrayMascarado();
    // NL-2.10c, condição do CISO: o código E de erros[].codigo aparece quando é válido, e fica mascarado quando não é.
    void codigoEDentroDeErrosAparece();
    // NL-2.10c, condição do CISO: a descida para em oito níveis; o objeto do nono nível vira <profundidade>.
    void profundidadeAcimaDeOitoViraMarcador();
    // NL-2.10c, condição do CISO: um array com mais de cem elementos mascara os cem primeiros e só conta o resto (<mais:N>).
    void arrayAcimaDeCemElementosSoContado();
    // NL-2.10c, condição do CISO: ao atingir o teto de 4 MiB por arquivo, nenhuma linha é cortada pela metade; o arquivo termina
    // com uma linha de corte que traz o total de respostas omitidas.
    void arquivoNoTetoParaComLinhaDeCorte();
};

void TstRespostaCrua::mascaraTrocaXmlEChave()
{
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"nfseXmlGZipB64\":\"" + kXmlSintetico + "\"}";
    const QByteArray mascarado = nl::mid::composicao::mascararRespostaCrua(corpo);
    QVERIFY2(!mascarado.contains(kXmlSintetico), "o XML da NFS-e não pode sair na resposta mascarada");
    QVERIFY2(!mascarado.contains(kChaveSintetica), "a chave não pode sair em claro na resposta mascarada");
    QVERIFY2(mascarado.contains("\"chaveAcesso\":\"" + QByteArray(44, '#') + "\""), "a chave tem de sair com # no mesmo tamanho");
}

void TstRespostaCrua::corpoQueNaoEJsonViraAviso()
{
    const QByteArray mascarado = nl::mid::composicao::mascararRespostaCrua("texto livre com 12345678901234 dentro");
    QVERIFY2(mascarado.contains("aviso"), "corpo que não é JSON tem de virar aviso");
    QVERIFY2(!mascarado.contains("texto livre"), "o texto do corpo não pode sair");
}

void TstRespostaCrua::gravacaoSai0600ComNomeSemId()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"nfseXmlGZipB64\":\"" + kXmlSintetico + "\"}";
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0001");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    QVERIFY2(!gravador.ultimoErro(), "a resposta tem de ser gravada na pasta de análise");
    QVERIFY2(gravador.caminho().endsWith(QStringLiteral(".jsonl")), "o arquivo é .jsonl: uma linha JSON por resposta");
    QVERIFY2(permissaoSoDoDono(gravador.caminho()), "o arquivo tem de ser 0600: dono lê e escreve, grupo e outros sem bit");
    QVERIFY2(!gravador.caminho().contains(QStringLiteral("DPS-ID-SINTETICO")), "o nome do arquivo não pode conter o Id da DPS");
    QFile arquivo(gravador.caminho());
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    QVERIFY2(!arquivo.readAll().contains(kXmlSintetico), "o XML não pode estar no arquivo gravado");
}

void TstRespostaCrua::respostaCruaGravadaPorAllowlist()
{
    // "protocolo" não está na allowlist: o valor não pode sair, só a forma <string:5>.
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"protocolo\":\"98765\",\"nfseXmlGZipB64\":\"" + kXmlSintetico + "\"}";
    const QByteArray mascarado = nl::mid::composicao::mascararRespostaCrua(corpo);
    QVERIFY2(!mascarado.contains("98765"), "um campo fora da allowlist não pode sair com o valor");
    QVERIFY2(mascarado.contains("\"protocolo\":\"<string:5>\""), "o campo tem de sair com a forma <tipo:tamanho>");
}

void TstRespostaCrua::campoNovoComCpfSomeDoArquivo()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    // CPF da fixture (tests/fixtures/certificados, CPF_OK) dentro de um campo que a allowlist não conhece.
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"novoCampo\":\"12345678909\"}";
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0002");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    QVERIFY2(!gravador.ultimoErro(), "a resposta tem de ser gravada na pasta de análise");
    QFile arquivo(gravador.caminho());
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    const QByteArray conteudo = arquivo.readAll();
    const qsizetype ocorrencias = conteudo.count("12345678909");
    QVERIFY2(ocorrencias == 0, qPrintable(QStringLiteral("CPF da fixture no arquivo gravado: %1 ocorrência(s) em %2 bytes varridos")
                                              .arg(ocorrencias)
                                              .arg(conteudo.size())));
}

namespace {

// Grava o corpo com GravadorRespostaCrua e devolve quantas vezes o CPF da fixture aparece no arquivo gravado (0 é o esperado).
qsizetype ocorrenciasDoCpfNoArquivo(const QByteArray& corpo, const char* idDps)
{
    QTemporaryDir pasta;
    if (!pasta.isValid()) {
        return -1;
    }
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), idDps);
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    if (gravador.ultimoErro() || gravador.caminho().isEmpty()) {
        return -1;
    }
    QFile arquivo(gravador.caminho());
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return -1;
    }
    return arquivo.readAll().count("12345678909");
}

} // namespace

void TstRespostaCrua::cpfAninhadoSomeDoArquivo()
{
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"titular\":{\"contato\":{\"cpf\":\"12345678909\"}}}";
    const qsizetype ocorrencias = ocorrenciasDoCpfNoArquivo(corpo, "DPS-ID-SINTETICO-0003");
    QVERIFY2(ocorrencias == 0, qPrintable(QStringLiteral("CPF aninhado no arquivo gravado: %1 ocorrência(s)").arg(ocorrencias)));
}

void TstRespostaCrua::cpfEmArraySomeDoArquivo()
{
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"itens\":[\"12345678909\",\"outro\"]}";
    const qsizetype ocorrencias = ocorrenciasDoCpfNoArquivo(corpo, "DPS-ID-SINTETICO-0004");
    QVERIFY2(ocorrencias == 0, qPrintable(QStringLiteral("CPF em array no arquivo gravado: %1 ocorrência(s)").arg(ocorrencias)));
}

void TstRespostaCrua::cpfNumericoSomeDoArquivo()
{
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"cpfNumerico\":12345678909}";
    const qsizetype ocorrencias = ocorrenciasDoCpfNoArquivo(corpo, "DPS-ID-SINTETICO-0005");
    QVERIFY2(ocorrencias == 0, qPrintable(QStringLiteral("CPF numérico no arquivo gravado: %1 ocorrência(s)").arg(ocorrencias)));
}

void TstRespostaCrua::respostaCruaDaRecusaGravadaPorAllowlist()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0006");
    gravador.registrar("POST", "/nfse", 400, QByteArrayLiteral("application/json; charset=utf-8"),
                       QByteArrayLiteral("{\"Codigo\":\"E1203\",\"Descricao\":\"texto livre com CPF 12345678909 da recusa\"}"));
    gravador.registrar("POST", "/nfse", 400, QByteArrayLiteral("application/json; charset=utf-8"),
                       QByteArrayLiteral("{\"erros\":[{\"codigo\":\"E0714\",\"descricao\":\"texto livre com CPF 12345678909 da recusa\"}]}"));
    QVERIFY2(!gravador.ultimoErro(), "a recusa tem de ser gravada na pasta de análise");
    QFile arquivo(gravador.caminho());
    QVERIFY2(arquivo.open(QIODevice::ReadOnly), "o arquivo da recusa tem de existir");
    const QByteArray conteudo = arquivo.readAll();
    QVERIFY2(conteudo.count('\n') == 2, qPrintable(QStringLiteral("uma linha JSON por resposta: %1 linha(s)").arg(conteudo.count('\n'))));
    QVERIFY2(conteudo.count("\"status\":400") == 2, "cada linha da recusa leva o status HTTP");
    QVERIFY2(conteudo.count("\"metodo\":\"POST\"") == 2, "cada linha leva o método");
    QVERIFY2(conteudo.count("\"rota\":\"/nfse\"") == 2, "cada linha leva a rota modelo");
    QVERIFY2(conteudo.count("\"contentType\":\"application/json") == 2, "cada linha leva o Content-Type");
    QVERIFY2(!conteudo.contains("texto livre"), "o texto livre da recusa não pode sair, em nenhuma das duas formas");
    QVERIFY2(conteudo.count("12345678909") == 0, "o CPF da recusa não pode sair");
}

void TstRespostaCrua::campoNovoComCpfNaRecusaSomeDoArquivo()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0007");
    // CPF da fixture dentro de um campo que a allowlist não conhece, numa recusa (400).
    gravador.registrar("POST", "/nfse", 400, QByteArrayLiteral("application/json"),
                       QByteArrayLiteral("{\"Codigo\":\"E1203\",\"novoCampo\":\"12345678909\"}"));
    QFile arquivo(gravador.caminho());
    QVERIFY2(arquivo.open(QIODevice::ReadOnly), "a recusa tem de estar gravada");
    const qsizetype ocorrencias = arquivo.readAll().count("12345678909");
    QVERIFY2(ocorrencias == 0, qPrintable(QStringLiteral("CPF da fixture na recusa gravada: %1 ocorrência(s)").arg(ocorrencias)));
}

void TstRespostaCrua::codigoEValidoApareceECodigoForaDoFormatoNao()
{
    const QByteArray valido = nl::mid::composicao::mascararRespostaCrua("{\"Codigo\":\"E0714\",\"Descricao\":\"x\"}");
    QVERIFY2(valido.contains("\"Codigo\":\"E0714\""), "um código E válido tem de aparecer no arquivo");
    const QByteArray comTexto = nl::mid::composicao::mascararRespostaCrua("{\"Codigo\":\"E1203 CPF 12345678909\"}");
    QVERIFY2(comTexto.contains("\"Codigo\":\"<string:"), "código E com texto junto fica mascarado");
    QVERIFY2(!comTexto.contains("12345678909"), "o CPF junto ao código não pode sair");
    const QByteArray outroCampo = nl::mid::composicao::mascararRespostaCrua("{\"Mensagem\":\"E0714\"}");
    QVERIFY2(outroCampo.contains("\"Mensagem\":\"<string:5>\""), "E0714 num campo de outro nome fica mascarado");
    const QByteArray erros = nl::mid::composicao::mascararRespostaCrua("{\"erros\":[{\"codigo\":\"E0714\",\"descricao\":\"x\"}]}");
    QVERIFY2(erros.contains("\"codigo\":\"E0714\""), "o código E da forma erros[].codigo também tem de aparecer");
}

void TstRespostaCrua::cpfComoStringEmObjetoDeArrayMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"itens\":[{\"cpf\":\"12345678909\"}]}");
    QVERIFY2(m.contains("\"cpf\":\"<string:11>\""), "o objeto dentro do array sai campo a campo, com o valor mascarado");
    QVERIFY2(!m.contains("12345678909"), "o CPF dentro de objeto de array não pode sair");
}

void TstRespostaCrua::cpfComoNumeroDentroDeArrayMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"itens\":[{\"n\":12345678909}]}");
    QVERIFY2(m.contains("\"n\":\"<number:"), "o número dentro do array sai como marcador de tipo e tamanho");
    QVERIFY2(!m.contains("12345678909"), "o CPF como número dentro de array não pode sair");
}

void TstRespostaCrua::codigoEDentroDeErrosAparece()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua(
        "{\"erros\":[{\"codigo\":\"E0714\",\"descricao\":\"texto\"},{\"codigo\":\"E1203 CPF 12345678909\"}]}");
    QVERIFY2(m.contains("\"codigo\":\"E0714\""), "o E0714 de erros[0].codigo tem de aparecer");
    QVERIFY2(m.contains("\"codigo\":\"<string:"), "o código com texto junto em erros[1] fica mascarado");
    QVERIFY2(!m.contains("12345678909"), "o CPF em erros[1] não pode sair");
}

void TstRespostaCrua::profundidadeAcimaDeOitoViraMarcador()
{
    // A raiz é o nível 1: até o nível 8 o objeto sai campo a campo; o objeto do nono nível vira <profundidade>.
    QByteArray corpo = "\"SEGREDO123\"";
    for (int nivel = 9; nivel >= 1; --nivel) {
        corpo = "{\"a" + QByteArray::number(nivel) + "\":" + corpo + "}";
    }
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua(corpo);
    QVERIFY2(m.contains("\"a8\":\"<profundidade>\""), "o objeto do nono nível vira <profundidade>");
    QVERIFY2(!m.contains("SEGREDO123"), "nada abaixo do oitavo nível pode sair");
}

void TstRespostaCrua::arrayAcimaDeCemElementosSoContado()
{
    QJsonArray itens;
    for (int i = 0; i < 150; ++i) {
        itens.append(QStringLiteral("v%1").arg(i));
    }
    const QByteArray corpo = QJsonDocument(QJsonObject{{QStringLiteral("itens"), itens}}).toJson(QJsonDocument::Compact);
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua(corpo);
    QVERIFY2(m.count("<string:") == 100, "os cem primeiros elementos saem mascarados, um a um");
    QVERIFY2(m.contains("<mais:50>"), "os cinquenta elementos além dos cem são só contados");
}

void TstRespostaCrua::arquivoNoTetoParaComLinhaDeCorte()
{
    // Cada resposta, depois de mascarada, ocupa cerca de 200 KB; 40 respostas passam dos 4 MiB de teto por arquivo.
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    QJsonObject grande;
    for (int k = 0; k < 1000; ++k) {
        grande.insert(QStringLiteral("campo%1_").arg(k, 200, 10, QLatin1Char('x')), QStringLiteral("valor"));
    }
    const QByteArray corpo = QJsonDocument(grande).toJson(QJsonDocument::Compact);
    constexpr int kEnviadas = 40;
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0009");
    for (int i = 0; i < kEnviadas; ++i) {
        gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    }
    QVERIFY2(!gravador.ultimoErro(), "a gravação até o teto não pode falhar");
    QFile arquivo(gravador.caminho());
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    const QByteArray conteudo = arquivo.readAll();
    const QByteArray chaveCorte = QByteArrayLiteral("{\"corte\":\"teto de 4194304 bytes atingido\",\"respostas_omitidas\":");
    const qsizetype inicioCorte = conteudo.lastIndexOf(chaveCorte);
    QVERIFY2(inicioCorte >= 0, "ao atingir o teto, o arquivo termina com a linha de corte");
    QVERIFY2(conteudo.count(chaveCorte) == 1, "só uma linha de corte no arquivo, sem contagem defasada");
    QVERIFY2(inicioCorte <= 4194304, "as respostas gravadas não passam do teto");
    int gravadas = 0;
    for (const QByteArray& linha : conteudo.split('\n')) {
        if (linha.isEmpty() || linha.startsWith(QByteArrayLiteral("{\"corte\""))) {
            continue;
        }
        QJsonParseError erro;
        QVERIFY2(QJsonDocument::fromJson(linha, &erro).isObject(), "cada resposta gravada é uma linha JSON inteira, nunca cortada");
        ++gravadas;
    }
    bool convertido = false;
    const qint64 omitidas = conteudo.mid(inicioCorte + chaveCorte.size()).toLongLong(&convertido);
    QVERIFY2(convertido, "a linha de corte traz o número de respostas omitidas");
    QVERIFY2(omitidas > 0, "o teto foi atingido, então há respostas omitidas");
    QVERIFY2(gravadas + omitidas == kEnviadas, "respostas gravadas mais omitidas têm de bater com as enviadas");
}

QTEST_APPLESS_MAIN(TstRespostaCrua)
#include "tst_resposta_crua.moc"
