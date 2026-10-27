// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do dhEmi (DA-1.27): corte de milissegundos, faixa de ano e de offset do pattern do XSD.

#include <nl/back/dps/dh_emi.h>

#include <QDate>
#include <QTime>

#include <string>
#include <utility>

namespace nl::back {
namespace {

constexpr int kAnoMinimo = 2000;
constexpr int kAnoMaximo = 2099;
constexpr int kOffsetMinimo = -11 * 3600;
constexpr int kOffsetMaximo = 12 * 3600;
constexpr int kSegundosPorHora = 3600;
constexpr int kSegundosPorMinuto = 60;

nl::base::Erro erroFormato(const char* motivo)
{
    return nl::base::Erro{"dhemi.formato", std::string{"Dps.infDPS.dhEmi"}, std::string{"dhEmi: "} + motivo};
}

} // namespace

std::expected<DhEmi, nl::base::Erro> DhEmi::de(const QDateTime& entrada)
{
    if (!entrada.isValid()) {
        return std::unexpected(erroFormato("data e hora inválidas"));
    }
    const int ano = entrada.date().year();
    if (ano < kAnoMinimo || ano > kAnoMaximo) {
        return std::unexpected(erroFormato("ano fora de 2000 a 2099"));
    }
    const int offset = entrada.offsetFromUtc();
    if (offset % kSegundosPorHora != 0 || offset < kOffsetMinimo || offset > kOffsetMaximo) {
        return std::unexpected(erroFormato("offset deve ser hora cheia entre -11:00 e +12:00"));
    }
    // Corta os milissegundos sem arredondar: 23:59:59,999 vira 23:59:59 do mesmo dia.
    const QTime hora = entrada.time();
    QDateTime semMilissegundos = entrada;
    semMilissegundos.setTime(QTime{hora.hour(), hora.minute(), hora.second()});
    return DhEmi{std::move(semMilissegundos)};
}

QString DhEmi::texto() const
{
    const int segundos = valor_.offsetFromUtc();
    const int absolutos = segundos < 0 ? -segundos : segundos;
    const QString sinal = segundos < 0 ? QStringLiteral("-") : QStringLiteral("+");
    const QString horas = QString::number(absolutos / kSegundosPorHora).rightJustified(2, QLatin1Char{'0'});
    const QString minutos = QString::number((absolutos % kSegundosPorHora) / kSegundosPorMinuto).rightJustified(2, QLatin1Char{'0'});
    return valor_.toString(QStringLiteral("yyyy-MM-dd'T'HH:mm:ss")) + sinal + horas + QLatin1Char{':'} + minutos;
}

DhEmi::DhEmi(QDateTime valor)
    : valor_{std::move(valor)}
{
}

} // namespace nl::back
