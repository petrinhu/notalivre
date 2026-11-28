#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mutantes do catálogo de localidades (NL-3.M, DA-3M.4; NF-12, NF-33, L-27, L-35, L-36). Pelo mesmo padrão do mutante do log
# do 4xx (mutante_log_4xx.sh): roda no job nativo ubuntu26.04, a cada PR ou push, e só quando o diff toca o catálogo (passo
# `caminhos_catalogo` do ci.yml; fail-secure: sem base confiável, roda).
#
# Sabota só uma CÓPIA extraída do SHA (git archive), fora do workspace. Cada par tem o seu arquivo de fonte (ARQS_REL): a
# sabotagem é uma troca de UMA linha por trecho, com contagem exata 1 antes e depois, e o executável alvo tem de mudar de
# sha256 antes de contar a morte. Erro de compilação do mutante é PARADA, nunca morte (o nl_warnings põe -Werror).
#
# Escolha dos mutantes (um por par, cada um com o slot que o mata pela asserção do próprio teste):
#   1. PINO_SEM_CONFERENCIA (src/mid/tabelas/carregar_catalogo.cpp, conferirPino): a comparação com o pino ganha uma
#      alternativa sempre verdadeira (o SHA-256 tem 64 caracteres). Um SHA256SUMS fora do pino passa. Morre em
#      tst_catalogo_localidades manifestoForaDoPinRecusado, pela QVERIFY2 de que a carga tem de recusar.
#   2. HASH_ARQUIVO_SEM_CONFERENCIA (src/mid/tabelas/carregar_catalogo.cpp, conferirArquivo): a comparação do SHA-256 de cada
#      JSON com o manifesto ganha uma alternativa sempre verdadeira (o rótulo nunca é vazio). Um JSON divergente passa. Morre
#      em tst_catalogo_localidades tabelaDivergenteDoManifestoRecusada, pela QVERIFY2 de que a carga tem de recusar.
#   3. GUARDA_TABELAS_FORA_HOMOLOGACAO (apps/cli/configuracao_cli.cpp, conferirTabelasDeAmbiente): a guarda ganha uma condição
#      extra, o diretório vazio. Como lerConfiguracao recusa chave com valor vazio, [tabelas] definido sempre tem diretório não
#      vazio, então a guarda nunca dispara: é a remoção da guarda. Morre em tst_configuracao_cli (ctest cli.configuracao) no
#      slot tabelasPorConfiguracaoRecusadoForaDeHomologacao, pela QVERIFY2 de que [tabelas] fora de Homologação é recusado.
#   4. ORDEM_PARSE_ANTES_DO_HASH (src/mid/tabelas/carregar_catalogo.cpp, carregar): antes da conferência do hash de cada JSON,
#      o parse entra na frente (par de INSERÇÃO: o trecho antigo continua 1 vez dentro do novo). Com uma tabela adulterada e
#      malformada, sai tabelas.json_invalido no lugar de tabelas.arquivo_divergente. Morre em tst_catalogo_localidades
#      tabelaAdulteradaEMalformadaRecusadaPeloHash, pela QVERIFY2 do código de erro.
#
# Ordem por par (cada passo exige o anterior):
#   a. restaura todos os fontes da cópia ao pristino e confere o sha256 de cada um;
#   b. recompila o alvo do par (sem sabotagem) e grava o sha256 do executável de base;
#   c. LINHA DE BASE: o teste passa na cópia (ctest rc 0). Sem isso, PARADA: não há o que matar;
#   d. confere a troca antes (1 ocorrência do trecho antigo, 0 do novo) e troca pelo trecho do mutante; confere depois
#      (o antigo fica 0 vezes, ou 1 vez no par de inserção; o novo fica 1 vez) e imprime o sha256 do fonte antes e depois;
#   e. recompila o alvo mutado e confere que o executável mudou (sha256 diferente da base). Igual é PARADA;
#   f. MORTE: o teste falha (rc diferente de 0) E a saída tem a asserção exata do slot, na forma do QTest
#      "FAIL!  : Classe::slot() 'expr' returned FALSE. (mensagem)", com o nome e a mensagem do par. Falha sem essa linha
#      (ambiente, timeout, crash, outra asserção) NÃO é morte: é PARADA. Com rc 0, é VIVO: o mutante sobreviveu;
#   g. a linha FAIL da asserção sai na saída padrão deste script (o log do contêiner a leva);
#   h. restaura todos os fontes ao pristino e confere o sha256 de cada um.
#
# Contagem: encontradas (tamanho da lista de pares), aplicadas, mortas, sobreviventes. Sai por trap EXIT em todo caminho,
# inclusive zero. Sobrevivente ou par não aplicado sai com 1; PARADA sai com 2.
#
# Uso: mutante_catalogo.sh REPO SHA TRABALHO
#   REPO       repositório git de onde sai o SHA
#   SHA        commit (40 hex)
#   TRABALHO   pasta NOVA fora do workspace (ex.: $RUNNER_TEMP/mutante-catalogo)
# Variáveis (do job): QT_PREFIXO ou CMAKE_PREFIX_PATH (Qt 6.12), NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD),
# NL_TMP_BASE (TMPDIR da cópia, como o preset). Exige CMake com ctest --no-tests=error (3.22 ou mais novo).
set -u

