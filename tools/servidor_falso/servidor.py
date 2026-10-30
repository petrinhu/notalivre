# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Servidor falso da API da Sefin Nacional, HTTPS em 127.0.0.1 (NL-2.6). Ferramenta de teste, nunca do app (L-33).

Mutual TLS: o certificado de cliente é obrigatório (CERT_REQUIRED, AC de teste). POST /nfse desfaz o envelope,
valida o XSD com xmllint e a assinatura com xmlsec1 (oráculo), e responde com a chave de 50 posições.
INFERÊNCIA até a NL-2.10: códigos de erro (exceto E0714, do Anexo I), a forma do JSON, o status 201 e a chave.
Endpoints de evento e de DFe respondem 501: fora desta fatia, sem teste, sem comportamento inventado.
"""

import base64
import gzip
import hashlib
import http.server
import json
import re
import ssl
import threading

from . import envelope
from .referencia import id_do_infdps
from .roteiro import SO_POST_NFSE, Roteiro

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


def registro_do_post(corpo):
    """Id e nDPS do que chegou num POST /nfse, para a conferência de dupla emissão (nunca o texto da DPS)."""
    try:
        xml = envelope.decodificar(corpo)
    except envelope.ErroEnvelope:
        return {"id": None, "nDPS": None}
    try:
        id_dps = id_do_infdps(xml)
    except ValueError:
        id_dps = None
    numero = re.search(rb"<nDPS>([^<]*)</nDPS>", xml)
    return {"id": id_dps, "nDPS": numero.group(1).decode("ascii", "replace") if numero else None}


def corpo_da_nota(chave, xml):
    # INFERÊNCIA: o campo leva a DPS assinada no lugar do XML da NFS-e (sem schema de NFS-e aqui).
    return {"chaveAcesso": chave, "nfseXmlGZipB64": envelope.gzip_b64(xml)}


# Teto do XML descompactado no cliente (limites_leiaute.h, DA-2.39). INFERÊNCIA do CTO, a medir na NL-2.10.
TETO_XML_NFSE = 5 * 1024 * 1024


def corpo_nfse_defeituosa(chave, falha):
    if falha == "nfse_base64_caractere_invalido":
        # "@@@@" fora do alfabeto, com tamanho múltiplo de 4: só a checagem de alfabeto recusa (DA-2.41, m7)
        return {"chaveAcesso": chave, "nfseXmlGZipB64": envelope.gzip_b64(b"<NFSe/>") + "@@@@"}
    if falha == "nfse_base64_igual_no_meio":
        # "=" no meio do Base64 (rodada 5, revisão). O gzip é escolhido com tamanho múltiplo de 3, para o Base64 não ter
        # preenchimento. Depois vem "=AAA": um decodificador que para no primeiro "=" devolve um gzip válido, e só a regra
        # de "=" só no fim recusa. Sem isso, o caso não discrimina a regra (mutante da revisão).
        dados = None
        for k in range(64):
            candidato = gzip.compress(b"<NFSe/>" + b" " * k)
            if len(candidato) % 3 == 0:
                dados = candidato
                break
        b64 = base64.b64encode(dados).decode("ascii")
        return {"chaveAcesso": chave, "nfseXmlGZipB64": b64 + "=AAA"}
    if falha == "nfse_gzip_dois_membros":
        dois = gzip.compress(b"<NFSe/>") + gzip.compress(b"<NFSe2/>")
        return {"chaveAcesso": chave, "nfseXmlGZipB64": base64.b64encode(dois).decode("ascii")}
    if falha == "nfse_base64_invalido":
        return {"chaveAcesso": chave, "nfseXmlGZipB64": envelope.gzip_b64(b"<NFSe/>") + "@"}  # "@" fora do Base64
    if falha == "nfse_gzip_invalido":
        return {"chaveAcesso": chave, "nfseXmlGZipB64": base64.b64encode(b"isto nao e gzip").decode("ascii")}
    return {"chaveAcesso": chave, "nfseXmlGZipB64": envelope.gzip_b64(b"\0" * (TETO_XML_NFSE + 1))}


class ServidorFalso(http.server.HTTPServer):
    allow_reuse_address = True

    def __init__(self, certs, oraculo, roteiro=None, tls_maximo=None):
        self.contexto = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.contexto.load_cert_chain(certs.servidor_cert, certs.servidor_key)
        self.contexto.minimum_version = ssl.TLSVersion.TLSv1_2  # explícito: nada abaixo de TLS 1.2 (rodada 2, item 4)
        # Cenário com versão máxima limitada (rodada 4, DA-2.41): "1.2" faz o servidor negociar só TLS 1.2, em que ele
        # valida o certificado do cliente antes do Finished. Nenhum outro valor é aceito.
        if tls_maximo is not None:
            if tls_maximo != "1.2":
                raise ValueError(f"tls_maximo desconhecido: {tls_maximo}")
            self.contexto.maximum_version = ssl.TLSVersion.TLSv1_2
        self.contexto.verify_mode = ssl.CERT_REQUIRED
        self.contexto.load_verify_locations(certs.ca)
        self.oraculo = oraculo
        self.roteiro = roteiro if roteiro is not None else Roteiro()
        self.dps_aceitas = {}
        self.notas = {}
        self.requisicoes = 0
        # Dupla emissão (DA-2.41): o Id e o nDPS de cada POST /nfse que chega, antes de qualquer falha roteirizada.
        self.posts_recebidos = []
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
        if self.command == "POST" and caminho == "/nfse":
            srv.posts_recebidos.append(registro_do_post(corpo))  # antes de qualquer falha: o que a Sefin recebeu
        if self.command == "GET" and caminho == "/_teste/posts":
            return self._json(200, srv.posts_recebidos)  # registro para o teste, fora da API da Sefin
        falha = srv.roteiro.consumir(self.command, caminho)
        if falha == "erro5xx":
            return self._json(503, {"Codigo": "INDISPONIVEL", "Descricao": "falha roteirizada"})
        if falha == "timeout":
            srv.liberar.wait(5)
            return self._json(504, {"Codigo": "TIMEOUT", "Descricao": "falha roteirizada"})
        if falha == "conflito409":
            return self._json(409, {"Codigo": "DPS_DUPLICADA", "Descricao": "falha roteirizada"})
        if falha is not None and falha.startswith("rejeicao:"):
            # NL-2.8: código E da recusa, e um texto livre que traz um CPF, para provar que o cliente não o leva ao log.
            codigo = falha.split(":", 1)[1]
            return self._json(400, {"Codigo": codigo,
                                    "Descricao": "texto livre devolvido pela API com CPF 12345678909"})
        if falha == "consulta_404_outro":
            # DA-2.41: a consulta respondeu 404 com um código que não é E2404 (o cliente tem de tratar como Indeterminado)
            return self._json(404, {"Codigo": "E2999", "Descricao": "fora do catálogo"})
        if falha == "corpo_truncado":
            return self._corpo_truncado()
        if falha == "eco_accept_encoding":
            return self._eco_accept_encoding()
        if falha == "content_encoding_gzip":
            return self._content_encoding_gzip()
        if falha == "redireciona":
            return self._redireciona()
        if falha == "corpo_sem_tamanho":
            return self._corpo_sem_tamanho()
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
        if falha == "redireciona_post":
            return self._redireciona()  # 302 sem processar: a DPS não foi gravada
        if falha is not None and falha.startswith("rejeicao_bruta:"):
            # Codigo com formato fora do que o cliente aceita, e um CPF no texto livre (DA-2.41, código E validado)
            return self._json(400, {"Codigo": falha.split(":", 1)[1],
                                    "Descricao": "texto livre devolvido pela API com CPF 12345678909"})
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
        if falha in SO_POST_NFSE:
            return self._json(201, corpo_nfse_defeituosa(chave, falha))
        if falha == "post_sem_chave":
            return self._json(201, {"Codigo": "OK", "Descricao": "201 sem chaveAcesso"})
        if falha == "status202":
            return self._json(202, corpo_da_nota(chave, xml))
        if falha == "post_ce_gzip":
            # 201 com Content-Encoding gzip: o transporte recusa (protocolo) depois do envio, a DPS já está gravada
            dados = gzip.compress(json.dumps(corpo_da_nota(chave, xml)).encode("utf-8"))
            return self._corpo_bruto(201, "application/json; charset=utf-8", dados, extra={"Content-Encoding": "gzip"})
        if falha is not None and falha.startswith("rejeicao_gerada:"):
            # a DPS já está gravada e a recusa traz o código E (E0014: conjunto já gerado), DA-2.41
            return self._json(400, {"Codigo": falha.split(":", 1)[1], "Descricao": "DPS já gerada"})
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

    def _eco_accept_encoding(self):
        # Devolve, como corpo, o Accept-Encoding que chegou: a conferência do cabeçalho é do lado do servidor.
        dados = self.headers.get("Accept-Encoding", "").encode("ascii", "replace")
        self._corpo_bruto(200, "text/plain; charset=us-ascii", dados)

    def _content_encoding_gzip(self):
        # 200 com Content-Encoding gzip e corpo comprimido de verdade, mesmo sem o cliente ter pedido gzip.
        dados = gzip.compress(json.dumps({"Codigo": "CONTENT_ENCODING_GZIP", "Descricao": "falha roteirizada"})
                              .encode("utf-8"))
        self._corpo_bruto(200, "application/json; charset=utf-8", dados, extra={"Content-Encoding": "gzip"})

    def _redireciona(self):
        # 302 para outra rota: o cliente que segue o Location chegaria em /dps/SEGUIDO (aqui, 404 sem a falha).
        self.send_response(302)
        self.send_header("Location", "/dps/SEGUIDO")
        self.send_header("Content-Length", "0")
        self.end_headers()

    def _corpo_sem_tamanho(self):
        # 200 sem Content-Length: com HTTP/1.0 o corpo termina no fechamento da conexão.
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.end_headers()
        self.wfile.write(b"x" * 4096)
        self.wfile.flush()
        self.close_connection = True

    def _corpo_bruto(self, status, tipo, dados, extra=None):
        self.send_response(status)
        self.send_header("Content-Type", tipo)
        self.send_header("Content-Length", str(len(dados)))
        for nome, valor in (extra or {}).items():
            self.send_header(nome, valor)
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(dados)

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
