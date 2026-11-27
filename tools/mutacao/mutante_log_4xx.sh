#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mutantes do log do 4xx sem código E (NL-3.L, passo 15; NF-12, NF-33, L-27, L-35, L-36, L-49). Privacidade: o corpo da
# resposta da Sefin nunca pode ir ao log. Roda no job nativo ubuntu26.04, a cada PR ou push em onda-* e main, e só quando o
# diff toca o log do 4xx (passo `caminhos` do ci.yml; fail-secure: sem base confiável, roda).
#
# Sabota só uma CÓPIA extraída do SHA (git archive), fora do workspace: uma linha do fonte do evento
# src/mid/sefin/cliente/registro_resposta_sem_codigo.cpp recebe, em cada par, um mutante. Diferente do mTLS (servidor em
# Python, lido em tempo de execução), o alvo é C++: cada par recompila só o executável do slot (o mesmo alvo que o
# tst_* do preset debug constrói), com -Werror do nl_warnings. Erro de compilação do mutante é PARADA, nunca morte.
#
# Escolha dos mutantes (um por par, cada um com slot que o mata pela asserção do próprio arquivo de teste):
#   1. MIDIA_CRUA: tipoMidiaParaLog devolve o Content-Type cru no lugar de "outro". Entra no log o cabeçalho que o servidor
#      mandou (aqui, um CPF no Content-Type). Morre em tst_registro_resposta_sem_codigo tipoConteudoForaDoConjuntoViraOutro.
#      Escolhido porque é o vazamento de privacidade do conjunto fechado: o conjunto existe para isso.
#   2. CABECALHO_NO_CAMPO: o campo tipoMidia da função registrarRespostaSemCodigo grava o cabeçalho cru, em vez do valor
#      do conjunto fechado. Fica na função, e não na chamada de cliente_sefin.cpp, de propósito: pela chamada, o valor
#      sempre passa por registrarRespostaSemCodigo, e o mutante na função cobre os dois caminhos. Morre em
#      mid.sefin.cliente status403HtmlSemCodigoDefinitivoSemNovaTentativa (o servidor falso devolve
#      "text/html; charset=utf-8" e o log tem de trazer "html").
#   3. BYTES_NEGATIVOS: bytesResposta negativo deixa de virar 0. Viola a asserção real de semTipoConteudoViraAusente
#      ("tamanho negativo tem de virar 0"). Morre em tst_registro_resposta_sem_codigo semTipoConteudoViraAusente.
#
# Ordem por par (cada passo exige o anterior):
#   a. restaura o fonte pristino da cópia e confere o sha256;
#   b. recompila o alvo do par (sem sabotagem) e grava o sha256 do executável de base;
#   c. LINHA DE BASE: o slot passa na cópia (ctest rc 0). Sem isso, PARADA: não há o que matar;
#   d. confere a troca antes (1 ocorrência do trecho antigo, 0 do novo) e troca pelo trecho do mutante; confere depois
#      (0 do antigo, 1 do novo) e imprime o sha256 do fonte antes e depois;
#   e. recompila o alvo mutado e confere que o executável mudou (sha256 diferente da base). Igual é PARADA: o mutante não
#      entrou no binário;
#   f. MORTE: o slot falha (rc diferente de 0) E a saída tem a asserção exata do slot, na forma do QTest
#      "FAIL!  : Classe::slot() 'expr' returned FALSE. (mensagem)", com o nome e a mensagem do par. Falha sem essa linha
#      (ambiente, timeout, crash, outra asserção) NÃO é morte: é PARADA. Com rc 0, é VIVO: o mutante sobreviveu;
#   g. a linha FAIL da asserção sai na saída padrão deste script (o log do contêiner a leva);
#   h. restaura o fonte pristino e confere o sha256 de volta.
#
# Contagem: encontradas (tamanho da lista de pares), aplicadas, mortas, sobreviventes. Sai por trap EXIT em todo caminho,
# inclusive zero. Sobrevivente ou par não aplicado sai com 1; PARADA sai com 2.
#
# Uso: mutante_log_4xx.sh REPO SHA TRABALHO
#   REPO       repositório git de onde sai o SHA
#   SHA        commit (40 hex)
#   TRABALHO   pasta NOVA fora do workspace (ex.: $RUNNER_TEMP/mutante-log-4xx)
# Variáveis (do job): QT_PREFIXO ou CMAKE_PREFIX_PATH (Qt 6.12), NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD),
# NL_TMP_BASE (TMPDIR da cópia, como o preset). Exige CMake com ctest --no-tests=error (3.22 ou mais novo).
set -u

