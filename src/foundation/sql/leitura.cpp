// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do resultado (NL-3.1a, bloco I-G): store, fetch e conversão para Valor. Buffer inicial de 256 bytes por coluna
// (kBufferInicialLeitura). Os buffers são dimensionados e ligados UMA vez, antes de mysql_stmt_bind_result: o conector
// copia o array MYSQL_BIND nesse bind, então trocar um buffer depois não tem efeito. Truncamento (101) não é sucesso nem
// erro: a coluna com erro é relida por mysql_stmt_fetch_column, com um buffer do tamanho real.

#include "nl/foundation/sql/estado.h"
#include "nl/foundation/sql/leitura.h"

#include <mysql.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace nl::foundation::sql::detalhe {
namespace {

constexpr unsigned long kTamanhoInteiro = sizeof(qint64);

bool ehInteiro(enum_field_types tipo)
{
    return tipo == MYSQL_TYPE_TINY || tipo == MYSQL_TYPE_SHORT || tipo == MYSQL_TYPE_LONG
        || tipo == MYSQL_TYPE_INT24 || tipo == MYSQL_TYPE_LONGLONG;
}

bool ehData(enum_field_types tipo)
{
    return tipo == MYSQL_TYPE_DATE || tipo == MYSQL_TYPE_NEWDATE;
}

// Valor de uma coluna. A cópia do inteiro é limitada a min(tamanho, buffer.size()): nunca lê além do buffer.
Valor valorDaColuna(const MYSQL_FIELD& campo, const QByteArray& bytes, bool nulo, const MYSQL_TIME& tempo)
{
    if (nulo) {
        return Nulo{};
    }
    if (ehInteiro(campo.type)) {
        qint64 valor = 0;
        std::memcpy(&valor, bytes.constData(), std::min<std::size_t>(static_cast<std::size_t>(bytes.size()), kTamanhoInteiro));
        return valor;
    }
    if (ehData(campo.type)) {
        return QDate(static_cast<int>(tempo.year), static_cast<int>(tempo.month), static_cast<int>(tempo.day));
    }
    return bytes;
}

} // namespace

std::expected<Linhas, nl::base::Erro> lerLinhas(MYSQL_STMT* stmt)
{
    MYSQL_RES* metadados = mysql_stmt_result_metadata(stmt);
    if (metadados == nullptr) {
        return Linhas{};
    }
    const unsigned int colunas = mysql_num_fields(metadados);
    my_bool atualizaTamanho = 1;
    mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &atualizaTamanho);
    if (mysql_stmt_store_result(stmt) != 0) {
        nl::base::Erro erro = erroDeLeitura(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
        mysql_free_result(metadados);
        return std::unexpected(erro);
    }
    MYSQL_FIELD* campos = mysql_fetch_fields(metadados);

    // Buffers estáveis: dimensionados e ligados antes do bind_result, e não trocados depois (nem redimensionados).
    std::vector<MYSQL_BIND> ligacoes(colunas);
    std::vector<QByteArray> buffers(colunas);
    std::vector<unsigned long> tamanhos(colunas, 0UL);
    std::vector<my_bool> nulos(colunas, 0);
    std::vector<my_bool> erros(colunas, 0);
    std::vector<MYSQL_TIME> tempos(colunas);
    for (unsigned int i = 0; i < colunas; ++i) {
        const bool inteiro = ehInteiro(campos[i].type);
        const bool data = ehData(campos[i].type);
        const unsigned long tamanho = inteiro ? kTamanhoInteiro
            : data ? static_cast<unsigned long>(sizeof(MYSQL_TIME)) : kBufferInicialLeitura;
        buffers[i] = QByteArray(static_cast<qsizetype>(tamanho), '\0');
        MYSQL_BIND& ligacao = ligacoes[i];
        ligacao = MYSQL_BIND{};
        ligacao.buffer_type = inteiro ? MYSQL_TYPE_LONGLONG : data ? MYSQL_TYPE_DATE : campos[i].type;
        ligacao.buffer = data ? static_cast<void*>(&tempos[i]) : static_cast<void*>(buffers[i].data());
        ligacao.buffer_length = tamanho;
        ligacao.length = &tamanhos[i];
        ligacao.is_null = &nulos[i];
        ligacao.error = &erros[i];
    }
    if (mysql_stmt_bind_result(stmt, ligacoes.data()) != 0) {
        nl::base::Erro erro = erroDeLeitura(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
        mysql_free_result(metadados);
        return std::unexpected(erro);
    }

    Linhas linhas;
    while (true) {
        const int rc = mysql_stmt_fetch(stmt);
        if (rc == MYSQL_NO_DATA) {
            break;
        }
        if (rc == 1) {
            nl::base::Erro erro = erroDeLeitura(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
            mysql_free_result(metadados);
            return std::unexpected(erro);
        }
        // Bytes desta linha por coluna: o buffer ligado, limitado ao length; a coluna truncada é relida inteira.
        std::vector<QByteArray> bytes(colunas);
        for (unsigned int i = 0; i < colunas; ++i) {
            if (nulos[i] != 0 || ehInteiro(campos[i].type) || ehData(campos[i].type)) {
                continue;
            }
            const unsigned long lido = std::min(tamanhos[i], static_cast<unsigned long>(buffers[i].size()));
            bytes[i] = buffers[i].left(static_cast<qsizetype>(lido));
            if (erros[i] != 0) {
                QByteArray real(static_cast<qsizetype>(tamanhos[i]), '\0');
                MYSQL_BIND releitura = ligacoes[i];
                unsigned long comprimento = 0;
                my_bool nulo = 0;
                my_bool erro = 0;
                releitura.buffer = real.data();
                releitura.buffer_length = tamanhos[i];
                releitura.length = &comprimento;
                releitura.is_null = &nulo;
                releitura.error = &erro;
                if (mysql_stmt_fetch_column(stmt, &releitura, i, 0) != 0) {
                    nl::base::Erro erroLeitura = erroDeLeitura(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
                    mysql_free_result(metadados);
                    return std::unexpected(erroLeitura);
                }
                bytes[i] = real.left(static_cast<qsizetype>(std::min(comprimento, static_cast<unsigned long>(real.size()))));
            }
        }
        Linha linha;
        linha.reserve(static_cast<qsizetype>(colunas));
        for (unsigned int i = 0; i < colunas; ++i) {
            const QByteArray& valor = (ehInteiro(campos[i].type) || ehData(campos[i].type)) ? buffers[i] : bytes[i];
            linha.append(valorDaColuna(campos[i], valor, nulos[i] != 0, tempos[i]));
        }
        linhas.append(std::move(linha));
    }
    mysql_free_result(metadados);
    return linhas;
}

} // namespace nl::foundation::sql::detalhe
