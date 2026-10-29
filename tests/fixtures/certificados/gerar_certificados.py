#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Gerador das fixtures de certificados de teste da NL-2.1 (roda dentro do contêiner, nunca no repositório).

Uso: python3 -I gerar_certificados.py DIR_SAIDA

Cria em DIR_SAIDA os 29 arquivos do inventário fixo (ver test_certificados.py): uma AC de teste (raiz, com
pathlen 1), uma AC subordinada de teste (pathlen 0, assinada pela AC), certificados de cliente (CNPJ no otherName
2.16.76.1.3.3, CPF no 2.16.76.1.3.1, EKU clientAuth, EKU sem clientAuth, sem EKU, sem otherName, vencido, revogado
com LCR, outro CNPJ raiz, CNPJ alfanumérico), .pfx moderno (PBES2 e AES-256) de cada cliente, um .pfx legado
(RC2-40 no cert bag e 3DES no key bag), o certificado do servidor falso (SAN localhost e 127.0.0.1, EKU
serverAuth), um certificado autoassinado de outra raiz e dois .pfx de cadeia (com a subordinada completa, e sem
ela, para a cadeia incompleta).

Senha de teste: aleatória a cada execução, gravada em senha_teste.txt (0600). Não há senha literal no script.
A AC privada e as chaves dos clientes vivem numa pasta de trabalho (estado_ac) que é apagada ao fim.
Chamadas ao openssl: sequenciais, por argv (nunca por shell); nenhum laço gasta processo por item além dos perfis.
"""
import os
import secrets
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

VAR_SENHA = "NL_SENHA_TESTE"
N_ARQUIVOS = 29
ESTADO = "estado_ac"
SUBJ_AC = "/O=NotaLivre teste/CN=AC TESTE NOTALIVRE NAO USAR"
SUBJ_SUBAC = "/O=NotaLivre teste/CN=AC TESTE SUBORDINADA NAO USAR"
SUBJ_OUTRA_RAIZ = "/O=TESTE NOTALIVRE/CN=TESTE cadeia_outra_raiz"

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
    oid: "str | None"   # None: certificado sem otherName de identificação (SemInscricao)
    valor: str
    eku: str            # "clientauth", "emailprotection" ou "ausente"
    situacao: str       # ok, vencido ou revogado


CLIENTES = (
    Cliente("cnpj_ok", OID_CNPJ, CNPJ_OK, "clientauth", "ok"),
    Cliente("cpf_ok", OID_CPF, CONTEUDO_CPF, "clientauth", "ok"),
    Cliente("sem_clientauth", OID_CNPJ, CNPJ_OK, "emailprotection", "ok"),
    Cliente("vencido", OID_CNPJ, CNPJ_OK, "clientauth", "vencido"),
    Cliente("revogado", OID_CNPJ, CNPJ_OK, "clientauth", "revogado"),
    Cliente("outro_cnpj_raiz", OID_CNPJ, CNPJ_OUTRO_RAIZ, "clientauth", "ok"),
    Cliente("cnpj_alfanumerico", OID_CNPJ, CNPJ_ALFANUMERICO, "clientauth", "ok"),
    Cliente("eku_ausente", OID_CNPJ, CNPJ_OK, "ausente", "ok"),
    Cliente("sem_identificacao", None, "", "clientauth", "ok"),
)

# Datas do certificado vencido: já expirado desde 2021 (openssl ca aceita a janela no passado).
VENCIDO_INICIO = "20200101000000Z"
VENCIDO_FIM = "20210101000000Z"


def gravar_senha(caminho, senha):
    """Grava a senha de teste já com modo 0600 na criação: sem chmod depois, não existe janela com 0644."""
    caminho.unlink(missing_ok=True)
    fd = os.open(caminho, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="ascii") as arquivo:
        arquivo.write(senha + "\n")


def rodar(argv, env=None):
    resultado = subprocess.run(argv, capture_output=True, text=True, env=env, check=False)
    if resultado.returncode != 0:
        sys.stderr.write(f"FALHA ({argv[0]} {argv[1]}) rc={resultado.returncode}\n{resultado.stderr}")
        sys.exit(1)
    return resultado


def escrever_configuracao(saida, estado):
    """Config do openssl ca e das extensões de cada perfil. Um perfil por cliente, mais o do servidor, da AC e
    da subordinada. A AC raiz tem pathlen 1 para admitir uma subordinada; a subordinada tem pathlen 0."""
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
        "basicConstraints = critical,CA:TRUE,pathlen:1",
        "keyUsage = critical,keyCertSign,cRLSign",
        "subjectKeyIdentifier = hash",
        "authorityKeyIdentifier = keyid:always",
        "[ext_subac]",
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
        uso = "digitalSignature,nonRepudiation,keyEncipherment" if c.eku == "clientauth" else "digitalSignature"
        linhas += [
            f"[ext_{c.nome}]",
            "basicConstraints = critical,CA:FALSE",
            f"keyUsage = critical,{uso}",
        ]
        if c.eku == "clientauth":
            linhas.append("extendedKeyUsage = clientAuth")
        elif c.eku == "emailprotection":
            linhas.append("extendedKeyUsage = emailProtection")
        linhas += [
            "subjectKeyIdentifier = hash",
            "authorityKeyIdentifier = keyid:always",
        ]
        if c.oid is not None:
            linhas.append(f"subjectAltName = otherName:{c.oid};OCT:{c.valor}")
    (estado / "ac.cnf").write_text("\n".join(linhas) + "\n", encoding="ascii")
    return estado / "ac.cnf"


def gerar_ac(saida, estado, cnf, env):
    rodar(["openssl", "genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048",
           "-out", str(estado / "ac.key")])
    rodar(["openssl", "req", "-x509", "-new", "-key", str(estado / "ac.key"), "-config", str(cnf),
           "-extensions", "ext_ac", "-days", "3650", "-subj", SUBJ_AC, "-out", str(saida / "ac.pem")], env)


def gerar_subordinada(saida, estado, cnf, env):
    """AC subordinada (pathlen 0) assinada pela AC raiz de teste, por openssl x509 -req (sem banco de emissões)."""
    rodar(["openssl", "genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048",
           "-out", str(estado / "sub_ac.key")])
    rodar(["openssl", "req", "-new", "-key", str(estado / "sub_ac.key"), "-subj", SUBJ_SUBAC,
           "-out", str(estado / "sub_ac.csr")], env)
    rodar(["openssl", "x509", "-req", "-in", str(estado / "sub_ac.csr"), "-CA", str(saida / "ac.pem"),
           "-CAkey", str(estado / "ac.key"), "-set_serial", "0x2001", "-days", "365",
           "-extfile", str(cnf), "-extensions", "ext_subac", "-out", str(saida / "sub_ac.pem")], env)


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


def gerar_cadeias(saida, estado, cnf, env):
    """Folha CNPJ assinada pela subordinada (cnpj_sub); autoassinado de outra raiz (cadeia_outra_raiz)."""
    rodar(["openssl", "genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048",
           "-out", str(estado / "cnpj_sub.key")])
    rodar(["openssl", "req", "-new", "-key", str(estado / "cnpj_sub.key"),
           "-subj", "/O=TESTE NOTALIVRE/CN=TESTE cnpj_sub", "-out", str(estado / "cnpj_sub.csr")], env)
    rodar(["openssl", "x509", "-req", "-in", str(estado / "cnpj_sub.csr"), "-CA", str(saida / "sub_ac.pem"),
           "-CAkey", str(estado / "sub_ac.key"), "-set_serial", "0x3001", "-days", "365",
           "-extfile", str(cnf), "-extensions", "ext_cnpj_ok", "-out", str(estado / "cnpj_sub.pem")], env)
    # Cadeia completa para o .pfx: subordinada e raiz de teste, nesta ordem. Só existe no estado (apagado ao fim).
    (estado / "cadeia_sub.pem").write_text(
        (saida / "sub_ac.pem").read_text(encoding="ascii") + (saida / "ac.pem").read_text(encoding="ascii"),
        encoding="ascii")
    # Autoassinado de outra raiz: CNPJ no otherName, EKU clientAuth, sem relação com a AC de teste.
    rodar(["openssl", "req", "-x509", "-new", "-config", str(cnf), "-newkey", "rsa:2048", "-nodes",
           "-keyout", str(estado / "cadeia_outra_raiz.key"), "-out", str(saida / "cadeia_outra_raiz.pem"),
           "-days", "365", "-subj", SUBJ_OUTRA_RAIZ,
           "-addext", "basicConstraints=critical,CA:FALSE",
           "-addext", "keyUsage=critical,digitalSignature",
           "-addext", "extendedKeyUsage=clientAuth",
           "-addext", f"subjectAltName=otherName:{OID_CNPJ};OCT:{CNPJ_OUTRO_RAIZ}"], env)


def exportar_pfx(saida, nome_saida, cert, chave, env, certfile=None, legado=False):
    """Exporta um .pfx. certfile: arquivo com a cadeia a incluir (None = sem cadeia no contêiner)."""
    cifras = ["-certpbe", "PBE-SHA1-RC2-40", "-keypbe", "PBE-SHA1-3DES"] if legado else \
             ["-certpbe", "AES-256-CBC", "-keypbe", "AES-256-CBC"]
    # -legacy vem depois de "pkcs12": é opção do subcomando, não do openssl.
    argv = ["openssl", "pkcs12"] + (["-legacy"] if legado else []) + ["-export", "-inkey", str(chave),
                   "-in", str(cert), "-name", "TESTE " + nome_saida, "-passout", f"env:{VAR_SENHA}", *cifras]
    if certfile is not None:
        argv += ["-certfile", str(certfile)]
    argv += ["-out", str(saida / nome_saida)]
    rodar(argv, env)


def exportar_todos(saida, estado, env):
    for c in CLIENTES:
        exportar_pfx(saida, c.nome + ".pfx", saida / (c.nome + ".pem"), estado / (c.nome + ".key"), env,
                     certfile=saida / "ac.pem")
    exportar_pfx(saida, "cnpj_ok_legado.pfx", saida / "cnpj_ok.pem", estado / "cnpj_ok.key", env,
                 certfile=saida / "ac.pem", legado=True)
    exportar_pfx(saida, "cadeia_outra_raiz.pfx", saida / "cadeia_outra_raiz.pem",
                 estado / "cadeia_outra_raiz.key", env)
    exportar_pfx(saida, "cnpj_sub.pfx", estado / "cnpj_sub.pem", estado / "cnpj_sub.key", env,
                 certfile=estado / "cadeia_sub.pem")
    exportar_pfx(saida, "cnpj_sub_sem_intermediaria.pfx", estado / "cnpj_sub.pem", estado / "cnpj_sub.key", env)


def nomes_de_saida():
    """Nomes do inventário que o gerador grava em DIR_SAIDA (usado para limpar execução anterior)."""
    nomes = ["ac.pem", "sub_ac.pem", "crl.pem", "servidor.pem", "servidor.key", "cnpj_ok_legado.pfx",
             "cadeia_outra_raiz.pem", "cadeia_outra_raiz.pfx", "cnpj_sub.pfx", "cnpj_sub_sem_intermediaria.pfx",
             "senha_teste.txt"]
    for c in CLIENTES:
        nomes += [c.nome + ".pem", c.nome + ".pfx"]
    return nomes


def gerar(saida):
    saida.mkdir(parents=True, exist_ok=True)
    for nome in nomes_de_saida():
        (saida / nome).unlink(missing_ok=True)

    estado = saida / ESTADO
    if estado.exists():
        shutil.rmtree(estado)
    (estado / "newcerts").mkdir(parents=True, mode=0o700)
    (estado / "index.txt").write_text("", encoding="ascii")
    (estado / "serial").write_text("1000\n", encoding="ascii")
    (estado / "crlnumber").write_text("01\n", encoding="ascii")

    senha = secrets.token_urlsafe(24)
    gravar_senha(saida / "senha_teste.txt", senha)
    env = dict(os.environ, **{VAR_SENHA: senha})

    try:
        cnf = escrever_configuracao(saida, estado)
        gerar_ac(saida, estado, cnf, env)
        gerar_subordinada(saida, estado, cnf, env)
        gerar_clientes(saida, estado, cnf, env)
        gerar_servidor(saida, estado, cnf, env)
        gerar_cadeias(saida, estado, cnf, env)
        exportar_todos(saida, estado, env)
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