REPO="${1:?uso: mutante_catalogo.sh REPO SHA TRABALHO}"
SHA="${2:?uso: mutante_catalogo.sh REPO SHA TRABALHO}"
TRAB="${3:?uso: mutante_catalogo.sh REPO SHA TRABALHO}"
QT="${QT_PREFIXO:-${CMAKE_PREFIX_PATH:?defina QT_PREFIXO ou CMAKE_PREFIX_PATH}}"
ZIP_CACHE="${NL_XSD_ZIP_CACHE:?defina NL_XSD_ZIP_CACHE (diretório com o ZIP do XSD)}"
export TMPDIR="${NL_TMP_BASE:-/var/tmp}"

# Vetores paralelos: o índice i descreve o par i (mutante, arquivo de fonte, alvo, nome CTest, slot, classe e mensagem).
MUTANTES=(PINO_SEM_CONFERENCIA HASH_ARQUIVO_SEM_CONFERENCIA GUARDA_TABELAS_FORA_HOMOLOGACAO ORDEM_PARSE_ANTES_DO_HASH)
ARQS_REL=(
    src/mid/tabelas/carregar_catalogo.cpp
    src/mid/tabelas/carregar_catalogo.cpp
    apps/cli/configuracao_cli.cpp
    src/mid/tabelas/carregar_catalogo.cpp
)
# Trecho antigo e trecho do mutante, literais de UMA linha cada (grep -c conta linhas; troca por Python com contagem exata).
ANTIGOS=(
    'if (std::string_view(obtido) == pino) {'
    'if (hashSha256(bytes) == esperado) {'
    'if (!homologacao && cfg.tabelasDefinido) {'
    'if (const auto conferido = conferirArquivo(*bytes, (*hashes)[i], kTabelas[i].rotulo); !conferido) {'
)
NOVOS=(
    'if (std::string_view(obtido) == pino || obtido.size() == kTamanhoSha256Hex) {'
    'if (hashSha256(bytes) == esperado || !rotulo.empty()) {'
    'if (!homologacao && cfg.tabelasDefinido && cfg.tabelasDiretorio.isEmpty()) {'
    'if (const auto lido = registrosDe(*bytes, kTabelas[i].rotulo); !lido) { return std::unexpected(lido.error()); } if (const auto conferido = conferirArquivo(*bytes, (*hashes)[i], kTabelas[i].rotulo); !conferido) {'
)
ALVOS=(tst_catalogo_localidades tst_catalogo_localidades tst_configuracao_cli tst_catalogo_localidades)
# Nome completo do teste no CTest: mid.tabelas.<slot> (catálogo) ou cli.configuracao (um teste que roda todos os slots da CLI).
CTEST_NOMES=(
    mid.tabelas.manifestoForaDoPinRecusado
    mid.tabelas.tabelaDivergenteDoManifestoRecusada
    cli.configuracao
    mid.tabelas.tabelaAdulteradaEMalformadaRecusadaPeloHash
)
SLOTS=(
    manifestoForaDoPinRecusado
    tabelaDivergenteDoManifestoRecusada
    tabelasPorConfiguracaoRecusadoForaDeHomologacao
    tabelaAdulteradaEMalformadaRecusadaPeloHash
)
# Classe do QTest (sai no "FAIL!  : Classe::slot()") e mensagem da QVERIFY2 do slot (sai entre parênteses no fim da linha).
CLASSES=(TstCatalogoLocalidades TstCatalogoLocalidades TstConfiguracaoCli TstCatalogoLocalidades)
MENSAGENS=(
    "SHA256SUMS fora do pino tem de ser recusado"
    "JSON divergente do manifesto tem de ser recusado"
    "[tabelas] definido fora de Homologação tem de ser recusado"
    "a conferência do hash vem antes do parse"
)

