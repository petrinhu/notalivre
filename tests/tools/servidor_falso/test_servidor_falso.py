# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Testes do servidor falso da API (NL-2.6). Cada caso roda como um processo do ctest: test_<nome>.

INFERÊNCIA até a NL-2.10: respostas, códigos (exceto E0714, catálogo do Anexo I), chave de 50 posições e o conteúdo
do campo de NFS-e são do servidor falso (pesquisa g). O oráculo é o xmllint e o xmlsec1, nunca o código do app.
"""

import base64
import gzip
import re
import http.client
import json
import os
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import unittest
import zlib
from pathlib import Path
from unittest import mock

RAIZ = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(RAIZ / "tools"))

from servidor_falso import certificados, envelope, oraculo, referencia, roteiro, servidor  # noqa: E402

FIXTURE_DPS = RAIZ / "tests" / "fixtures" / "dps" / "valida_dps_mei_minima.xml"


def _assinar_no_teste(xmlsec1, template, chave, certificado, trabalho, versao):
    """Assinatura do SETUP, feita pelo próprio teste (regra de versão escrita aqui, não em oraculo).

    Isolada de propósito: se o código do oráculo quebrar a verificação, o teste de verificação morre na
    asserção de verificação, não aqui. A regra de versão é a mesma: a linha 1.3 precisa de --lax-key-search.
    """
    os.makedirs(trabalho, exist_ok=True)
    entrada = os.path.join(trabalho, "template.xml")
    saida = os.path.join(trabalho, "assinado.xml")
    with open(entrada, "wb") as f:
        f.write(template)
    lax = ["--lax-key-search"] if versao >= (1, 3) else []
    comando = [xmlsec1, "--sign", *lax, "--id-attr:Id", "infDPS",
               "--privkey-pem", f"{chave},{certificado}", "--output", saida, entrada]
    r = subprocess.run(comando, capture_output=True, text=True, env={**os.environ, **_ambiente_arnes()})
    if r.returncode != 0 or not os.path.isfile(saida) or os.path.getsize(saida) == 0:
        raise RuntimeError(f"setup não assinou a DPS de referência (rc={r.returncode}): {r.stderr}")
    with open(saida, "rb") as f:
        return f.read()


def _ambiente_arnes():
    return oraculo.ambiente_do_arnes(oraculo.ler_os_release())


def _sem_keyinfo(xml):
    """DPS com o KeyInfo vazio. O KeyInfo não está no Reference, então a assinatura continua válida."""
    return re.sub(rb"<KeyInfo>.*?</KeyInfo>", b"<KeyInfo/>", xml, flags=re.S)


class TesteServidorFalso(unittest.TestCase):
    def setUp(self):
        base = os.environ["NL_SF_DIR"]
        os.makedirs(base, exist_ok=True)
        self.dir = tempfile.mkdtemp(prefix="caso-", dir=base)
        self.addCleanup(shutil.rmtree, self.dir, True)
        self.certs = certificados.obter_certificados(os.path.join(self.dir, "pki"))
        xsd = os.path.join(os.environ["NL_XSD_PRODREST_DIR"], "DPS_v1.01.xsd")
        self.oraculo = oraculo.Oraculo(os.environ["NL_XMLLINT"], os.environ["NL_XMLSEC1"], xsd,
                                       self.certs.ca, os.path.join(self.dir, "trabalho"))
        versao = self.oraculo.versao
        base_xml = FIXTURE_DPS.read_bytes()
        template = referencia.montar_template(base_xml, referencia.id_do_infdps(base_xml))
        self.dps = _assinar_no_teste(os.environ["NL_XMLSEC1"], template, self.certs.cliente_key,
                                     self.certs.cliente_cert, os.path.join(self.dir, "setup"), versao)
        self.id_dps = referencia.id_do_infdps(self.dps)

    # --- auxiliares de teste ---------------------------------------------------------------------

    def subir(self, roteiro_=None):
        srv = servidor.ServidorFalso(self.certs, self.oraculo, roteiro_)
        srv.iniciar()
        self.addCleanup(srv.encerrar)
        return srv

    def conexao(self, srv, com_certificado=True, timeout=10):
        ctx = ssl.create_default_context(cafile=self.certs.ca)
        if com_certificado:
            ctx.load_cert_chain(self.certs.cliente_cert, self.certs.cliente_key)
        return http.client.HTTPSConnection("127.0.0.1", srv.porta, timeout=timeout, context=ctx)

    def enviar(self, srv, metodo, caminho, corpo=None, timeout=10):
        conn = self.conexao(srv, timeout=timeout)
        try:
            cabecalhos = {"Content-Type": "application/json"} if corpo is not None else {}
            conn.request(metodo, caminho, body=corpo, headers=cabecalhos)
            resp = conn.getresponse()
            dados = resp.read()
            corpo_json = json.loads(dados) if dados else None
            return resp.status, corpo_json
        finally:
            conn.close()

    def postar_dps(self, srv, xml=None, timeout=10):
        return self.enviar(srv, "POST", "/nfse", envelope.envelope(self.dps if xml is None else xml), timeout)

    # --- casos -----------------------------------------------------------------------------------

    def test_dpsValidaRecebeChave50(self):
        srv = self.subir()
        status, corpo = self.postar_dps(srv)
        self.assertEqual(status, 201, corpo)
        chave = corpo["chaveAcesso"]
        self.assertRegex(chave, r"^[0-9]{50}$")  # INFERÊNCIA: chave de 50 posições (pesquisa g)
        bruto = base64.b64decode(corpo["nfseXmlGZipB64"])
        self.assertEqual(bruto[:2], b"\x1f\x8b")  # GZip RFC 1952
        # INFERÊNCIA: o campo traz a própria DPS assinada no lugar do XML da NFS-e (não há schema de NFS-e aqui)
        self.assertEqual(gzip.decompress(bruto), self.dps)
        self.assertEqual(self.enviar(srv, "GET", f"/nfse/{chave}")[1]["chaveAcesso"], chave)
        self.assertEqual(self.enviar(srv, "GET", f"/dps/{self.id_dps}")[1]["chaveAcesso"], chave)

    def test_dpsAdulteradaRejeitada(self):
        srv = self.subir()
        adulterada = self.dps.replace(b"<vServ>100.00", b"<vServ>999.00")
        self.assertNotEqual(adulterada, self.dps)
        ok_xsd, saida = self.oraculo.validar_xsd(adulterada)
        self.assertTrue(ok_xsd, f"a adulteração tem que continuar válida no XSD para provar a assinatura: {saida}")
        status, corpo = self.postar_dps(srv, adulterada)
        self.assertEqual(status, 400, corpo)
        self.assertEqual(corpo["Codigo"], "E0714")  # FATO: regra de assinatura, Anexo I aba RN, linha 653

        sem_tpamb = self.dps.replace(b"<tpAmb>2</tpAmb>", b"")
        ok_xsd, _ = self.oraculo.validar_xsd(sem_tpamb)
        self.assertFalse(ok_xsd, "sem tpAmb o XML tem que reprovar no XSD")
        status, corpo = self.postar_dps(srv, sem_tpamb)
        self.assertEqual(status, 400, corpo)
        self.assertEqual(corpo["Codigo"], "XSD_INVALIDO")  # código do servidor falso (INFERÊNCIA)

    def test_assinaturaSha1AceitaPeloOraculo(self):
        self.assertIn(referencia.URI_RSA_SHA1.encode(), self.dps)
        self.assertIn(referencia.URI_SHA1.encode(), self.dps)
        # Etapa 1, isolada do servidor: o oráculo verifica a DPS assinada (falha aqui, não no setup).
        ok, saida = self.oraculo.verificar_assinatura(self.dps)
        self.assertTrue(ok, f"o oráculo recusou a DPS assinada em SHA-1: {saida}")
        # Etapa 2: o mesmo comando, direto no xmlsec1, com o arquivo gravado.
        caminho = os.path.join(self.dir, "verificar-direto.xml")
        with open(caminho, "wb") as f:
            f.write(self.dps)
        r = subprocess.run(self.oraculo.comando_verificar(caminho), env=self.oraculo._env(),
                           capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        # Etapa 3: o servidor aceita a mesma DPS.
        srv = self.subir()
        status, corpo = self.postar_dps(srv)
        self.assertEqual(status, 201, corpo)

    def test_semCertificadoRecusado(self):
        srv = self.subir()
        recusado = False
        try:
            conn = self.conexao(srv, com_certificado=False)
            conn.request("POST", "/nfse", body=envelope.envelope(self.dps),
                         headers={"Content-Type": "application/json"})
            conn.getresponse()
        except (ssl.SSLError, OSError, http.client.HTTPException):
            recusado = True
        self.assertTrue(recusado, "handshake sem certificado de cliente tinha que ser recusado")
        self.assertEqual(srv.requisicoes, 0, "a requisição não pode chegar ao manipulador")
        self.assertEqual(srv.dps_aceitas, {})
        # controle positivo: com certificado, a mesma DPS é aceita
        status, corpo = self.postar_dps(srv)
        self.assertEqual(status, 201, corpo)

    def test_falhaRoteirizadaAconteceUmaVez(self):
        for falha in ("erro5xx", "conflito409"):
            with self.subTest(falha=falha):
                rot = roteiro.Roteiro()
                rot.agendar("POST", "/nfse", falha)
                srv = self.subir(rot)
                esperado = 503 if falha == "erro5xx" else 409
                primeira, _ = self.postar_dps(srv)
                self.assertEqual(primeira, esperado)
                segunda, corpo = self.postar_dps(srv)
                self.assertEqual(segunda, 201, corpo)  # a segunda chamada vê sucesso
                self.assertEqual(rot.pendentes(), 0)

        with self.subTest(falha="timeout"):
            rot = roteiro.Roteiro()
            rot.agendar("POST", "/nfse", "timeout")
            srv = self.subir(rot)
            with self.assertRaises((socket.timeout, TimeoutError, OSError)):
                self.postar_dps(srv, timeout=0.5)
            srv.liberar.set()  # o teste solta o servidor; nada é gravado antes do timeout
            status, corpo = self.postar_dps(srv)
            self.assertEqual(status, 201, corpo)
            self.assertEqual(rot.pendentes(), 0)

        with self.subTest(falha="conexao_cortada_apos_aceite"):
            rot = roteiro.Roteiro()
            rot.agendar("POST", "/nfse", "conexao_cortada_apos_aceite")
            srv = self.subir(rot)
            with self.assertRaises((http.client.HTTPException, ConnectionError, OSError)):
                self.postar_dps(srv)
            self.assertEqual(rot.pendentes(), 0)
            # a consulta do cliente vê que a DPS foi aceita: HEAD e GET /dps/{id}
            self.assertEqual(self.enviar(srv, "HEAD", f"/dps/{self.id_dps}")[0], 200)
            self.assertIn("chaveAcesso", self.enviar(srv, "GET", f"/dps/{self.id_dps}")[1])

    def test_variavelSha1SoNoFedora44(self):
        self.assertEqual(oraculo.ambiente_do_arnes({"ID": "fedora", "VERSION_ID": "44"}),
                         {"OPENSSL_ENABLE_SHA1_SIGNATURES": "1"})
        self.assertEqual(oraculo.ambiente_do_arnes({"ID": "ubuntu", "VERSION_ID": "26.04"}), {})
        self.assertEqual(oraculo.ambiente_do_arnes({"ID": "fedora", "VERSION_ID": "43"}), {})

    def test_verificacaoLaxObrigatoriaNa13(self):
        # Cadeia que a busca estrita da 1.3 não acha (medida em 1.3.9 e 1.3.12): KeyInfo vazio, a chave do emissor
        # entra pelo armazém de chaves (--pubkey-cert-pem) e a AC de teste continua em --trusted-pem. Estrito: KEY-NOT-FOUND;
        # com --lax-key-search: rc 0. Na 1.2 a opção não existe e a mesma cadeia verifica em estrito (rc 0).
        doc = _sem_keyinfo(self.dps)
        caminho = os.path.join(self.dir, "sem-keyinfo.xml")
        with open(caminho, "wb") as f:
            f.write(doc)
        cliente = self.certs.cliente_cert
        ok, saida = self.oraculo.verificar_assinatura(doc, chaves_publicas=[cliente])
        self.assertTrue(ok, f"o oráculo recusou a DPS sem KeyInfo com a chave do emissor: {saida}")
        comando = self.oraculo.comando_verificar(caminho, chaves_publicas=[cliente])
        if self.oraculo.versao >= (1, 3):
            self.assertIn("--lax-key-search", comando)
            sem_lax = [a for a in comando if a != "--lax-key-search"]
            r = subprocess.run(sem_lax, env=self.oraculo._env(), capture_output=True, text=True)
            self.assertNotEqual(r.returncode, 0, "na 1.3 a busca estrita tinha de falhar sem --lax-key-search")
        else:
            self.assertNotIn("--lax-key-search", comando)

    def test_oraculoSemAssinarSemChamador(self):
        # Sem chamador na árvore (grep em git ls-files), oraculo.assinar e referencia.assinar saem do código.
        self.assertFalse(hasattr(oraculo.Oraculo, "assinar"))
        self.assertFalse(hasattr(referencia, "assinar"))

    def test_rotasDeEventoEDfeRespondem501(self):
        # Fora da NL-2.6, por DA-2.27: POST /nfse/{chave}/eventos é da NL-4.1 e GET /DFe/{NSU} é da NL-4.2.
        srv = self.subir()
        chave = "1" * 50
        self.assertEqual(self.enviar(srv, "POST", f"/nfse/{chave}/eventos", b"{}")[0], 501)
        self.assertEqual(self.enviar(srv, "GET", "/DFe/1")[0], 501)
        # GET de eventos de NFS-e cai na consulta de NFS-e (404) até a NL-4.2 modelar os eventos do ADN
        self.assertEqual(self.enviar(srv, "GET", f"/nfse/{chave}/eventos")[0], 404)

    def test_ambienteDoXmlsec1SoNoFedora44(self):
        # Captura o ambiente que o oráculo ENTREGA ao xmlsec1 (não o que a função de regra devolve).
        xsd = os.path.join(os.environ["NL_XSD_PRODREST_DIR"], "DPS_v1.01.xsd")
        trabalho = os.path.join(self.dir, "ambiente")
        original = subprocess.run
        capturados = []

        def gravar(args, **kwargs):
            if "--verify" in args:  # só a verificação: a leitura da versão também chama o xmlsec1
                capturados.append(kwargs.get("env") or {})
            return original(args, **kwargs)

        with mock.patch.object(oraculo.subprocess, "run", side_effect=gravar):
            ubuntu = oraculo.Oraculo(os.environ["NL_XMLLINT"], os.environ["NL_XMLSEC1"], xsd, self.certs.ca,
                                     trabalho, os_release={"ID": "ubuntu", "VERSION_ID": "26.04"})
            ubuntu.verificar_assinatura(self.dps)
            fedora = oraculo.Oraculo(os.environ["NL_XMLLINT"], os.environ["NL_XMLSEC1"], xsd, self.certs.ca,
                                     trabalho, os_release={"ID": "fedora", "VERSION_ID": "44"})
            fedora.verificar_assinatura(self.dps)
        self.assertEqual(len(capturados), 2, "o xmlsec1 tinha que ser chamado uma vez por oráculo")
        self.assertNotIn("OPENSSL_ENABLE_SHA1_SIGNATURES", capturados[0],
                         "fora do fedora44 a variável não pode chegar ao xmlsec1")
        self.assertEqual(capturados[1].get("OPENSSL_ENABLE_SHA1_SIGNATURES"), "1",
                         "no fedora44 a variável tem de chegar ao xmlsec1")

    def test_envelopeZlibCruRecusadoCom400(self):
        srv = self.subir()
        xml_zlib_dentro_do_json = json.dumps({
            envelope.CHAVE_ENVELOPE: base64.b64encode(zlib.compress(self.dps)).decode("ascii")}).encode("utf-8")
        status, corpo = self.enviar(srv, "POST", "/nfse", xml_zlib_dentro_do_json)
        self.assertEqual(status, 400, corpo)
        self.assertEqual(corpo["Codigo"], "ENVELOPE_INVALIDO")
        status, corpo = self.enviar(srv, "POST", "/nfse", zlib.compress(self.dps))  # zlib cru, sem JSON
        self.assertEqual(status, 400, corpo)
        self.assertEqual(corpo["Codigo"], "ENVELOPE_INVALIDO")
        sem_chave = json.dumps({"outraChave": envelope.gzip_b64(self.dps)}).encode("utf-8")
        status, corpo = self.enviar(srv, "POST", "/nfse", sem_chave)
        self.assertEqual(status, 400, corpo)
        self.assertEqual(corpo["Codigo"], "ENVELOPE_INVALIDO")
        self.assertEqual(srv.dps_aceitas, {}, "nenhum envelope inválido pode gravar DPS")

    def test_tlsMinimoTls12RecusaTls11(self):
        srv = self.subir()
        self.assertEqual(srv.contexto.minimum_version, ssl.TLSVersion.TLSv1_2)
        ctx = ssl.create_default_context(cafile=self.certs.ca)
        ctx.load_cert_chain(self.certs.cliente_cert, self.certs.cliente_key)
        ctx.minimum_version = ssl.TLSVersion.TLSv1_1
        ctx.maximum_version = ssl.TLSVersion.TLSv1_1
        recusado = False
        try:
            conn = http.client.HTTPSConnection("127.0.0.1", srv.porta, timeout=10, context=ctx)
            conn.request("GET", "/dps/" + self.id_dps)
            conn.getresponse()
        except (ssl.SSLError, OSError, http.client.HTTPException):
            recusado = True
        self.assertTrue(recusado, "cliente TLS 1.1 tinha que ser recusado")
        self.assertEqual(srv.requisicoes, 0)

    def test_escutaSoEmLoopback(self):
        srv = self.subir()
        self.assertEqual(srv.server_address[0], "127.0.0.1", "o servidor falso escuta só em loopback")
        self.assertEqual(self.enviar(srv, "GET", f"/dps/{self.id_dps}")[0], 404)


if __name__ == "__main__":
    suite = unittest.TestSuite([TesteServidorFalso(sys.argv[1])])
    resultado = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if resultado.wasSuccessful() else 1)
