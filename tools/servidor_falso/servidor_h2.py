# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Servidor falso que recusa HTTP/2 com HTTP_1_1_REQUIRED (NL-2.10e, E5, DA-2.80).

Só a biblioteca padrão do Python (ssl, socket, struct). Nenhum pacote novo (L-51).

Comportamento, como o Fm-s/open-nfse relata da Sefin (referência, L-37): se o cliente negocia h2 por ALPN, o servidor aceita o
prefácio e as configurações e recusa o primeiro HEADERS com RST_STREAM de código 0xd (HTTP_1_1_REQUIRED). Se o cliente negocia
http/1.1, responde 200 com um corpo JSON mínimo.

Saída (stdout): "PORTA <n>" ao subir; "ALPN <valor>" por conexão, com o protocolo que o servidor escolheu; "RECUSA_H2 <stream>" quando
manda o RST_STREAM; "HTTP200" quando responde em HTTP/1.1.

Uso: servidor_h2.py <pasta-de-fixtures>   (usa ac.pem, servidor.pem e servidor.key da pasta)
"""

import os
import socket
import ssl
import struct
import sys
import threading

PREFACIO = b"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
HTTP_1_1_REQUIRED = 0xD
RESPOSTA_HTTP11 = (b"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 2\r\n"
                   b"Connection: close\r\n\r\n{}")


def _ler_exato(conexao, n):
    dados = b""
    while len(dados) < n:
        bloco = conexao.recv(n - len(dados))
        if not bloco:
            raise EOFError("conexão fechada antes do fim do quadro")
        dados += bloco
    return dados


def _quadro(tipo, flags, stream, carga):
    return struct.pack(">I", len(carga))[1:] + struct.pack(">BBI", tipo, flags, stream & 0x7FFFFFFF) + carga


def _atender_h2(conexao):
    # Prefácio do cliente, depois quadros. Recusa o primeiro HEADERS com RST_STREAM(HTTP_1_1_REQUIRED).
    if _ler_exato(conexao, len(PREFACIO)) != PREFACIO:
        return
    conexao.sendall(_quadro(0x4, 0x0, 0, b""))  # SETTINGS do servidor, vazio
    while True:
        cabecalho = _ler_exato(conexao, 9)
        tamanho = struct.unpack(">I", b"\x00" + cabecalho[:3])[0]
        tipo, flags, stream = cabecalho[3], cabecalho[4], struct.unpack(">I", cabecalho[5:9])[0] & 0x7FFFFFFF
        _ler_exato(conexao, tamanho) if tamanho else b""
        if tipo == 0x4 and not (flags & 0x1):
            conexao.sendall(_quadro(0x4, 0x1, 0, b""))  # ACK das configurações do cliente
        elif tipo == 0x1:  # HEADERS
            print(f"RECUSA_H2 {stream}", flush=True)
            conexao.sendall(_quadro(0x3, 0x0, stream, struct.pack(">I", HTTP_1_1_REQUIRED)))
            return


def _atender_http11(conexao):
    buffer = b""
    while b"\r\n\r\n" not in buffer:
        bloco = conexao.recv(4096)
        if not bloco:
            return
        buffer += bloco
    conexao.sendall(RESPOSTA_HTTP11)
    print("HTTP200", flush=True)


def _atender(bruto, contexto):
    try:
        conexao = contexto.wrap_socket(bruto, server_side=True)
    except (ssl.SSLError, OSError):
        bruto.close()
        return
    try:
        alpn = conexao.selected_alpn_protocol()
        print(f"ALPN {alpn or 'nenhum'}", flush=True)
        if alpn == "h2":
            _atender_h2(conexao)
        else:
            _atender_http11(conexao)
    except (EOFError, OSError, ssl.SSLError):
        pass
    finally:
        conexao.close()


def main():
    pasta = sys.argv[1]
    contexto = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    contexto.minimum_version = ssl.TLSVersion.TLSv1_2
    contexto.load_cert_chain(os.path.join(pasta, "servidor.pem"), os.path.join(pasta, "servidor.key"))
    if "--tls11" in sys.argv[2:]:
        # NL-2.10e (R-B3, piso TLS 1.2): o servidor fala só TLS 1.1. O cliente que aceita abaixo de 1.2 completa o handshake; o que
        # respeita o piso recusa. SECLEVEL=0 é preciso para o OpenSSL 3 negociar 1.1 do lado do servidor (teste, nunca produção).
        contexto.minimum_version = ssl.TLSVersion.TLSv1_1
        contexto.maximum_version = ssl.TLSVersion.TLSv1_1
        contexto.set_ciphers("DEFAULT:@SECLEVEL=0")
    else:
        contexto.set_alpn_protocols(["h2", "http/1.1"])
    servidor = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    servidor.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    servidor.bind(("127.0.0.1", 0))
    servidor.listen(16)
    print(f"PORTA {servidor.getsockname()[1]}", flush=True)
    while True:
        bruto, _ = servidor.accept()
        threading.Thread(target=_atender, args=(bruto, contexto), daemon=True).start()


if __name__ == "__main__":
    main()
