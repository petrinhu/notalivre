// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do átomo de posse do descritor da senha (NL-2.27b, DA-2.72; revisão de segurança da rodada 2: MS, SLC e MI). Só
// pipes e arquivos sintéticos: nenhum terminal, nenhum .pfx e nenhuma senha real.
#include <nl/mid/cofre/tomada_descritor.h>

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace {

// Pipe cujo escritor já está fechado: o lado de leitura é o descritor pedido. Devolve -1 se o pipe não pôde ser criado.
int pipeSemEscritor()
{
    int par[2] = {-1, -1};
    if (pipe(par) != 0) {
        return -1;
    }
    close(par[1]);
    return par[0];
}

} // namespace

class TstTomadaDescritor : public QObject
{
    Q_OBJECT

private slots:
    // MS: a posse sai com FD_CLOEXEC, para um exec posterior não herdá-la (DA-2.72, "fechamento no exec").
    void tomadaTemFechamentoNoExec();
    // SLC: o número pedido fica fechado depois da tomada ("movendo-o", DA-2.72). Sem isso, o número original vazaria.
    void numeroPedidoFechadoDepoisDaTomada();
    // MI: se o número alto passa a ser outro arquivo antes da leitura, a conferência recusa com cofre.descritor_trocado.
    void trocaDeArquivoNoNumeroAltoERecusada();
    // Guarda de borda: um número fechado na entrada não é tomado (cofre.descritor_invalido), e nada é fechado.
    void fechadoNaEntradaRecusaSemTomar();
};

void TstTomadaDescritor::tomadaTemFechamentoNoExec()
{
    const int pedido = pipeSemEscritor();
    QVERIFY(pedido >= 0);
    const auto tomada = nl::mid::cofre::tomarDescritorDeSenha(pedido);
    QVERIFY2(tomada.has_value(), "um pipe comum tem de ser tomado");
    const int flags = fcntl(tomada->fd, F_GETFD);
    QVERIFY(flags != -1);
    QVERIFY2((flags & FD_CLOEXEC) != 0, "a posse tem de sair com FD_CLOEXEC: um exec não pode herdá-la");
    close(tomada->fd);
}

void TstTomadaDescritor::numeroPedidoFechadoDepoisDaTomada()
{
    const int pedido = pipeSemEscritor();
    QVERIFY(pedido >= 0);
    const auto tomada = nl::mid::cofre::tomarDescritorDeSenha(pedido);
    QVERIFY2(tomada.has_value(), "um pipe comum tem de ser tomado");
    errno = 0;
    QCOMPARE(fcntl(pedido, F_GETFD), -1);
    QCOMPARE(errno, EBADF);
    close(tomada->fd);
}

void TstTomadaDescritor::trocaDeArquivoNoNumeroAltoERecusada()
{
    const int pedido = pipeSemEscritor();
    QVERIFY(pedido >= 0);
    const auto tomada = nl::mid::cofre::tomarDescritorDeSenha(pedido);
    QVERIFY2(tomada.has_value(), "um pipe comum tem de ser tomado");
    QVERIFY2(nl::mid::cofre::conferirTomadaDeDescritor(*tomada).has_value(), "sem troca, a conferência tem de passar");

    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString caminho = pasta.filePath(QStringLiteral("outro-sintetico.txt"));
    QFile outro(caminho);
    QVERIFY(outro.open(QIODevice::WriteOnly));
    outro.write("conteudo sintetico\n");
    outro.close();
    const int abertoDepois = ::open(QFile::encodeName(caminho).constData(), O_RDONLY);
    QVERIFY(abertoDepois >= 0);
    // Troca o objeto no número alto: o outro arquivo passa a ser o descritor tomado.
    QVERIFY2(dup2(abertoDepois, tomada->fd) == tomada->fd, "a troca no número alto tem de funcionar para o teste");
    close(abertoDepois);

    const auto conferida = nl::mid::cofre::conferirTomadaDeDescritor(*tomada);
    QVERIFY2(!conferida.has_value(), "um descritor trocado depois da tomada tem de ser recusado antes da leitura");
    QCOMPARE(conferida.error().motivo, nl::mid::cofre::ErroDaTomada::DescritorTrocado);
    close(tomada->fd);
}

void TstTomadaDescritor::fechadoNaEntradaRecusaSemTomar()
{
    const auto tomada = nl::mid::cofre::tomarDescritorDeSenha(999);
    QVERIFY(!tomada.has_value());
    QCOMPARE(tomada.error().motivo, nl::mid::cofre::ErroDaTomada::DescritorInvalido);
}

QTEST_APPLESS_MAIN(TstTomadaDescritor)
#include "tst_tomada_descritor.moc"
