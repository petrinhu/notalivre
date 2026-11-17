// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Falhas do envio (NL-3.C, plano seção 3.3 e 7.2 só do átomo). Ver falhas_do_envio.h. Sem rede e sem texto: a causa é tipada.
#include <nl/mid/sefin/cliente/falhas_do_envio.h>

#include <utility>

namespace nl::mid::sefin::cliente {
namespace {

using nl::back::CausaFalhaSefin;
using nl::back::FalhaSefin;
using nl::back::MotivoFalhaSefin;
using nl::foundation::transporte::CausaErro;
using nl::foundation::transporte::ErroTransporte;
using nl::foundation::transporte::TipoErro;

// Falha sem código E e sem texto: o motivo e a causa são tudo o que o Mid sabe.
FalhaSefin falha(MotivoFalhaSefin motivo, CausaFalhaSefin causa)
{
    return FalhaSefin{motivo, {}, {}, causa};
}

} // namespace

CausaFalhaSefin causaDoTransporte(const ErroTransporte& erro)
{
    switch (erro.causa) {
    case CausaErro::Tls:
        // Sem codigosSsl, a conexão segura foi recusada sem erro de verificação do servidor: provável certificado do emissor.
        return erro.codigosSsl.empty() ? CausaFalhaSefin::CertificadoDoEmissorProvavel : CausaFalhaSefin::ServidorNaoConfirmado;
    case CausaErro::Timeout:
    case CausaErro::Conexao:
    case CausaErro::Protocolo:
        return CausaFalhaSefin::Rede;
    case CausaErro::Configuracao:
        return CausaFalhaSefin::ConfiguracaoDaConexao;
    }
    std::unreachable();
}

FalhaSefin falhaSemBytesEscritos(const ErroTransporte& erro, bool reenvio)
{
    // R-10 b (CISO): no reenvio a Sefin pode já ter a DPS; zero bytes vira transitória sem número, nunca causa de recusa.
    if (reenvio) {
        return falha(MotivoFalhaSefin::Transitoria, CausaFalhaSefin::NaoGerada);
    }
    const CausaFalhaSefin causa = causaDoTransporte(erro);
    if (causa == CausaFalhaSefin::Rede) {
        return falha(MotivoFalhaSefin::Transitoria, causa);
    }
    return falha(MotivoFalhaSefin::Definitiva, causa);
}

FalhaSefin falhaDeRedeNaConsulta(const ErroTransporte& erro)
{
    const MotivoFalhaSefin motivo = erro.tipo == TipoErro::Transitorio ? MotivoFalhaSefin::Transitoria : MotivoFalhaSefin::Definitiva;
    return falha(motivo, causaDoTransporte(erro));
}

FalhaSefin indeterminadoDepoisDaConsulta(const FalhaSefin& consulta)
{
    return falha(MotivoFalhaSefin::Indeterminado, consulta.causa);
}

} // namespace nl::mid::sefin::cliente
