// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do canal de segredo por descritor (NL-2.27, passo 2; DA-2.16). Senhas de teste sintéticas: nenhum dado real, nenhum .pfx.
#include <nl/mid/cofre/descritor_de_segredo.h>

#include <QByteArray>
#include <QFile>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <array>
#include <cerrno>
#include <fcntl.h>
#include <pty.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

// Pipe com o conteúdo já escrito e o lado de escrita fechado: o descritor de leitura devolve esses bytes e depois EOF.
int pipeCom(const QByteArray& conteudo)
{
    int par[2] = {-1, -1};
    if (pipe(par) != 0) {
        return -1;
    }
    if (!conteudo.isEmpty()) {
        if (write(par[1], conteudo.constData(), static_cast<size_t>(conteudo.size())) != conteudo.size()) {
            close(par[0]);
            close(par[1]);
            return -1;
        }
    }
    close(par[1]);
    return par[0];
}

// Arquivo comum com o modo pedido e o conteúdo dado; devolve um descritor de leitura aberto, ou -1.
int arquivoComModo(const QString& caminho, QFileDevice::Permissions modo, const QByteArray& conteudo)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::WriteOnly)) {
        return -1;
    }
    if (arquivo.write(conteudo) != conteudo.size()) {
        return -1;
    }
    arquivo.close();
    if (!QFile::setPermissions(caminho, modo)) {
        return -1;
    }
    return ::open(QFile::encodeName(caminho).constData(), O_RDONLY);
}

// Conteúdo da senha lida, como bytes. Só para comparar nos testes; nunca vai a log.
QByteArray conteudoDe(const nl::foundation::pkcs12::SenhaSegura& senha)
{
    return QByteArray(senha.comoC(), static_cast<qsizetype>(senha.tamanho()));
}

} // namespace

class TstDescritorDeSegredo : public QObject
{
    Q_OBJECT

private slots:
    // A senha vem do descritor, uma vez, e o descritor é fechado depois da leitura (EBADF em F_GETFD).
    void senhaPeloDescritorLidaUmaVezEFechada();
    // Só uma quebra final sai: "abc\n" vira "abc"; "abc\n\n" vira "abc\n"; "abc" fica "abc".
    void quebraFinalUnicaDescartada();
    // Sem bytes úteis (EOF imediato, ou só a quebra), a senha é vazia e a leitura recusa.
    void descritorVazioRecusado();
    // Acima de 512 bytes a leitura recusa; 512 bytes passam.
    void descritorAcimaDoTetoRecusado();
    // Os descritores 0, 1 e 2 são recusados antes de qualquer leitura.
    void descritores0a2Recusados();
    // Um terminal é recusado antes de qualquer leitura: a senha tem de vir de um canal sem eco.
    void descritorTerminalRecusado();
    // Arquivo comum com permissão para grupo ou outros é recusado; 0600 passa.
    void arquivoLegivelPorGrupoRecusado();
    // NL-2.27 (I4, revisão de segurança): a recusa também fecha o descritor (DA-2.16, "zerado e fechado").
    void recusaFechaODescritor();
    // Descritor negativo ou que não está aberto é recusado como inválido (guarda de borda).
    void descritorFechadoOuInvalidoRecusado();
    // NL-2.27, Emenda 1 (item 4): 512 bytes de senha mais a quebra final passam; a quebra não conta no teto, como no terminal.
    void noTetoComQuebraFinalPassa();
    // NL-2.27 (aceite de mutação): o leitor zera o buffer do chamador em sucesso e em recusa.
    void zeraOBufferDoChamador();
    // NL-2.27 (DA-2.68): a conferência do descritor não lê nenhum byte e não fecha o descritor.
    void conferirSemLerNemFechar();
};

void TstDescritorDeSegredo::senhaPeloDescritorLidaUmaVezEFechada()
{
    const int fd = pipeCom(QByteArrayLiteral("senha-de-teste\n"));
    QVERIFY(fd >= 0);
    const auto senha = nl::mid::cofre::lerSenhaDoDescritor(fd);
    QVERIFY2(senha.has_value(), "a senha tem de ser lida do descritor");
    QCOMPARE(conteudoDe(*senha), QByteArrayLiteral("senha-de-teste"));
    errno = 0;
    QCOMPARE(fcntl(fd, F_GETFD), -1);
    QCOMPARE(errno, EBADF);
}

