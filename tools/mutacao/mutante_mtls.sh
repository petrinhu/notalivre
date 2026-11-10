#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mutantes de mTLS no servidor falso (NL-2.11, DA-2.60 item 2; NL-3.0j, E6; L-36, L-27, L-49). Roda a cada push em onda-* e
# main e em pull_request, no job nativo ubuntu26.04 (não em fatia/*).
#
# Sabota só uma CÓPIA extraída do SHA (git archive), fora do workspace: a linha que exige o certificado de cliente em
# tools/servidor_falso/servidor.py recebe, em cada par, um valor mais fraco (CERT_OPTIONAL ou CERT_NONE). O lançador é
# Python, lido em tempo de execução (NL_LANCADOR), então a cópia não precisa de recompilar além do alvo tst_transporte_https.
#
# Pares (mutante, slot). Cada par exige que o slot passe sem sabotagem e que falhe com a asserção exata do slot:
#   1. CERT_OPTIONAL  x semCertificadoDeClienteHandshakeRecusado (TLS 1.2, fechamento adiado)
#   2. CERT_OPTIONAL  x recusaSemCertificadoTls13FechamentoAdiado (TLS 1.3, fechamento adiado)
#   3. CERT_NONE      x certificadoDeOutraAcRecusado (TLS 1.2, fechamento adiado)
#   4. CERT_NONE      x recusaOutraAcTls13FechamentoAdiado (TLS 1.3, fechamento adiado)
#
# Ordem por par (cada passo exige o anterior):
#   a. restaura o servidor.py pristino da cópia;
#   b. LINHA DE BASE: o slot passa na cópia (ctest rc 0). Sem isso, PARADA: não há o que matar;
#   c. confere a linha de exigência antes (1 ocorrência) e troca pelo valor do mutante; confere depois (0 ocorrências da
#      exigência de certificado, 1 do valor do mutante) e imprime o sha256 antes e depois;
#   d. MORTE: o slot falha (rc diferente de 0) E o log tem a asserção exata do slot. Falha sem essa asserção (ambiente,
#      timeout, crash, outra asserção) NÃO é morte: é PARADA. Com rc 0, é VIVO: o mutante sobreviveu;
#   e. a linha FAIL da asserção sai na saída padrão deste script (o log do contêiner a leva);
#   f. restaura o servidor.py pristino e confere o sha256 de volta.
#
# Contagem: encontradas (tamanho da lista de pares), aplicadas, mortas, sobreviventes. Sai por trap EXIT em todo caminho,
# inclusive zero. Sobrevivente ou par não aplicado sai com 1.
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

# Pares (mutante, slot). Os quatro vetores têm o mesmo tamanho; o índice i descreve o par i.
MUTANTES=(CERT_OPTIONAL CERT_OPTIONAL CERT_NONE CERT_NONE)
SLOTS_NOMES=(semCertificadoDeClienteHandshakeRecusado recusaSemCertificadoTls13FechamentoAdiado certificadoDeOutraAcRecusado recusaOutraAcTls13FechamentoAdiado)
ASSERCOES=()
for nome in "${SLOTS_NOMES[@]}"; do
    ASSERCOES+=("FAIL!  : TesteTransporteHttps::${nome}() '!resposta.has_value()' returned FALSE")
done

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
        printf '%s\n' "- Mutantes do servidor falso (DA-2.60, NL-3.0j): $contagem" >> "$GITHUB_STEP_SUMMARY"
    fi
}
trap imprime_contagem EXIT

if [ -e "$TRAB" ]; then
    echo "PARADA: o trabalho já existe ($TRAB); o mutante precisa de pasta nova"
    exit 2
fi
mkdir -p "$TRAB/src" || exit 2
git -C "$REPO" archive "$SHA" | tar -x -C "$TRAB/src" || { echo "PARADA: git archive de $SHA falhou"; exit 2; }

# Configura e constrói a cópia, sem sabotagem (fidelidade ao preset: mesmo TMPDIR; banco de comandos exportado).
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

SERVIDOR="$TRAB/src/tools/servidor_falso/servidor.py"
PRISTINO="$TRAB/servidor.pristino.py"
cp -- "$SERVIDOR" "$PRISTINO" || { echo "PARADA: cópia pristina do servidor falhou"; exit 2; }
sha_pristino=$(sha256sum "$SERVIDOR" | cut -d' ' -f1)
echo "servidor.py pristino: sha256=$sha_pristino"