REPO="${1:?uso: mutante_log_4xx.sh REPO SHA TRABALHO}"
SHA="${2:?uso: mutante_log_4xx.sh REPO SHA TRABALHO}"
TRAB="${3:?uso: mutante_log_4xx.sh REPO SHA TRABALHO}"
QT="${QT_PREFIXO:-${CMAKE_PREFIX_PATH:?defina QT_PREFIXO ou CMAKE_PREFIX_PATH}}"
ZIP_CACHE="${NL_XSD_ZIP_CACHE:?defina NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD)}"
export TMPDIR="${NL_TMP_BASE:-/var/tmp}"

ARQ_REL="src/mid/sefin/cliente/registro_resposta_sem_codigo.cpp"

# Vetores paralelos: o índice i descreve o par i.
MUTANTES=(MIDIA_CRUA CABECALHO_NO_CAMPO BYTES_NEGATIVOS)
# Trecho antigo e trecho do mutante, literais (troca por Python com contagem exata, sem escape de sed).
ANTIGOS=(
    'return "outro";'
    'fl::Campo{"tipoMidia", tipoMidiaParaLog(tipoConteudo)},'
    'std::to_string(bytesResposta < 0 ? 0 : bytesResposta)'
)
NOVOS=(
    'return midia.toStdString();'
    'fl::Campo{"tipoMidia", tipoConteudo.toByteArray().toStdString()},'
    'std::to_string(bytesResposta)'
)
ALVOS=(tst_registro_resposta_sem_codigo tst_cliente_sefin_falhas_envio tst_registro_resposta_sem_codigo)
# Nome do teste no CTest: mid.sefin.respostaSemCodigo.<slot> (unitário) ou mid.sefin.cliente.<slot> (cliente).
CTEST_PREFIXOS=(mid.sefin.respostaSemCodigo mid.sefin.cliente mid.sefin.respostaSemCodigo)
SLOTS=(tipoConteudoForaDoConjuntoViraOutro status403HtmlSemCodigoDefinitivoSemNovaTentativa semTipoConteudoViraAusente)
# Classe do QTest (sai no "FAIL!  : Classe::slot()") e mensagem da QVERIFY2 do slot (sai entre parênteses no fim da linha).
CLASSES=(TstRegistroRespostaSemCodigo TesteClienteSefinFalhasEnvio TstRegistroRespostaSemCodigo)
MENSAGENS=(
    "tipo fora do conjunto tem de virar outro"
    "o log tem de dizer que o corpo era HTML"
    "tamanho negativo tem de virar 0"
)

