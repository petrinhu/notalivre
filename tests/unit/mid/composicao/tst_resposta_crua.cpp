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

#include <sys/stat.h>

namespace {

// Chave sintética de 44 caracteres (não é chave real de NFS-e).
const QByteArray kChaveSintetica = "12345678901234567890123456789012345678901234";
const QByteArray kXmlSintetico = "H4sIAAAAAAAAAPNIzcnJVyjPL8pKUQQAjAqUFgUAAAA";

// Dono lê e escreve; grupo e outros sem nenhum bit (0600). O Qt devolve o bit de usuário junto do de dono, então a
// igualdade exata com ReadOwner|WriteOwner nunca bate; por isso a conferência é por máscara.
// Nome de chave só com letras, de 63 caracteres e largura fixa. A allowlist fechada (Emenda 3) não deixa sair em claro um nome
// desses, que não é lido pelo cliente: ele sai como forma <chave:63:letras:k>, e a forma conta no tamanho da resposta. Por isso
// o teste precisa de 3000 chaves por corpo para cruzar o teto.
QString nomeDeChaveLongo(int k)
{
    const char a = static_cast<char>('a' + k / 676 % 26);
    const char b = static_cast<char>('a' + k / 26 % 26);
    const char c = static_cast<char>('a' + k % 26);
    return QStringLiteral("campo") + QLatin1Char(a) + QLatin1Char(b) + QLatin1Char(c) + QString(55, QLatin1Char('x'));
}

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
    // NL-2.10c, Emenda 2, MÉDIA-1 (CISO, 31/10/2026 23:18:32): um nome de chave só sai em claro se casar com
    // ^[A-Za-z_][A-Za-z0-9_]{0,63}$ e tiver no máximo 2 dígitos. Fora disso, sai como <chave:N>, com N o tamanho do nome.
    void cpfComoNomeDeChaveMascarado();
    void cnpjNumericoComoNomeDeChaveMascarado();
    void cnpjAlfanumericoComoNomeDeChaveMascarado();
    // Guarda: os nomes que a Sefin usa de fato (e a forma dos objetos) continuam legíveis.
    void nomesConhecidosDaSefinContinuamLegiveis();
    // NL-2.10c, Emenda 2, BAIXA-2: o Content-Type só sai em claro se for application/json ou application/problem+json,
    // com ou sem charset simples. Qualquer outro valor sai como <string:N>.
    void tipoDeConteudoDesconhecidoMascarado();
    // NL-2.10c, Emenda 2, BAIXA-3: o arquivo nasce 0600, conferido com o umask do processo em 0022.
    void arquivoNasceComPermissao0600();
    // NL-2.10c, Emenda 2, BAIXA-4: a pasta respostas nasce 0700 (sem bit de grupo nem de outros).
    void pastaDeRespostasNasceComPermissao0700();
    // NL-2.10c, Emenda 2, BAIXA-5: uma segunda execução com o mesmo Id não parte a linha nem apaga as respostas da primeira.
    void mesmoIdDuasVezesNaoPartLinhaNemCorte();
    // NL-2.10c, Emenda 3, MÉDIA-1 (allowlist fechada): o CNPJ alfanumérico com 12 letras e 2 dígitos verificadores não pode sair.
    void nomeDeCnpjAlfanumericoDe12LetrasMascarado();
    // Emenda 3, MÉDIA-1: nome que não é lido pelo cliente vira forma <chave:N:classe:k>, nunca o nome.
    void nomeDesconhecidoViraForma();
    // Emenda 3, INFO-9: dois nomes de mesmo tamanho não colidem; a ordem k distingue os dois.
    void chavesDeMesmoTamanhoNaoColidem();
    // Emenda 3, M3: nome de forma inválida com até dois dígitos ("9a", "a-b", "a.b", "aç") sai mascarado.
    void formaInvalidaComAteDoisDigitosMascarada();
    // Emenda 3, BAIXA-2': charset com dígitos no Content-Type não pode sair; a lista fechada é utf-8 e utf8.
    void charsetComDigitosMascarado();
    // Emenda 3, BAIXA-2': utf-8 e utf8 saem, sem diferenciar maiúsculas.
    void charsetUtf8SemDiferenciarMaiusculas();
    // Emenda 3, BAIXA-4b: pasta respostas existente com modo largo passa a 0700 na abertura.
    void pastaExistenteComModoLargoVira0700();
    // Emenda 3, INFO-3: arquivo existente com modo largo passa a 0600 na abertura.
    void arquivoExistenteComModoLargoVira0600();
    // Emenda 3, M10: uma terceira execução com o mesmo Id, depois do corte, mantém uma linha de corte com o total certo.
    void terceiraExecucaoDepoisDoCorteMantemUmaLinhaDeCorte();
    // Emenda 3, INFO-8: o teto de 4 MiB também é exercitado com as chaves já mascaradas.
    void tetoComChavesMascaradasContaAsRespostas();
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
    QVERIFY2(mascarado.contains("\"<chave:9:letras:"), "o nome fora da allowlist sai como forma <chave:N:classe:k>");
    QVERIFY2(mascarado.contains("\":\"<string:5>\""), "o valor do campo tem de sair com a forma <tipo:tamanho>");
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
    QVERIFY2(outroCampo.contains("\"<chave:8:letras:1>\":\"<string:5>\""), "E0714 num campo de outro nome fica mascarado");
    const QByteArray erros = nl::mid::composicao::mascararRespostaCrua("{\"erros\":[{\"codigo\":\"E0714\",\"descricao\":\"x\"}]}");
    QVERIFY2(erros.contains("\"codigo\":\"E0714\""), "o código E da forma erros[].codigo também tem de aparecer");
}

