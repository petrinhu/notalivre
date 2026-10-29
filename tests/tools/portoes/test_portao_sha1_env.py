# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Testes do portão da variável de SHA-1 (NL-2.6, DA-2.10), rodados pelo ctest como test_<nome>."""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

RAIZ = Path(__file__).resolve().parents[3]
PORTAO = RAIZ / "tools" / "portoes" / "portao_sha1_env.py"
VARIAVEL = "OPENSSL_ENABLE_SHA1_SIGNATURES"
LINHA_CONTAGEM = re.compile(r"encontrados=(\d+) analisados=(\d+) violacoes=(\d+) permitidos=(\d+)")


class TestePortaoSha1(unittest.TestCase):
    def _arvore(self, arquivos):
        base = os.environ.get("NL_TMP_BASE", "/var/tmp")
        raiz = tempfile.mkdtemp(prefix="sha1env-", dir=base)
        self.addCleanup(shutil.rmtree, raiz, True)
        for rel, texto in arquivos.items():
            destino = os.path.join(raiz, rel)
            os.makedirs(os.path.dirname(destino), exist_ok=True)
            with open(destino, "w", encoding="utf-8") as f:
                f.write(texto)
        return raiz

    def _rodar(self, raiz):
        r = subprocess.run([sys.executable, "-I", str(PORTAO), raiz], capture_output=True, text=True)
        return r.returncode, r.stdout + r.stderr

    def test_portaoSha1ArvoreRealLimpa(self):
        rc, saida = self._rodar(str(RAIZ))
        m = LINHA_CONTAGEM.search(saida)
        self.assertIsNotNone(m, saida)
        self.assertGreaterEqual(int(m.group(1)), 1, "varredura vazia não é árvore limpa")
        self.assertEqual(int(m.group(3)), 0, saida)
        self.assertEqual(rc, 0, saida)

    def test_portaoSha1SabotagemReprova(self):
        raiz = self._arvore({
            "src/vazamento.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("violacoes=1", saida)
        self.assertIn("src/vazamento.cpp", saida)

    def test_portaoSha1ViolacaoEmSrcOutReprova(self):
        # Nome de diretório não é isenção: "out" dentro de src/ é código como qualquer outro.
        raiz = self._arvore({
            "src/out/vazamento.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("violacoes=1", saida)
        self.assertIn("src/out/vazamento.cpp", saida)

    def test_portaoSha1ViolacaoEmSrcBuildXReprova(self):
        raiz = self._arvore({
            "src/build_x/vazamento.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("violacoes=1", saida)
        self.assertIn("src/build_x/vazamento.cpp", saida)

    def test_portaoSha1ArnesPermitido(self):
        raiz = self._arvore({
            "tools/servidor_falso/oraculo.py": f'ENV_SHA1 = "{VARIAVEL}"\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 0, saida)
        self.assertIn("violacoes=0", saida)
        self.assertIn("permitidos=1", saida)

    def test_portaoSha1PisoVazioReprova(self):
        raiz = self._arvore({})
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("encontrados=0", saida)

    def test_ciHigieneLigaPortaoSha1(self):
        # Job higiene do ci.yml (fronteira: o job seguinte, camadas). O portão tem de rodar sobre o workspace inteiro.
        texto = (RAIZ / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")
        self.assertIn("\n  higiene:", texto)
        bloco = texto.split("\n  higiene:", 1)[1].split("\n  camadas:", 1)[0]
        self.assertIn("python3 -I /src/tools/portoes/portao_sha1_env.py /src", bloco, "job higiene sem o portão SHA-1")
        self.assertIn("portao_sha1_env", bloco)


if __name__ == "__main__":
    suite = unittest.TestSuite([TestePortaoSha1(sys.argv[1])])
    resultado = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if resultado.wasSuccessful() else 1)
