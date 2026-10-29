# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da NL-2.1 (fixtures de certificados de teste). Stdlib, python3 -I, um processo por teste:
#   python3 -I test_certificados.py <nome-do-teste>
# Os certificados são gerados por gerar_certificados.py na preparação do teste (fixture do CTest), em
# NL_CERTIFICADOS_DIR. Nada é versionado. Os valores esperados (CNPJ, CPF, posições, nomes de algoritmo)
# ficam AQUI, escritos à mão, e não são importados do gerador: golden gerado pelo próprio código não prova nada.
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import importlib.util

DIR = Path(os.environ.get("NL_CERTIFICADOS_DIR", ""))
VAR_SENHA = "NL_SENHA_TESTE"

CNPJ_OK = "12345678000195"
CNPJ_OUTRO_RAIZ = "98765432000198"
CNPJ_ALFANUMERICO = "12ABC34501DE35"
CPF_OK = "12345678909"
# Layout do DOC-ICP-04 v8.0, 7.1.2.3, otherName 2.16.76.1.3.1: data (8), CPF (11), NIS (11), RG (15), órgão/UF (10).
CONTEUDO_CPF = "01011990" + CPF_OK + "00000000000" + "000000000000000" + "SSPTESTE00"

# EKU clientAuth presente (sem_identificacao tem EKU, mas não tem otherName: SemInscricao).
CLIENTES_COM_CLIENTAUTH = ["cnpj_ok", "cpf_ok", "outro_cnpj_raiz", "cnpj_alfanumerico", "vencido", "revogado",
                           "sem_identificacao", "folha_futura", "cn_com_cnpj_sem_san", "outro_oid_sem_inscricao"]
# sem_clientauth: EKU emailProtection (presente, sem clientAuth). eku_ausente: sem extensão EKU.
CLIENTES = CLIENTES_COM_CLIENTAUTH + ["sem_clientauth", "eku_ausente", "eku_malformada"]
PFX_MODERNOS = [c + ".pfx" for c in CLIENTES]
# Certificados de cadeia: subordinada da AC de teste, autoassinado de outra raiz, e a folha da subordinada em dois .pfx.
EXTRAS_PEM = ["sub_ac.pem", "cadeia_outra_raiz.pem"]
PFX_EXTRAS = ["cadeia_outra_raiz.pfx", "cnpj_sub.pfx", "cnpj_sub_sem_intermediaria.pfx",
              "intermediaria_vencida.pfx", "cadeia_outra_sub.pfx", "sem_mac.pfx", "chave_sem_cifra.pfx"]
# .pfx que levam a AC de teste no bag de CA (o pfx de cadeia de outra raiz e o sem intermediária não levam).
PFX_COM_AC = PFX_MODERNOS

# Inventário fixo do diretório de saída (piso e teto de varredura, L-36 e L-11). Nenhum item extra, nenhuma pasta.
ARQUIVOS_ESPERADOS = (
    ["ac.pem", "crl.pem", "senha_teste.txt", "servidor.pem", "servidor.key", "cnpj_ok_legado.pfx"]
    + EXTRAS_PEM
    + [c + ".pem" for c in CLIENTES]
    + PFX_MODERNOS
    + PFX_EXTRAS
)
N_ARQUIVOS_ESPERADOS = 41

OID_CNPJ = "2.16.76.1.3.3"
OID_CPF = "2.16.76.1.3.1"
EKU_CLIENTE = "TLS Web Client Authentication"


def rodar(argv, env=None):
    return subprocess.run(argv, capture_output=True, text=True, env=env, check=False)


def saida_toda(resultado):
    return resultado.stdout + resultado.stderr


def linhas_do_bag(txt, prefixo):
    # Cada bag tem a própria cifra: a linha do cert bag e a do key bag são conferidas separadamente.
    return [linha for linha in txt.splitlines() if linha.startswith(prefixo)]