encontradas=${#MUTANTES[@]}
aplicadas=0
mortas=0
sobreviventes=0
contagem=""

imprime_contagem() {
    sobreviventes=$((aplicadas - mortas))
    contagem="encontradas=$encontradas aplicadas=$aplicadas mortas=$mortas sobreviventes=$sobreviventes"
    echo "mutantes catalogo: $contagem"
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        printf '%s\n' "- Mutantes do catalogo de localidades (NL-3.M, DA-3M.4): $contagem" >> "$GITHUB_STEP_SUMMARY"
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

# Pristino e sha256 de cada fonte que algum par sabota. Tirados antes de qualquer sabotagem.
PRISTINOS=()
SHAS_PRISTINOS=()
for i in "${!MUTANTES[@]}"; do
    arq="$TRAB/src/${ARQS_REL[$i]}"
    pristino="$TRAB/pristino-$i.cpp"
    cp -- "$arq" "$pristino" || { echo "PARADA: cópia pristina de ${ARQS_REL[$i]} falhou"; exit 2; }
    PRISTINOS+=("$pristino")
    sha_p=$(sha256sum "$arq" | cut -d' ' -f1)
    SHAS_PRISTINOS+=("$sha_p")
    echo "fonte pristino: ${ARQS_REL[$i]} sha256=$sha_p (par $((i + 1)))"
done

# Caminho do executável de um alvo dentro da árvore de build da cópia.
executavel_do_alvo() {
    find "$TRAB/build" -type f -name "$1" -perm -100 2>/dev/null | head -n 1
}

# Restaura TODOS os fontes da cópia ao pristino e confere o sha256 de cada um. Qualquer diferença é PARADA (exit 2).
# Variáveis locais: o laço de restauração não pode sobrescrever as variáveis do par (ver o mutante do log, mesma regra).
restaurar_todos() {
    local j arq_j sha_volta_j
    for j in "${!MUTANTES[@]}"; do
        arq_j="$TRAB/src/${ARQS_REL[$j]}"
        cp -- "${PRISTINOS[$j]}" "$arq_j" || { echo "PARADA: restauração de ${ARQS_REL[$j]} falhou"; exit 2; }
        sha_volta_j=$(sha256sum "$arq_j" | cut -d' ' -f1)
        if [ "$sha_volta_j" != "${SHAS_PRISTINOS[$j]}" ]; then
            echo "PARADA: ${ARQS_REL[$j]} não voltou ao pristino (sha256 $sha_volta_j)"
            exit 2
        fi
    done
}

for i in "${!MUTANTES[@]}"; do
    mutante="${MUTANTES[$i]}"
    arq_rel="${ARQS_REL[$i]}"
    alvo="${ALVOS[$i]}"
    slot="${SLOTS[$i]}"
    classe="${CLASSES[$i]}"
    mensagem="${MENSAGENS[$i]}"
    ctest_nome="${CTEST_NOMES[$i]}"
    regex_ctest="^$(printf '%s' "$ctest_nome" | sed 's/\./[.]/g')\$"
    # A mensagem vai para a regex com os metacaracteres escapados ([ ] ( ) . * ^ $ { } | + ?): "[tabelas]" é literal.
    mensagem_re=$(printf '%s' "$mensagem" | sed 's/[][\.*^$(){}|+?]/\\&/g')
    # Linha FAIL exata do QTest: "FAIL!  : Classe::slot() 'expr' returned FALSE. (mensagem)". Sem a expressão no padrão.
    padrao_fail="FAIL!  : ${classe}::${slot}[(][)] .*[(]${mensagem_re}[)]"
    echo "--- par $((i + 1))/$encontradas: mutante=$mutante arquivo=$arq_rel alvo=$alvo teste=$ctest_nome"

    # a. restaura todos os fontes ao pristino antes de cada par e confere o sha256 de cada um.
    restaurar_todos
    # O arquivo deste par, por índice explícito, e a conferência de que ele é o pristino DAQUELE arquivo.
    arq="$TRAB/src/${ARQS_REL[$i]}"
    sha_do_par=$(sha256sum "$arq" | cut -d' ' -f1)
    echo "par $((i + 1)): arquivo=${ARQS_REL[$i]} sha256 pristino=${SHAS_PRISTINOS[$i]} sha256 atual=$sha_do_par"
    if [ "$sha_do_par" != "${SHAS_PRISTINOS[$i]}" ]; then
        echo "PARADA: o arquivo do par $((i + 1)) (${ARQS_REL[$i]}) não é o pristino dele antes da sabotagem"
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

    # c. linha de base: o teste passa na cópia sem sabotagem.
    ctest --test-dir "$TRAB/build" --no-tests=error -R "$regex_ctest" --output-on-failure > "$TRAB/base-$i.log" 2>&1
    rc_base=$?
    if [ "$rc_base" -ne 0 ]; then
        echo "PARADA: a linha de base de $ctest_nome não passa na cópia (rc=$rc_base); sem base não há mutante a matar ($TRAB/base-$i.log)"
        exit 2
    fi
    echo "linha de base: $ctest_nome passa na cópia (rc 0)"

    # d. sabotagem: só o trecho escolhido, no fonte do par, na cópia (nunca o arquivo rastreado do workspace).
    antigo="${ANTIGOS[$i]}"
    novo="${NOVOS[$i]}"
    # Num par de inserção, o trecho novo contém o antigo: o antigo tem de continuar 1 vez depois da troca.
    if [[ "$novo" == *"$antigo"* ]]; then
        antigo_esperado_depois=1
    else
        antigo_esperado_depois=0
    fi
    antes=$(grep -c -F -- "$antigo" "$arq" || true)
    novo_antes=$(grep -c -F -- "$novo" "$arq" || true)
    if [ "$antes" != 1 ] || [ "$novo_antes" != 0 ]; then
        echo "PARADA: o trecho antigo não existia 1 vez antes da sabotagem (antigo=$antes novo=$novo_antes)"
        exit 1
    fi
    sha_antes=$(sha256sum "$arq" | cut -d' ' -f1)
    if ! trocar_trecho "$arq" "$antigo" "$novo"; then
        echo "PARADA: a troca do mutante $mutante não aplicou exatamente uma vez"
        exit 1
    fi
    antigo_depois=$(grep -c -F -- "$antigo" "$arq" || true)
    novo_depois=$(grep -c -F -- "$novo" "$arq" || true)
    sha_depois=$(sha256sum "$arq" | cut -d' ' -f1)
    echo "fonte: trecho antigo antes=$antes depois=$antigo_depois; trecho do $mutante depois=$novo_depois; sha256 antes=$sha_antes depois=$sha_depois"
    if [ "$antigo_depois" != "$antigo_esperado_depois" ] || [ "$novo_depois" != 1 ] || [ "$sha_antes" = "$sha_depois" ]; then
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

    # f. morte: o teste falha E a saída tem a asserção exata do slot.
    ctest --test-dir "$TRAB/build" --no-tests=error -R "$regex_ctest" --output-on-failure > "$TRAB/ctest-$i.log" 2>&1
    rc=$?
    casos=$(grep -c -E -- "$padrao_fail" "$TRAB/ctest-$i.log" || true)
    # g. a linha FAIL vai para a saída padrão deste script (o log do contêiner a leva).
    grep -E -- "$padrao_fail" "$TRAB/ctest-$i.log" | sed 's/^/linha FAIL: /' || true
    if [ "$rc" -eq 0 ]; then
        echo "mutante=$mutante teste=$ctest_nome rc=0 veredito=VIVO (o teste passou com o mutante: a sabotagem não o pegou) sha=$SHA"
    elif [ "$casos" -lt 1 ]; then
        echo "mutante=$mutante teste=$ctest_nome rc=$rc veredito=NAO-MORTO (falha sem a asserção do slot: ambiente, timeout, crash ou outra asserção) sha=$SHA"
        echo "PARADA: falha que não é a morte esperada ($TRAB/ctest-$i.log)"
        exit 1
    else
        mortas=$((mortas + 1))
        echo "mutante=$mutante teste=$ctest_nome rc=$rc veredito=MORTO (asserção do slot) sha=$SHA"
    fi

    # h. restaura todos os fontes ao pristino e confere o sha256 de cada um.
    restaurar_todos
done

if [ "$mortas" -ne "$encontradas" ]; then
    exit 1
fi
exit 0
