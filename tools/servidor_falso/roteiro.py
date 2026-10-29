# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Roteiro de falhas do servidor falso: cada falha agendada vale UMA vez para a rota (NL-2.6, falha acontece uma vez).

Falhas: erro5xx (503 antes de processar), timeout (espera a liberação do teste, sem responder), conflito409
(409 antes de processar), conexao_cortada_apos_aceite (aceita e grava a DPS, depois corta a conexão sem resposta),
corpo_truncado (GET responde 200 com Content-Length e fecha a conexão no meio do corpo).
"""

FALHAS = ("erro5xx", "timeout", "conexao_cortada_apos_aceite", "conflito409", "corpo_truncado")


class Roteiro:
    def __init__(self):
        self._filas = []

    def agendar(self, metodo, caminho, falha):
        if falha not in FALHAS:
            raise ValueError(f"falha desconhecida: {falha}")
        if falha == "conexao_cortada_apos_aceite" and (metodo, caminho) != ("POST", "/nfse"):
            raise ValueError("conexao_cortada_apos_aceite só vale para POST /nfse")
        if falha == "corpo_truncado" and metodo != "GET":
            raise ValueError("corpo_truncado só vale para GET (corte no meio da resposta)")
        self._filas.append((metodo, caminho, falha))

    def consumir(self, metodo, caminho):
        for i, (m, c, falha) in enumerate(self._filas):
            if m == metodo and c == caminho:
                del self._filas[i]
                return falha
        return None

    def pendentes(self):
        return len(self._filas)
