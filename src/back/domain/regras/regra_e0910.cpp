// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0910 (NL-3.L). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0910.h>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/indicador_destinatario.h>

namespace nl::back {

std::string_view RegraE0910::codigo() const noexcept
{
    return "E0910";
}

std::optional<nl::base::Erro> RegraE0910::verificar(const Dps& dps) const
{
    if (dps.dados().indDest.valor() != IndicadorDestinatario::Valor::outroDestinatario) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0910", std::string{"Dps.infDPS.dest"},
                          "indDest 1 (destinatário diferente do tomador) exige o grupo dest; o app ainda não tem o grupo dest, então a DPS "
                          "é recusada aqui (regra E0910, linha Excel 300). Use indDest 0 quando o destinatário é o próprio tomador"};
}

} // namespace nl::back