void TstRespostaCrua::cpfComoStringEmObjetoDeArrayMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"itens\":[{\"cpf\":\"12345678909\"}]}");
    QVERIFY2(m.contains("\"<chave:3:letras:1>\":\"<string:11>\""), "o objeto dentro do array sai campo a campo, com o nome em forma");
    QVERIFY2(!m.contains("12345678909"), "o CPF dentro de objeto de array não pode sair");
}

void TstRespostaCrua::cpfComoNumeroDentroDeArrayMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"itens\":[{\"n\":12345678909}]}");
    QVERIFY2(m.contains("\"<chave:1:letras:1>\":\"<number:"), "o número dentro do array sai como marcador de tipo e tamanho");
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
    // A estrutura inteira, e não só um trecho: oito objetos aninhados, cada um com uma chave só (k=1), e o nono nível como
    // <profundidade> sob a chave do oitavo. Um trecho não prova o nível, porque a7 sai igual a a8 (achado R3-M26 do QA).
    QByteArray esperado = QByteArrayLiteral("\"<profundidade>\"");
    for (int i = 0; i < 8; ++i) {
        esperado = QByteArrayLiteral("{\"<chave:2:alfanumerico:1>\":") + esperado + "}";
    }
    QVERIFY2(m == esperado, "a estrutura tem de ter oito objetos aninhados, e o nono nível vira <profundidade> sob a chave do oitavo");
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
    for (int k = 0; k < 3000; ++k) {
        grande.insert(nomeDeChaveLongo(k), QStringLiteral("valor"));
    }
    const QByteArray corpo = QJsonDocument(grande).toJson(QJsonDocument::Compact);
    constexpr int kEnviadas = 80;
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
    // O número vai até o '}' que fecha a linha. toLongLong exige a string inteira numérica, então o resto da linha é cortado antes.
    const QByteArray resto = conteudo.mid(inicioCorte + chaveCorte.size());
    bool convertido = false;
    const qint64 omitidas = resto.left(resto.indexOf('}')).toLongLong(&convertido);
    QVERIFY2(convertido, "a linha de corte traz o número de respostas omitidas");
    QVERIFY2(omitidas > 0, "o teto foi atingido, então há respostas omitidas");
    QVERIFY2(gravadas + omitidas == kEnviadas, "respostas gravadas mais omitidas têm de bater com as enviadas");
}

