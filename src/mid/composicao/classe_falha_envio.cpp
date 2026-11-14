// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Classe da falha de envio (NL-3.C, plano seção 1.5). Ver classe_falha_envio.h. A tabela é a da seção 1.5; o que não está
// listado cai em EstadoIncerto (C4), a conservadora: nunca manda usar outro número.
#include <nl/mid/composicao/classe_falha_envio.h>

#include <utility>

namespace nl::mid::composicao {
namespace {

using Causa = nl::back::CausaFalhaSefin;
using Motivo = nl::back::MotivoFalhaSefin;

// Transitoria: rede é C3; sem causa, consulta sem número e sefin indisponível são C6; o resto é C4.
ClasseFalhaEnvio classeDaTransitoria(Causa causa)
{
    switch (causa) {
    case Causa::Rede:
        return ClasseFalhaEnvio::RedeAntesDoEnvio;
    case Causa::Nenhuma:
    case Causa::SefinIndisponivel:
    case Causa::NaoGerada:
        return ClasseFalhaEnvio::NaoConcluido;
    case Causa::CertificadoDoEmissorProvavel:
    case Causa::ServidorNaoConfirmado:
    case Causa::ConfiguracaoDaConexao:
    case Causa::PreparacaoDoEnvio:
    case Causa::RespostaSemCodigoValido:
        return ClasseFalhaEnvio::EstadoIncerto;
    }
    std::unreachable();
}

// Definitiva: certificado, servidor, configuração, preparo e resposta sem código têm classe própria; o resto é C4.
ClasseFalhaEnvio classeDaDefinitiva(Causa causa)
{
    switch (causa) {
    case Causa::CertificadoDoEmissorProvavel:
        return ClasseFalhaEnvio::CertificadoProvavel;
    case Causa::ServidorNaoConfirmado:
        return ClasseFalhaEnvio::ServidorNaoConfirmado;
    case Causa::ConfiguracaoDaConexao:
        return ClasseFalhaEnvio::ConfiguracaoDaConexao;
    case Causa::PreparacaoDoEnvio:
        return ClasseFalhaEnvio::FalhaAntesDoEnvio;
    case Causa::RespostaSemCodigoValido:
        return ClasseFalhaEnvio::RespostaSemCodigo;
    case Causa::Nenhuma:
    case Causa::Rede:
    case Causa::SefinIndisponivel:
    case Causa::NaoGerada:
        return ClasseFalhaEnvio::EstadoIncerto;
    }
    std::unreachable();
}

// Rejeitada sem código E: só a falta de causa e a resposta sem código válido são C7b; o resto é C4.
ClasseFalhaEnvio classeDaRejeitadaSemCodigo(Causa causa)
{
    if (causa == Causa::Nenhuma || causa == Causa::RespostaSemCodigoValido) {
        return ClasseFalhaEnvio::RespostaSemCodigo;
    }
    return ClasseFalhaEnvio::EstadoIncerto;
}

} // namespace

std::optional<ClasseFalhaEnvio> classeDaFalha(const nl::back::FalhaSefin& falha) noexcept
{
    // Código E presente em Rejeitada ou DpsNaoGerada: caminho antigo (sefin.<E>), sem classe. Nos outros motivos o código E é ignorado.
    const bool comCodigoE = !falha.codigoE.empty();
    switch (falha.motivo) {
    case Motivo::Transitoria:
        return classeDaTransitoria(falha.causa);
    case Motivo::DpsNaoGerada:
        if (comCodigoE) {
            return std::nullopt;
        }
        return ClasseFalhaEnvio::EstadoIncerto;
    case Motivo::DpsDuplicada:
        return std::nullopt;
    case Motivo::Rejeitada:
        if (comCodigoE) {
            return std::nullopt;
        }
        return classeDaRejeitadaSemCodigo(falha.causa);
    case Motivo::Definitiva:
        return classeDaDefinitiva(falha.causa);
    case Motivo::Indeterminado:
        return falha.causa == Causa::CertificadoDoEmissorProvavel ? ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel
                                                                  : ClasseFalhaEnvio::EstadoIncerto;
    case Motivo::ComparacaoIndisponivel:
        return ClasseFalhaEnvio::ComparacaoIndisponivel;
    }
    std::unreachable();
}

} // namespace nl::mid::composicao