for i in "${!MUTANTES[@]}"; do
    mutante="${MUTANTES[$i]}"
    nome="${SLOTS_NOMES[$i]}"
    slot="transporte.$nome"
    regex_slot="^transporte[.]${nome}\$"
    assercao="${ASSERCOES[$i]}"
    echo "--- par $((i + 1))/$encontradas: mutante=$mutante slot=$slot"

    # a. restaura o pristino antes de cada par.
    cp -- "$PRISTINO" "$SERVIDOR" || { echo "PARADA: restauração do servidor falhou"; exit 2; }

    # b. linha de base: o slot passa na cópia sem sabotagem.
    ctest --test-dir "$TRAB/build" --no-tests=error -R "$regex_slot" --output-on-failure > "$TRAB/base-$nome.log" 2>&1
    rc_base=$?
    if [ "$rc_base" -ne 0 ]; then
        echo "PARADA: a linha de base de $slot não passa na cópia (rc=$rc_base); sem base não há mutante a matar ($TRAB/base-$nome.log)"
        exit 2
    fi
    echo "linha de base: $slot passa na cópia (rc 0)"

    # c. sabotagem: só a linha de exigência de certificado na cópia (nunca o arquivo rastreado do workspace).
    exigencia='self\.contexto\.verify_mode = ssl\.CERT_REQUIRED'
    novo_valor="ssl.$mutante"
    antes=$(grep -c -E "$exigencia" "$SERVIDOR" || true)
    if [ "$antes" != 1 ]; then
        echo "PARADA: a linha de exigência não existia 1 vez antes da sabotagem (ocorrências=$antes)"
        exit 1
    fi
    sha_antes=$(sha256sum "$SERVIDOR" | cut -d' ' -f1)
    sed -i "s/self\\.contexto\\.verify_mode = ssl\\.CERT_REQUIRED/self.contexto.verify_mode = $novo_valor/" "$SERVIDOR"
    req_depois=$(grep -c -F 'ssl.CERT_REQUIRED' "$SERVIDOR" || true)
    novo_depois=$(grep -c -F "self.contexto.verify_mode = $novo_valor" "$SERVIDOR" || true)
    sha_depois=$(sha256sum "$SERVIDOR" | cut -d' ' -f1)
    echo "servidor.py: CERT_REQUIRED antes=$antes depois=$req_depois; $novo_valor depois=$novo_depois; sha256 antes=$sha_antes depois=$sha_depois"
    if [ "$req_depois" != 0 ] || [ "$novo_depois" != 1 ]; then
        echo "PARADA: a sabotagem não chegou ao servidor da cópia (CERT_REQUIRED depois=$req_depois, $novo_valor depois=$novo_depois)"
        exit 1
    fi
    aplicadas=$((aplicadas + 1))

    # d. morte: o slot falha E o log tem a asserção exata do slot.
    ctest --test-dir "$TRAB/build" --no-tests=error -R "$regex_slot" --output-on-failure > "$TRAB/ctest-$nome.log" 2>&1
    rc=$?
    casos=$(grep -c -F "$assercao" "$TRAB/ctest-$nome.log" || true)
    # e. a linha FAIL vai para a saída padrão deste script (o log do contêiner a leva).
    grep -F "$assercao" "$TRAB/ctest-$nome.log" | sed 's/^/linha FAIL: /' || true
    if [ "$rc" -eq 0 ]; then
        echo "mutante=$mutante slot=$slot rc=0 veredito=VIVO (o slot passou com o mutante: a sabotagem não o pegou) sha=$SHA"
    elif [ "$casos" -lt 1 ]; then
        echo "mutante=$mutante slot=$slot rc=$rc veredito=NAO-MORTO (falha sem a asserção do slot: ambiente, timeout, crash ou outra asserção) sha=$SHA"
        echo "PARADA: falha que não é a morte esperada ($TRAB/ctest-$nome.log)"
        exit 1
    else
        mortas=$((mortas + 1))
        echo "mutante=$mutante slot=$slot rc=$rc veredito=MORTO (asserção do slot) sha=$SHA"
    fi

    # f. restaura o pristino e confere o sha256 de volta.
    cp -- "$PRISTINO" "$SERVIDOR" || { echo "PARADA: restauração do servidor falhou"; exit 2; }
    sha_volta=$(sha256sum "$SERVIDOR" | cut -d' ' -f1)
    if [ "$sha_volta" != "$sha_pristino" ]; then
        echo "PARADA: o servidor da cópia não voltou ao pristino (sha256 $sha_volta)"
        exit 2
    fi
done

if [ "$mortas" -ne "$encontradas" ]; then
    exit 1
fi
exit 0