def corpo_pem(caminho):
    # Só o base64 do certificado, sem BEGIN/END: o pkcs12 imprime o mesmo corpo, com outros cabeçalhos em volta.
    linhas = caminho.read_text(encoding="ascii").splitlines()
    inicio = linhas.index("-----BEGIN CERTIFICATE-----")
    fim = linhas.index("-----END CERTIFICATE-----")
    return "\n".join(linhas[inicio + 1:fim])


def carregar_gerador():
    # Importa o gerador pelo caminho do arquivo (ele não é pacote), só para testar uma função dele.
    caminho = Path(__file__).with_name("gerar_certificados.py")
    spec = importlib.util.spec_from_file_location("gerar_certificados_teste", caminho)
    modulo = importlib.util.module_from_spec(spec)
    # Registrado antes do exec: o dataclass do gerador consulta sys.modules[__module__] (Python 3.14 exige).
    sys.modules[spec.name] = modulo
    spec.loader.exec_module(modulo)
    return modulo


def env_com_senha():
    senha = (DIR / "senha_teste.txt").read_text(encoding="ascii").strip()
    return dict(os.environ, **{VAR_SENHA: senha})


def dv_cnpj(base12):
    # Módulo 11 sobre ASCII - 48 (IN RFB 2.229/2024, vetor da RFB Q.14). Serve ao numérico e ao alfanumérico.
    valores = [ord(c) - 48 for c in base12]
    pesos1 = [5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2]
    pesos2 = [6] + pesos1

    def digito(vals, pesos):
        resto = sum(v * p for v, p in zip(vals, pesos)) % 11
        return 0 if resto < 2 else 11 - resto

    d1 = digito(valores, pesos1)
    d2 = digito(valores + [d1], pesos2)
    return base12 + str(d1) + str(d2)


def dv_cpf(base9):
    digitos = [int(c) for c in base9]
    d1 = (sum(v * p for v, p in zip(digitos, range(10, 1, -1))) * 10) % 11 % 10
    digitos.append(d1)
    d2 = (sum(v * p for v, p in zip(digitos, range(11, 1, -1))) * 10) % 11 % 10
    return base9 + str(d1) + str(d2)


def othername_valores(pem):
    # Lê o SAN pelo asn1parse (texto do OpenSSL, sem decodificador próprio) e devolve {OID: valor} dos otherName.
    # Dois processos por certificado: um para achar o offset do SAN, outro para abrir o seu conteúdo.
    linhas = rodar(["openssl", "asn1parse", "-in", str(pem)]).stdout.splitlines()
    offset = None
    for i, linha in enumerate(linhas):
        if "X509v3 Subject Alternative Name" in linha:
            for prox in linhas[i + 1:i + 3]:
                if "OCTET STRING" in prox:
                    offset = prox.split(":", 1)[0].strip()
                    break
            break
    if offset is None:
        raise AssertionError(f"SAN não encontrado em {pem}")
    interno = rodar(["openssl", "asn1parse", "-in", str(pem), "-strparse", offset]).stdout.splitlines()
    valores = {}
    oid = None
    for linha in interno:
        m = re.search(r"OBJECT\s*:([0-9.]+)\s*$", linha)
        if m:
            oid = m.group(1)
            continue
        m = re.search(r"OCTET STRING\s*:(.+)$", linha)
        if m and oid:
            valores[oid] = m.group(1).strip()
            oid = None
    return valores


