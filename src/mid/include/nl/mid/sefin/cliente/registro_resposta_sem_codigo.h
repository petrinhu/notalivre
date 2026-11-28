// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Evento sefin.resposta_sem_codigo (NL-3.L, passo 15): o 4xx do envio sem código E sai no log só com o status, o tipo de conteúdo
// num conjunto fechado e o tamanho do corpo em bytes, nunca o texto do corpo. Diz se a recusa veio de uma porta de entrada (HTML)
// ou da aplicação (JSON) sem dado pessoal. Decisão do líder, 27/11/2026, por AskUserQuestion: "Tipo e tamanho do corpo
// (Recomendado)".
#ifndef NL_MID_SEFIN_CLIENTE_REGISTRO_RESPOSTA_SEM_CODIGO_H
#define NL_MID_SEFIN_CLIENTE_REGISTRO_RESPOSTA_SEM_CODIGO_H

#include <nl/foundation/log/log.h>

#include <QByteArrayView>
#include <QtGlobal>

#include <string>

namespace nl::mid::sefin::cliente {

/// Tipo de conteúdo do Content-Type num conjunto fechado: "json", "html", "xml", "texto", "ausente" (cabeçalho vazio) ou "outro".
/// Só o tipo de mídia antes do ";" conta, sem diferença de maiúscula; nada do texto do cabeçalho passa adiante.
std::string tipoMidiaParaLog(QByteArrayView tipoConteudo);

/// Registra o 4xx sem código E: status, tipoMidia (tipoMidiaParaLog) e bytesResposta (inteiro, negativo vira 0).
void registrarRespostaSemCodigo(nl::foundation::log::Log& registro, int status, QByteArrayView tipoConteudo, qsizetype bytesResposta);

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_REGISTRO_RESPOSTA_SEM_CODIGO_H
