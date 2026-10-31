// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da resposta crua para análise local (NL-2.9, DA-2.59 item 4): a mascaragem troca o XML da NFS-e e a chave, e a
// gravação sai em 0600 com nome que não contém o Id. Fixtures sintéticas, sem dado real.
#include <nl/mid/composicao/resposta_crua.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileDevice>
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
    const auto gravado = nl::mid::composicao::gravarRespostaCrua(corpo, pasta.path(), "DPS-ID-SINTETICO-0001");
    QVERIFY2(gravado.has_value(), "a resposta tem de ser gravada na pasta de análise");
    QVERIFY2(permissaoSoDoDono(*gravado), "o arquivo tem de ser 0600: dono lê e escreve, grupo e outros sem bit");
    QVERIFY2(!gravado->contains("DPS-ID-SINTETICO"), "o nome do arquivo não pode conter o Id da DPS");
    QFile arquivo(*gravado);
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
    const auto gravado = nl::mid::composicao::gravarRespostaCrua(corpo, pasta.path(), "DPS-ID-SINTETICO-0002");
    QVERIFY2(gravado.has_value(), "a resposta tem de ser gravada na pasta de análise");
    QFile arquivo(*gravado);
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    const QByteArray conteudo = arquivo.readAll();
    const qsizetype ocorrencias = conteudo.count("12345678909");
    QVERIFY2(ocorrencias == 0, qPrintable(QStringLiteral("CPF da fixture no arquivo gravado: %1 ocorrência(s) em %2 bytes varridos")
                                              .arg(ocorrencias)
                                              .arg(conteudo.size())));
}

QTEST_APPLESS_MAIN(TstRespostaCrua)
#include "tst_resposta_crua.moc"
