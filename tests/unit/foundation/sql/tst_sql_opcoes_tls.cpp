// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Opções de TLS do conector lidas de volta, sem servidor (NL-3.1k; parecer do CISO, R3): cada modo é aplicado pelo átomo
// do motor numa sessão nova do conector e conferido por mysql_get_optionv (mariadb_lib.c:4181-4186 e :4269-4271).

#include "nl/foundation/sql/estado.h"

#include <mysql.h>

#include <QByteArray>
#include <QtTest>

#include <memory>

namespace detalhe = nl::foundation::sql::detalhe;

namespace {

// Impressão SHA-256 de teste, em hexadecimal. O átomo não a confere: quem confere é a validação, antes dele.
const QByteArray kImpressao = QByteArrayLiteral("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

struct FechaSessao {
    void operator()(MYSQL* sessao) const noexcept { mysql_close(sessao); }
};
using Sessao = std::unique_ptr<MYSQL, FechaSessao>;

// Valor lógico da opção lido do conector; -1 quando a leitura falha.
int lerOpcaoLogica(MYSQL* sessao, enum mysql_option opcao)
{
    my_bool valor = 2;
    if (mysql_get_optionv(sessao, opcao, &valor) != 0) {
        return -1;
    }
    return static_cast<int>(valor);
}

// Impressão guardada pelo conector; nulo quando não há.
const char* impressaoGuardada(MYSQL* sessao)
{
    const char* guardada = nullptr;
    if (mysql_get_optionv(sessao, MARIADB_OPT_TLS_PEER_FP, &guardada) != 0) {
        return nullptr;
    }
    return guardada;
}

} // namespace

class TstSqlOpcoesTls : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void modoFixadoLigaVerificacaoEFixaImpressao();
    void modoFixadoGuardaCopiaPropriaDaImpressao();
    void modoAbertoDesligaVerificacaoEObrigacaoDeTls();
};

void TstSqlOpcoesTls::modoFixadoLigaVerificacaoEFixaImpressao()
{
    Sessao sessao{mysql_init(nullptr)};
    QVERIFY(sessao != nullptr);
    QVERIFY2(detalhe::aplicarModoFixado(sessao.get(), kImpressao).has_value(), "o modo fixado não aplicou as opções");
    QCOMPARE(lerOpcaoLogica(sessao.get(), MYSQL_OPT_SSL_VERIFY_SERVER_CERT), 1);
    // A impressão liga o TLS no próprio conector (mariadb_lib.c:3996-3999): o fixado nunca abre sem TLS.
    QCOMPARE(lerOpcaoLogica(sessao.get(), MYSQL_OPT_SSL_ENFORCE), 1);
    const char* guardada = impressaoGuardada(sessao.get());
    QVERIFY(guardada != nullptr);
    QCOMPARE(QByteArray(guardada), kImpressao);
}

void TstSqlOpcoesTls::modoFixadoGuardaCopiaPropriaDaImpressao()
{
    Sessao sessao{mysql_init(nullptr)};
    QVERIFY(sessao != nullptr);
    // Cópia profunda: o buffer é de quem chama, e o teste o suja depois de aplicar o modo.
    QByteArray impressaoDoChamador(kImpressao.constData(), kImpressao.size());
    QVERIFY(detalhe::aplicarModoFixado(sessao.get(), impressaoDoChamador).has_value());
    const char* guardada = impressaoGuardada(sessao.get());
    QVERIFY(guardada != nullptr);
    QVERIFY(guardada != impressaoDoChamador.constData());
    impressaoDoChamador.fill('x');
    QCOMPARE(QByteArray(guardada), kImpressao);
}

void TstSqlOpcoesTls::modoAbertoDesligaVerificacaoEObrigacaoDeTls()
{
    Sessao sessao{mysql_init(nullptr)};
    QVERIFY(sessao != nullptr);
    QVERIFY2(detalhe::aplicarModoAberto(sessao.get()).has_value(), "o modo aberto não aplicou as opções");
    QCOMPARE(lerOpcaoLogica(sessao.get(), MYSQL_OPT_SSL_VERIFY_SERVER_CERT), 0);
    QCOMPARE(lerOpcaoLogica(sessao.get(), MYSQL_OPT_SSL_ENFORCE), 0);
}

QTEST_GUILESS_MAIN(TstSqlOpcoesTls)
#include "tst_sql_opcoes_tls.moc"
