# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Certificados de teste do servidor falso (NL-2.6).

PONTO ÚNICO: a NL-2.1 substitui obter_certificados() pelas fixtures de AC de teste dela. Nada aqui é versionado:
cada chamada gera, num diretório descartável do build, uma AC de teste, um certificado de servidor (SAN localhost e
127.0.0.1) e um certificado de cliente com EKU clientAuth. Não são certificados ICP-Brasil.
"""

import collections
import os
import subprocess

Certificados = collections.namedtuple("Certificados", "ca servidor_cert servidor_key cliente_cert cliente_key")

NOME_CLIENTE = "TESTE NOTALIVRE CLIENTE"
DIAS = "2"


def _executar(args):
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"comando falhou ({r.returncode}): {' '.join(args)}\n{r.stderr}")


def _emitir(diretorio, nome, cn, extensoes):
    """Gera chave e certificado $nome assinados pela AC de $diretorio (ca.key e ca.pem já existem)."""
    p = lambda arquivo: os.path.join(diretorio, arquivo)
    with open(p(f"ext_{nome}.cnf"), "w", encoding="ascii") as f:
        f.write(extensoes)
    _executar(["openssl", "req", "-newkey", "rsa:2048", "-nodes", "-keyout", p(f"{nome}.key"),
               "-out", p(f"{nome}.csr"), "-subj", f"/CN={cn}"])
    _executar(["openssl", "x509", "-req", "-in", p(f"{nome}.csr"), "-CA", p("ca.pem"), "-CAkey", p("ca.key"),
               "-CAserial", p("ca.srl"), "-CAcreateserial", "-out", p(f"{nome}.pem"), "-days", DIAS,
               "-extfile", p(f"ext_{nome}.cnf")])


def obter_certificados(diretorio):
    """Gera a AC, o servidor e o cliente de teste em $diretorio e devolve os caminhos. Ponto único (NL-2.1 troca)."""
    os.makedirs(diretorio, exist_ok=True)
    p = lambda arquivo: os.path.join(diretorio, arquivo)
    _executar(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", p("ca.key"),
               "-out", p("ca.pem"), "-days", DIAS, "-subj", "/CN=AC de teste NotaLivre",
               "-addext", "basicConstraints=critical,CA:TRUE",
               "-addext", "keyUsage=critical,keyCertSign,cRLSign"])
    _emitir(diretorio, "servidor", "localhost",
            "basicConstraints=CA:FALSE\n"
            "keyUsage=critical,digitalSignature,keyEncipherment\n"
            "extendedKeyUsage=serverAuth\n"
            "subjectAltName=DNS:localhost,IP:127.0.0.1\n")
    _emitir(diretorio, "cliente", NOME_CLIENTE,
            "basicConstraints=CA:FALSE\n"
            "keyUsage=critical,digitalSignature,keyEncipherment\n"
            "extendedKeyUsage=clientAuth\n")
    return Certificados(
        ca=p("ca.pem"),
        servidor_cert=p("servidor.pem"),
        servidor_key=p("servidor.key"),
        cliente_cert=p("cliente.pem"),
        cliente_key=p("cliente.key"),
    )
