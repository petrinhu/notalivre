# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Servidor falso da API da Sefin Nacional, HTTPS em 127.0.0.1 (NL-2.6). Ferramenta de teste, nunca do app (L-33).

Mutual TLS: o certificado de cliente é obrigatório (CERT_REQUIRED, AC de teste). POST /nfse desfaz o envelope,
valida o XSD com xmllint e a assinatura com xmlsec1 (oráculo), e responde com a chave de 50 posições.
INFERÊNCIA até a NL-2.10: códigos de erro (exceto E0714, do Anexo I), a forma do JSON, o status 201 e a chave.
Endpoints de evento e de DFe respondem 501: fora desta fatia, sem teste, sem comportamento inventado.
"""

import hashlib
import http.server
import json
import ssl
import threading

from . import envelope
from .referencia import id_do_infdps
from .roteiro import Roteiro

ROTA_DPS = "/dps/"
ROTA_NFSE = "/nfse/"
# Fora desta fatia por decisão do CTO, DA-2.27 (/var/tmp/notalivre-logs/decisoes_autonomas_onda2.md): POST /nfse/{chave}/eventos
# é da NL-4.1 e GET /DFe/{NSU} é da NL-4.2, e ambas respondem 501 até lá (o plano, linha 523, já aponta para essas fatias).
# GET /nfse/{chave}/eventos cai na consulta de NFS-e e devolve 404 (NFSE_NAO_ENCONTRADA) até a NL-4.2.
PREFIXO_NAO_IMPLEMENTADO = ("/DFe/",)


def chave_de_teste(id_dps):
    """INFERÊNCIA: 50 dígitos derivados do Id da DPS. Não é o layout oficial da chave de acesso."""
    digest = hashlib.sha256(id_dps.encode("ascii")).hexdigest()
    return str(int(digest, 16) % 10**50).zfill(50)


def corpo_da_nota(chave, xml):
    # INFERÊNCIA: o campo leva a DPS assinada no lugar do XML da NFS-e (sem schema de NFS-e aqui).
    return {"chaveAcesso": chave, "nfseXmlGZipB64": envelope.gzip_b64(xml)}


class ServidorFalso(http.server.HTTPServer):
    allow_reuse_address = True

    def __init__(self, certs, oraculo, roteiro=None):
        self.contexto = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.contexto.load_cert_chain(certs.servidor_cert, certs.servidor_key)
        self.contexto.minimum_version = ssl.TLSVersion.TLSv1_2  # explícito: nada abaixo de TLS 1.2 (rodada 2, item 4)
        self.contexto.verify_mode = ssl.CERT_REQUIRED
        self.contexto.load_verify_locations(certs.ca)
        self.oraculo = oraculo
        self.roteiro = roteiro if roteiro is not None else Roteiro()
        self.dps_aceitas = {}
        self.notas = {}
        self.requisicoes = 0
        self.liberar = threading.Event()
        super().__init__(("127.0.0.1", 0), ManipuladorApi)

    @property
    def porta(self):
        return self.server_address[1]

    def get_request(self):
        sock, endereco = super().get_request()
        try:
            return self.contexto.wrap_socket(sock, server_side=True), endereco
        except OSError:
            sock.close()
            raise

    def iniciar(self):
        self._thread = threading.Thread(target=self.serve_forever, daemon=True)
        self._thread.start()

    def encerrar(self):
        self.liberar.set()
        self.shutdown()
        self.server_close()
        self._thread.join(5)


class ManipuladorApi(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"

    def log_message(self, formato, *args):
        pass

    def do_POST(self):
        self._tratar()

    def do_GET(self):
        self._tratar()

    def do_HEAD(self):
        self._tratar()

    def _tratar(self):
        srv = self.server
        srv.requisicoes += 1
        caminho = self.path.split("?", 1)[0]
        corpo = self._ler_corpo()
        falha = srv.roteiro.consumir(self.command, caminho)
        if falha == "erro5xx":
            return self._json(503, {"Codigo": "INDISPONIVEL", "Descricao": "falha roteirizada"})
        if falha == "timeout":
            srv.liberar.wait(5)
            return self._json(504, {"Codigo": "TIMEOUT", "Descricao": "falha roteirizada"})
        if falha == "conflito409":
            return self._json(409, {"Codigo": "DPS_DUPLICADA", "Descricao": "falha roteirizada"})
        if falha == "corpo_truncado":
            return self._corpo_truncado()
        if self.command == "POST" and caminho == "/nfse":
            return self._postar_dps(corpo, falha)
        if self.command in ("GET", "HEAD") and caminho.startswith(ROTA_DPS):
            return self._consultar_dps(caminho[len(ROTA_DPS):])
        if self.command == "GET" and caminho.startswith(ROTA_NFSE):
            return self._consultar_nfse(caminho[len(ROTA_NFSE):])
        if caminho.startswith(PREFIXO_NAO_IMPLEMENTADO) or (caminho.startswith(ROTA_NFSE) and "/eventos" in caminho):
            return self._json(501, {"Codigo": "NAO_IMPLEMENTADO", "Descricao": "fora da NL-2.6"})
        return self._json(404, {"Codigo": "ROTA_INEXISTENTE", "Descricao": caminho})

    def _ler_corpo(self):
        tamanho = int(self.headers.get("Content-Length") or 0)
        return self.rfile.read(tamanho) if tamanho else b""

    def _postar_dps(self, corpo, falha):
        srv = self.server
        try:
            xml = envelope.decodificar(corpo)
        except envelope.ErroEnvelope as erro:
            return self._json(400, {"Codigo": "ENVELOPE_INVALIDO", "Descricao": str(erro)})
        ok_xsd, _ = srv.oraculo.validar_xsd(xml)
        if not ok_xsd:
            return self._json(400, {"Codigo": "XSD_INVALIDO", "Descricao": "fora do XSD DPS_v1.01"})
        try:
            id_dps = id_do_infdps(xml)
        except ValueError as erro:
            return self._json(400, {"Codigo": "ID_DPS_AUSENTE", "Descricao": str(erro)})
        ok_assinatura, _ = srv.oraculo.verificar_assinatura(xml)
        if not ok_assinatura:
            return self._json(400, {"Codigo": "E0714", "Descricao": "Assinatura inválida"})
        if id_dps in srv.dps_aceitas:
            return self._json(409, {"Codigo": "DPS_DUPLICADA", "Descricao": id_dps})
        chave = chave_de_teste(id_dps)
        srv.dps_aceitas[id_dps] = chave
        srv.notas[chave] = xml
        if falha == "conexao_cortada_apos_aceite":
            self.close_connection = True  # aceita e grava, mas corta a conexão sem resposta
            return None
        return self._json(201, corpo_da_nota(chave, xml))

    def _consultar_dps(self, id_dps):
        chave = self.server.dps_aceitas.get(id_dps)
        if chave is None:
            # INFERÊNCIA (ACBr, pesquisa g): 404 com E2404 quando a DPS ainda não gerou NFS-e
            return self._json(404, {"Codigo": "E2404",
                                    "Descricao": "Não foi gerada uma NFS-e com o identificador de DPS informado"})
        return self._json(200, {"chaveAcesso": chave})

    def _consultar_nfse(self, chave):
        xml = self.server.notas.get(chave)
        if xml is None:
            return self._json(404, {"Codigo": "NFSE_NAO_ENCONTRADA", "Descricao": chave})
        return self._json(200, corpo_da_nota(chave, xml))

    def _corpo_truncado(self):
        # Status 200 e Content-Length do corpo inteiro, mas só metade vai antes do corte: o cliente lê corpo incompleto.
        dados = json.dumps({"Codigo": "CORPO_TRUNCADO", "Descricao": "falha roteirizada " + "x" * 200},
                           ensure_ascii=False).encode("utf-8")
        try:
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(dados)))
            self.end_headers()
            self.wfile.write(dados[: len(dados) // 2])
            self.wfile.flush()
        except OSError:
            pass  # o cliente já desistiu: nada a fazer
        self.close_connection = True  # fecha no meio do corpo

    def _json(self, status, corpo):
        dados = json.dumps(corpo, ensure_ascii=False).encode("utf-8")
        try:
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(dados)))
            self.end_headers()
            if self.command != "HEAD":
                self.wfile.write(dados)
        except OSError:
            self.close_connection = True  # o cliente já desistiu (timeout roteirizado): nada a fazer
