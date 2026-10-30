// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparador real do infDPS (NL-2.8). Ver comparador_infdps_c14n.h.
// Fase vermelha: nunca compara. Os testes de igual e de diferente reprovam por asserção.
#include <nl/mid/sefin/cliente/comparador_infdps_c14n.h>

namespace nl::mid::sefin::cliente {

ResultadoComparacaoInfDps ComparadorInfDpsC14n::comparar(const nl::back::IdDps&, const QByteArray&,
                                                         const QByteArray&) const {
    return ResultadoComparacaoInfDps::Indisponivel;
}

} // namespace nl::mid::sefin::cliente
