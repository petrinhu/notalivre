# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Roteiro de falhas do servidor falso: cada falha agendada vale UMA vez para a rota (NL-2.6, falha acontece uma vez).

Falhas: erro5xx (503 antes de processar), timeout (espera a liberação do teste, sem responder), conflito409
(409 antes de processar), conexao_cortada_apos_aceite (aceita e grava a DPS, depois corta a conexão sem resposta),
corpo_truncado (GET responde 200 com Content-Length e fecha a conexão no meio do corpo), eco_accept_encoding (GET
devolve no corpo o Accept-Encoding recebido) e content_encoding_gzip (GET responde 200 com Content-Encoding gzip).
NFS-e defeituosa no sucesso do POST /nfse (NL-2.8, DA-2.39): nfse_base64_invalido (caractere fora do Base64),
nfse_gzip_invalido (Base64 válido de um conteúdo que não é gzip) e nfse_acima_do_teto (gzip válido, acima do teto).
"""

import re

FALHAS = ("erro5xx", "timeout", "conexao_cortada_apos_aceite", "conflito409", "corpo_truncado",
          "eco_accept_encoding", "content_encoding_gzip", "redireciona", "corpo_sem_tamanho",
          "nfse_base64_invalido", "nfse_gzip_invalido", "nfse_acima_do_teto", "post_sem_chave", "status202", "redireciona_post", "post_ce_gzip", "consulta_404_outro", "nfse_base64_caractere_invalido", "nfse_gzip_dois_membros", "nfse_base64_igual_no_meio", "conflito409_gravado")
SO_GET = ("corpo_truncado", "eco_accept_encoding", "content_encoding_gzip", "redireciona", "corpo_sem_tamanho", "consulta_404_outro")
SO_POST_NFSE = ("nfse_base64_invalido", "nfse_gzip_invalido", "nfse_acima_do_teto", "nfse_base64_caractere_invalido", "nfse_gzip_dois_membros", "nfse_base64_igual_no_meio")  # sucesso do POST com NFS-e defeituosa


RE_REJEICAO = re.compile(r"^rejeicao:(E\d{4})$")  # rejeicao:<código E>, NL-2.8: 400 com o código e texto livre com CPF


RE_REJEICAO_BRUTA = re.compile(r"^rejeicao_bruta:\S+$")  # Codigo da API sem validar: o cliente tem de recusar o formato
RE_REJEICAO_GERADA = re.compile(r"^rejeicao_gerada:(E\d{4})$")  # grava a DPS e recusa com E: a nota já existe


def eh_rejeicao(falha):
    return (RE_REJEICAO.match(falha) is not None or RE_REJEICAO_BRUTA.match(falha) is not None
            or RE_REJEICAO_GERADA.match(falha) is not None)


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
        if falha in SO_POST_NFSE and (metodo, caminho) != ("POST", "/nfse"):
            raise ValueError(f"{falha} só vale para POST /nfse")
        self._filas.append((metodo, caminho, falha))

    def consumir(self, metodo, caminho):
        for i, (m, c, falha) in enumerate(self._filas):
            if m == metodo and c == caminho:
                del self._filas[i]
                return falha
        return None

    def pendentes(self):
        return len(self._filas)