class TesteCertificados(unittest.TestCase):

    def setUp(self):
        self.assertTrue(DIR.is_dir(), f"NL_CERTIFICADOS_DIR ausente: '{DIR}' (fixture não gerada)")
        self.ac = DIR / "ac.pem"
        self.crl = DIR / "crl.pem"
        cnf = tempfile.mkstemp(prefix="nl21-cnf-vazio-", suffix=".cnf", dir=os.environ.get("TMPDIR") or "/var/tmp")
        os.close(cnf[0])
        self.cnf_vazio = Path(cnf[1])
        self.cnf_vazio.write_text("# sem secao de provider: so o default, como o OpenSSL 3 sem configuracao\n",
                                  encoding="ascii")
        self.addCleanup(self.cnf_vazio.unlink, missing_ok=True)

    def env_isolado(self):
        return dict(env_com_senha(), OPENSSL_CONF=str(self.cnf_vazio))

    def fixturesGeradasSaoLidasPeloOpenssl(self):
        presentes = sorted(p.name for p in DIR.iterdir())
        self.assertEqual(presentes, sorted(ARQUIVOS_ESPERADOS), "inventário do diretório diferente do esperado")
        self.assertEqual(len(presentes), N_ARQUIVOS_ESPERADOS)
        modo = stat.S_IMODE((DIR / "senha_teste.txt").stat().st_mode)
        self.assertEqual(oct(modo), "0o600", "arquivo de senha precisa ser 0600")
        for pem in ["ac.pem", "servidor.pem"] + EXTRAS_PEM + [c + ".pem" for c in CLIENTES]:
            r = rodar(["openssl", "x509", "-in", str(DIR / pem), "-noout"])
            self.assertEqual(r.returncode, 0, f"{pem} não lido pelo openssl: {saida_toda(r)}")
        for pfx in PFX_MODERNOS + PFX_EXTRAS + ["cnpj_ok_legado.pfx"]:
            args = ["openssl", "pkcs12", "-in", str(DIR / pfx), "-noout", "-passin", f"env:{VAR_SENHA}"]
            if pfx.endswith("_legado.pfx"):
                args.insert(2, "-legacy")
            r = rodar(args, env=self.env_isolado())
            self.assertEqual(r.returncode, 0, f"{pfx} não aberto: {saida_toda(r)}")
        chave = rodar(["openssl", "pkey", "-in", str(DIR / "servidor.key"), "-pubout"]).stdout
        cert = rodar(["openssl", "x509", "-in", str(DIR / "servidor.pem"), "-pubkey", "-noout"]).stdout
        self.assertTrue(chave and chave == cert, "chave do servidor não confere com o certificado")

    def pfxLegadoFalhaSemLegacyComUnsupported(self):
        legado = str(DIR / "cnpj_ok_legado.pfx")
        base = ["openssl", "pkcs12", "-in", legado, "-noout", "-passin", f"env:{VAR_SENHA}"]
        r = rodar(base, env=self.env_isolado())
        self.assertNotEqual(r.returncode, 0, "legado abriu sem -legacy: o teste perdeu o sentido")
        txt = saida_toda(r)
        self.assertIn("0308010C", txt)
        self.assertIn("unsupported", txt)
        controle = rodar(base[:2] + ["-legacy"] + base[2:], env=self.env_isolado())
        self.assertEqual(controle.returncode, 0, f"controle com -legacy falhou: {saida_toda(controle)}")
        moderno = ["openssl", "pkcs12", "-in", str(DIR / "cnpj_ok.pfx"), "-noout", "-passin", f"env:{VAR_SENHA}"]
        r_mod = rodar(moderno, env=self.env_isolado())
        self.assertEqual(r_mod.returncode, 0, f"moderno não abre sem legacy: {saida_toda(r_mod)}")

    def pfxModernoUsaPbes2Aes256(self):
        for pfx in PFX_MODERNOS:
            r = rodar(["openssl", "pkcs12", "-in", str(DIR / pfx), "-info", "-noout", "-passin",
                       f"env:{VAR_SENHA}"], env=self.env_isolado())
            txt = saida_toda(r)
            self.assertEqual(r.returncode, 0, f"{pfx}: {txt}")
            certs = linhas_do_bag(txt, "PKCS7 Encrypted data:")
            chaves = linhas_do_bag(txt, "Shrouded Keybag:")
            self.assertTrue(certs and chaves, f"{pfx}: bag não encontrado")
            for linha in certs + chaves:
                self.assertIn("PBES2, PBKDF2, AES-256-CBC", linha, pfx)
                self.assertNotIn("RC2", linha, pfx)
                self.assertNotIn("3DES", linha, pfx)

    def pfxLegadoUsaRc2EDes(self):
        r = rodar(["openssl", "pkcs12", "-legacy", "-in", str(DIR / "cnpj_ok_legado.pfx"), "-info", "-noout",
                   "-passin", f"env:{VAR_SENHA}"], env=env_com_senha())
        txt = saida_toda(r)
        self.assertEqual(r.returncode, 0, txt)
        certs = linhas_do_bag(txt, "PKCS7 Encrypted data:")
        chaves = linhas_do_bag(txt, "Shrouded Keybag:")
        self.assertTrue(certs and chaves, "bag não encontrado")
        for linha in certs:
            self.assertIn("pbeWithSHA1And40BitRC2-CBC", linha)
        for linha in chaves:
            self.assertIn("pbeWithSHA1And3-KeyTripleDES-CBC", linha)
            self.assertNotIn("AES", linha)

    def cnpjNoOtherNameConfereComOid(self):
        esperados = {
            "cnpj_ok": CNPJ_OK,
            "sem_clientauth": CNPJ_OK,
            "eku_ausente": CNPJ_OK,
            "vencido": CNPJ_OK,
            "revogado": CNPJ_OK,
            "outro_cnpj_raiz": CNPJ_OUTRO_RAIZ,
            "cnpj_alfanumerico": CNPJ_ALFANUMERICO,
        }
        for nome, cnpj in esperados.items():
            valores = othername_valores(DIR / (nome + ".pem"))
            self.assertEqual(valores.get(OID_CNPJ), cnpj, nome)
        self.assertNotEqual(CNPJ_OUTRO_RAIZ[:8], CNPJ_OK[:8], "raiz do 'outro CNPJ' tem de ser diferente")

    def cpfNoOtherNameNasPosicoes9a19(self):
        valores = othername_valores(DIR / "cpf_ok.pem")
        conteudo = valores.get(OID_CPF)
        self.assertIsNotNone(conteudo, "otherName 2.16.76.1.3.1 ausente")
        self.assertEqual(len(conteudo), 55)
        self.assertEqual(conteudo[0:8], "01011990")
        self.assertEqual(conteudo[8:19], CPF_OK, "CPF precisa estar nas posições 9 a 19")
        self.assertEqual(conteudo, CONTEUDO_CPF)
        self.assertNotIn(OID_CNPJ, valores, "certificado de CPF não traz otherName de CNPJ")

    def ekuClientAuthSoNoCertificadoDeCliente(self):
        for nome in CLIENTES_COM_CLIENTAUTH:
            r = rodar(["openssl", "x509", "-in", str(DIR / (nome + ".pem")), "-noout", "-ext", "extendedKeyUsage"])
            self.assertEqual(r.returncode, 0, f"{nome}: {saida_toda(r)}")
            self.assertIn(EKU_CLIENTE, r.stdout, nome)
        # sem_clientauth: EKU presente, sem clientAuth (o caso que a NL-2.2 recusa como SemAutenticacaoCliente).
        r = rodar(["openssl", "x509", "-in", str(DIR / "sem_clientauth.pem"), "-noout", "-ext", "extendedKeyUsage"])
        self.assertEqual(r.returncode, 0, f"sem_clientauth: {saida_toda(r)}")
        self.assertNotIn(EKU_CLIENTE, r.stdout, "sem_clientauth tem clientAuth")
        self.assertIn("E-mail Protection", r.stdout, "sem_clientauth deveria ter EKU emailProtection")
        # eku_ausente: nenhuma extensão EKU (o caso que a NL-2.2 aceita, com evento sem PII).
        r = rodar(["openssl", "x509", "-in", str(DIR / "eku_ausente.pem"), "-noout", "-ext", "extendedKeyUsage"])
        self.assertEqual(r.returncode, 0, f"eku_ausente: {saida_toda(r)}")
        self.assertEqual(r.stdout.strip(), "", "eku_ausente deveria não ter EKU")
        ku = rodar(["openssl", "x509", "-in", str(DIR / "cnpj_ok.pem"), "-noout", "-ext", "keyUsage"]).stdout
        self.assertIn("Key Usage: critical", ku)
        self.assertIn("Digital Signature", ku)

    def vencidoRejeitadoPeloVerify(self):
        r = rodar(["openssl", "verify", "-CAfile", str(self.ac), str(DIR / "vencido.pem")])
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("certificate has expired", saida_toda(r))
        controle = rodar(["openssl", "verify", "-CAfile", str(self.ac), str(DIR / "cnpj_ok.pem")])
        self.assertEqual(controle.returncode, 0, saida_toda(controle))

    def revogadoRejeitadoPorLcr(self):
        base = ["openssl", "verify", "-crl_check", "-CAfile", str(self.ac), "-CRLfile", str(self.crl)]
        r = rodar(base + [str(DIR / "revogado.pem")])
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("certificate revoked", saida_toda(r))
        controle = rodar(base + [str(DIR / "cnpj_ok.pem")])
        self.assertEqual(controle.returncode, 0, saida_toda(controle))

    def dados_x509(self, caminho):
        # Sujeito, impressão digital SHA-256 e basicConstraints do certificado: a identidade inteira da AC.
        r = rodar(["openssl", "x509", "-in", str(caminho), "-noout", "-subject", "-fingerprint", "-sha256",
                   "-ext", "basicConstraints"])
        self.assertEqual(r.returncode, 0, saida_toda(r))
        return r.stdout.strip()

    def pfxTemCadeiaDaAc(self):
        # O bag de CA tem de conter EXATAMENTE a AC de teste (um certificado, o mesmo sujeito, a mesma impressão
        # digital e CA:TRUE). Um certificado de cliente no bag, ou um bundle com a AC e o cliente, reprova.
        referencia = self.dados_x509(DIR / "ac.pem")
        self.assertIn("CA:TRUE", referencia)
        alvos = [(pfx, []) for pfx in PFX_COM_AC] + [("cnpj_ok_legado.pfx", ["-legacy"])]
        for pfx, extra in alvos:
            argv = ["openssl", "pkcs12"] + extra + ["-in", str(DIR / pfx), "-cacerts", "-nokeys",
                                                    "-passin", f"env:{VAR_SENHA}"]
            r = rodar(argv, env=self.env_isolado())
            self.assertEqual(r.returncode, 0, f"{pfx}: {saida_toda(r)}")
            blocos = re.findall(r"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----", r.stdout, re.S)
            self.assertEqual(len(blocos), 1, f"{pfx}: bag de CA com {len(blocos)} certificados, esperado 1 (só a AC)")
            fd, tmp = tempfile.mkstemp(prefix="nl21-cadeia-", suffix=".pem", dir=os.environ.get("TMPDIR") or "/var/tmp")
            self.addCleanup(os.unlink, tmp)
            with os.fdopen(fd, "w", encoding="ascii") as arquivo:
                arquivo.write(blocos[0] + "\n")
            self.assertEqual(self.dados_x509(tmp), referencia, f"{pfx}: certificado do bag de CA não é a AC de teste")

    def subAcEhCaPathlenZeroAssinadaPelaAc(self):
        r = rodar(["openssl", "x509", "-in", str(DIR / "sub_ac.pem"), "-noout", "-ext", "basicConstraints"])
        self.assertEqual(r.returncode, 0, saida_toda(r))
        self.assertIn("CA:TRUE, pathlen:0", r.stdout, "subordinada precisa ser CA com pathlen 0")
        v = rodar(["openssl", "verify", "-CAfile", str(self.ac), str(DIR / "sub_ac.pem")])
        self.assertEqual(v.returncode, 0, saida_toda(v))

    def cnpjSubVemDaSubAc(self):
        # A folha do .pfx com a subordinada confere com a subordinada; sem a subordinada, a cadeia é incompleta.
        folha = rodar(["openssl", "pkcs12", "-in", str(DIR / "cnpj_sub.pfx"), "-clcerts", "-nokeys",
                       "-passin", f"env:{VAR_SENHA}"], env=self.env_isolado())
        self.assertEqual(folha.returncode, 0, saida_toda(folha))
        blocos = re.findall(r"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----", folha.stdout, re.S)
        self.assertEqual(len(blocos), 1, "cnpj_sub.pfx deveria ter uma folha")
        fd, tmp = tempfile.mkstemp(prefix="nl21-folha-", suffix=".pem", dir=os.environ.get("TMPDIR") or "/var/tmp")
        self.addCleanup(os.unlink, tmp)
        with os.fdopen(fd, "w", encoding="ascii") as arquivo:
            arquivo.write(blocos[0] + "\n")
        completa = rodar(["openssl", "verify", "-CAfile", str(self.ac), "-untrusted", str(DIR / "sub_ac.pem"), tmp])
        self.assertEqual(completa.returncode, 0, saida_toda(completa))
        incompleta = rodar(["openssl", "verify", "-CAfile", str(self.ac), tmp])
        self.assertNotEqual(incompleta.returncode, 0)
        self.assertIn("unable to get local issuer certificate", saida_toda(incompleta))

    def cnpjSubSemIntermediariaSemCa(self):
        r = rodar(["openssl", "pkcs12", "-in", str(DIR / "cnpj_sub_sem_intermediaria.pfx"), "-cacerts", "-nokeys",
                   "-passin", f"env:{VAR_SENHA}"], env=self.env_isolado())
        self.assertEqual(r.returncode, 0, saida_toda(r))
        self.assertNotIn("BEGIN CERTIFICATE", r.stdout, "pfx sem intermediária não deveria levar CA")

    def cadeiaForaDaAcEhAutoassinadaSemCa(self):
        pem = DIR / "cadeia_outra_raiz.pem"
        r = rodar(["openssl", "x509", "-in", str(pem), "-noout", "-issuer", "-subject"])
        self.assertEqual(r.returncode, 0, saida_toda(r))
        emissor, sujeito = [linha.split("=", 1)[1] for linha in r.stdout.strip().splitlines()[:2]]
        self.assertEqual(emissor, sujeito, "cadeia_outra_raiz deveria ser autoassinado")
        self.assertEqual(othername_valores(pem).get(OID_CNPJ), CNPJ_OUTRO_RAIZ)
        cas = rodar(["openssl", "pkcs12", "-in", str(DIR / "cadeia_outra_raiz.pfx"), "-cacerts", "-nokeys",
                     "-passin", f"env:{VAR_SENHA}"], env=self.env_isolado())
        self.assertEqual(cas.returncode, 0, saida_toda(cas))
        self.assertNotIn("BEGIN CERTIFICATE", cas.stdout, "pfx de outra raiz não deveria levar CA")

    def semIdentificacaoSemOids(self):
        # Sem otherName nenhum (nem CNPJ nem CPF): o caso que a NL-2.2 devolve como SemInscricao.
        pem = DIR / "sem_identificacao.pem"
        # Sem SAN nenhum: othername_valores lança sem SAN (é a regra dele), então confere-se só a extensão.
        r = rodar(["openssl", "x509", "-in", str(pem), "-noout", "-ext", "subjectAltName"])
        self.assertEqual(r.returncode, 0, saida_toda(r))
        self.assertEqual(r.stdout.strip(), "", "sem_identificacao não deveria ter subjectAltName")

    def cnpjsDeTesteTemDvValido(self):
        # Lê o identificador de DENTRO dos certificados gerados (otherName) e confere o DV com o algoritmo
        # independente deste arquivo. Assim prova o gerador, não só as constantes escritas aqui.
        for nome, cnpj in [("cnpj_ok", CNPJ_OK), ("outro_cnpj_raiz", CNPJ_OUTRO_RAIZ),
                           ("cnpj_alfanumerico", CNPJ_ALFANUMERICO)]:
            lido = othername_valores(DIR / (nome + ".pem")).get(OID_CNPJ)
            self.assertEqual(lido, cnpj, nome)
            self.assertEqual(dv_cnpj(lido[:12]), lido, f"DV do CNPJ de {nome}")
        cpf = othername_valores(DIR / "cpf_ok.pem").get(OID_CPF, "")[8:19]
        self.assertEqual(cpf, CPF_OK)
        self.assertEqual(dv_cpf(cpf[:9]), cpf, "DV do CPF lido do otherName")

    def servidorFalsoTemSanEEkuServerAuth(self):
        servidor = DIR / "servidor.pem"
        san = rodar(["openssl", "x509", "-in", str(servidor), "-noout", "-ext", "subjectAltName"]).stdout
        self.assertIn("DNS:localhost", san)
        self.assertIn("IP Address:127.0.0.1", san)
        eku = rodar(["openssl", "x509", "-in", str(servidor), "-noout", "-ext", "extendedKeyUsage"]).stdout
        self.assertIn("TLS Web Server Authentication", eku)
        r = rodar(["openssl", "verify", "-CAfile", str(self.ac), str(servidor)])
        self.assertEqual(r.returncode, 0, saida_toda(r))