namespace {

// Fixa o umask do processo no trecho do teste e devolve o anterior ao sair do escopo (BAIXA-3: o teste confere com 0022).
class UmaskFixado
{
public:
    explicit UmaskFixado(mode_t novo) : anterior_(umask(novo)) {}
    ~UmaskFixado() { umask(anterior_); }
    UmaskFixado(const UmaskFixado&) = delete;
    UmaskFixado& operator=(const UmaskFixado&) = delete;

private:
    mode_t anterior_;
};

// Corpo grande: 1000 chaves de 200 caracteres. Mascarado, cada resposta ocupa cerca de 200 KB.
QByteArray corpoGrande()
{
    QJsonObject grande;
    for (int k = 0; k < 3000; ++k) {
        grande.insert(nomeDeChaveLongo(k), QStringLiteral("valor"));
    }
    return QJsonDocument(grande).toJson(QJsonDocument::Compact);
}

// Linhas não vazias do arquivo gravado.
QList<QByteArray> linhasDoArquivo(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    QList<QByteArray> linhas;
    for (const QByteArray& linha : arquivo.readAll().split('\n')) {
        if (!linha.isEmpty()) {
            linhas.append(linha);
        }
    }
    return linhas;
}

} // namespace

void TstRespostaCrua::cpfComoNomeDeChaveMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"12345678909\":\"valor\"}");
    QVERIFY2(!m.contains("12345678909"), "um CPF como nome de chave não pode sair em claro");
    QVERIFY2(m.contains("\"<chave:11:digitos:1>\""), "o nome fora da allowlist sai como forma <chave:N:classe:k>");
}

void TstRespostaCrua::cnpjNumericoComoNomeDeChaveMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"11222333000181\":\"valor\"}");
    QVERIFY2(!m.contains("11222333000181"), "um CNPJ numérico como nome de chave não pode sair em claro");
    QVERIFY2(m.contains("\"<chave:14:digitos:1>\""), "o nome com 14 dígitos sai como forma <chave:14:digitos:k>");
}

void TstRespostaCrua::cnpjAlfanumericoComoNomeDeChaveMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"AB12CD34EF56GH78\":\"valor\"}");
    QVERIFY2(!m.contains("AB12CD34EF56GH78"), "um CNPJ alfanumérico com mais de 2 dígitos como nome de chave não pode sair");
    QVERIFY2(m.contains("\"<chave:16:alfanumerico:1>\""), "o nome alfanumérico com 8 dígitos sai como forma");
}

void TstRespostaCrua::nomesConhecidosDaSefinContinuamLegiveis()
{
    // Emenda 3, MÉDIA-1: só os nomes que o cliente lê saem em claro (chaveAcesso, nfseXmlGZipB64, erro, erros, Codigo, codigo).
    // Descricao e idDps não são lidos pelo cliente, então saem como forma.
    const QByteArray corpo = "{\"chaveAcesso\":\"" + kChaveSintetica + "\",\"idDps\":\"DPS-X\",\"nfseXmlGZipB64\":\"" + kXmlSintetico
                             + "\",\"erro\":{\"Codigo\":\"E0714\",\"Descricao\":\"texto\"},\"erros\":[{\"codigo\":\"E0714\",\"descricao\":\"texto\"}]}";
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua(corpo);
    for (const char* nome : {"chaveAcesso", "nfseXmlGZipB64", "erro", "erros", "Codigo", "codigo"}) {
        QVERIFY2(m.contains(QByteArray("\"") + nome + "\":"), nome);
    }
    QVERIFY2(!m.contains("\"Descricao\":"), "Descricao não é lido pelo cliente: sai como forma, nunca como nome");
    QVERIFY2(!m.contains("\"idDps\":"), "idDps não é lido pelo cliente: sai como forma, nunca como nome");
}

void TstRespostaCrua::tipoDeConteudoDesconhecidoMascarado()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0111");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json; charset=utf-8"), QByteArrayLiteral("{\"a\":1}"));
    gravador.registrar("POST", "/nfse", 400, QByteArrayLiteral("text/html; charset=utf-8 <script>x</script>"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!gravador.ultimoErro(), "a gravação tem de funcionar nesta montagem");
    QFile arquivo(gravador.caminho());
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    const QByteArray conteudo = arquivo.readAll();
    QVERIFY2(conteudo.contains("\"contentType\":\"application/json; charset=utf-8\""), "o tipo conhecido sai em claro");
    QVERIFY2(!conteudo.contains("text/html"), "o tipo desconhecido não pode sair em claro");
    QVERIFY2(conteudo.contains("\"contentType\":\"<string:"), "o tipo desconhecido sai como <string:N>");
}

