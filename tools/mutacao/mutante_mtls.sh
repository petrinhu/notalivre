#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mutante CERT_OPTIONAL no servidor falso (NL-2.11, DA-2.60 item 2; L-36, L-27, L-49). Roda a cada push em onda-* e main e em
# pull_request, no job nativo ubuntu26.04 (não em fatia/*).
#
# Sabota só uma CÓPIA extraída do SHA (git archive), fora do workspace: CERT_REQUIRED vira CERT_OPTIONAL em
# tools/servidor_falso/servidor.py. O lançador é Python, lido em tempo de execução (NL_LANCADOR), então a cópia não
# precisa de recompilar além do próprio alvo tst_transporte_https.
#
# Ordem (cada passo exige o anterior):
#   1. configura e constrói a cópia, sem sabotagem;
#   2. LINHA DE BASE: o slot passa na cópia (ctest rc 0). Sem isso, PARADA: não há o que matar;
#   3. confere CERT_REQUIRED antes (1 ocorrência de código) e troca por CERT_OPTIONAL; confere CERT_REQUIRED depois (0) e
#      CERT_OPTIONAL depois (1), e imprime o sha256 do servidor antes e depois;
#   4. MORTE: o slot falha E o log tem a assinatura exata da asserção do handshake sem certificado. Falha sem essa
#      assinatura (ambiente, timeout, crash, outra asserção) NÃO é morte: é PARADA.
#
# Contagem: encontradas (tamanho da lista MUTANTES), aplicadas, mortas, sobreviventes. Sai por trap EXIT em todo caminho,
# inclusive zero. Sobrevivente ou mutante não aplicado sai com 1.
#
# Uso: mutante_mtls.sh REPO SHA TRABALHO
#   REPO       repositório git de onde sai o SHA
#   SHA        commit (40 hex)
#   TRABALHO   pasta NOVA fora do workspace (ex.: $RUNNER_TEMP/mutante-mtls)
# Variáveis (do job): QT_PREFIXO ou CMAKE_PREFIX_PATH (Qt 6.12), NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD),
# NL_TMP_BASE (TMPDIR da cópia, como o preset). Exige CMake com ctest --no-tests=error (3.22 ou mais novo).
set -u

REPO="${1:?uso: mutante_mtls.sh REPO SHA TRABALHO}"
SHA="${2:?uso: mutante_mtls.sh REPO SHA TRABALHO}"
TRAB="${3:?uso: mutante_mtls.sh REPO SHA TRABALHO}"
QT="${QT_PREFIXO:-${CMAKE_PREFIX_PATH:?defina QT_PREFIXO ou CMAKE_PREFIX_PATH}}"
ZIP_CACHE="${NL_XSD_ZIP_CACHE:?defina NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD)}"
export TMPDIR="${NL_TMP_BASE:-/var/tmp}"

SLOT="transporte.semCertificadoDeClienteHandshakeRecusado"
REGEX_SLOT='^transporte[.]semCertificadoDeClienteHandshakeRecusado$'
ASSERCAO="FAIL!  : TesteTransporteHttps::semCertificadoDeClienteHandshakeRecusado() '!resposta.has_value()' returned FALSE"
MUTANTES=(CERT_OPTIONAL)