class TesteAlgoritmos(unittest.TestCase):
    # Vetores publicados (RFB Q.14, Q.17, Q.23 Ex2 e Ex3; Serpro 1.1 e 1.2). O vetor X1 (AA345678/0003-29) fica de
    # fora: a pesquisa Onda 1 (a5) mostra que ele diverge da regra, então não serve de oráculo.
    def dvAlfanumericoVetoresOficiais(self):
        vetores = {
            "12ABC34501DE35": "12ABC34501DE",  # Serpro e RFB, exemplo completo (V1)
            "AA345678000A29": "AA345678000A",  # RFB Q.23 Ex2 (V2)
            "12345678000A08": "12345678000A",  # RFB Q.23 Ex3 (V3): único vetor com resto 1, DV1 = 0
            "12345678000195": "123456780001",  # RFB Q.17, numérico
        }
        for esperado, base in vetores.items():
            self.assertEqual(dv_cnpj(base), esperado, base)


class TesteGerador(unittest.TestCase):
    # Não usa a fixture: gera em diretórios temporários próprios (duas gerações, para comparar as senhas).

    def gerar_em_tmp(self):
        base = Path(tempfile.mkdtemp(prefix="nl21-gera-", dir=os.environ.get("TMPDIR") or "/var/tmp"))
        self.addCleanup(shutil.rmtree, base, True)
        saida = base / "out"
        r = rodar([sys.executable, "-I", str(Path(__file__).with_name("gerar_certificados.py")), str(saida)])
        self.assertEqual(r.returncode, 0, saida_toda(r))
        return saida

    def senhaAleatoriaPorExecucao(self):
        primeira = (self.gerar_em_tmp() / "senha_teste.txt").read_text(encoding="ascii").strip()
        segunda = (self.gerar_em_tmp() / "senha_teste.txt").read_text(encoding="ascii").strip()
        self.assertGreaterEqual(len(primeira), 20, "senha curta demais")
        self.assertNotEqual(primeira, segunda, "duas gerações deram a mesma senha: senha constante")

    def senhaNasceCom0600(self):
        # O arquivo tem de nascer com 0600: a função não pode depender de chmod depois de gravar (janela com 0644).
        gerador = carregar_gerador()
        base = Path(tempfile.mkdtemp(prefix="nl21-senha-", dir=os.environ.get("TMPDIR") or "/var/tmp"))
        self.addCleanup(shutil.rmtree, base, True)
        alvo = base / "senha_teste.txt"
        velho = os.umask(0o022)
        self.addCleanup(os.umask, velho)
        with mock.patch("os.chmod", side_effect=AssertionError("gravar_senha não pode depender de chmod")):
            gerador.gravar_senha(alvo, "senha-de-teste-aleatoria")
        self.assertEqual(oct(stat.S_IMODE(alvo.stat().st_mode)), "0o600")
        self.assertEqual(alvo.read_text(encoding="ascii").strip(), "senha-de-teste-aleatoria")


