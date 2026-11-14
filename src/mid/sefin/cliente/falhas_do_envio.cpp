// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Falhas do envio (NL-3.C, plano seção 3.3). Ver falhas_do_envio.h. Corpos provisórios: os testes ainda devem ver vermelho.
#include <nl/mid/sefin/cliente/falhas_do_envio.h>

namespace nl::mid::sefin::cliente {

nl::back::CausaFalhaSefin causaDoTransporte(const nl::foundation::transporte::ErroTransporte& /*erro*/)
{
    return nl::back::CausaFalhaSefin::Nenhuma;
}

nl::back::FalhaSefin falhaSemBytesEscritos(const nl::foundation::transporte::ErroTransporte& /*erro*/, bool /*reenvio*/)
{
    return nl::back::FalhaSefin{nl::back::MotivoFalhaSefin::Transitoria, {}, {}, nl::back::CausaFalhaSefin::Nenhuma};
}

nl::back::FalhaSefin falhaDeRedeNaConsulta(const nl::foundation::transporte::ErroTransporte& /*erro*/)
{
    return nl::back::FalhaSefin{nl::back::MotivoFalhaSefin::Transitoria, {}, {}, nl::back::CausaFalhaSefin::Nenhuma};
}

nl::back::FalhaSefin indeterminadoDepoisDaConsulta(const nl::back::FalhaSefin& /*consulta*/)
{
    return nl::back::FalhaSefin{nl::back::MotivoFalhaSefin::Transitoria, {}, {}, nl::back::CausaFalhaSefin::Nenhuma};
}

} // namespace nl::mid::sefin::cliente