void TstDescritorDeSegredo::quebraFinalUnicaDescartada()
{
    const auto um = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc\n")));
    QVERIFY2(um.has_value(), "abc com quebra final tem de ser lido");
    QCOMPARE(conteudoDe(*um), QByteArrayLiteral("abc"));
    const auto dois = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc\n\n")));
    QVERIFY2(dois.has_value(), "abc com duas quebras tem de ser lido");
    QCOMPARE(conteudoDe(*dois), QByteArrayLiteral("abc\n"));
    const auto nenhuma = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc")));
    QVERIFY2(nenhuma.has_value(), "abc sem quebra tem de ser lido");
    QCOMPARE(conteudoDe(*nenhuma), QByteArrayLiteral("abc"));
    // NL-2.27, Emenda 1 (item 4) e revisão de segurança (B2): espaço, tabulação e CR finais são mantidos; só a quebra de linha sai.
    const auto espaco = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc \n")));
    QVERIFY2(espaco.has_value(), "abc com espaço final tem de ser lido");
    QCOMPARE(conteudoDe(*espaco), QByteArrayLiteral("abc "));
    const auto cr = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc\r\n")));
    QVERIFY2(cr.has_value(), "abc com CRLF tem de ser lido");
    QCOMPARE(conteudoDe(*cr), QByteArrayLiteral("abc\r"));
    // NL-2.27 (revisão, R2-B1): espaço, tab e CR finais sem quebra de linha também são mantidos. Sem estes casos, um leitor que apara
    // espaço ou CR (X2 e X2b) passava por verde.
    const auto sozinho = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc ")));
    QVERIFY2(sozinho.has_value(), "abc com espaço final, sem quebra, tem de ser lido");
    QCOMPARE(conteudoDe(*sozinho), QByteArrayLiteral("abc "));
    const auto retorno = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc\r")));
    QVERIFY2(retorno.has_value(), "abc com CR final, sem quebra, tem de ser lido");
    QCOMPARE(conteudoDe(*retorno), QByteArrayLiteral("abc\r"));
    const auto tabulado = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc\t")));
    QVERIFY2(tabulado.has_value(), "abc com tabulação final, sem quebra, tem de ser lido");
    QCOMPARE(conteudoDe(*tabulado), QByteArrayLiteral("abc\t"));
    const auto tabComQuebra = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("abc\t\n")));
    QVERIFY2(tabComQuebra.has_value(), "abc com tabulação e quebra tem de ser lido");
    QCOMPARE(conteudoDe(*tabComQuebra), QByteArrayLiteral("abc\t"));
}

void TstDescritorDeSegredo::descritorVazioRecusado()
{
    const auto eof = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArray()));
    QVERIFY(!eof.has_value());
    QCOMPARE(eof.error(), nl::mid::cofre::ErroDoSegredo::SenhaVazia);
    const auto soQuebra = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArrayLiteral("\n")));
    QVERIFY(!soQuebra.has_value());
    QCOMPARE(soQuebra.error(), nl::mid::cofre::ErroDoSegredo::SenhaVazia);
}

void TstDescritorDeSegredo::descritorAcimaDoTetoRecusado()
{
    const auto grande = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArray(513, 'x')));
    QVERIFY(!grande.has_value());
    QCOMPARE(grande.error(), nl::mid::cofre::ErroDoSegredo::AcimaDoTeto);
    const auto noTeto = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArray(512, 'y')));
    QVERIFY2(noTeto.has_value(), "512 bytes tem de passar: é o teto");
    QCOMPARE(noTeto->tamanho(), std::size_t{512});
}

