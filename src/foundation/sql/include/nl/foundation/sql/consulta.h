// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Consulta e Ddl da Foundation sql (NL-3.1a, CONTRACT 12.1): só texto literal, conferido em tempo de compilação.

#ifndef NL_FOUNDATION_SQL_CONSULTA_H
#define NL_FOUNDATION_SQL_CONSULTA_H

#include <QByteArrayView>

#include <cstddef>

namespace nl::foundation::sql {
namespace detalhe {
void textoSqlVazio();              // declaradas e nunca definidas como constexpr: chamá-las num consteval reprova a compilação,
void textoSqlComPontoEVirgula();   // e o nome aparece no diagnóstico do compilador
void textoSqlComAspas();
void textoSqlComComentario();
template <std::size_t N>
consteval void conferirTextoSql(const char (&texto)[N]) {   // LÊ cada byte: é isso que faz um array local não compilar
    if (N < 2 || texto[N - 1] != '\0') textoSqlVazio();
    for (std::size_t i = 0; i + 1 < N; ++i) {
        const char c = texto[i];
        if (c == ';') textoSqlComPontoEVirgula();
        if (c == '\'' || c == '"') textoSqlComAspas();
        if (c == '#' || (c == '-' && texto[i + 1] == '-') || (c == '/' && texto[i + 1] == '*')) textoSqlComComentario();
    }
}
} // namespace detalhe
class Consulta {
public:
    template <std::size_t N>
    static consteval Consulta de(const char (&texto)[N]) { detalhe::conferirTextoSql(texto); return Consulta(texto, N - 1); }
    [[nodiscard]] QByteArrayView texto() const noexcept { return QByteArrayView(dados_, qsizetype(tamanho_)); }
private:
    consteval Consulta(const char* dados, std::size_t tamanho) : dados_(dados), tamanho_(tamanho) {}
    const char* dados_;
    std::size_t tamanho_;
};
class Ddl {   // mesmo formato e mesma conferência; tipo distinto: executar() não aceita Ddl, executarDdl() não aceita Consulta
public:
    template <std::size_t N>
    static consteval Ddl de(const char (&texto)[N]) { detalhe::conferirTextoSql(texto); return Ddl(texto, N - 1); }
    [[nodiscard]] QByteArrayView texto() const noexcept { return QByteArrayView(dados_, qsizetype(tamanho_)); }
private:
    consteval Ddl(const char* dados, std::size_t tamanho) : dados_(dados), tamanho_(tamanho) {}
    const char* dados_;
    std::size_t tamanho_;
};
} // namespace nl::foundation::sql

#endif // NL_FOUNDATION_SQL_CONSULTA_H