void TstRespostaCrua::arquivoNasceComPermissao0600()
{
    UmaskFixado umask0022(0022);
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0112");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!gravador.ultimoErro(), "a gravação tem de funcionar nesta montagem");
    QVERIFY2(permissaoSoDoDono(gravador.caminho()), "o arquivo tem de nascer 0600, com o umask do processo em 0022");
}

void TstRespostaCrua::pastaDeRespostasNasceComPermissao0700()
{
    UmaskFixado umask0022(0022);
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0113");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!gravador.ultimoErro(), "a gravação tem de funcionar nesta montagem");
    const QFileDevice::Permissions grupoEOutros = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                                  QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
    const QFileDevice::Permissions permissoes = QFile::permissions(pasta.path() + QStringLiteral("/respostas"));
    QVERIFY2((permissoes & grupoEOutros) == 0, "a pasta respostas tem de nascer 0700: sem bit de grupo nem de outros");
}

void TstRespostaCrua::mesmoIdDuasVezesNaoPartLinhaNemCorte()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const char* id = "DPS-ID-SINTETICO-0114";
    nl::mid::composicao::GravadorRespostaCrua primeira(pasta.path(), id);
    for (int i = 0; i < 3; ++i) {
        primeira.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), QByteArrayLiteral("{\"a\":1}"));
    }
    QVERIFY2(!primeira.ultimoErro(), "a primeira execução tem de gravar");
    constexpr int kEnviadas = 80;
    const QByteArray corpo = corpoGrande();
    nl::mid::composicao::GravadorRespostaCrua segunda(pasta.path(), id);
    for (int i = 0; i < kEnviadas; ++i) {
        segunda.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    }
    QVERIFY2(!segunda.ultimoErro(), "a segunda execução tem de gravar até o teto");
    QVERIFY2(segunda.cortado(), "a segunda execução tem de atingir o teto");
    const QList<QByteArray> linhas = linhasDoArquivo(segunda.caminho());
    QVERIFY2(linhas.size() >= 4, "as respostas da primeira execução e a linha de corte têm de estar no arquivo");
    const QByteArray marcaCorte = QByteArrayLiteral("{\"corte\":");
    int linhasDeCorte = 0;
    for (const QByteArray& linha : linhas) {
        if (linha.startsWith(marcaCorte)) {
            ++linhasDeCorte;
        }
    }
    QVERIFY2(linhasDeCorte == 1, "só pode haver uma linha de corte");
    QVERIFY2(linhas.last().startsWith(marcaCorte), "a linha de corte é a última do arquivo");
    for (int i = 0; i < 3; ++i) {
        QVERIFY2(linhas[i].contains("\"<chave:1:letras:1>\":\"<number:1>\""), "as respostas da primeira execução continuam inteiras no arquivo");
    }
    for (qsizetype i = 0; i < linhas.size() - 1; ++i) {
        QJsonParseError erro;
        QVERIFY2(QJsonDocument::fromJson(linhas[i], &erro).isObject(), "nenhuma linha de resposta pode sair partida");
    }
    const QByteArray marcaOmitidas = QByteArrayLiteral("\"respostas_omitidas\":");
    QVERIFY2(linhas.last().contains(marcaOmitidas), "a linha de corte traz o total de respostas omitidas");
    const QByteArray depois = linhas.last().mid(linhas.last().indexOf(marcaOmitidas) + marcaOmitidas.size());
    bool convertido = false;
    const qint64 omitidas = depois.left(depois.indexOf('}')).toLongLong(&convertido);
    QVERIFY2(convertido, "o total omitido tem de ser um número");
    QVERIFY2((linhas.size() - 1) + omitidas == 3 + kEnviadas, "respostas gravadas mais omitidas têm de bater com as enviadas das duas execuções");
}

void TstRespostaCrua::nomeDeCnpjAlfanumericoDe12LetrasMascarado()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"ABCDEFGHIJKL12\":\"x\"}");
    QVERIFY2(!m.contains("ABCDEFGHIJKL12"), "o CNPJ alfanumérico com 12 letras e 2 dígitos verificadores não pode sair em claro");
    QVERIFY2(m.contains("\"<chave:14:alfanumerico:1>\""), "o nome sai como forma: tamanho, classe e ordem");
}

void TstRespostaCrua::nomeDesconhecidoViraForma()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"zzz\":1}");
    QVERIFY2(!m.contains("\"zzz\""), "um nome que o cliente não lê não pode sair em claro");
    QVERIFY2(m.contains("\"<chave:3:letras:1>\""), "o nome desconhecido sai como forma <chave:N:classe:k>");
}

