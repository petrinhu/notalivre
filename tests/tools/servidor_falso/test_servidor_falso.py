# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Testes do servidor falso da API (NL-2.6). Cada caso roda como um processo do ctest: test_<nome>.

INFERÊNCIA até a NL-2.10: respostas, códigos (exceto E0714, catálogo do Anexo I), chave de 50 posições e o conteúdo
do campo de NFS-e são do servidor falso (pesquisa g). O oráculo é o xmllint e o xmlsec1, nunca o código do app.
"""

import base64
import gzip
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
from pathlib import Path

RAIZ = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(RAIZ / "tools"))

from servidor_falso import certificados, envelope, oraculo, referencia, roteiro, servidor  # noqa: E402

FIXTURE_DPS = RAIZ / "tests" / "fixtures" / "dps" / "valida_dps_mei_minima.xml"


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
        self.dps = referencia.assinar(self.oraculo, FIXTURE_DPS.read_bytes(),
                                      self.certs.cliente_key, self.certs.cliente_cert)
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
        caminho = os.path.join(self.dir, "verificar-direto.xml")
        with open(caminho, "wb") as f:
            f.write(self.dps)
        comando = self.oraculo.comando_verificar(caminho)
        r = subprocess.run(comando, env=self.oraculo._env(), capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
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


if __name__ == "__main__":
    suite = unittest.TestSuite([TesteServidorFalso(sys.argv[1])])
    resultado = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if resultado.wasSuccessful() else 1)
