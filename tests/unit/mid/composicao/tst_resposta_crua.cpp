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
    QVERIFY2(QFile::permissions(*gravado) == (QFileDevice::ReadOwner | QFileDevice::WriteOwner), "o arquivo tem de ser 0600");
    QVERIFY2(!gravado->contains("DPS-ID-SINTETICO"), "o nome do arquivo não pode conter o Id da DPS");
    QFile arquivo(*gravado);
    QVERIFY(arquivo.open(QIODevice::ReadOnly));
    QVERIFY2(!arquivo.readAll().contains(kXmlSintetico), "o XML não pode estar no arquivo gravado");
}

QTEST_APPLESS_MAIN(TstRespostaCrua)
#include "tst_resposta_crua.moc"
