#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mutante CERT_OPTIONAL no servidor falso (NL-2.11, DA-2.60 item 2; L-36 e L-27). Roda a cada push, no job nativo ubuntu26.04.
# Sabota só uma CÓPIA extraída do SHA (git archive), fora do workspace: CERT_REQUIRED vira CERT_OPTIONAL em
# tools/servidor_falso/servidor.py. A cópia é configurada e só o alvo tst_transporte_https é reconstruído nela, porque o
# lançador do binário (NL_LANCADOR) é gravado em tempo de compilação e tem de apontar para a cópia. O slot
# transporte.semCertificadoDeClienteHandshakeRecusado tem de morrer (ficar vermelho). A linha de base (vivo) é o ctest
# do próprio workspace no mesmo job. Sai com 1 se o slot não morrer, ou se a sabotagem não chegar ao código.
#
# Uso: mutante_mtls.sh REPO SHA TRABALHO
#   REPO       repositório git de onde sai o SHA
#   SHA        commit (40 hex)
#   TRABALHO   pasta NOVA fora do workspace (ex.: $RUNNER_TEMP/mutante-mtls)
# Variáveis (do job): QT_PREFIXO ou CMAKE_PREFIX_PATH (Qt 6.12), NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD)
set -u

REPO="${1:?uso: mutante_mtls.sh REPO SHA TRABALHO}"
SHA="${2:?uso: mutante_mtls.sh REPO SHA TRABALHO}"
TRAB="${3:?uso: mutante_mtls.sh REPO SHA TRABALHO}"
QT="${QT_PREFIXO:-${CMAKE_PREFIX_PATH:?defina QT_PREFIXO ou CMAKE_PREFIX_PATH}}"
ZIP_CACHE="${NL_XSD_ZIP_CACHE:?defina NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD)}"
SLOT="transporte.semCertificadoDeClienteHandshakeRecusado"

if [ -e "$TRAB" ]; then
    echo "PARADA: o trabalho já existe ($TRAB); o mutante precisa de pasta nova"
    exit 2
fi
mkdir -p "$TRAB/src" || exit 2
git -C "$REPO" archive "$SHA" | tar -x -C "$TRAB/src" || { echo "PARADA: git archive de $SHA falhou"; exit 2; }

# Sabotagem: só o texto do CERT_REQUIRED na cópia (nunca o arquivo rastreado do workspace).
SERVIDOR="$TRAB/src/tools/servidor_falso/servidor.py"
sed -i 's/self\.contexto\.verify_mode = ssl\.CERT_REQUIRED/self.contexto.verify_mode = ssl.CERT_OPTIONAL/' "$SERVIDOR"
chegou=$(grep -c 'self.contexto.verify_mode = ssl.CERT_OPTIONAL' "$SERVIDOR" || true)
if [ "$chegou" != 1 ]; then
    echo "PARADA: a sabotagem não chegou ao servidor da cópia (ocorrências=$chegou)"
    exit 1
fi

cmake -S "$TRAB/src" -B "$TRAB/build" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="$QT" -DNL_XSD_ZIP_CACHE="$ZIP_CACHE" > "$TRAB/configure.log" 2>&1
rc_cfg=$?
if [ "$rc_cfg" -ne 0 ]; then
    echo "PARADA: configure da cópia falhou (rc=$rc_cfg); ver $TRAB/configure.log"
    exit 2
fi

cmake --build "$TRAB/build" --target tst_transporte_https -j 4 > "$TRAB/build.log" 2>&1
rc_bld=$?
if [ "$rc_bld" -ne 0 ]; then
    echo "PARADA: build do alvo tst_transporte_https na cópia falhou (rc=$rc_bld); ver $TRAB/build.log"
    exit 2
fi

ctest --test-dir "$TRAB/build" -R "^transporte[.]semCertificadoDeClienteHandshakeRecusado\$" --output-on-failure \
    > "$TRAB/ctest.log" 2>&1
rc=$?

# Contagem do passo (DA-2.60): encontradas = mutantes da lista (um hoje); aplicadas = sabotagem chegou ao código;
# mortas = o slot falhou com o mutante; sobreviventes = aplicadas que não morreram. Sobrevivente reprova.
encontradas=1
aplicadas=1
mortas=0
if [ "$rc" -ne 0 ]; then
    mortas=1
    veredito="MORTO (o slot falhou com o mutante)"
else
    veredito="VIVO (o slot passou com o mutante: a sabotagem não o pegou)"
fi
sobreviventes=$((aplicadas - mortas))
contagem="encontradas=$encontradas aplicadas=$aplicadas mortas=$mortas sobreviventes=$sobreviventes"
echo "mutantes servidor falso: $contagem"
echo "mutante=CERT_OPTIONAL slot=$SLOT rc=$rc veredito=$veredito sha=$SHA"
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    printf '%s\n' "- Mutantes do servidor falso (DA-2.60): $contagem; CERT_OPTIONAL: $veredito" >> "$GITHUB_STEP_SUMMARY"
fi
if [ "$encontradas" -ne "$aplicadas" ] || [ "$sobreviventes" -ne 0 ]; then
    echo "PARADA: mutantes sobreviventes ou não aplicados ($contagem)"
    exit 1
fi
exit 0
