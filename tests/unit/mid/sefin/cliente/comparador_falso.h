// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: dublê da porta ComparadorInfDps.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_COMPARADOR_FALSO_H
#define NL_TESTE_COMPARADOR_FALSO_H

#include <nl/mid/sefin/cliente/comparador_infdps.h>
#include <nl/back/dps/id_dps.h>
#include <QByteArray>

namespace bk = nl::back;
namespace mc = nl::mid::sefin::cliente;

namespace {

// Comparador de teste (adendo da DA-2.41): devolve o resultado configurado e guarda o que recebeu.
class ComparadorFalso final : public mc::ComparadorInfDps {
public:
    explicit ComparadorFalso(mc::ResultadoComparacaoInfDps resultado) : resultado_(resultado) {}
    mc::ResultadoComparacaoInfDps comparar(const bk::IdDps&, const QByteArray& dpsXml,
                                           const QByteArray& nfseXml) const override
    {
        chamadas_++;
        dpsRecebido_ = dpsXml;
        nfseRecebido_ = nfseXml;
        return resultado_;
    }
    mutable int chamadas_ = 0;
    mutable QByteArray dpsRecebido_;
    mutable QByteArray nfseRecebido_;

private:
    mc::ResultadoComparacaoInfDps resultado_;
};

} // namespace

#endif // NL_TESTE_COMPARADOR_FALSO_H