void TstRespostaCrua::chavesDeMesmoTamanhoNaoColidem()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"ab\":1,\"cd\":2}");
    QVERIFY2(m.contains("\"<chave:2:letras:1>\""), "a primeira chave de dois caracteres sai com ordem 1");
    QVERIFY2(m.contains("\"<chave:2:letras:2>\""), "a segunda chave de dois caracteres sai com ordem 2, sem colidir com a primeira");
}

void TstRespostaCrua::formaInvalidaComAteDoisDigitosMascarada()
{
    const QByteArray m = nl::mid::composicao::mascararRespostaCrua("{\"9a\":1,\"a-b\":2,\"a.b\":3,\"aç\":4}");
    QVERIFY2(!m.contains("\"9a\""), "um nome que começa com dígito não pode sair em claro");
    QVERIFY2(!m.contains("\"a-b\""), "um nome com hífen não pode sair em claro");
    QVERIFY2(!m.contains("\"a.b\""), "um nome com ponto não pode sair em claro");
    QVERIFY2(!m.contains("a\xc3\xa7"), "um nome com caractere não ASCII não pode sair em claro");
    QVERIFY2(m.contains("\"<chave:2:alfanumerico:1>\""), "9a sai como forma alfanumérica");
    QVERIFY2(m.contains("\"<chave:3:outro:2>\""), "a-b sai como forma de classe outro");
}

void TstRespostaCrua::charsetComDigitosMascarado()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0121");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json; charset=12345678909"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!gravador.ultimoErro(), "a gravação tem de funcionar nesta montagem");
    QFile arquivo(gravador.caminho());
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    const QByteArray conteudo = arquivo.readAll();
    QVERIFY2(!conteudo.contains("12345678909"), "um charset com dígitos não é nome de conjunto de caracteres: não pode sair em claro");
    QVERIFY2(conteudo.contains("\"contentType\":\"<string:"), "o Content-Type fora da lista fechada sai como <string:N>");
}

void TstRespostaCrua::charsetUtf8SemDiferenciarMaiusculas()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0126");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json; charset=UTF-8"), QByteArrayLiteral("{\"a\":1}"));
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json; charset=utf8"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!gravador.ultimoErro(), "a gravação tem de funcionar nesta montagem");
    QFile arquivo(gravador.caminho());
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    const QByteArray conteudo = arquivo.readAll();
    QVERIFY2(conteudo.contains("\"contentType\":\"application/json; charset=UTF-8\""), "utf-8 em maiúsculas sai em claro");
    QVERIFY2(conteudo.contains("\"contentType\":\"application/json; charset=utf8\""), "utf8 sai em claro");
}

void TstRespostaCrua::pastaExistenteComModoLargoVira0700()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString respostas = pasta.path() + QStringLiteral("/respostas");
    QVERIFY(QDir().mkpath(respostas));
    QVERIFY(QFile::setPermissions(respostas, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                                 QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                 QFileDevice::ExeOther));
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0122");
    gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!gravador.ultimoErro(), "a gravação tem de funcionar nesta montagem");
    const QFileDevice::Permissions grupoEOutros = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                                  QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
    QVERIFY2((QFile::permissions(respostas) & grupoEOutros) == 0,
             "a pasta respostas existente com modo largo tem de passar a 0700 na abertura");
}

void TstRespostaCrua::arquivoExistenteComModoLargoVira0600()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::GravadorRespostaCrua primeiro(pasta.path(), "DPS-ID-SINTETICO-0123");
    primeiro.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!primeiro.ultimoErro(), "a primeira gravação tem de funcionar");
    const QString caminho = primeiro.caminho();
    QVERIFY2(QFile::setPermissions(caminho, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup |
                                                QFileDevice::ReadOther),
             "o arquivo de teste tem de ficar com modo largo");
    nl::mid::composicao::GravadorRespostaCrua segundo(pasta.path(), "DPS-ID-SINTETICO-0123");
    segundo.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), QByteArrayLiteral("{\"a\":1}"));
    QVERIFY2(!segundo.ultimoErro(), "a segunda gravação tem de funcionar");
    QVERIFY2(permissaoSoDoDono(caminho), "o arquivo existente com modo largo tem de voltar a 0600 na abertura");
}

