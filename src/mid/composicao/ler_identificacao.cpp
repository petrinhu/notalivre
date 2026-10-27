// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor da identificação da DPS (NL-1.7): tpAmb, dhEmi, verAplic, serie, nDps, dCompet e cLocEmi.
#include "campos_json.h"
#include "leitores_dps.h"

#include <nl/back/dps/serie.h>
#include <nl/back/dps/tipo_ambiente.h>

#include <limits>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<Identificacao, nl::base::Erro> lerIdentificacao(const QJsonObject& raiz)
{
    const auto tpAmbTexto = campos::textoObrigatorio(raiz, "", "tpAmb");
    if (!tpAmbTexto) {
        return std::unexpected(tpAmbTexto.error());
    }
    const auto tpAmb = nb::TipoAmbiente::de(*tpAmbTexto);
    if (!tpAmb) {
        return std::unexpected(tpAmb.error());
    }

    // Data inválida não é checada aqui: QDateTime/QDate inválidos caem em DhEmi::de e em Dps::montar ("dhemi.formato", "dps.dcompet").
    const auto dhEmiTexto = campos::textoObrigatorio(raiz, "", "dhEmi");
    if (!dhEmiTexto) {
        return std::unexpected(dhEmiTexto.error());
    }
    const QDateTime dhEmi = QDateTime::fromString(*dhEmiTexto, Qt::ISODate);

    const auto verAplicTexto = campos::textoObrigatorio(raiz, "", "verAplic");
    if (!verAplicTexto) {
        return std::unexpected(verAplicTexto.error());
    }
    const auto verAplic = nb::VerAplic::de(*verAplicTexto, "verAplic");
    if (!verAplic) {
        return std::unexpected(verAplic.error());
    }

    const auto serieNumero = campos::inteiroObrigatorio(raiz, "", "serie");
    if (!serieNumero) {
        return std::unexpected(serieNumero.error());
    }
    if (*serieNumero > std::numeric_limits<int>::max()) {
        return std::unexpected(nl::base::Erro{"entrada.tipo_invalido", std::string{"serie"},
                                              std::string{"campo serie fora da faixa de inteiro"}});
    }
    const auto serie = nb::Serie::de(static_cast<int>(*serieNumero));
    if (!serie) {
        return std::unexpected(serie.error());
    }

    const auto nDpsNumero = campos::inteiroObrigatorio(raiz, "", "nDps");
    if (!nDpsNumero) {
        return std::unexpected(nDpsNumero.error());
    }
    const auto nDps = nb::NumeroDps::de(*nDpsNumero);
    if (!nDps) {
        return std::unexpected(nDps.error());
    }

    const auto dCompetTexto = campos::textoObrigatorio(raiz, "", "dCompet");
    if (!dCompetTexto) {
        return std::unexpected(dCompetTexto.error());
    }
    const QDate dCompet = QDate::fromString(*dCompetTexto, Qt::ISODate);

    const auto cLocEmiTexto = campos::textoObrigatorio(raiz, "", "cLocEmi");
    if (!cLocEmiTexto) {
        return std::unexpected(cLocEmiTexto.error());
    }
    const auto cLocEmi = nb::CodigoMunicipio::de(*cLocEmiTexto);
    if (!cLocEmi) {
        return std::unexpected(cLocEmi.error());
    }

    return Identificacao{
        .tpAmb = *tpAmb,
        .dhEmi = dhEmi,
        .verAplic = *verAplic,
        .serie = *serie,
        .nDps = *nDps,
        .dCompet = dCompet,
        .cLocEmi = *cLocEmi,
    };
}

} // namespace nl::mid::composicao::leitores
