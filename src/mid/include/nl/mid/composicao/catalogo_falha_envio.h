// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo da falha de envio sem código E (NL-3.C, plano seções 1.5 e 3.4): texto fixo por classe, aprovado pelo líder em
// 14/11/2026, e código estável envio.* (ou sefin.comparacao_indisponivel). Átomo próprio (L-34): o texto muda quando o
// líder muda o texto, sem tocar a regra de classe nem o código de saída.
#ifndef NL_MID_COMPOSICAO_CATALOGO_FALHA_ENVIO_H
#define NL_MID_COMPOSICAO_CATALOGO_FALHA_ENVIO_H

#include <array>
#include <string_view>

namespace nl::mid::composicao {

/// Classe de uma falha do envio (tabela da seção 1.5 do plano; C1 a C9, com C7a e C7b separadas).
enum class ClasseFalhaEnvio {
    CertificadoProvavel,           ///< C1: conexão segura recusada sem erro na verificação do servidor, antes do envio.
    ServidorNaoConfirmado,         ///< C2: conexão segura recusada com erro na verificação do servidor, antes do envio.
    RedeAntesDoEnvio,              ///< C3: rede antes do envio; pode repetir com o mesmo número.
    EstadoIncerto,                 ///< C4: o envio pode ter chegado e a consulta não disse o que aconteceu.
    EstadoIncertoCertificadoProvavel, ///< C5: C4, com provável problema no certificado.
    NaoConcluido,                  ///< C6: a Sefin informou que a nota não foi concluída; pode repetir com o mesmo número.
    FalhaAntesDoEnvio,             ///< C7a: falha ao preparar a nota, antes de qualquer envio (defeito do programa).
    RespostaSemCodigo,             ///< C7b: a Sefin respondeu sem código que o programa reconheça.
    ConfiguracaoDaConexao,         ///< C8: configuração da conexão segura incompleta; nada saiu da máquina.
    ComparacaoIndisponivel,        ///< C9: já existe nota com este número e a comparação não foi possível.
};

inline constexpr std::array<ClasseFalhaEnvio, 10> kTodasAsClasses{
    ClasseFalhaEnvio::CertificadoProvavel,
    ClasseFalhaEnvio::ServidorNaoConfirmado,
    ClasseFalhaEnvio::RedeAntesDoEnvio,
    ClasseFalhaEnvio::EstadoIncerto,
    ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel,
    ClasseFalhaEnvio::NaoConcluido,
    ClasseFalhaEnvio::FalhaAntesDoEnvio,
    ClasseFalhaEnvio::RespostaSemCodigo,
    ClasseFalhaEnvio::ConfiguracaoDaConexao,
    ClasseFalhaEnvio::ComparacaoIndisponivel,
};

// Códigos estáveis: o script, o log e o código de saída usam estas constantes; nenhuma string repetida.
inline constexpr std::string_view kCodigoCertificadoProvavel = "envio.certificado_provavel";
inline constexpr std::string_view kCodigoServidorNaoConfirmado = "envio.servidor_nao_confirmado";
inline constexpr std::string_view kCodigoRedeAntesDoEnvio = "envio.rede_antes_do_envio";
inline constexpr std::string_view kCodigoEstadoIncerto = "envio.estado_incerto";
inline constexpr std::string_view kCodigoEstadoIncertoCertificadoProvavel = "envio.estado_incerto_certificado_provavel";
inline constexpr std::string_view kCodigoNaoConcluido = "envio.nao_concluido";
inline constexpr std::string_view kCodigoFalhaAntesDoEnvio = "envio.falha_antes_do_envio";
inline constexpr std::string_view kCodigoRespostaSemCodigo = "envio.resposta_sem_codigo";
inline constexpr std::string_view kCodigoConfiguracaoDaConexao = "envio.configuracao_da_conexao";
inline constexpr std::string_view kCodigoComparacaoIndisponivel = "sefin.comparacao_indisponivel";

/// Código estável e texto fixo de uma classe. Os dois são de conjunto fechado.
struct TextoDaFalha {
    std::string_view codigo;
    std::string_view texto;
};

/// Texto e código da classe.
[[nodiscard]] TextoDaFalha textoDaClasse(ClasseFalhaEnvio classe) noexcept;

/// Verdadeiro só para os 10 códigos do catálogo (um por classe).
[[nodiscard]] bool ehFalhaDeEnvio(std::string_view codigo) noexcept;

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_CATALOGO_FALHA_ENVIO_H