class TesteSegredos(unittest.TestCase):
    # Não usa a fixture de certificados: roda com repositório temporário próprio.

    def nenhumaChavePrivadaRastreadaNoGit(self):
        # Prova que o portão do projeto (.gitleaks.toml) reprova chave privada COMMITADA, num repositório
        # temporário fora do projeto. O controle sem segredo garante que o portão não reprova tudo.
        raiz = Path(__file__).resolve().parents[3]
        config = raiz / ".gitleaks.toml"
        self.assertTrue(config.is_file(), f"config ausente: {config} (fail-closed)")
        gitleaks = shutil.which("gitleaks")
        self.assertIsNotNone(gitleaks, "gitleaks ausente neste contêiner: não instalado pela NL-2.1 (L-51)")
        git = shutil.which("git")
        self.assertIsNotNone(git, "git ausente neste contêiner")
        base = Path(tempfile.mkdtemp(prefix="nl21-chave-", dir=os.environ.get("TMPDIR") or "/var/tmp"))
        self.addCleanup(shutil.rmtree, base, True)

        def repo_com(nome, conteudo_extra=None):
            repo = base / nome
            repo.mkdir()
            self.assertEqual(rodar([git, "init", "-q", str(repo)]).returncode, 0)
            if conteudo_extra:
                r = rodar(["openssl", "genpkey", "-algorithm", "EC", "-pkeyopt", "ec_paramgen_curve:P-256",
                           "-out", str(repo / "chave.pem")])
                self.assertEqual(r.returncode, 0, saida_toda(r))
            else:
                (repo / "leia_me.txt").write_text("sem segredo\n", encoding="ascii")
            ident = ["-c", "user.name=teste", "-c", "user.email=teste@invalido.example", "-c", "commit.gpgsign=false"]
            self.assertEqual(rodar([git, "-C", str(repo), "add", "-A"]).returncode, 0)
            r = rodar([git, "-C", str(repo)] + ident + ["commit", "-q", "-m", "teste"])
            self.assertEqual(r.returncode, 0, saida_toda(r))
            return repo

        def portao(repo):
            relatorio = base / (repo.name + ".json")
            return rodar([gitleaks, "git", str(repo), "--config", str(config), "--no-banner", "--redact",
                          "--exit-code", "1", "--report-format", "json", "--report-path", str(relatorio)]).returncode

        com_chave = repo_com("com_chave", conteudo_extra=True)
        sem_segredo = repo_com("sem_segredo")
        self.assertEqual(portao(com_chave), 1, "gitleaks deveria reprovar a chave privada commitada")
        self.assertEqual(portao(sem_segredo), 0, "controle sem segredo deveria passar")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("uso: test_certificados.py <nome-do-teste>")
    nome = sys.argv.pop(1)
    classes = [TesteCertificados, TesteGerador, TesteAlgoritmos, TesteSegredos]
    classe = next((c for c in classes if hasattr(c, nome)), None)
    if classe is None:
        sys.exit(f"teste desconhecido: {nome}")
    suite = unittest.TestSuite([classe(nome)])
    resultado = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if resultado.wasSuccessful() else 1)
