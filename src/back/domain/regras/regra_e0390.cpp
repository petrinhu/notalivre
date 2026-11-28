// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0390 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0390.h>
#include <QString>

#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/servico.h>

namespace nl::back {

std::string_view RegraE0390::codigo() const noexcept
{
    return "E0390";
}

namespace {

/// Item 12 (cTribNac 12xxxx) ou subitem 17.10 (cTribNac 1710xx): os dois exigem atvEvento no Anexo I de 24/11/2026.
bool exigeAtividadeEvento(const QString& cTribNac)
{
    return cTribNac.startsWith(QStringLiteral("12")) || cTribNac.startsWith(QStringLiteral("1710"));
}

} // namespace

std::optional<nl::base::Erro> RegraE0390::verificar(const Dps& dps) const
{
    if (!exigeAtividadeEvento(dps.dados().serv.cTribNac().texto())) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0390", std::string{"Dps.infDPS.serv.atvEvento"},
                          "cTribNac do item 12 ou do subitem 17.10 exige o grupo atvEvento; o app ainda não tem o grupo atvEvento, então a DPS é recusada aqui (regra E0390, linha Excel 414)"};
}

} // namespace nl::back
