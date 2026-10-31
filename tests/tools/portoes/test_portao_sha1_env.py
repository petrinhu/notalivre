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

    def _repo(self, arquivos, rastreados=()):
        """Árvore que é repositório git. Só os caminhos em rastreados entram no índice (o git os rastreia)."""
        raiz = self._arvore(arquivos)
        subprocess.run(["git", "-C", raiz, "init", "-q"], check=True)
        for rel in rastreados:
            subprocess.run(["git", "-C", raiz, "add", "--", rel], check=True)
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

    def test_portaoSha1PycDerivadoNaoReprova(self):
        # Bytecode de um .py permitido (o oráculo compilado no __pycache__) não é violação: o fonte já é varrido.
        # Sem essa regra, o CI reprovou o build (run 40804430962): o .pyc de oraculo.py tinha a string.
        raiz = self._arvore({
            "tools/servidor_falso/oraculo.py": f'ENV_SHA1 = "{VARIAVEL}"\n',
            "tools/servidor_falso/__pycache__/oraculo.cpython-314.pyc": f"lixo {VARIAVEL} lixo",
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 0, saida)
        self.assertIn("violacoes=0", saida)

    def test_portaoSha1PycSemOrigemReprova(self):
        # Pyc órfão (sem .py de origem na árvore) com a string é violação: a regra acima não pode cegar o portão.
        raiz = self._arvore({
            "src/limpo.cpp": "int a = 1;\n",
            "src/out/__pycache__/fantasma.cpython-314.pyc": f"lixo {VARIAVEL} lixo",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("violacoes=1", saida)

    def test_portaoSha1VazamentoEmThirdPartyReprova(self):
        # Escopo do portão é a árvore inteira (L-36): third_party/ entra na varredura (rodada 4, S16).
        raiz = self._arvore({
            "third_party/vendor/vazamento.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("third_party/vendor/vazamento.cpp", saida)

    def test_portaoSha1PastaNovaNaRaizReprova(self):
        # Pasta nova na raiz também entra: a lista de raízes deixou de existir (rodada 4, S17).
        raiz = self._arvore({
            "novo_modulo/vazamento.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("novo_modulo/vazamento.cpp", saida)

    def test_portaoSha1PycComSufixoFalsoNaoDerivadoReprova(self):
        # oraculo.evil.cpython-314.pyc não é bytecode de oraculo.py: o stem não pode conter ponto (rodada 4, S03).
        raiz = self._arvore({
            "tools/servidor_falso/oraculo.py": f'ENV_SHA1 = "{VARIAVEL}"\n',
            "tools/servidor_falso/__pycache__/oraculo.evil.cpython-314.pyc": f"lixo {VARIAVEL} lixo",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("violacoes=1", saida)
        self.assertIn("oraculo.evil.cpython-314.pyc", saida)

    def test_portaoSha1PycSemVariavelNaFonteReprova(self):
        # Bytecode com a variável não herda permissão de uma fonte que não a contém (rodada 4, S04).
        raiz = self._arvore({
            "tools/servidor_falso/oraculo.py": "ENV_SHA1 = 'sem a variavel'\n",
            "tools/servidor_falso/__pycache__/oraculo.cpython-314.pyc": f"lixo {VARIAVEL} lixo",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("violacoes=1", saida)

    def test_portaoSha1MarkdownDocumentalPassa(self):
        # Exceção escrita no script: .md é documentação, nunca compilada nem executada (ADR e plano citam a variável).
        raiz = self._arvore({
            "docs/adr/documento.md": f"A variável {VARIAVEL} é citada por nome.\n",
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 0, saida)
        self.assertIn("violacoes=0", saida)

    def test_portaoSha1WorktreeNaRaizPassa(self):
        # Exceção 4 (L-55): worktrees/ na RAIZ é cópia de trabalho NÃO rastreada: o git a ignora, o portão também.
        raiz = self._repo({
            "worktrees/nl-x/tools/servidor_falso/vazado.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 0, saida)
        self.assertIn("violacoes=0", saida)

    def test_portaoSha1WorktreeRastreadoNaRaizReprova(self):
        # A poda vale só para o que o git não rastreia (rodada 6, achado 2): arquivo rastreado em worktrees/ é varrido.
        raiz = self._repo({
            "worktrees/nl-x/tools/servidor_falso/vazado.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        }, rastreados=["worktrees/nl-x/tools/servidor_falso/vazado.cpp", "src/limpo.cpp"])
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("worktrees/nl-x/tools/servidor_falso/vazado.cpp", saida)

    def test_portaoSha1MarkdownSrcReprova(self):
        # Exceção de .md só em docs/ e na raiz (rodada 6, achado 1): markdown dentro de src/ é varrido.
        raiz = self._arvore({
            "src/notas.md": f"A variável {VARIAVEL} aparece aqui.\n",
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("src/notas.md", saida)

    def test_portaoSha1WorktreeAninhadoReprova(self):
        # A exceção vale só na raiz: worktrees/ em outro nível é pasta comum e continua sendo varrida.
        raiz = self._arvore({
            "src/worktrees/vazado.cpp": f'const char *v = "{VARIAVEL}=1";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("src/worktrees/vazado.cpp", saida)

    def test_portaoSha1TesteXmldsigPermitidoPeloCaminhoExato(self):
        # DA-2.37: o teste do assinador cita a variável para provar que o app não muda com ela. Entra por caminho exato.
        raiz = self._arvore({
            "tests/unit/foundation/xmldsig/tst_assinador_xml.cpp": f'const char *v = "{VARIAVEL}";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 0, saida)
        self.assertIn("violacoes=0", saida)

    def test_portaoSha1OutroTesteDoXmldsigReprova(self):
        # DA-2.37: a permissão é do arquivo exato. Outro arquivo na mesma pasta de testes continua reprovando.
        raiz = self._arvore({
            "tests/unit/foundation/xmldsig/outro_teste.cpp": f'const char *v = "{VARIAVEL}";\n',
            "src/limpo.cpp": "int a = 1;\n",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("tests/unit/foundation/xmldsig/outro_teste.cpp", saida)

    def test_portaoSha1BuildComCacheNaRaizNaoReprova(self):
        # Exceção 5 (DA-2.62): _build na raiz com CMakeCache.txt é saída do build; o objeto do teste permitido não reprova.
        raiz = self._arvore({
            "src/codigo.cpp": "int x = 1;\n",
            "_build/CMakeCache.txt": "CMAKE_BUILD_TYPE:STRING=Release\n",
            "_build/tests/unit/foundation/xmldsig/CMakeFiles/tst.dir/tst.cpp.o": "OPENSSL_ENABLE_SHA1_SIGNATURES",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 0, saida)
        self.assertIn("violacoes=0", saida)

    def test_portaoSha1BuildSemCacheNaRaizReprova(self):
        # Sem CMakeCache.txt, _build na raiz não é build do CMake: é arquivo comum e tem de ser varrido.
        raiz = self._arvore({
            "src/codigo.cpp": "int x = 1;\n",
            "_build/saida.o": "OPENSSL_ENABLE_SHA1_SIGNATURES",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("VIOLACAO _build/saida.o", saida)

    def test_portaoSha1BuildAninhadoComCacheReprova(self):
        # Só a raiz é podada: um _build dentro de src/ com CMakeCache.txt continua sendo varrido.
        raiz = self._arvore({
            "src/_build/CMakeCache.txt": "CMAKE_BUILD_TYPE:STRING=Release\n",
            "src/_build/saida.o": "OPENSSL_ENABLE_SHA1_SIGNATURES",
        })
        rc, saida = self._rodar(raiz)
        self.assertEqual(rc, 1, saida)
        self.assertIn("VIOLACAO src/_build/saida.o", saida)

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