encontradas=${#MUTANTES[@]}
aplicadas=0
mortas=0
sobreviventes=0
contagem=""

imprime_contagem() {
    sobreviventes=$((aplicadas - mortas))
    contagem="encontradas=$encontradas aplicadas=$aplicadas mortas=$mortas sobreviventes=$sobreviventes"
    echo "mutantes servidor falso: $contagem"
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        printf '%s\n' "- Mutantes do servidor falso (DA-2.60): $contagem" >> "$GITHUB_STEP_SUMMARY"
    fi
}
trap imprime_contagem EXIT

if [ -e "$TRAB" ]; then
    echo "PARADA: o trabalho já existe ($TRAB); o mutante precisa de pasta nova"
    exit 2
fi
mkdir -p "$TRAB/src" || exit 2
git -C "$REPO" archive "$SHA" | tar -x -C "$TRAB/src" || { echo "PARADA: git archive de $SHA falhou"; exit 2; }

# 1. Configura e constrói a cópia, sem sabotagem (fidelidade ao preset: mesmo TMPDIR; banco de comandos exportado).
echo "cmake: $(cmake --version | head -n 1)"
cmake -S "$TRAB/src" -B "$TRAB/build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
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

# 2. Linha de base: o slot passa na cópia sem sabotagem.
ctest --test-dir "$TRAB/build" --no-tests=error -R "$REGEX_SLOT" --output-on-failure > "$TRAB/base.log" 2>&1
rc_base=$?
if [ "$rc_base" -ne 0 ]; then
    echo "PARADA: a linha de base do slot não passa na cópia (rc=$rc_base); sem base não há mutante a matar ($TRAB/base.log)"
    exit 2
fi
echo "linha de base: $SLOT passa na cópia (rc 0)"

# 3. Sabotagem: só o texto do CERT_REQUIRED na cópia (nunca o arquivo rastreado do workspace). Confere antes e depois.
SERVIDOR="$TRAB/src/tools/servidor_falso/servidor.py"
antes=$(grep -c -F 'self.contexto.verify_mode = ssl.CERT_REQUIRED' "$SERVIDOR" || true)
if [ "$antes" != 1 ]; then
    echo "PARADA: CERT_REQUIRED não existia 1 vez antes da sabotagem (ocorrências=$antes)"
    exit 1
fi
sha_antes=$(sha256sum "$SERVIDOR" | cut -d' ' -f1)
sed -i 's/self\.contexto\.verify_mode = ssl\.CERT_REQUIRED/self.contexto.verify_mode = ssl.CERT_OPTIONAL/' "$SERVIDOR"
req_depois=$(grep -c -F 'ssl.CERT_REQUIRED' "$SERVIDOR" || true)
opt_depois=$(grep -c -F 'self.contexto.verify_mode = ssl.CERT_OPTIONAL' "$SERVIDOR" || true)
sha_depois=$(sha256sum "$SERVIDOR" | cut -d' ' -f1)
echo "servidor.py: CERT_REQUIRED antes=$antes depois=$req_depois; CERT_OPTIONAL depois=$opt_depois; sha256 antes=$sha_antes depois=$sha_depois"
if [ "$req_depois" != 0 ] || [ "$opt_depois" != 1 ]; then
    echo "PARADA: a sabotagem não chegou ao servidor da cópia (CERT_REQUIRED depois=$req_depois, CERT_OPTIONAL depois=$opt_depois)"
    exit 1
fi
aplicadas=1

# 4. Morte: o slot falha E o log tem a assinatura exata da asserção do handshake sem certificado.
ctest --test-dir "$TRAB/build" --no-tests=error -R "$REGEX_SLOT" --output-on-failure > "$TRAB/ctest.log" 2>&1
rc=$?
assercao=$(grep -c -F "$ASSERCAO" "$TRAB/ctest.log" || true)
if [ "$rc" -eq 0 ]; then
    echo "mutante=${MUTANTES[0]} slot=$SLOT rc=0 veredito=VIVO (o slot passou com o mutante: a sabotagem não o pegou) sha=$SHA"
    exit 1
fi
if [ "$assercao" -lt 1 ]; then
    echo "mutante=${MUTANTES[0]} slot=$SLOT rc=$rc veredito=NAO-MORTO (falha sem a asserção do handshake: ambiente, timeout, crash ou outra asserção) sha=$SHA"
    echo "PARADA: falha que não é a morte esperada ($TRAB/ctest.log)"
    exit 1
fi
mortas=1
echo "mutante=${MUTANTES[0]} slot=$SLOT rc=$rc veredito=MORTO (asserção do handshake sem certificado) sha=$SHA"
exit 0
