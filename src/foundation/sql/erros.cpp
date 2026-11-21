// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tradução de códigos do conector para nl::base::Erro (NL-3.1a, bloco I-G). A mensagem vem do servidor e nunca leva a
// senha. A recusa de TLS (2026) tem código próprio; a conexão perdida (2013 e 2006) também.

#include "nl/foundation/sql/estado.h"

#include <optional>
#include <string>

namespace nl::foundation::sql::detalhe {
namespace {

bool conexaoPerdida(unsigned erro) { return erro == 2013 || erro == 2006; }

nl::base::Erro comCodigo(const char* codigo, const char* mensagem)
{
    return nl::base::Erro(codigo, std::nullopt, mensagem != nullptr ? std::string(mensagem) : std::string());
}

} // namespace

nl::base::Erro erroDeConexao(unsigned erro, const char* mensagem)
{
    if (erro == 2026) {
        return comCodigo("sql.tls_recusado", mensagem);
    }
    if (conexaoPerdida(erro)) {
        return comCodigo("sql.conexao_perdida", mensagem);
    }
    return comCodigo("sql.conexao_recusada", mensagem);
}

nl::base::Erro erroDePreparo(unsigned erro, const char* mensagem)
{
    return comCodigo(conexaoPerdida(erro) ? "sql.conexao_perdida" : "sql.preparo", mensagem);
}

nl::base::Erro erroDeExecucao(unsigned erro, const char* mensagem)
{
    return comCodigo(conexaoPerdida(erro) ? "sql.conexao_perdida" : "sql.execucao", mensagem);
}

nl::base::Erro erroDeLeitura(unsigned erro, const char* mensagem)
{
    return comCodigo(conexaoPerdida(erro) ? "sql.conexao_perdida" : "sql.leitura", mensagem);
}

nl::base::Erro erroDeOpcao(const char* mensagem)
{
    return comCodigo("sql.opcao_recusada", mensagem);
}

} // namespace nl::foundation::sql::detalhe