void TstRespostaCrua::terceiraExecucaoDepoisDoCorteMantemUmaLinhaDeCorte()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const char* id = "DPS-ID-SINTETICO-0124";
    const QByteArray corpo = corpoGrande();
    constexpr int kPorExecucao = 80;
    nl::mid::composicao::GravadorRespostaCrua primeira(pasta.path(), id);
    for (int i = 0; i < kPorExecucao; ++i) {
        primeira.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    }
    QVERIFY2(primeira.cortado(), "a primeira execução tem de atingir o teto");
    nl::mid::composicao::GravadorRespostaCrua segunda(pasta.path(), id);
    for (int i = 0; i < kPorExecucao; ++i) {
        segunda.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    }
    nl::mid::composicao::GravadorRespostaCrua terceira(pasta.path(), id);
    for (int i = 0; i < kPorExecucao; ++i) {
        terceira.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    }
    QVERIFY2(!terceira.ultimoErro(), "a terceira execução tem de gravar a linha de corte");
    const QList<QByteArray> linhas = linhasDoArquivo(terceira.caminho());
    QVERIFY2(!linhas.isEmpty(), "o arquivo tem de ter linhas");
    const QByteArray marcaCorte = QByteArrayLiteral("{\"corte\":");
    int linhasDeCorte = 0;
    for (const QByteArray& linha : linhas) {
        if (linha.startsWith(marcaCorte)) {
            ++linhasDeCorte;
        }
    }
    QVERIFY2(linhasDeCorte == 1, "depois de três execuções só pode haver uma linha de corte");
    QVERIFY2(linhas.last().startsWith(marcaCorte), "a linha de corte é a última do arquivo");
    const QByteArray marcaOmitidas = QByteArrayLiteral("\"respostas_omitidas\":");
    QVERIFY2(linhas.last().contains(marcaOmitidas), "a linha de corte traz o total de respostas omitidas");
    const QByteArray depois = linhas.last().mid(linhas.last().indexOf(marcaOmitidas) + marcaOmitidas.size());
    bool convertido = false;
    const qint64 omitidas = depois.left(depois.indexOf('}')).toLongLong(&convertido);
    QVERIFY2(convertido, "o total omitido tem de ser um número");
    QVERIFY2((linhas.size() - 1) + omitidas == 3 * kPorExecucao,
             "respostas gravadas mais omitidas das três execuções têm de bater com o total enviado");
}

void TstRespostaCrua::tetoComChavesMascaradasContaAsRespostas()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    constexpr int kEnviadas = 80;
    const QByteArray corpo = corpoGrande();
    nl::mid::composicao::GravadorRespostaCrua gravador(pasta.path(), "DPS-ID-SINTETICO-0125");
    for (int i = 0; i < kEnviadas; ++i) {
        gravador.registrar("POST", "/nfse", 201, QByteArrayLiteral("application/json"), corpo);
    }
    QVERIFY2(gravador.cortado(), "o teto tem de ser atingido com as chaves já mascaradas");
    QFile arquivo(gravador.caminho());
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    const QByteArray conteudo = arquivo.readAll();
    QVERIFY2(conteudo.contains("\"<chave:63:letras:"), "as chaves de 63 letras saem como forma, e a resposta mascarada é a que conta no teto");
    const QList<QByteArray> linhas = linhasDoArquivo(gravador.caminho());
    QVERIFY2(!linhas.isEmpty(), "o arquivo tem de ter linhas");
    const QByteArray marcaOmitidas = QByteArrayLiteral("\"respostas_omitidas\":");
    QVERIFY2(linhas.last().contains(marcaOmitidas), "a linha de corte traz o total de respostas omitidas");
    const QByteArray depois = linhas.last().mid(linhas.last().indexOf(marcaOmitidas) + marcaOmitidas.size());
    bool convertido = false;
    const qint64 omitidas = depois.left(depois.indexOf('}')).toLongLong(&convertido);
    QVERIFY2(convertido, "o total omitido tem de ser um número");
    QVERIFY2((linhas.size() - 1) + omitidas == kEnviadas, "respostas gravadas mais omitidas têm de bater com as enviadas");
}

QTEST_APPLESS_MAIN(TstRespostaCrua)
#include "tst_resposta_crua.moc"
