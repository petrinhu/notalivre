// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Data e hora de emissão da DPS (dhEmi, DA-1.27): regra pura, sem I/O.

#ifndef NL_BACK_DPS_DH_EMI_H
#define NL_BACK_DPS_DH_EMI_H

#include <expected>

#include <QDateTime>
#include <QString>

#include <nl/base/erro.h>

namespace nl::back {

/// dhEmi (TSDateTimeUTC, tiposSimples_v1.01.xsd:146). Corta os milissegundos sem arredondar, para que o dhEmi nunca passe
/// de dhProc (E0008, linha Excel 153). Recusa o que o pattern do XSD não aceita. Nunca troca o offset da entrada.
/// Invariante: só se constrói por DhEmi::de. O construtor é privado.
class DhEmi
{
public:
    /// Erro: "dhemi.formato" (data inválida, ano fora de 2000 a 2099, ou offset que não é hora cheia entre -11:00 e +12:00).
    static std::expected<DhEmi, nl::base::Erro> de(const QDateTime& entrada);

    /// AAAA-MM-DDThh:mm:ss com o offset da entrada (por exemplo "2026-10-25T10:00:00-03:00"), sem milissegundos.
    QString texto() const;

    /// O mesmo valor como QDateTime, já sem milissegundos e com o offset da entrada.
    const QDateTime& valor() const noexcept { return valor_; }

    bool operator==(const DhEmi& outro) const = default;

private:
    explicit DhEmi(QDateTime valor);

    QDateTime valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_DH_EMI_H
