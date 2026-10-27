// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0246 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0246.h>
#include <QString>

#include <variant>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/endereco_exterior.h>
#include <nl/back/dps/tomador.h>

namespace nl::back {

std::string_view RegraE0246::codigo() const noexcept
{
    return "E0246";
}

std::optional<nl::base::Erro> RegraE0246::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    if (!dados.toma || !dados.toma->endereco()) {
        return std::nullopt;
    }
    const auto* exterior = std::get_if<EnderecoExterior>(&dados.toma->endereco()->local());
    if (exterior == nullptr) {
        return std::nullopt;
    }
    // ZZ é recusado sem checar ambGer (Obs 2 da linha Excel 284): ambGer é campo da NFS-e gerada (LEIAUTE, linha Excel 15),
    // não da DPS. O app só envia à Sefin Nacional, que emite com ambGer = 2 (RN, linha Excel 25), então a condição é sempre verdadeira (DA-1.28).
    const QString pais = exterior->cPais().texto();
    if (pais == QStringLiteral("BR") || pais == QStringLiteral("ZZ") || !catalogo_.paisExiste(exterior->cPais())) {
        return nl::base::Erro{"E0246", std::string{"Dps.infDPS.toma.end.endExt.cPais"},
                              "cPais do tomador no exterior não existe na ISO2, é BR ou é ZZ (regra E0246, linha Excel 284)"};
    }
    return std::nullopt;
}

} // namespace nl::back
