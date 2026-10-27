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

DIR = Path(os.environ.get("NL_CERTIFICADOS_DIR", ""))
VAR_SENHA = "NL_SENHA_TESTE"

CNPJ_OK = "12345678000195"
CNPJ_OUTRO_RAIZ = "98765432000198"
CNPJ_ALFANUMERICO = "12ABC34501DE35"
CPF_OK = "12345678909"
# Layout do DOC-ICP-04 v8.0, 7.1.2.3, otherName 2.16.76.1.3.1: data (8), CPF (11), NIS (11), RG (15), órgão/UF (10).
CONTEUDO_CPF = "01011990" + CPF_OK + "00000000000" + "000000000000000" + "SSPTESTE00"

CLIENTES_COM_CLIENTAUTH = ["cnpj_ok", "cpf_ok", "outro_cnpj_raiz", "cnpj_alfanumerico", "vencido", "revogado"]
CLIENTES = CLIENTES_COM_CLIENTAUTH + ["sem_clientauth"]
PFX_MODERNOS = [c + ".pfx" for c in CLIENTES]

# Inventário fixo do diretório de saída (piso e teto de varredura, L-36 e L-11). Nenhum item extra, nenhuma pasta.
ARQUIVOS_ESPERADOS = (
    ["ac.pem", "crl.pem", "senha_teste.txt", "servidor.pem", "servidor.key", "cnpj_ok_legado.pfx"]
    + [c + ".pem" for c in CLIENTES]
    + PFX_MODERNOS
)
N_ARQUIVOS_ESPERADOS = 20

OID_CNPJ = "2.16.76.1.3.3"
OID_CPF = "2.16.76.1.3.1"
EKU_CLIENTE = "TLS Web Client Authentication"


def rodar(argv, env=None):
    return subprocess.run(argv, capture_output=True, text=True, env=env, check=False)


def saida_toda(resultado):
    return resultado.stdout + resultado.stderr


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
        for pem in ["ac.pem", "servidor.pem"] + [c + ".pem" for c in CLIENTES]:
            r = rodar(["openssl", "x509", "-in", str(DIR / pem), "-noout"])
            self.assertEqual(r.returncode, 0, f"{pem} não lido pelo openssl: {saida_toda(r)}")
        for pfx in PFX_MODERNOS + ["cnpj_ok_legado.pfx"]:
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
            self.assertIn("PBES2, PBKDF2, AES-256-CBC", txt, pfx)
            self.assertNotIn("RC2", txt, pfx)

    def pfxLegadoUsaRc2EDes(self):
        r = rodar(["openssl", "pkcs12", "-legacy", "-in", str(DIR / "cnpj_ok_legado.pfx"), "-info", "-noout",
                   "-passin", f"env:{VAR_SENHA}"], env=env_com_senha())
        txt = saida_toda(r)
        self.assertEqual(r.returncode, 0, txt)
        self.assertIn("pbeWithSHA1And40BitRC2-CBC", txt)
        self.assertIn("pbeWithSHA1And3-KeyTripleDES-CBC", txt)

    def cnpjNoOtherNameConfereComOid(self):
        esperados = {
            "cnpj_ok": CNPJ_OK,
            "sem_clientauth": CNPJ_OK,
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
            self.assertIn(EKU_CLIENTE, r.stdout, nome)
        r = rodar(["openssl", "x509", "-in", str(DIR / "sem_clientauth.pem"), "-noout", "-ext", "extendedKeyUsage"])
        self.assertNotIn(EKU_CLIENTE, r.stdout, "sem_clientauth tem clientAuth")
        self.assertEqual(r.stdout.strip(), "", "sem_clientauth deveria não ter EKU")
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
    classe = TesteSegredos if hasattr(TesteSegredos, nome) else TesteCertificados
    suite = unittest.TestSuite([classe(nome)])
    resultado = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if resultado.wasSuccessful() else 1)
