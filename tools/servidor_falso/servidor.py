# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Servidor falso da API, HTTPS em 127.0.0.1 (NL-2.6). ESQUELETO do commit vermelho: responde 501 a tudo e não exige
certificado de cliente. A implementação entra no commit verde, com a mesma interface.
"""

import http.server
import ssl
import threading

from .roteiro import Roteiro


class ServidorFalso(http.server.HTTPServer):
    allow_reuse_address = True

    def __init__(self, certs, oraculo, roteiro=None):
        self.contexto = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.contexto.load_cert_chain(certs.servidor_cert, certs.servidor_key)
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
        self.server.requisicoes += 1
        self.send_response(501)
        self.send_header("Content-Length", "0")
        self.end_headers()
