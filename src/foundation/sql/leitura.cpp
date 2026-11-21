// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do resultado (NL-3.1a, bloco I-G): store, fetch e conversão para Valor. Buffer inicial de 256 bytes por coluna
// (kBufferInicialLeitura). Truncamento (101) não é sucesso nem erro: a coluna com erro é relida com o tamanho real.

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

// Valor de uma coluna da linha atual, a partir do buffer e do length (sem NUL: o tamanho é o do length).
Valor valorDaColuna(const MYSQL_FIELD& campo, const QByteArray& buffer, unsigned long tamanho, bool nulo,
                    const MYSQL_TIME& tempo)
{
    if (nulo) {
        return Nulo{};
    }
    if (ehInteiro(campo.type)) {
        qint64 valor = 0;
        std::memcpy(&valor, buffer.constData(), std::min<unsigned long>(tamanho, kTamanhoInteiro));
        return valor;
    }
    if (ehData(campo.type)) {
        return QDate(static_cast<int>(tempo.year), static_cast<int>(tempo.month), static_cast<int>(tempo.day));
    }
    return QByteArray(buffer.constData(), static_cast<qsizetype>(tamanho));
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

    std::vector<MYSQL_BIND> ligacoes(colunas);
    std::vector<QByteArray> buffers(colunas);
    std::vector<unsigned long> tamanhos(colunas, 0UL);
    std::vector<my_bool> nulos(colunas, 0);
    std::vector<my_bool> erros(colunas, 0);
    std::vector<MYSQL_TIME> tempos(colunas);
    const unsigned long tamanhoInicial = kBufferInicialLeitura;
    for (unsigned int i = 0; i < colunas; ++i) {
        MYSQL_BIND& ligacao = ligacoes[i];
        if (ehInteiro(campos[i].type)) {
            ligacao.buffer_type = MYSQL_TYPE_LONGLONG;
        } else if (ehData(campos[i].type)) {
            ligacao.buffer_type = MYSQL_TYPE_DATE;
        } else {
            ligacao.buffer_type = campos[i].type;
        }
        ligacao.length = &tamanhos[i];
        ligacao.is_null = &nulos[i];
        ligacao.error = &erros[i];
    }

    Linhas linhas;
    if (mysql_stmt_bind_result(stmt, ligacoes.data()) != 0) {
        nl::base::Erro erro = erroDeLeitura(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
        mysql_free_result(metadados);
        return std::unexpected(erro);
    }
    while (true) {
        // Cada linha começa com o buffer inicial; a releitura de uma coluna truncada vale só para esta linha.
        for (unsigned int i = 0; i < colunas; ++i) {
            const bool data = ehData(campos[i].type);
            const bool inteiro = ehInteiro(campos[i].type);
            const unsigned long tamanhoDaColuna = inteiro ? kTamanhoInteiro
                : data ? static_cast<unsigned long>(sizeof(MYSQL_TIME)) : tamanhoInicial;
            buffers[i] = QByteArray(static_cast<qsizetype>(tamanhoDaColuna), '\0');
            ligacoes[i].buffer = buffers[i].data();
            ligacoes[i].buffer_length = tamanhoDaColuna;
            if (data) {
                tempos[i] = MYSQL_TIME{};
                ligacoes[i].buffer = &tempos[i];
            }
            erros[i] = 0;
        }
        const int rc = mysql_stmt_fetch(stmt);
        if (rc == MYSQL_NO_DATA) {
            break;
        }
        if (rc == 1) {
            nl::base::Erro erro = erroDeLeitura(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
            mysql_free_result(metadados);
            return std::unexpected(erro);
        }
        if (rc == MYSQL_DATA_TRUNCATED) {
            for (unsigned int i = 0; i < colunas; ++i) {
                if (erros[i] == 0 || ehInteiro(campos[i].type) || ehData(campos[i].type)) {
                    continue;
                }
                buffers[i] = QByteArray(static_cast<qsizetype>(tamanhos[i]), '\0');
                ligacoes[i].buffer = buffers[i].data();
                ligacoes[i].buffer_length = tamanhos[i];
                if (mysql_stmt_fetch_column(stmt, &ligacoes[i], i, 0) != 0) {
                    nl::base::Erro erro = erroDeLeitura(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
                    mysql_free_result(metadados);
                    return std::unexpected(erro);
                }
            }
        }
        Linha linha;
        linha.reserve(static_cast<qsizetype>(colunas));
        for (unsigned int i = 0; i < colunas; ++i) {
            linha.append(valorDaColuna(campos[i], buffers[i], tamanhos[i], nulos[i] != 0, tempos[i]));
        }
        linhas.append(std::move(linha));
    }
    mysql_free_result(metadados);
    return linhas;
}

} // namespace nl::foundation::sql::detalhe