void TstDescritorDeSegredo::descritores0a2Recusados()
{
    // NL-2.27 (rodada 4, I4b): a conferência roda num filho. Uma mutação que feche 0, 1 ou 2 não pode apagar a saída do QTest do pai
    // (fechar o fd 1 apagava a saída e a falha virava "Required regular expression not found"). O filho não escreve nada e sai com
    // uma máscara: bit k = o fd k foi fechado pela recusa; bit 3 (8) = a recusa de 0, 1 ou 2 devolveu outro erro que não DescritorInvalido.
    const pid_t filho = fork();
    QVERIFY2(filho >= 0, "fork tem de funcionar para conferir os descritores 0, 1 e 2");
    if (filho == 0) {
        int mascara = 0;
        for (const int fd : {0, 1, 2}) {
            const auto r = nl::mid::cofre::lerSenhaDoDescritor(fd);
            if (r.has_value() || r.error() != nl::mid::cofre::ErroDoSegredo::DescritorInvalido) {
                mascara |= 8;
            }
        }
        for (const int fd : {0, 1, 2}) {
            if (fcntl(fd, F_GETFD) == -1) {
                mascara |= (1 << fd);
            }
        }
        _exit(mascara);
    }
    int estado = 0;
    QVERIFY2(waitpid(filho, &estado, 0) == filho, "o pai tem de esperar o filho da conferência");
    QVERIFY2(WIFEXITED(estado), "o filho da conferência tem de sair normalmente");
    const int mascara = WEXITSTATUS(estado);
    QStringList problemas;
    for (int fd = 0; fd <= 2; ++fd) {
        if (mascara & (1 << fd)) {
            problemas << QStringLiteral("o fd %1 foi fechado pela recusa").arg(fd);
        }
    }
    if (mascara & 8) {
        problemas << QStringLiteral("a recusa de 0, 1 ou 2 devolveu outro erro que não DescritorInvalido");
    }
    QVERIFY2(mascara == 0, qPrintable(QStringLiteral("máscara %1: %2").arg(mascara).arg(problemas.join(QStringLiteral("; ")))));
}

void TstDescritorDeSegredo::descritorTerminalRecusado()
{
    int mestre = -1;
    int escravo = -1;
    QVERIFY(openpty(&mestre, &escravo, nullptr, nullptr, nullptr) == 0);
    const auto r = nl::mid::cofre::lerSenhaDoDescritor(escravo);
    QVERIFY2(!r.has_value(), "um terminal não pode ser canal de senha");
    QCOMPARE(r.error(), nl::mid::cofre::ErroDoSegredo::DescritorTerminal);
    // NL-2.27 (revisão, R2-B3, I4c): a recusa de terminal fecha o escravo pelo leitor (DA-2.16). Arquivo legível fecha o mesmo caminho.
    errno = 0;
    QCOMPARE(fcntl(escravo, F_GETFD), -1);
    QCOMPARE(errno, EBADF);
    // O escravo é fechado pelo próprio leitor na recusa (I4): fechá-lo aqui de novo poderia fechar um descritor reaproveitado.
    close(mestre);
}

void TstDescritorDeSegredo::arquivoLegivelPorGrupoRecusado()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString caminho = pasta.path() + QStringLiteral("/segredo");
    const QByteArray conteudo = QByteArrayLiteral("senha-de-teste\n");
    const int legivelPorGrupo = arquivoComModo(caminho, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup, conteudo);
    QVERIFY(legivelPorGrupo >= 0);
    const auto grupo = nl::mid::cofre::lerSenhaDoDescritor(legivelPorGrupo);
    QVERIFY2(!grupo.has_value(), "arquivo legível por grupo não pode ser canal de senha");
    QCOMPARE(grupo.error(), nl::mid::cofre::ErroDoSegredo::LegivelPorOutros);
    const int legivelPorOutros = arquivoComModo(caminho, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadOther, conteudo);
    QVERIFY(legivelPorOutros >= 0);
    const auto outros = nl::mid::cofre::lerSenhaDoDescritor(legivelPorOutros);
    QVERIFY2(!outros.has_value(), "arquivo legível por outros não pode ser canal de senha");
    QCOMPARE(outros.error(), nl::mid::cofre::ErroDoSegredo::LegivelPorOutros);
    const int soDono = arquivoComModo(caminho, QFileDevice::ReadOwner | QFileDevice::WriteOwner, conteudo);
    QVERIFY(soDono >= 0);
    const auto dono = nl::mid::cofre::lerSenhaDoDescritor(soDono);
    QVERIFY2(dono.has_value(), "arquivo 0600 tem de ser aceito");
    QCOMPARE(conteudoDe(*dono), QByteArrayLiteral("senha-de-teste"));
}

