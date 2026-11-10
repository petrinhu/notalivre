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
    parser.add_argument("--prefixo", default="",
                        help="prefixo do caminho da API (DA-2.5, p.ex. /SefinNacional); sem a opção, a API fica na raiz")
    # NL-3.0j: cenários de fechamento para os slots de recusa de certificado (ordem do CISO, seção 4).
    parser.add_argument("--fechamento-adiado", action="store_true",
                        help="depois de falha de handshake, lê até o cliente fechar antes de fechar o socket (sem RST sobre o alerta)")
    parser.add_argument("--corte-rst-apos-handshake", action="store_true",
                        help="cada conexão com handshake concluído é cortada com RST, sem ler a requisição")
    parser.add_argument("--corte-rst-antes-tls", action="store_true",
                        help="cada conexão aceita é cortada com RST antes do TLS (sonda do RST)")
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
    srv = servidor.ServidorFalso(certs, oraculo, rota, tls_maximo=args.tls_maximo, prefixo=args.prefixo,
                                 fechamento_adiado=args.fechamento_adiado,
                                 corte_rst_apos_handshake=args.corte_rst_apos_handshake,
                                 corte_rst_antes_tls=args.corte_rst_antes_tls)
    srv.iniciar()
    print(f"PORTA {srv.porta}", flush=True)
    sys.stdin.read()  # fica no ar até o dono fechar o stdin
    srv.encerrar()
    # NL-3.0j (M2): conexões TCP aceitas, para o slot medir se o cliente reconectou (CISO, seção 4).
    print(f"CONEXOES {srv.conexoes}", flush=True)
    # NL-3.0j (E2): uma linha por conexão aceita, com o handshake, as requisições completas e os bytes entregues ao parser.
    for indice, reg in enumerate(srv.registro_conexoes, 1):
        print(f"CONEXAO_DETALHE {indice} handshake={int(reg['handshake'])} requisicoes={reg['requisicoes']} "
              f"bytes={reg['bytes']}", flush=True)
    # Ao sair, diz quantas requisições chegaram (DA-2.51): o e2e confere zero quando a CLI recusa antes do envio.
    print(f"REQUISICOES {srv.requisicoes}", flush=True)
    # NL-2.10a: o caminho de cada POST /nfse que chegou, para o e2e provar a rota (sem corpo, sem PII).
    print("CAMINHOS " + ",".join(p["caminho"] for p in srv.posts_recebidos if "caminho" in p), flush=True)
    # NL-2.10a, rodada 2: o URI do SignatureMethod de cada POST /nfse, para o e2e conferir o algoritmo do XML que chegou.
    print("ALGORITMOS " + ",".join(p.get("algoritmo") or "" for p in srv.posts_recebidos if "caminho" in p), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
