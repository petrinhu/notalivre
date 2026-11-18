// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste de integração com o banco (NL-3.1h, V-d): abre a sessão do par, roda SELECT 1 e confere que a sessão está sem TLS
// dos dois lados. Falta de variável ou de senha faz o teste falhar com mensagem; nunca pula.
// Um slot por função: o registro é por slot (integracao.banco.<slot>), e só conexaoBasica existe nesta fatia.
#include <QtTest>

#include "ambiente_banco.h"
#include "conexao_banco.h"
#include "tls_da_sessao.h"

class TesteConexaoBanco : public QObject {
    Q_OBJECT

private slots:
    void conexaoBasica();
};

void TesteConexaoBanco::conexaoBasica()
{
    const auto dados = dadosDoAmbiente();
    QVERIFY2(dados.has_value(), "NL_BANCO_* ausente ou inválido: o teste do banco falha, nunca pula");
    const auto senha = senhaDoArquivo(dados->arquivoSenha);
    QVERIFY2(senha.has_value(), "arquivo de senha do par ausente ou vazio");
    auto conexao = conectar(*dados, *senha);
    QVERIFY2(conexao != nullptr, "o servidor recusou a conexão do par");
    QVERIFY2(mysql_query(conexao.get(), "SELECT 1") == 0, "SELECT 1 falhou na sessão do par");
    MYSQL_RES *resultado = mysql_store_result(conexao.get());
    QVERIFY2(resultado != nullptr, "SELECT 1 sem resultado");
    mysql_free_result(resultado);
    QVERIFY2(!tlsDoCliente(conexao.get()), "TLS do lado do cliente sem pedido");
    QVERIFY2(tlsDoServidor(conexao.get()).isEmpty(), "TLS do lado do servidor sem pedido, ou leitura do status falhou");
}

QTEST_GUILESS_MAIN(TesteConexaoBanco)
#include "tst_conexao_banco.moc"
