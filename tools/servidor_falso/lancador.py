# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Lançador do servidor falso para os testes C++ de integração do transporte (NL-2.5). Ferramenta de teste.

Uso: python3 -I lancador.py PASTA_FIXTURES [--falha METODO CAMINHO FALHA]...

Usa a AC de teste da NL-2.1 (ac.pem) e o certificado de servidor dela (servidor.pem e servidor.key, SAN localhost e
127.0.0.1). O servidor só confia nessa AC. Imprime "PORTA n" em stdout e fica no ar até o stdin fechar.
"""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from servidor_falso import certificados, roteiro, servidor  # noqa: E402


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("fixtures")
    parser.add_argument("--falha", nargs=3, action="append", default=[],
                        metavar=("METODO", "CAMINHO", "FALHA"))
    args = parser.parse_args()
    pasta = Path(args.fixtures)
    certs = certificados.Certificados(
        ca=str(pasta / "ac.pem"),
        servidor_cert=str(pasta / "servidor.pem"),
        servidor_key=str(pasta / "servidor.key"),
        cliente_cert=None,
        cliente_key=None,
    )
    rota = roteiro.Roteiro()
    for metodo, caminho, falha in args.falha:
        rota.agendar(metodo, caminho, falha)
    srv = servidor.ServidorFalso(certs, None, rota)
    srv.iniciar()
    print(f"PORTA {srv.porta}", flush=True)
    sys.stdin.read()  # fica no ar até o dono fechar o stdin
    srv.encerrar()
    return 0


if __name__ == "__main__":
    sys.exit(main())
