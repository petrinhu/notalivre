// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Parametro da Foundation sql (NL-3.1a, CONTRACT 12.1): valor vinculado, nunca texto concatenado.

#ifndef NL_FOUNDATION_SQL_PARAMETRO_H
#define NL_FOUNDATION_SQL_PARAMETRO_H

#include <QByteArray>
#include <QByteArrayView>
#include <QDate>
#include <QList>
#include <QString>
#include <QtGlobal>

#include <concepts>
#include <type_traits>
#include <variant>

namespace nl::foundation::sql {
struct Nulo {};
class Parametro {   // só pelas fábricas; o valor é copiado para dentro do Parametro
public:
    static Parametro nulo();
    static Parametro inteiro(qint64 valor);
    static Parametro texto(QByteArrayView utf8);
    template <typename T>
        requires std::same_as<std::remove_cvref_t<T>, QString>
    static Parametro texto(T&&) = delete;   // texto de usuário entra já em UTF-8; template restrito, para não criar ambiguidade com const char* e QByteArray
    static Parametro bytes(QByteArrayView dados);
    static Parametro data(QDate dia);                  // dia civil, sem fuso
    using Valor = std::variant<Nulo, qint64, QByteArray, QDate>;
    [[nodiscard]] const Valor& valor() const noexcept;
    [[nodiscard]] bool ehBytes() const noexcept;        // separa texto de blob no vínculo
private:
    Parametro(Valor valor, bool bytes);
    Valor valor_;
    bool bytes_;
};
using Valor = Parametro::Valor;
using Linha = QList<Valor>;
using Linhas = QList<Linha>;
} // namespace nl::foundation::sql

#endif // NL_FOUNDATION_SQL_PARAMETRO_H
