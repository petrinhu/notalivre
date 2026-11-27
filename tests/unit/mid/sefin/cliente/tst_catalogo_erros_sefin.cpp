// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do catálogo de erros da Sefin (NL-3.L): os códigos do Anexo I de 24/11/2026 que a fatia trouxe têm mensagem própria,
// que diz o que corrigir (ponto 5 ratificado pelo líder em 27/11/2026, "Como rejeição ativa (Recomendado)"). Código fora do
// catálogo continua com a mensagem genérica.
#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>

#include <QString>
#include <QtTest>

#include <string_view>

namespace mc = nl::mid::sefin::cliente;

namespace {

QString mensagemDe(std::string_view codigo)
{
    const std::string_view texto = mc::CatalogoErrosSefin::mensagem(codigo);
    return QString::fromUtf8(texto.data(), static_cast<qsizetype>(texto.size()));
}

} // namespace

class TstCatalogoErrosSefin : public QObject
{
    Q_OBJECT

private slots:
    void codigosDoEnderecoTemMensagemPropria_data();
    void codigosDoEnderecoTemMensagemPropria();
    void e1235DizQueOLeiauteMudou();
    void codigoForaDoCatalogoFicaComAGenerica();
};

void TstCatalogoErrosSefin::codigosDoEnderecoTemMensagemPropria_data()
{
    QTest::addColumn<QString>("codigo");
    for (const char* codigo : {"E0238", "E0244", "E0245", "E0653", "E0901", "E0908", "E0910", "E0920", "E0921", "E0922", "E0924",
                               "E1200", "E1209", "E1235"}) {
        QTest::newRow(codigo) << QString::fromLatin1(codigo);
    }
}

void TstCatalogoErrosSefin::codigosDoEnderecoTemMensagemPropria()
{
    QFETCH(QString, codigo);
    const QByteArray latin = codigo.toLatin1();
    const std::string_view chave{latin.constData(), static_cast<std::size_t>(latin.size())};
    QVERIFY2(mc::CatalogoErrosSefin::conhece(chave), qPrintable(codigo));
    QVERIFY2(mensagemDe(chave) != mensagemDe("E9999"), qPrintable(codigo + QStringLiteral(" ficou com a mensagem genérica")));
}

void TstCatalogoErrosSefin::e1235DizQueOLeiauteMudou()
{
    const QString texto = mensagemDe("E1235");
    QVERIFY2(texto.contains(QStringLiteral("leiaute")), qPrintable(texto));
    QVERIFY2(texto.contains(QStringLiteral("atualize o NotaLivre")), qPrintable(texto));
}

void TstCatalogoErrosSefin::codigoForaDoCatalogoFicaComAGenerica()
{
    QVERIFY(!mc::CatalogoErrosSefin::conhece("E9999"));
    QCOMPARE(mensagemDe("E9999"), QStringLiteral("A Sefin recusou o envio sem um código conhecido pelo app."));
}

QTEST_APPLESS_MAIN(TstCatalogoErrosSefin)
#include "tst_catalogo_erros_sefin.moc"