encontradas=${#MUTANTES[@]}
aplicadas=0
mortas=0
sobreviventes=0
contagem=""

imprime_contagem() {
    sobreviventes=$((aplicadas - mortas))
    contagem="encontradas=$encontradas aplicadas=$aplicadas mortas=$mortas sobreviventes=$sobreviventes"
    echo "mutantes log 4xx: $contagem"
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        printf '%s\n' "- Mutantes do log do 4xx sem código (NL-3.L, passo 15): $contagem" >> "$GITHUB_STEP_SUMMARY"
    fi
}
trap imprime_contagem EXIT

# Troca o trecho ANTIGO pelo NOVO no arquivo, só se ele aparecer exatamente uma vez. Rc 1 se a contagem for outra.
trocar_trecho() {
    ANTIGO="$2" NOVO="$3" python3 -I -c '
import os, sys
caminho = sys.argv[1]
texto = open(caminho, encoding="utf-8").read()
antigo = os.environ["ANTIGO"]
novo = os.environ["NOVO"]
contagem = texto.count(antigo)
if contagem != 1:
    print("ocorrencias=%d" % contagem)
    sys.exit(1)
open(caminho, "w", encoding="utf-8").write(texto.replace(antigo, novo, 1))
' "$1"
}

if [ -e "$TRAB" ]; then
    echo "PARADA: o trabalho já existe ($TRAB); o mutante precisa de pasta nova"
    exit 2
fi
mkdir -p "$TRAB/src" || exit 2
git -C "$REPO" archive "$SHA" | tar -x -C "$TRAB/src" || { echo "PARADA: git archive de $SHA falhou"; exit 2; }

# Configura a cópia com os mesmos parâmetros do preset debug (Ninja, Debug, banco de comandos exportado, TMPDIR do preset).
echo "cmake: $(cmake --version | head -n 1)"
cmake -S "$TRAB/src" -B "$TRAB/build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_PREFIX_PATH="$QT" -DNL_XSD_ZIP_CACHE="$ZIP_CACHE" > "$TRAB/configure.log" 2>&1
rc_cfg=$?
if [ "$rc_cfg" -ne 0 ]; then
    echo "PARADA: configure da cópia falhou (rc=$rc_cfg); ver $TRAB/configure.log"
    exit 2
fi

ARQ="$TRAB/src/$ARQ_REL"
PRISTINO="$TRAB/pristino.cpp"
cp -- "$ARQ" "$PRISTINO" || { echo "PARADA: cópia pristina do fonte falhou"; exit 2; }
sha_pristino=$(sha256sum "$ARQ" | cut -d' ' -f1)
echo "fonte pristino: $ARQ_REL sha256=$sha_pristino"

# Caminho do executável de um alvo dentro da árvore de build da cópia.
executavel_do_alvo() {
    find "$TRAB/build" -type f -name "$1" -perm -100 2>/dev/null | head -n 1
}

for i in "${!MUTANTES[@]}"; do
    mutante="${MUTANTES[$i]}"
    alvo="${ALVOS[$i]}"
    slot="${SLOTS[$i]}"
    classe="${CLASSES[$i]}"
    mensagem="${MENSAGENS[$i]}"
    ctest_nome="${CTEST_PREFIXOS[$i]}.$slot"
    regex_slot="^${CTEST_PREFIXOS[$i]//./[.]}[.]${slot}\$"
    # Linha FAIL exata do QTest: "FAIL!  : Classe::slot() 'expr' returned FALSE. (mensagem)". Sem a expressão no padrão.
    padrao_fail="FAIL!  : ${classe}::${slot}[(][)] .*[(]${mensagem}[)]"
    echo "--- par $((i + 1))/$encontradas: mutante=$mutante alvo=$alvo slot=$ctest_nome"

    # a. restaura o fonte pristino antes de cada par e confere o sha256.
    cp -- "$PRISTINO" "$ARQ" || { echo "PARADA: restauração do fonte falhou"; exit 2; }
    sha_volta=$(sha256sum "$ARQ" | cut -d' ' -f1)
    if [ "$sha_volta" != "$sha_pristino" ]; then
        echo "PARADA: o fonte da cópia não voltou ao pristino (sha256 $sha_volta)"
        exit 2
    fi

    # b. recompila o alvo sem sabotagem e grava o sha256 do executável de base.
    cmake --build "$TRAB/build" --target "$alvo" -j 4 > "$TRAB/build-base-$i.log" 2>&1
    rc_bld=$?
    if [ "$rc_bld" -ne 0 ]; then
        echo "PARADA: build de base do alvo $alvo na cópia falhou (rc=$rc_bld); ver $TRAB/build-base-$i.log"
        exit 2
    fi
    exe=$(executavel_do_alvo "$alvo")
    if [ -z "$exe" ]; then
        echo "PARADA: executável $alvo não achado na árvore de build da cópia"
        exit 2
    fi
    sha_exe_base=$(sha256sum "$exe" | cut -d' ' -f1)

    # c. linha de base: o slot passa na cópia sem sabotagem.
    ctest --test-dir "$TRAB/build" --no-tests=error -R "$regex_slot" --output-on-failure > "$TRAB/base-$i.log" 2>&1
    rc_base=$?
    if [ "$rc_base" -ne 0 ]; then
        echo "PARADA: a linha de base de $ctest_nome não passa na cópia (rc=$rc_base); sem base não há mutante a matar ($TRAB/base-$i.log)"
        exit 2
    fi
    echo "linha de base: $ctest_nome passa na cópia (rc 0)"

    # d. sabotagem: só o trecho escolhido, no fonte da cópia (nunca o arquivo rastreado do workspace).
    antigo="${ANTIGOS[$i]}"
    novo="${NOVOS[$i]}"
    antes=$(grep -c -F -- "$antigo" "$ARQ" || true)
    novo_antes=$(grep -c -F -- "$novo" "$ARQ" || true)
    if [ "$antes" != 1 ] || [ "$novo_antes" != 0 ]; then
        echo "PARADA: o trecho antigo não existia 1 vez antes da sabotagem (antigo=$antes novo=$novo_antes)"
        exit 1
    fi
    sha_antes=$(sha256sum "$ARQ" | cut -d' ' -f1)
    if ! trocar_trecho "$ARQ" "$antigo" "$novo"; then
        echo "PARADA: a troca do mutante $mutante não aplicou exatamente uma vez"
        exit 1
    fi
    antigo_depois=$(grep -c -F -- "$antigo" "$ARQ" || true)
    novo_depois=$(grep -c -F -- "$novo" "$ARQ" || true)
    sha_depois=$(sha256sum "$ARQ" | cut -d' ' -f1)
    echo "fonte: trecho antigo antes=$antes depois=$antigo_depois; trecho do $mutante depois=$novo_depois; sha256 antes=$sha_antes depois=$sha_depois"
    if [ "$antigo_depois" != 0 ] || [ "$novo_depois" != 1 ] || [ "$sha_antes" = "$sha_depois" ]; then
        echo "PARADA: a sabotagem não chegou ao fonte da cópia (antigo depois=$antigo_depois, novo depois=$novo_depois)"
        exit 1
    fi
    aplicadas=$((aplicadas + 1))

    # e. recompila o alvo mutado: erro de compilação é PARADA, e o executável tem de mudar.
    cmake --build "$TRAB/build" --target "$alvo" -j 4 > "$TRAB/build-mutante-$i.log" 2>&1
    rc_bld=$?
    if [ "$rc_bld" -ne 0 ]; then
        echo "PARADA: o mutante $mutante não compila o alvo $alvo (rc=$rc_bld); não é morte ($TRAB/build-mutante-$i.log)"
        exit 1
    fi
    sha_exe_mutante=$(sha256sum "$exe" | cut -d' ' -f1)
    echo "executável $alvo: sha256 base=$sha_exe_base mutante=$sha_exe_mutante"
    if [ "$sha_exe_base" = "$sha_exe_mutante" ]; then
        echo "PARADA: o executável $alvo não mudou com o mutante $mutante (o mutante não entrou no binário)"
        exit 1
    fi

    # f. morte: o slot falha E a saída tem a asserção exata do slot.
    ctest --test-dir "$TRAB/build" --no-tests=error -R "$regex_slot" --output-on-failure > "$TRAB/ctest-$i.log" 2>&1
    rc=$?
    casos=$(grep -c -E -- "$padrao_fail" "$TRAB/ctest-$i.log" || true)
    # g. a linha FAIL vai para a saída padrão deste script (o log do contêiner a leva).
    grep -E -- "$padrao_fail" "$TRAB/ctest-$i.log" | sed 's/^/linha FAIL: /' || true
    if [ "$rc" -eq 0 ]; then
        echo "mutante=$mutante slot=$ctest_nome rc=0 veredito=VIVO (o slot passou com o mutante: a sabotagem não o pegou) sha=$SHA"
    elif [ "$casos" -lt 1 ]; then
        echo "mutante=$mutante slot=$ctest_nome rc=$rc veredito=NAO-MORTO (falha sem a asserção do slot: ambiente, timeout, crash ou outra asserção) sha=$SHA"
        echo "PARADA: falha que não é a morte esperada ($TRAB/ctest-$i.log)"
        exit 1
    else
        mortas=$((mortas + 1))
        echo "mutante=$mutante slot=$ctest_nome rc=$rc veredito=MORTO (asserção do slot) sha=$SHA"
    fi

    # h. restaura o fonte pristino e confere o sha256 de volta.
    cp -- "$PRISTINO" "$ARQ" || { echo "PARADA: restauração do fonte falhou"; exit 2; }
    sha_volta=$(sha256sum "$ARQ" | cut -d' ' -f1)
    if [ "$sha_volta" != "$sha_pristino" ]; then
        echo "PARADA: o fonte da cópia não voltou ao pristino (sha256 $sha_volta)"
        exit 2
    fi
done

if [ "$mortas" -ne "$encontradas" ]; then
    exit 1
fi
exit 0
