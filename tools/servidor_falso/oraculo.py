# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Oráculo independente do servidor falso: xmllint (XSD) e xmlsec1 (assinatura), sempre por processo próprio.

Uma chamada por requisição, em série (L-11). Versão do xmlsec1 detectada: na 1.3 a busca de chave é estrita e a
chave só acha a AC com --lax-key-search depois do comando (DA-2.10). Na 1.2 a opção não existe.

ARNÊS DO FEDORA44 (DA-2.10, L-77): a política de cripto da imagem fedora44 recusa RSA-SHA-1. Só nesse ambiente, a
variável OPENSSL_ENABLE_SHA1_SIGNATURES=1 entra no ambiente do xmlsec1, e a linha abaixo vai para o log. Nenhum outro
arquivo do repositório pode citar a variável (tools/portoes/portao_sha1_env.py reprova).

Armadilha medida: a 1.3.9 com a opção antes do comando sai com rc 0 e não grava o arquivo de saída. Por isso a
assinatura confere que o arquivo existe e não está vazio, além do rc.
"""

import os
import re
import subprocess
import sys
import tempfile

ENV_SHA1 = "OPENSSL_ENABLE_SHA1_SIGNATURES"
ID_ATTR = ["--id-attr:Id", "infDPS"]


class ErroOraculo(RuntimeError):
    """O oráculo não produziu o resultado esperado (ferramenta ausente, saída inválida)."""


def versao_xmlsec1(binario):
    """Devolve (maior, menor) da linha 'xmlsec1 X.Y.Z' de --version."""
    r = subprocess.run([binario, "--version"], capture_output=True, text=True)
    m = re.search(r"xmlsec1 (\d+)\.(\d+)", r.stdout + r.stderr)
    if not m:
        raise ErroOraculo(f"não consegui ler a versão de {binario}")
    return int(m.group(1)), int(m.group(2))


def argumentos_lax(versao):
    """Opção de busca de chave por versão: só a linha 1.3 precisa (e aceita) --lax-key-search."""
    return ["--lax-key-search"] if versao >= (1, 3) else []


def ler_os_release(caminho="/etc/os-release"):
    dados = {}
    with open(caminho, encoding="utf-8") as f:
        for linha in f:
            if "=" in linha:
                chave, valor = linha.strip().split("=", 1)
                dados[chave] = valor.strip('"')
    return dados


def ambiente_do_arnes(os_release):
    """Variáveis extras do oráculo. Só o fedora44 recebe a variável de SHA-1 (DA-2.10)."""
    if os_release.get("ID") == "fedora" and os_release.get("VERSION_ID") == "44":
        return {ENV_SHA1: "1"}
    return {}


class Oraculo:
    def __init__(self, xmllint, xmlsec1, xsd_raiz, ca_pem, diretorio_trabalho, os_release=None):
        self.xmllint = xmllint
        self.xmlsec1 = xmlsec1
        self.xsd_raiz = xsd_raiz
        self.ca_pem = ca_pem
        self.trabalho = diretorio_trabalho
        os.makedirs(self.trabalho, exist_ok=True)
        self.versao = versao_xmlsec1(xmlsec1)
        self.ambiente = ambiente_do_arnes(os_release if os_release is not None else ler_os_release())
        if self.ambiente:
            print(f"oraculo: {ENV_SHA1}=1 (arnês fedora44, DA-2.10)", file=sys.stderr, flush=True)

    def _env(self):
        """Ambiente do xmlsec1: o do processo sem a variável de SHA-1, mais o do arnês (só no fedora44)."""
        env = {chave: valor for chave, valor in os.environ.items() if chave != ENV_SHA1}
        env.update(self.ambiente)
        return env

    def _escrever(self, dados, nome):
        fd, caminho = tempfile.mkstemp(prefix=f"{nome}-", suffix=".xml", dir=self.trabalho)
        with os.fdopen(fd, "wb") as f:
            f.write(dados)
        return caminho

    def validar_xsd(self, xml):
        """xmllint --schema sobre o XML (stdin). Devolve (ok, saída de erro)."""
        r = subprocess.run([self.xmllint, "--noout", "--nonet", "--schema", self.xsd_raiz, "-"],
                           input=xml, capture_output=True)
        return r.returncode == 0, r.stderr.decode("utf-8", errors="replace")

    def comando_verificar(self, caminho, chaves_publicas=()):
        """Comando de verificação. chaves_publicas (certificados PEM) entram no armazém de chaves do xmlsec1.

        Por padrão a chamada não passa chave: só a AC de teste em --trusted-pem. Os testes de cadeia sem KeyInfo
        passam o certificado do emissor (DA-2.27); na 1.3 a busca relaxada é obrigatória para essa cadeia.
        """
        chaves = [arg for pem in chaves_publicas for arg in ("--pubkey-cert-pem", pem)]
        return [self.xmlsec1, "--verify", *argumentos_lax(self.versao), *ID_ATTR,
                "--trusted-pem", self.ca_pem, *chaves, caminho]

    def verificar_assinatura(self, xml, chaves_publicas=()):
        """xmlsec1 --verify com a AC de teste. Devolve (ok, saída)."""
        caminho = self._escrever(xml, "verificar")
        r = subprocess.run(self.comando_verificar(caminho, chaves_publicas), env=self._env(),
                           capture_output=True, text=True)
        return r.returncode == 0, r.stdout + r.stderr
