// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Posse do descritor da senha na entrada da CLI (NL-2.27b, DA-2.72). A CLI confere o número pedido e toma posse dele antes de qualquer
// abertura de arquivo e antes da QCoreApplication: o descritor passa a um número alto com fechamento no exec, e a identidade (dispositivo
// e inode) fica guardada. Só vale o descritor tomado na entrada: a leitura confere a identidade de novo. Só a Mid usa este header.
#ifndef NL_MID_COFRE_TOMADA_DESCRITOR_H
#define NL_MID_COFRE_TOMADA_DESCRITOR_H

#include <sys/types.h>

#include <expected>
#include <string>

namespace nl::mid::cofre {

/// Motivo de a posse do descritor não ter sido tomada, ou de a identidade ter mudado até a leitura.
enum class ErroDaTomada {
    DescritorInvalido,  ///< 0 a 2, número fora de faixa, ou descritor fechado na entrada (cofre.descritor_invalido).
    DescritorTerminal,  ///< O descritor é um terminal (cofre.descritor_terminal).
    LegivelPorOutros,   ///< Arquivo comum com permissão para grupo ou outros; só o tipo vai em tipo (cofre.descritor_legivel_por_outros).
    FalhaDeTomada,      ///< A duplicação para o número alto falhou (cofre.descritor_invalido).
    DescritorTrocado,   ///< Na leitura, o descritor não tem o dispositivo e o inode tomados na entrada (cofre.descritor_trocado).
};

/// Recusa da tomada: o motivo, e o tipo do descritor recusado, para a mensagem. O tipo nunca traz nome nem caminho de arquivo (ADR-0005,
/// DA-2.73): a mensagem diz só o tipo e o que fazer.
struct RecusaDaTomada {
    ErroDaTomada motivo = ErroDaTomada::DescritorInvalido;
    std::string tipo;
};

/// Descritor tomado na entrada, com a identidade do arquivo que estava no número pedido.
struct TomadaDeDescritor {
    int fd = -1;
    dev_t dispositivo = 0;
    ino_t inodo = 0;
};

/// Toma posse do descritor pedido. Não lê nenhum byte. Em sucesso, o número pedido é fechado e o descritor novo sai com FD_CLOEXEC.
/// Recusa de 0 a 2 e de descritor não aberto não fecha nada (não são do app); as outras recusas fecham o número pedido.
std::expected<TomadaDeDescritor, RecusaDaTomada> tomarDescritorDeSenha(int numeroPedido);

/// Confere, na leitura, que o descritor é o mesmo que foi tomado na entrada (mesmo dispositivo e inode).
std::expected<void, RecusaDaTomada> conferirTomadaDeDescritor(const TomadaDeDescritor& tomada);

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_TOMADA_DESCRITOR_H
