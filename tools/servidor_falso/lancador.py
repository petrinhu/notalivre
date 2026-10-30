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
    parser.add_argument("--tls-maximo", choices=["1.2"], default=None,
                        help="versão máxima de TLS do cenário (rodada 4, DA-2.41); sem a opção, TLS 1.3 é o máximo")
    parser.add_argument("--com-oraculo", action="store_true",
                        help="liga o oráculo (xmllint e xmlsec1 de NL_XMLLINT e NL_XMLSEC1, XSD de NL_XSD_PRODREST_DIR)")
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
    oraculo = None
    if args.com_oraculo:
        import os
        from servidor_falso import oraculo as oraculo_mod
        base = os.environ.get("NL_TMP_BASE", "/var/tmp")
        oraculo = oraculo_mod.Oraculo(os.environ["NL_XMLLINT"], os.environ["NL_XMLSEC1"],
                                      os.path.join(os.environ["NL_XSD_PRODREST_DIR"], "DPS_v1.01.xsd"),
                                      certs.ca, os.path.join(base, "lancador-oraculo-%d" % os.getpid()))
    srv = servidor.ServidorFalso(certs, oraculo, rota, tls_maximo=args.tls_maximo)
    srv.iniciar()
    print(f"PORTA {srv.porta}", flush=True)
    sys.stdin.read()  # fica no ar até o dono fechar o stdin
    srv.encerrar()
    return 0


if __name__ == "__main__":
    sys.exit(main())
