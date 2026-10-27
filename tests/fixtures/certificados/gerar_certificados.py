#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Gerador das fixtures de certificados de teste da NL-2.1 (roda dentro do contêiner, nunca no repositório).

Uso: python3 -I gerar_certificados.py DIR_SAIDA

Cria em DIR_SAIDA os 20 arquivos do inventário fixo (ver test_certificados.py): uma AC de teste, certificados
de cliente (CNPJ no otherName 2.16.76.1.3.3, CPF no 2.16.76.1.3.1, EKU clientAuth ou sem EKU, vencido,
revogado com LCR, outro CNPJ raiz, CNPJ alfanumérico), .pfx moderno (PBES2 e AES-256) de cada cliente, um
.pfx legado (RC2-40 no cert bag e 3DES no key bag) e o certificado do servidor falso (SAN localhost e
127.0.0.1, EKU serverAuth).

Senha de teste: aleatória a cada execução, gravada em senha_teste.txt (0600). Não há senha literal no script.
A AC privada e as chaves dos clientes vivem numa pasta de trabalho (estado_ac) que é apagada ao fim.
Chamadas ao openssl: número fixo (cerca de 40), sequenciais, por argv (nunca por shell).
"""
import os
import secrets
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

VAR_SENHA = "NL_SENHA_TESTE"
N_ARQUIVOS = 20
ESTADO = "estado_ac"
SUBJ_AC = "/O=NotaLivre teste/CN=AC TESTE NOTALIVRE NAO USAR"

CNPJ_OK = "12345678000195"
CNPJ_OUTRO_RAIZ = "98765432000198"
CNPJ_ALFANUMERICO = "12ABC34501DE35"
# DOC-ICP-04 v8.0, 7.1.2.3: data (8), CPF (11), NIS (11), RG (15), órgão/UF (10). CPF nas posições 9 a 19.
CONTEUDO_CPF = "01011990" + "12345678909" + "00000000000" + "000000000000000" + "SSPTESTE00"

OID_CNPJ = "2.16.76.1.3.3"
OID_CPF = "2.16.76.1.3.1"


@dataclass(frozen=True)
class Cliente:
    nome: str
    oid: str
    valor: str
    clientauth: bool
    situacao: str  # ok, vencido ou revogado


CLIENTES = (
    Cliente("cnpj_ok", OID_CNPJ, CNPJ_OK, True, "ok"),
    Cliente("cpf_ok", OID_CPF, CONTEUDO_CPF, True, "ok"),
    Cliente("sem_clientauth", OID_CNPJ, CNPJ_OK, False, "ok"),
    Cliente("vencido", OID_CNPJ, CNPJ_OK, True, "vencido"),
    Cliente("revogado", OID_CNPJ, CNPJ_OK, True, "revogado"),
    Cliente("outro_cnpj_raiz", OID_CNPJ, CNPJ_OUTRO_RAIZ, True, "ok"),
    Cliente("cnpj_alfanumerico", OID_CNPJ, CNPJ_ALFANUMERICO, True, "ok"),
)

# Datas do certificado vencido: já expirado desde 2021 (openssl ca aceita a janela no passado).
VENCIDO_INICIO = "20200101000000Z"
VENCIDO_FIM = "20210101000000Z"


def rodar(argv, env=None):
    resultado = subprocess.run(argv, capture_output=True, text=True, env=env, check=False)
    if resultado.returncode != 0:
        sys.stderr.write(f"FALHA ({argv[0]} {argv[1]}) rc={resultado.returncode}\n{resultado.stderr}")
        sys.exit(1)
    return resultado


def escrever_configuracao(saida, estado):
    """Config do openssl ca e das extensões de cada perfil. Um perfil por cliente, mais o do servidor e da AC."""
    linhas = [
        "[ca]", "default_ca = CA_TESTE",
        "[CA_TESTE]",
        f"dir = {estado}",
        f"database = {estado}/index.txt",
        f"new_certs_dir = {estado}/newcerts",
        f"serial = {estado}/serial",
        f"crlnumber = {estado}/crlnumber",
        f"certificate = {saida}/ac.pem",
        f"private_key = {estado}/ac.key",
        "default_md = sha256", "default_days = 365", "default_crl_days = 30",
        "unique_subject = no", "policy = policy_any", "copy_extensions = none",
        "[policy_any]", "commonName = supplied",
        "[req]", "distinguished_name = dn", "prompt = no",
        "[dn]", "CN = AC TESTE NOTALIVRE NAO USAR",
        "[ext_ac]",
        "basicConstraints = critical,CA:TRUE,pathlen:0",
        "keyUsage = critical,keyCertSign,cRLSign",
        "subjectKeyIdentifier = hash",
        "authorityKeyIdentifier = keyid:always",
        "[ext_servidor]",
        "basicConstraints = CA:FALSE",
        "keyUsage = critical,digitalSignature,keyEncipherment",
        "extendedKeyUsage = serverAuth",
        "subjectKeyIdentifier = hash",
        "authorityKeyIdentifier = keyid:always",
        "subjectAltName = DNS:localhost,IP:127.0.0.1",
    ]
    for c in CLIENTES:
        uso = "digitalSignature,nonRepudiation,keyEncipherment" if c.clientauth else "digitalSignature"
        linhas += [
            f"[ext_{c.nome}]",
            "basicConstraints = critical,CA:FALSE",
            f"keyUsage = critical,{uso}",
        ]
        if c.clientauth:
            linhas.append("extendedKeyUsage = clientAuth")
        linhas += [
            "subjectKeyIdentifier = hash",
            "authorityKeyIdentifier = keyid:always",
            f"subjectAltName = otherName:{c.oid};OCT:{c.valor}",
        ]
    (estado / "ac.cnf").write_text("\n".join(linhas) + "\n", encoding="ascii")
    return estado / "ac.cnf"


def gerar_ac(saida, estado, cnf, env):
    rodar(["openssl", "genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048",
           "-out", str(estado / "ac.key")])
    rodar(["openssl", "req", "-x509", "-new", "-key", str(estado / "ac.key"), "-config", str(cnf),
           "-extensions", "ext_ac", "-days", "3650", "-subj", SUBJ_AC, "-out", str(saida / "ac.pem")], env)


def emitir(cnf, estado, nome, chave_pem, csr_subj, extensao, saida_pem, env, datas=()):
    csr = estado / (nome + ".csr")
    rodar(["openssl", "genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048", "-out", str(chave_pem)])
    rodar(["openssl", "req", "-new", "-key", str(chave_pem), "-subj", csr_subj, "-out", str(csr)], env)
    rodar(["openssl", "ca", "-batch", "-notext", "-config", str(cnf), "-extensions", extensao,
           *datas, "-in", str(csr), "-out", str(saida_pem)], env)


def gerar_clientes(saida, estado, cnf, env):
    for c in CLIENTES:
        datas = ("-startdate", VENCIDO_INICIO, "-enddate", VENCIDO_FIM) if c.situacao == "vencido" else ()
        emitir(cnf, estado, c.nome, estado / (c.nome + ".key"), f"/O=TESTE NOTALIVRE/CN=TESTE {c.nome}",
               f"ext_{c.nome}", saida / (c.nome + ".pem"), env, datas)
    revogado = next(c for c in CLIENTES if c.situacao == "revogado")
    rodar(["openssl", "ca", "-batch", "-config", str(cnf), "-revoke", str(saida / (revogado.nome + ".pem"))], env)
    rodar(["openssl", "ca", "-batch", "-config", str(cnf), "-gencrl", "-out", str(saida / "crl.pem")], env)


def gerar_servidor(saida, estado, cnf, env):
    emitir(cnf, estado, "servidor", saida / "servidor.key", "/O=TESTE NOTALIVRE/CN=localhost",
           "ext_servidor", saida / "servidor.pem", env)


def exportar_pfx(saida, estado, nome, env, legado=False):
    cifras = ["-certpbe", "PBE-SHA1-RC2-40", "-keypbe", "PBE-SHA1-3DES"] if legado else \
             ["-certpbe", "AES-256-CBC", "-keypbe", "AES-256-CBC"]
    # -legacy vem depois de "pkcs12": é opção do subcomando, não do openssl.
    argv = ["openssl", "pkcs12"] + (["-legacy"] if legado else []) + ["-export", "-inkey", str(estado / (nome + ".key")),
                   "-in", str(saida / (nome + ".pem")), "-certfile", str(saida / "ac.pem"),
                   "-name", "TESTE " + nome, "-passout", f"env:{VAR_SENHA}", *cifras,
                   "-out", str(saida / (nome + ("_legado" if legado else "") + ".pfx"))]
    rodar(argv, env)


def gerar(saida):
    saida.mkdir(parents=True, exist_ok=True)
    for c in CLIENTES:
        (saida / (c.nome + ".pem")).unlink(missing_ok=True)
        (saida / (c.nome + ".pfx")).unlink(missing_ok=True)
    for nome in ("ac.pem", "crl.pem", "servidor.pem", "servidor.key", "cnpj_ok_legado.pfx", "senha_teste.txt"):
        (saida / nome).unlink(missing_ok=True)

    estado = saida / ESTADO
    if estado.exists():
        shutil.rmtree(estado)
    (estado / "newcerts").mkdir(parents=True, mode=0o700)
    (estado / "index.txt").write_text("", encoding="ascii")
    (estado / "serial").write_text("1000\n", encoding="ascii")
    (estado / "crlnumber").write_text("01\n", encoding="ascii")

    senha = secrets.token_urlsafe(24)
    senha_arq = saida / "senha_teste.txt"
    senha_arq.write_text(senha + "\n", encoding="ascii")
    os.chmod(senha_arq, 0o600)
    env = dict(os.environ, **{VAR_SENHA: senha})

    try:
        cnf = escrever_configuracao(saida, estado)
        gerar_ac(saida, estado, cnf, env)
        gerar_clientes(saida, estado, cnf, env)
        gerar_servidor(saida, estado, cnf, env)
        for c in CLIENTES:
            exportar_pfx(saida, estado, c.nome, env)
        exportar_pfx(saida, estado, "cnpj_ok", env, legado=True)
    finally:
        shutil.rmtree(estado, ignore_errors=True)

    presentes = sorted(p.name for p in saida.iterdir())
    if len(presentes) != N_ARQUIVOS:
        sys.stderr.write(f"FALHA: {len(presentes)} entradas em {saida}, esperado {N_ARQUIVOS}: {presentes}\n")
        return 1
    print(f"gerados={len(presentes)} saida={saida}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("uso: gerar_certificados.py DIR_SAIDA")
    sys.exit(gerar(Path(sys.argv[1])))
