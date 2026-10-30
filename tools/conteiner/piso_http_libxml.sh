#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Piso do teste xmldsig.contextoDoParserBloqueiaRede (L-36, CISO 10:02). A libxml2 2.15 não tem cliente HTTP (NEWS 2.15.0),
# então o teste pula ali. A rodada reprova se nenhuma imagem rodou o teste: zero é sinal de piso quebrado, não de código limpo.
#
# Uso:
#   piso_http_libxml.sh <arquivo_piso>...
#   Cada arquivo é o piso_http_contextoDoParser.txt da pasta de build de uma imagem (o teste o grava a cada execução).
#
# Saída: uma linha por arquivo, e o resumo analisados/rodou/pulou/sem_marca. rc 1 se nenhum arquivo marca "rodou".
set -u

if [ "$#" -eq 0 ]; then
    echo "PISO-HTTP: REPROVADO, nenhum arquivo de piso informado"
    exit 1
fi

analisados=0
rodou=0
pulou=0
sem_marca=0
for arquivo in "$@"; do
    analisados=$((analisados + 1))
    if [ ! -f "$arquivo" ]; then
        echo "PISO-HTTP: sem arquivo: $arquivo"
        sem_marca=$((sem_marca + 1))
        continue
    fi
    linha=$(head -n 1 "$arquivo")
    case "$linha" in
        rodou*) rodou=$((rodou + 1)) ;;
        pulou*) pulou=$((pulou + 1)) ;;
        *) sem_marca=$((sem_marca + 1)) ;;
    esac
    echo "PISO-HTTP: $arquivo: $linha"
done

echo "PISO-HTTP: analisados=$analisados rodou=$rodou pulou=$pulou sem_marca=$sem_marca"
if [ "$rodou" -lt 1 ]; then
    echo "PISO-HTTP: REPROVADO, nenhuma imagem rodou o teste"
    exit 1
fi
exit 0
