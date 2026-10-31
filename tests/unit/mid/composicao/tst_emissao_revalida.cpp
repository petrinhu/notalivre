// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da revalidação do XML depois de assinar (NL-2.9, DA-2.51 e DA-2.56). O assinador é o port AssinadorDps, injetado pela
// fábrica da fachada: devolve um XML assinado que não confere no leiaute. A emissão tem de sair com xsd.pos_assinatura, antes
// de qualquer envio (a CLI traduz isso para o código 4). Só fixtures sintéticas; sem rede (o endpoint é inalcançável de
// propósito, e a emissão não pode chegar a ele).
#include <nl/back/ports/assinador_dps.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/mid/composicao/emissao_cli.h>
#include <nl/mid/composicao/registro_cli.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtTest>

#include <sstream>

namespace {

namespace comp = nl::mid::composicao;
namespace fp = nl::foundation::pkcs12;

// Assinador falso (DA-2.56): devolve XML que não confere no leiaute, como um defeito do assinador faria.
class AssinadorInvalido : public nl::back::AssinadorDps
{
public:
    std::expected<QByteArray, nl::base::Erro> assinar(QByteArrayView xml, QStringView idReferencia) override
    {
        static_cast<void>(xml);
        static_cast<void>(idReferencia);
        return QByteArray("<DPS xmlns=\"http://www.sped.fazenda.gov.br/nfse\"/>");
    }
};

QByteArray lerArquivo(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

QString caminhoDoCertificado(const char* nome)
{
    return QString::fromUtf8(NL_CERTIFICADOS_EMISSAO) + QStringLiteral("/") + QString::fromUtf8(nome);
}

} // namespace

class TstEmissaoRevalida : public QObject
{
    Q_OBJECT

private slots:
    // XML assinado que não confere no XSD: a emissão sai com xsd.pos_assinatura e nenhuma chamada de envio chega a ser feita.
    void xmlInvalidoAposAssinarSaiComXsdPosAssinatura();
};

void TstEmissaoRevalida::xmlInvalidoAposAssinarSaiComXsdPosAssinatura()
{
    const QByteArray json = lerArquivo(QString::fromUtf8(NL_ENTRADA_MEI));
    QVERIFY2(!json.isEmpty(), "a entrada de teste tem de existir");
    QByteArray senhaBruta = lerArquivo(caminhoDoCertificado("senha_teste.txt")).trimmed();
    QVERIFY2(!senhaBruta.isEmpty(), "a senha da fixture sintética tem de existir");
    auto senha = fp::SenhaSegura::de(senhaBruta);

    comp::PedidoEmissao pedido{
        json,
        comp::LocalizacaoEsquemaDps{QString::fromUtf8(NL_XSD_PRODREST_DIR), QString::fromUtf8(NL_XSD_PRODREST_MANIFESTO)},
        caminhoDoCertificado("cnpj_ok.pfx"),
        QStringLiteral("https://127.0.0.1:9"),  // inalcançável: a emissão não pode chegar ao envio
        caminhoDoCertificado("ac.pem"),
        QString(),
    };
    std::ostringstream destino;
    auto registro = comp::criarLogDaCli(destino);
    const comp::FabricaAssinador fabrica = [](const fp::CredencialX509&) {
        return std::unique_ptr<nl::back::AssinadorDps>(new AssinadorInvalido());
    };

    // DA-2.59 item 1: a emissão consome a senha por movimento; o chamador vê o objeto movido (vazio).
    const auto emitido = comp::emitirDpsComAssinador(pedido, std::move(senha), *registro, fabrica);
    QVERIFY2(!emitido.has_value(), "XML assinado inválido no XSD tem de ser recusado");
    QCOMPARE(QString::fromStdString(emitido.error().codigo), QStringLiteral("xsd.pos_assinatura"));
    QCOMPARE(senha.tamanho(), std::size_t{0});
}

QTEST_APPLESS_MAIN(TstEmissaoRevalida)
#include "tst_emissao_revalida.moc"
