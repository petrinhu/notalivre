// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo da falha de envio (NL-3.C, plano seção 3.4). Textos aprovados pelo líder em 14/11/2026, um literal por texto,
// sem concatenação. Conferidos pelo awk de igualdade da seção 4.5 do plano contra o arquivo aprovado.
#include <nl/mid/composicao/catalogo_falha_envio.h>

#include <algorithm>
#include <utility>

namespace nl::mid::composicao {

TextoDaFalha textoDaClasse(ClasseFalhaEnvio classe) noexcept
{
    switch (classe) {
    case ClasseFalhaEnvio::CertificadoProvavel:
        return TextoDaFalha{kCodigoCertificadoProvavel, "Nada foi enviado. É provável que o certificado digital do emissor não tenha sido aceito pelo sistema nacional da NFS-e. Confira se o certificado está válido e se é o do emissor. Depois, se for repetir, use o mesmo número."};
    case ClasseFalhaEnvio::ServidorNaoConfirmado:
        return TextoDaFalha{kCodigoServidorNaoConfirmado, "Nada foi enviado. O programa não confirmou que fala com o sistema nacional da NFS-e. O certificado do emissor não é a causa. Confira a data e a hora do computador. Em rede pública, trocar de rede pode ajudar. Se repetir, use o mesmo número."};
    case ClasseFalhaEnvio::RedeAntesDoEnvio:
        return TextoDaFalha{kCodigoRedeAntesDoEnvio, "Nada foi enviado. A conexão falhou antes de a nota sair do computador. Confira a internet. Depois, pode repetir com a mesma série e o mesmo número."};
    case ClasseFalhaEnvio::EstadoIncerto:
        return TextoDaFalha{kCodigoEstadoIncerto, "Não houve confirmação do sistema nacional da NFS-e. A nota pode ter sido emitida. Não use outro número. Repita só com a mesma série, o mesmo número e o mesmo arquivo."};
    case ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel:
        return TextoDaFalha{kCodigoEstadoIncertoCertificadoProvavel, "Não houve confirmação do sistema nacional da NFS-e, e a conferência indicou provável problema no certificado digital do emissor. A nota pode ter sido emitida. Não use outro número. Repita só com a mesma série, o mesmo número e o mesmo arquivo."};
    case ClasseFalhaEnvio::NaoConcluido:
        return TextoDaFalha{kCodigoNaoConcluido, "O sistema nacional da NFS-e não concluiu a emissão. Aguarde alguns minutos. Depois, pode repetir com a mesma série e o mesmo número."};
    case ClasseFalhaEnvio::FalhaAntesDoEnvio:
        return TextoDaFalha{kCodigoFalhaAntesDoEnvio, "Nada foi enviado. O programa falhou ao preparar a nota, antes de qualquer envio. Atualize o programa. Se a falha continuar, informe o erro a quem distribui o programa."};
    case ClasseFalhaEnvio::RespostaSemCodigo:
        return TextoDaFalha{kCodigoRespostaSemCodigo, "O programa não conseguiu ler a resposta do sistema nacional da NFS-e. A nota pode ter sido emitida. Não use outro número. Repita só com a mesma série, o mesmo número e o mesmo arquivo."};
    case ClasseFalhaEnvio::ConfiguracaoDaConexao:
        return TextoDaFalha{kCodigoConfiguracaoDaConexao, "Nada foi enviado. A configuração da conexão segura está incompleta. Atualize o programa ou informe o erro a quem o distribui."};
    case ClasseFalhaEnvio::ComparacaoIndisponivel:
        return TextoDaFalha{kCodigoComparacaoIndisponivel, "Não use outro número antes de conferir a nota que já existe com este número. O programa não conseguiu confirmar se ela é a mesma que você quer emitir."};
    }
    std::unreachable();
}

bool ehFalhaDeEnvio(std::string_view codigo) noexcept
{
    return std::ranges::any_of(kTodasAsClasses, [codigo](ClasseFalhaEnvio classe) {
        return textoDaClasse(classe).codigo == codigo;
    });
}

} // namespace nl::mid::composicao
