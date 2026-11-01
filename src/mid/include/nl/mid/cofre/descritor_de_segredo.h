// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Canal de segredo por descritor (NL-2.27, DA-2.16): a senha do certificado chega por um descritor herdado, `--senha-certificado-fd
// N`, lido uma vez até o fim, com teto, zerado e fechado. Nunca por argumento, ambiente ou arquivo de configuração. Só a Mid usa este
// header (DA-2.57b): a CLI não vê a senha, só repassa o número do descritor à fachada.
#ifndef NL_MID_COFRE_DESCRITOR_DE_SEGREDO_H
#define NL_MID_COFRE_DESCRITOR_DE_SEGREDO_H

#include <nl/foundation/pkcs12/senha_segura.h>

#include <expected>

namespace nl::mid::cofre {

/// Motivo de a senha não ter sido lida pelo descritor. Cada um vira um código estável `cofre.*` na fachada.
enum class ErroDoSegredo {
    DescritorInvalido,     ///< Número fora de faixa, descritor 0, 1 ou 2, ou descritor que não está aberto (cofre.descritor_invalido).
    DescritorTerminal,     ///< O descritor é um terminal: a senha tem de vir de um canal sem eco (cofre.descritor_terminal).
    LegivelPorOutros,      ///< Arquivo comum com permissão para grupo ou outros (cofre.descritor_legivel_por_outros).
    SenhaVazia,            ///< Sem bytes depois da quebra final descartada (cofre.senha_vazia).
    AcimaDoTeto,           ///< Mais de 512 bytes (cofre.senha_acima_do_teto).
    FalhaDeLeitura,        ///< A leitura do descritor falhou (cofre.descritor_falha_de_leitura).
};

/// Lê a senha de um descritor já aberto: uma vez, até o fim; descarta uma só quebra final; zera o buffer; fecha o descritor. Nada
/// do conteúdo sai em mensagem nem em log. A conferência do descritor (fstat, fcntl) acontece antes de qualquer leitura.
std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoSegredo> lerSenhaDoDescritor(int fd);

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_DESCRITOR_DE_SEGREDO_H