void TstDescritorDeSegredo::descritorFechadoOuInvalidoRecusado()
{
    const auto negativo = nl::mid::cofre::lerSenhaDoDescritor(-1);
    QVERIFY(!negativo.has_value());
    QCOMPARE(negativo.error(), nl::mid::cofre::ErroDoSegredo::DescritorInvalido);
    const auto fechado = nl::mid::cofre::lerSenhaDoDescritor(999);
    QVERIFY(!fechado.has_value());
    QCOMPARE(fechado.error(), nl::mid::cofre::ErroDoSegredo::DescritorInvalido);
}

void TstDescritorDeSegredo::noTetoComQuebraFinalPassa()
{
    const auto noTeto = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArray(512, 'y') + QByteArrayLiteral("\n")));
    QVERIFY2(noTeto.has_value(), "512 bytes mais a quebra final têm de passar: a quebra não conta no teto, como no terminal");
    QCOMPARE(noTeto->tamanho(), std::size_t{512});
    const auto acima = nl::mid::cofre::lerSenhaDoDescritor(pipeCom(QByteArray(513, 'x') + QByteArrayLiteral("\n")));
    QVERIFY(!acima.has_value());
    QCOMPARE(acima.error(), nl::mid::cofre::ErroDoSegredo::AcimaDoTeto);
}

void TstDescritorDeSegredo::zeraOBufferDoChamador()
{
    std::array<char, nl::mid::cofre::kTamanhoBufferDoSegredo> buffer{};
    buffer.fill('x');
    const auto senha = nl::mid::cofre::lerSenhaDoDescritorEm(pipeCom(QByteArrayLiteral("senha-de-teste\n")), buffer);
    QVERIFY2(senha.has_value(), "a senha tem de ser lida do descritor");
    QCOMPARE(conteudoDe(*senha), QByteArrayLiteral("senha-de-teste"));
    QVERIFY2(std::all_of(buffer.begin(), buffer.end(), [](char c) { return c == '\0'; }), "o buffer do chamador tem de sair zerado");
    buffer.fill('x');
    const auto grande = nl::mid::cofre::lerSenhaDoDescritorEm(pipeCom(QByteArray(600, 'z')), buffer);
    QVERIFY(!grande.has_value());
    QVERIFY2(std::all_of(buffer.begin(), buffer.end(), [](char c) { return c == '\0'; }), "a recusa também tem de zerar o buffer");
}

void TstDescritorDeSegredo::recusaFechaODescritor()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString caminho = pasta.path() + QStringLiteral("/segredo-recusado");
    const int legivelPorGrupo = arquivoComModo(caminho, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup,
                                               QByteArrayLiteral("senha-de-teste\n"));
    QVERIFY(legivelPorGrupo >= 0);
    const auto recusado = nl::mid::cofre::lerSenhaDoDescritor(legivelPorGrupo);
    QVERIFY2(!recusado.has_value(), "arquivo legível por grupo tem de ser recusado");
    QCOMPARE(recusado.error(), nl::mid::cofre::ErroDoSegredo::LegivelPorOutros);
    errno = 0;
    QCOMPARE(fcntl(legivelPorGrupo, F_GETFD), -1);
    QCOMPARE(errno, EBADF);
}

void TstDescritorDeSegredo::conferirSemLerNemFechar()
{
    const int fd = pipeCom(QByteArrayLiteral("abc\n"));
    QVERIFY(fd >= 0);
    const auto conferido = nl::mid::cofre::conferirDescritorDeSenha(fd);
    QVERIFY2(conferido.has_value(), "um pipe comum tem de passar na conferência");
    errno = 0;
    QVERIFY2(fcntl(fd, F_GETFD) != -1, "a conferência não pode fechar o descritor");
    const auto senha = nl::mid::cofre::lerSenhaDoDescritor(fd);
    QVERIFY2(senha.has_value(), "a conferência não pode consumir os bytes");
    QCOMPARE(conteudoDe(*senha), QByteArrayLiteral("abc"));
}

QTEST_APPLESS_MAIN(TstDescritorDeSegredo)
#include "tst_descritor_de_segredo.moc"
