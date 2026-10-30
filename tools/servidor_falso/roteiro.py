# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Roteiro de falhas do servidor falso: cada falha agendada vale UMA vez para a rota (NL-2.6, falha acontece uma vez).

Falhas: erro5xx (503 antes de processar), timeout (espera a liberação do teste, sem responder), conflito409
(409 antes de processar), conexao_cortada_apos_aceite (aceita e grava a DPS, depois corta a conexão sem resposta),
corpo_truncado (GET responde 200 com Content-Length e fecha a conexão no meio do corpo), eco_accept_encoding (GET
devolve no corpo o Accept-Encoding recebido) e content_encoding_gzip (GET responde 200 com Content-Encoding gzip).
"""

import re

FALHAS = ("erro5xx", "timeout", "conexao_cortada_apos_aceite", "conflito409", "corpo_truncado",
          "eco_accept_encoding", "content_encoding_gzip", "redireciona", "corpo_sem_tamanho")
SO_GET = ("corpo_truncado", "eco_accept_encoding", "content_encoding_gzip", "redireciona", "corpo_sem_tamanho")


RE_REJEICAO = re.compile(r"^rejeicao:(E\d{4})$")  # rejeicao:<código E>, NL-2.8: 400 com o código e texto livre com CPF


def eh_rejeicao(falha):
    return RE_REJEICAO.match(falha) is not None


class Roteiro:
    def __init__(self):
        self._filas = []

    def agendar(self, metodo, caminho, falha):
        if falha not in FALHAS and not eh_rejeicao(falha):
            raise ValueError(f"falha desconhecida: {falha}")
        if falha == "conexao_cortada_apos_aceite" and (metodo, caminho) != ("POST", "/nfse"):
            raise ValueError("conexao_cortada_apos_aceite só vale para POST /nfse")
        if falha in SO_GET and metodo != "GET":
            raise ValueError(f"{falha} só vale para GET")
        self._filas.append((metodo, caminho, falha))

    def consumir(self, metodo, caminho):
        for i, (m, c, falha) in enumerate(self._filas):
            if m == metodo and c == caminho:
                del self._filas[i]
                return falha
        return None

    def pendentes(self):
        return len(self._filas)
