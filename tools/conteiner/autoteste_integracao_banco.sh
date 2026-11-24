#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do harness de integração com banco (NL-3.1h, V-a; L-35, L-36). Cada caso exige o trecho específico da PARADA
# (ou da linha de resultado) que o harness precisa dar, e o rc. Um caso só passa com a saída e o rc que o harness deve dar.
# Não sobe contêiner: o docker é um de mentira no PATH. O de mentira lê a entrada do exec -i (como o docker real) e
# responde ao que o harness pede: portão de funções, servidor, prontidão, conta, plugin, versão, sonda e executor.
# Casos (28; os 26, 27 e 28 são do H-R do NL-3.1a, vermelhos até o H-G): servidor ausente; zero testes banco; build sem NL_TESTES_BANCO; junitxml do par anterior; junitxml com
# skipped; limpeza em falha; docker run sem o vetor da cota; par fora da matriz; arquivo de banco/ executado direto;
# portão de funções sem linha; portão de funções com faltam; só três linhas NL-TLS; volume do Qt ausente; classe disco
# sem PEM; controle verde; todos com dois pares; GRANT da conta de teste; só três linhas NL-CONECTOR; ruído e reimpressão
# sem número não contam; sem NL-PIDS; NL-PIDS sem número; NL-PIDS com eventos_max; corpo do executor; teto 256 do executor;
# resumo curto do ctest (manjaro: "100% tests passed out of 4", sem ", 0 tests failed"; NL-3.1h, v3.21).
# As linhas do executor no stub são as do log real do par (fedora45/mysql97 em 7f0986d, 19/11/2026), com o prefixo do
# ctest --verbose e do QTest (NL-3.1h, v3.19).
# Uso: autoteste_integracao_banco.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONTEINER="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-banco.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0
TOTAL=0
SHA_FIXO=0000000000000000000000000000000000000000
GRANT_ESPERADO="GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, CREATE TEMPORARY TABLES, DROP, ALTER, INDEX ON \`notalivre\\_%\`.* TO 'notalivre_teste'@'%';"

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

mkdir -p "$BASE/bin"
# Sem trava aqui: nenhum caso mede a fila, e o ~/.claude/bin/pesado não existe no CI (L-11).
export NL_TRAVA_DETIDA=1
cat > "$BASE/bin/docker" <<'STUB'
#!/bin/sh
# Docker de mentira (V-a, NL-3.1h): responde ao que o harness pede e grava a linha de comando no log.
echo "docker $*" >> "${STUB_LOG:-/dev/null}"
stub_plugin_padrao() {
    case "$1" in
        *mariadb*) echo mysql_native_password ;;
        *) echo caching_sha2_password ;;
    esac
}
case "$1" in
    image)
        [ -n "${STUB_SERVIDOR_AUSENTE:-}" ] && exit 1
        exit 0 ;;
    volume)
        [ -n "${STUB_VOLUME_AUSENTE:-}" ] && exit 1
        exit 0 ;;
    inspect) echo "none {}"; exit 0 ;;
    logs|stop|rm) exit 0 ;;
    exec)
        entrada=""
        case " $* " in *" -i "*) entrada="$(cat)" ;; esac
        case "$entrada" in
            *"SELECT plugin"*) echo "${STUB_PLUGIN:-$(stub_plugin_padrao "$*")}"; exit 0 ;;
        esac
        case "$*" in
            *"SELECT VERSION"*) echo 9.7.4; exit 0 ;;
            *ssl_cert*) printf 'ssl_cert\t\n'; exit 0 ;;
            *server-cert.pem*)
                if [ -z "${STUB_PEM_VAZIO:-}" ]; then
                    if [ -n "${STUB_PEM:-}" ]; then echo "$STUB_PEM"; else
                        printf '%s\n' '-----BEGIN CERTIFICATE-----' 'MIIB' '-----END CERTIFICATE-----'
                    fi
                fi
                exit 0 ;;
        esac
        # Entrada de SQL com GRANT: só as linhas GRANT vão a grant.log (a senha da conta nunca entra no arquivo).
        if [ -n "$entrada" ]; then
            printf '%s\n' "$entrada" | grep '^GRANT' >> "$(dirname "${STUB_LOG:-/dev/null}")/grant.log" 2>/dev/null
        fi
        exit 0 ;;
    run)
        case "$*" in
            *funcoes_no_ctest*)
                [ -n "${STUB_FUNCOES_AUSENTE:-}" ] && exit 0
                faltam=0
                [ -n "${STUB_FUNCOES_FALTA:-}" ] && faltam=1
                echo "funcoes_no_ctest: ctest=520 por_funcao=445 declaradas=454 registradas=445 faltam=$faltam sobram=0 encontrados=34 analisados=34 falharam=0"
                exit 0 ;;
            *impressao_servidor_banco*)
                classe=""
                while [ $# -gt 0 ]; do
                    case "$1" in *impressao_servidor_banco.sh) classe="$2"; break ;; esac
                    shift
                done
                case "$classe" in
                    disco) echo "NL-IMPRESSAO certificado=disco disco=AB sonda=AB usada=AB" ;;
                    memoria) echo "NL-IMPRESSAO certificado=memoria disco=nd sonda=AB usada=AB" ;;
                    nenhum) echo "NL-IMPRESSAO certificado=nenhum disco=nd sonda=sem_tls usada=nenhuma" ;;
                esac
                exit 0 ;;
            *--network*container:*)
                # O executor real grava o junitxml na pasta de build que o -v do executor monta (a da distro do par).
                jd="$STUB_JUNIT_DIR"
                for a in "$@"; do
                    case "$a" in *:/var/tmp/notalivre-build:z) jd="${a%%:*}/${NL_PRESET_BANCO:-banco}/junit" ;; esac
                done
                if [ -z "${STUB_SEM_JUNIT:-}" ]; then
                    for slot in sessaoPadrao modoAberto modoFixadoImpressaoCerta modoFixadoImpressaoTrocada; do
                        x="$jd/integracao.banco.$slot.xml"
                        if [ "$slot" = sessaoPadrao ] && [ -n "${STUB_JUNIT_XML:-}" ]; then
                            printf '%s\n' "$STUB_JUNIT_XML" > "$x"
                        else
                            printf '%s\n' "<testsuite tests=\"1\"><testcase classname=\"integracao.banco\" name=\"$slot\"/></testsuite>" > "$x"
                        fi
                    done
                fi
                # Linhas reais do log do par (fedora45/mysql97, 7f0986d): o ctest --verbose prefixa "NNN: ", e o QTest
                # prefixa "QINFO  : Classe::slot() " (dois espaços antes do dois-pontos).
                i=0
                while [ "$i" -lt 4 ]; do
                    case "$i" in
                        0) n=517; slot=sessaoPadrao; tls='resultado=tls cliente=TLS_AES_128_GCM_SHA256 servidor=TLS_AES_128_GCM_SHA256 versao=TLSv1.3 status=1 msg=nenhuma erro=0' ;;
                        1) n=518; slot=modoAberto; tls='resultado=sem_tls cliente=nulo servidor=vazio versao=vazio status=0 msg=nenhuma erro=0' ;;
                        2) n=519; slot=modoFixadoImpressaoCerta; tls='resultado=tls cliente=TLS_AES_128_GCM_SHA256 servidor=TLS_AES_128_GCM_SHA256 versao=TLSv1.3 status=1 msg=nenhuma erro=0' ;;
                        *) n=520; slot=modoFixadoImpressaoTrocada; tls='resultado=recusa cliente=nulo servidor=vazio versao=vazio status=5 msg=impressao erro=2026' ;;
                    esac
                    echo "$n: Test command: /var/tmp/notalivre-build/banco/tests/integracao/banco/tst_conexao_banco \"-o\" \"-,txt\" \"-o\" \"/var/tmp/notalivre-build/banco/junit/integracao.banco.$slot.xml,junitxml\" \"$slot\""
                    [ "$i" -ge "${STUB_LINHAS_CONECTOR:-4}" ] || echo "$n: QINFO  : TesteConexaoBanco::initTestCase() NL-CONECTOR cabecalho=3.4.11 execucao=3.4.11 biblioteca=/usr/local/lib/mariadb/libmariadb.so.3 prefixo=/usr/local"
                    [ "$i" -ne 3 ] || echo "$n: QINFO  : TesteConexaoBanco::$slot() NL-TLS-MSG conexao=$slot mensagem=TLS/SSL error: Fingerprint validation of peer certificate failed"
                    [ "$i" -ge "${STUB_LINHAS_TLS:-4}" ] || echo "$n: QINFO  : TesteConexaoBanco::$slot() NL-TLS conexao=$slot $tls"
                    echo "$n: PASS   : TesteConexaoBanco::$slot()"
                    i=$((i + 1))
                done
                if [ -n "${STUB_RUIDO:-}" ]; then
                    # Reimpressão sem número (a do --output-on-failure), linhas sem prefixo e ecos do harness: nada disso conta.
                    echo "QINFO  : TesteConexaoBanco::initTestCase() NL-CONECTOR cabecalho=3.4.11 execucao=3.4.11 biblioteca=/usr/local/lib/mariadb/libmariadb.so.3 prefixo=/usr/local"
                    echo "QINFO  : TesteConexaoBanco::sessaoPadrao() NL-TLS conexao=sessaoPadrao resultado=tls cliente=X servidor=X versao=TLSv1.3 status=1 msg=nenhuma erro=0"
                    echo "NL-TLS conexao=sessaoPadrao resultado=tls cliente=X servidor=X versao=TLSv1.3 status=1 msg=nenhuma erro=0"
                    echo "NL-CONECTOR cabecalho=3.4.11 execucao=3.4.11 biblioteca=/usr/local/lib/mariadb/libmariadb.so.3 prefixo=/usr/local"
                    echo "PARADA: linhas NL-TLS (par fedora45/mysql97: 0, esperadas 4)"
                    echo "NL-BANCO-TLS par=fedora45/mysql97 NL-TLS conexao=sessaoPadrao resultado=tls"
                fi
                # O ctest do manjaro omite ", 0 tests failed" (log real do par manjaro/mysql84 em 8d7dc5c, linha 92).
                if [ -z "${STUB_SEM_RESUMO:-}" ] && [ -n "${STUB_RESUMO_CURTO:-}" ]; then
                    echo "100% tests passed out of 4"
                elif [ -z "${STUB_SEM_RESUMO:-}" ]; then
                    echo "100% tests passed, 0 tests failed out of 4"
                fi
                # A linha NL-PIDS só existe quando o executor roda o corpo ctest_banco_no_conteiner.sh (v3.19).
                case "$*" in
                    *ctest_banco_no_conteiner.sh*)
                        [ -n "${STUB_SEM_PIDS:-}" ] || echo "${STUB_PIDS_LINHA:-NL-PIDS max=256 peak=12 eventos_max=0}" ;;
                esac
                exit "${STUB_EXEC_RC:-0}" ;;
            *--name*) exit "${STUB_SRV_RC:-0}" ;;
        esac
        exit 0 ;;
esac
exit 0
STUB
chmod +x "$BASE/bin/docker"

# Prepara um caso novo: pasta própria, builds das duas distros com a opção ligada e junitxml vazio. Nada é apagado aqui.
# Todo botão STUB_* é exportado (o docker de mentira é processo filho) e zerado antes de cada caso.
novo_caso() {
    CASO="$BASE/casos/$1"
    mkdir -p "$CASO/tmp" || return 1
    NL_TMP_BASE="$CASO/tmp"
    mkdir -p "$NL_TMP_BASE/notalivre-src-$SHA_FIXO-NL-3.1h" || return 1
    local d
    for d in fedora45 ubuntu26.04; do
        mkdir -p "$NL_TMP_BASE/notalivre-build-$d-NL-3.1h/banco/junit" || return 1
        printf 'NL_TESTES_BANCO:BOOL=ON\n' > "$NL_TMP_BASE/notalivre-build-$d-NL-3.1h/banco/CMakeCache.txt"
    done
    BUILD="$NL_TMP_BASE/notalivre-build-fedora45-NL-3.1h/banco"
    STUB_JUNIT_DIR="$BUILD/junit"
    STUB_LOG="$CASO/docker.log"
    export NL_TMP_BASE STUB_LOG STUB_JUNIT_DIR TMPDIR="$CASO/tmp" NL_FATIA=NL-3.1h NL_SHA="$SHA_FIXO"
    unset STUB_SERVIDOR_AUSENTE STUB_VOLUME_AUSENTE STUB_JUNIT_XML STUB_SEM_JUNIT STUB_SEM_RESUMO STUB_LINHAS_TLS STUB_LINHAS_CONECTOR \
        STUB_RUIDO STUB_SEM_PIDS STUB_PIDS_LINHA STUB_RESUMO_CURTO \
        STUB_EXEC_RC STUB_SRV_RC STUB_FUNCOES_AUSENTE STUB_FUNCOES_FALTA STUB_PLUGIN STUB_PEM STUB_PEM_VAZIO
    unset NL_PRESET_BANCO
    case ":$PATH:" in *":$BASE/bin:"*) ;; *) export PATH="$BASE/bin:$PATH" ;; esac
}

# Roda o comando e exige rc diferente de 0 e o trecho na saída (o harness precisa dar PARADA com a mensagem certa).
executar_caso() {
    local nome="$1" trecho="$2" saida rc
    shift 2
    TOTAL=$((TOTAL + 1))
    saida="$("$@" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"$trecho"* ]]; then
        ok "$nome (rc=$rc)"
    else
        falha "$nome: rc=$rc, esperado rc diferente de 0 e '$trecho' na saída"
    fi
}

# Roda o comando e exige rc 0 e os dois trechos na saída (controle verde: par certo).
executar_verde() {
    local nome="$1" trecho_a="$2" trecho_b="$3" saida rc
    shift 3
    TOTAL=$((TOTAL + 1))
    saida="$("$@" 2>&1)"; rc=$?
    if [ "$rc" -eq 0 ] && [[ "$saida" == *"$trecho_a"* ]] && [[ "$saida" == *"$trecho_b"* ]]; then
        ok "$nome (rc=$rc)"
    else
        falha "$nome: rc=$rc, esperado rc 0 e '$trecho_a' e '$trecho_b' na saída"
    fi
}

# Caso 1: servidor ausente reprova (a imagem do servidor não está no host).
novo_caso servidor_ausente || { echo "ABORTA: caso 1"; exit 1; }
export STUB_SERVIDOR_AUSENTE=1
executar_caso "servidor ausente reprova" "PARADA: servidor mysql97 ausente" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 2: zero testes banco reprova (nenhum junitxml de integracao.banco.*, nenhum resumo do executor e nenhuma linha NL-TLS).
novo_caso zero_testes || { echo "ABORTA: caso 2"; exit 1; }
export STUB_SEM_JUNIT=1 STUB_SEM_RESUMO=1 STUB_LINHAS_TLS=0 STUB_LINHAS_CONECTOR=0
executar_caso "label banco com zero testes reprova" "encontrados=0 analisados=0" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 3: build sem NL_TESTES_BANCO:BOOL=ON reprova (mutante 25).
novo_caso build_sem_opcao || { echo "ABORTA: caso 3"; exit 1; }
printf 'NL_TESTES_BANCO:BOOL=OFF\n' > "$BUILD/CMakeCache.txt"
executar_caso "build sem NL_TESTES_BANCO reprova" "PARADA: build banco sem NL_TESTES_BANCO" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 4: junitxml do par anterior não conta como resultado do par atual (mutante 28).
novo_caso junit_do_par_anterior || { echo "ABORTA: caso 4"; exit 1; }
export STUB_SEM_JUNIT=1
printf '<testsuite tests="1"><testcase classname="integracao.banco" name="conexaoBasica"/></testsuite>\n' \
    > "$BUILD/junit/integracao.banco.mysql97.xml"
executar_caso "junitxml do par anterior não conta" "encontrados=4 analisados=0" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 5: junitxml com skipped reprova (veto da v3.1: nenhum teste do banco pulado).
novo_caso junit_com_skipped || { echo "ABORTA: caso 5"; exit 1; }
export STUB_JUNIT_XML='<testsuite tests="1"><testcase classname="integracao.banco" name="conexaoBasica"><skipped/></testcase></testsuite>'
executar_caso "junitxml com skipped reprova" "pulados=1" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 6: a limpeza roda em falha. Com o executor vermelho (rc 1), o harness tem de chamar docker stop ou docker rm.
novo_caso limpeza_em_falha || { echo "ABORTA: caso 6"; exit 1; }
export STUB_EXEC_RC=1
TOTAL=$((TOTAL + 1))
bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97 >/dev/null 2>&1; rc=$?
if [ "$rc" -ne 0 ] && grep -Eq 'docker (stop|rm)' "$CASO/docker.log" 2>/dev/null; then
    ok "limpeza roda em falha (rc=$rc)"
else
    falha "limpeza roda em falha: rc=$rc, sem docker stop ou docker rm no log"
fi

# Caso 7: docker run sem o vetor da cota reprova. Varredura: todo docker run do harness e de banco/ leva o vetor sem
# --pids-limit (NL_COTAS_DOCKER_SEM_PIDS, NL-3.1h v3.11) e o próprio --pids-limit literal, nunca acima de 400 (teto duro; v3.16).
# Piso (L-36): zero docker run encontrado é varredura quebrada, e reprova.
TOTAL=$((TOTAL + 1))
total_run=$(grep -hF 'docker run' "$CONTEINER/integracao_banco.sh" "$CONTEINER"/banco/*.sh 2>/dev/null | wc -l)
sem_vetor=$(grep -hF 'docker run' "$CONTEINER/integracao_banco.sh" "$CONTEINER"/banco/*.sh 2>/dev/null \
    | grep -vF 'NL_COTAS_DOCKER_SEM_PIDS[@]' | wc -l)
acima_400=$(grep -hF 'docker run' "$CONTEINER/integracao_banco.sh" "$CONTEINER"/banco/*.sh 2>/dev/null \
    | grep -oE -- '--pids-limit[ =][0-9]+' | grep -oE '[0-9]+$' | awk '$1 > 400' | wc -l)
echo "docker_run_sem_vetor: docker_run=$total_run sem_vetor=$sem_vetor pids_acima_400=$acima_400"
if [ "$total_run" -gt 0 ] && [ "$sem_vetor" -eq 0 ] && [ "$acima_400" -eq 0 ]; then
    ok "docker run do harness leva o vetor da cota sem --pids-limit, e nenhum --pids-limit acima de 400 ($total_run chamadas)"
else
    falha "docker run do harness sem o vetor, acima de 400, ou varredura vazia (docker_run=$total_run sem_vetor=$sem_vetor pids_acima_400=$acima_400)"
fi

# Caso 8: par fora da matriz reprova (ubuntu26.04 tem só mysql84 e mariadb118 em matriz_banco.txt).
novo_caso par_fora_da_matriz || { echo "ABORTA: caso 8"; exit 1; }
executar_caso "par fora da matriz reprova" "PARADA: par ubuntu26.04/mysql97 fora" bash "$CONTEINER/integracao_banco.sh" ubuntu26.04 mysql97

# Caso 9: arquivo de banco/ executado direto sai com rc diferente de 0 (são carregados por ponto).
novo_caso banco_direto || { echo "ABORTA: caso 9"; exit 1; }
executar_caso "arquivo de banco/ executado direto reprova" "PARADA" bash "$CONTEINER/banco/listas.sh"

# Caso 10a: portão de funções sem linha reprova (o achado A1: a linha ausente não pode passar em silêncio).
novo_caso portao_sem_linha || { echo "ABORTA: caso 10a"; exit 1; }
export STUB_FUNCOES_AUSENTE=1
executar_caso "portão de funções sem linha reprova" "PARADA: portão de funções" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 10b: portão de funções com faltam=1 reprova.
novo_caso portao_com_faltam || { echo "ABORTA: caso 10b"; exit 1; }
export STUB_FUNCOES_FALTA=1
executar_caso "portão de funções com faltam reprova" "PARADA: portão de funções" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 11: só três linhas NL-TLS reprovam (as quatro conexões do C4' são obrigatórias).
novo_caso tres_linhas_tls || { echo "ABORTA: caso 11"; exit 1; }
export STUB_LINHAS_TLS=3
executar_caso "só três linhas NL-TLS reprovam" "PARADA: linhas NL-TLS (par fedora45/mysql97: 3, esperadas 4)" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 18: só três linhas NL-CONECTOR reprovam (a G-R do CISO: uma por processo do teste, NL-3.1h v3.18).
novo_caso tres_linhas_conector || { echo "ABORTA: caso 18"; exit 1; }
export STUB_LINHAS_CONECTOR=3
executar_caso "só três linhas NL-CONECTOR reprovam" "PARADA: linhas NL-CONECTOR (par fedora45/mysql97: 3, esperadas 4)" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 19: ruído e reimpressão sem número não contam (NL-3.1h v3.19): com 6 linhas a mais que citam NL-TLS ou NL-CONECTOR,
# o par segue verde e com exatamente 4 linhas de cada.
novo_caso ruido_nao_conta || { echo "ABORTA: caso 19"; exit 1; }
export STUB_RUIDO=1
executar_verde "ruído e reimpressão sem número não contam duas vezes" \
    "NL-BANCO-RESUMO pares_esperados=1 executados=1 verdes=1 vermelhos=0 n_por_par=4" "NL-BANCO-PIDS par=fedora45/mysql97" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 20: sem a linha NL-PIDS do executor, o par reprova (o pico do uid 1000 é obrigatório; pedido da coordenação, v3.19).
novo_caso sem_pids || { echo "ABORTA: caso 20"; exit 1; }
export STUB_SEM_PIDS=1
executar_caso "sem NL-PIDS reprova" "PARADA: linha NL-PIDS (par fedora45/mysql97: ausente ou repetida)" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 21: NL-PIDS com pico que não é número reprova (cgroup ilegível vira "ausente", nunca zero).
novo_caso pids_sem_numero || { echo "ABORTA: caso 21"; exit 1; }
export STUB_PIDS_LINHA='NL-PIDS max=256 peak=ausente eventos_max=0'
executar_caso "NL-PIDS sem número reprova" "PARADA: linha NL-PIDS (par fedora45/mysql97: peak=ausente, sem número)" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 22: NL-PIDS com eventos_max acima de zero reprova (o executor bateu no teto).
novo_caso pids_eventos || { echo "ABORTA: caso 22"; exit 1; }
export STUB_PIDS_LINHA='NL-PIDS max=256 peak=256 eventos_max=3'
executar_caso "NL-PIDS com eventos_max reprova" "PARADA: linha NL-PIDS (par fedora45/mysql97: eventos_max=3, o executor bateu no teto)" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 23: o corpo do executor imprime a NL-PIDS por último, também com o ctest vermelho, e devolve o rc do ctest.
novo_caso corpo_executor || { echo "ABORTA: caso 23"; exit 1; }
TOTAL=$((TOTAL + 1))
mkdir -p "$CASO/ctest_bin" "$CASO/cg"
printf '#!/bin/sh\necho "517: QINFO  : TesteConexaoBanco::sessaoPadrao() NL-TLS conexao=sessaoPadrao"\nexit 7\n' > "$CASO/ctest_bin/ctest"
chmod +x "$CASO/ctest_bin/ctest"
printf '256\n' > "$CASO/cg/pids.max"
printf '31\n' > "$CASO/cg/pids.peak"
printf 'max 0\n' > "$CASO/cg/pids.events"
saida="$(cd "$RAIZ" && PATH="$CASO/ctest_bin:$PATH" HOME="$CASO/home" NL_ARQ_CGROUP="$CASO/cg" bash "$CONTEINER/ctest_banco_no_conteiner.sh" 2>&1)"; rc=$?
ultima="$(printf '%s\n' "$saida" | tail -n 1)"
if [ "$rc" -eq 7 ] && [ "$ultima" = 'NL-PIDS max=256 peak=31 eventos_max=0' ] && [ -d "$CASO/home" ]; then
    ok "corpo do executor: NL-PIDS por último com o ctest vermelho, e o rc 7 preservado"
else
    falha "corpo do executor: rc=$rc (esperado 7), última linha '$ultima' (esperada 'NL-PIDS max=256 peak=31 eventos_max=0')"
fi

# Caso 24: o executor roda o corpo com bash e tem o teto de 256 do envelope da coordenação (v3.19).
TOTAL=$((TOTAL + 1))
n_teto="$(/usr/bin/grep -c -- '--pids-limit 256 --ulimit core=1' "$CONTEINER/banco/executor.sh" || true)"
n_corpo="$(/usr/bin/grep -c 'bash /src/tools/conteiner/ctest_banco_no_conteiner.sh' "$CONTEINER/banco/executor.sh" || true)"
if [ "$n_teto" = 1 ] && [ "$n_corpo" = 1 ]; then
    ok "executor com --pids-limit 256 e o corpo ctest_banco_no_conteiner.sh (teto=$n_teto corpo=$n_corpo)"
else
    falha "executor: teto 256=$n_teto e corpo=$n_corpo, esperados 1 e 1"
fi

# Caso 25: o resumo curto do ctest do manjaro conta os 4 testes (NL-3.1h v3.21: sem ", 0 tests failed", o harness lia 0).
novo_caso resumo_curto || { echo "ABORTA: caso 25"; exit 1; }
export STUB_RESUMO_CURTO=1
executar_verde "resumo curto do ctest do manjaro conta 4" \
    "NL-BANCO-RESUMO pares_esperados=1 executados=1 verdes=1 vermelhos=0 n_por_par=4" "encontrados=4 analisados=4 pulados=0 falharam=0 rc=0" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 12: volume do Qt ausente reprova antes de qualquer par (docker run -v nome: criaria volume vazio).
novo_caso volume_qt_ausente || { echo "ABORTA: caso 12"; exit 1; }
export STUB_VOLUME_AUSENTE=1
executar_caso "volume do Qt ausente reprova" "PARADA: volume do Qt ausente" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 13: classe disco sem o certificado no disco reprova (a sonda e o disco têm de bater).
novo_caso classe_disco_sem_pem || { echo "ABORTA: caso 13"; exit 1; }
export STUB_PEM_VAZIO=1
executar_caso "classe disco sem PEM reprova" "PARADA: classe do servidor" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 14: controle verde: par certo dá rc 0, com o resumo de 1 par verde e a linha NL-BANCO-TLS.
novo_caso controle_verde || { echo "ABORTA: caso 14"; exit 1; }
executar_verde "controle verde: par certo fica verde" \
    "NL-BANCO-RESUMO pares_esperados=1 executados=1 verdes=1 vermelhos=0 n_por_par=4" "NL-BANCO-PIDS par=fedora45/mysql97 max=256 peak=12 eventos_max=0" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 15: todos na distro ubuntu26.04 (dois pares em matriz_banco.txt): os dois pares rodam e ficam verdes (achado A5).
novo_caso todos_dois_pares || { echo "ABORTA: caso 15"; exit 1; }
executar_verde "todos com dois pares, os dois rodam" \
    "NL-BANCO-RESUMO pares_esperados=2 executados=2 verdes=2 vermelhos=0 n_por_par=4" "NL-BANCO-TLS par=ubuntu26.04/mariadb118 NL-TLS conexao=sessaoPadrao resultado=tls" \
    bash "$CONTEINER/integracao_banco.sh" ubuntu26.04 todos

# Caso 16: a conta de teste recebe exatamente o GRANT esperado, com a crase escapada, e o shell não executa nada.
novo_caso grant_conta_teste || { echo "ABORTA: caso 16"; exit 1; }
TOTAL=$((TOTAL + 1))
saida="$(bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97 2>&1)"; rc=$?
n_grant="$(/usr/bin/grep -cF -- "$GRANT_ESPERADO" "$CASO/grant.log" 2>/dev/null || true)"
if [ "$rc" -eq 0 ] && [ "$n_grant" = 1 ] && [[ "$saida" != *"command not found"* ]]; then
    ok "GRANT da conta de teste com crase escapada (rc=$rc, grant=$n_grant)"
else
    falha "GRANT da conta de teste: rc=$rc, grant_esperado_em_grant_log=$n_grant, 'command not found' na saída"
fi

# Caso 26 (H-R, NL-3.1a): NL_PRESET_BANCO fora de banco e banco-asan é PARADA, antes de qualquer par.
novo_caso preset_fora_da_lista || { echo "ABORTA: caso 26"; exit 1; }
export NL_PRESET_BANCO=outro
executar_caso "NL_PRESET_BANCO fora de banco e banco-asan reprova" "PARADA: NL_PRESET_BANCO" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 27 (H-R, NL-3.1a): com banco-asan, o harness lê o CMakeCache do próprio preset, e não o do banco (que está ON aqui).
# O build do banco-asan fica ao lado de banco/, na raiz da distro: $(dirname "$BUILD")/banco-asan.
novo_caso cache_banco_asan || { echo "ABORTA: caso 27"; exit 1; }
mkdir -p "$(dirname "$BUILD")/banco-asan" || { echo "ABORTA: caso 27 (pasta)"; exit 1; }
printf 'NL_TESTES_BANCO:BOOL=OFF\n' > "$(dirname "$BUILD")/banco-asan/CMakeCache.txt"
export NL_PRESET_BANCO=banco-asan
executar_caso "banco-asan lê o próprio cache: sem NL_TESTES_BANCO reprova" "PARADA: build banco sem NL_TESTES_BANCO" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 28 (H-R, NL-3.1a; v1g): com banco-asan, a chamada do executor leva só o nome do preset, e nenhum ajuste de ambiente
# de sanitizer. O ambiente do sanitizer vem do test preset, pelo ctest --preset (o caso não cita as variáveis).
novo_caso preset_no_executor || { echo "ABORTA: caso 28"; exit 1; }
mkdir -p "$(dirname "$BUILD")/banco-asan/junit" || { echo "ABORTA: caso 28 (pasta)"; exit 1; }
printf 'NL_TESTES_BANCO:BOOL=ON\n' > "$(dirname "$BUILD")/banco-asan/CMakeCache.txt"
export NL_PRESET_BANCO=banco-asan
TOTAL=$((TOTAL + 1))
saida="$(bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97 2>&1)"; rc=$?
linha_executor="$(/usr/bin/grep -F 'ctest_banco_no_conteiner.sh' "$CASO/docker.log" 2>/dev/null | head -n 1)"
n_ajustes="$(printf '%s\n' "$linha_executor" | /usr/bin/grep -c '_OPTIONS=' || true)"
if [ "$rc" -eq 0 ] && [[ "$linha_executor" == *"-e NL_PRESET_BANCO=banco-asan"* ]] && [ "$n_ajustes" = 0 ]; then
    ok "chamada do executor com banco-asan leva só o preset, sem ajuste de ambiente (rc=$rc)"
else
    falha "chamada do executor com banco-asan: rc=$rc, sem '-e NL_PRESET_BANCO=banco-asan' ou com ajuste de ambiente ($n_ajustes)"
fi

echo "casos=$TOTAL falharam=$FALHAS"
if [ "$TOTAL" -ne 28 ]; then
    echo "AUTOTESTE INTEGRACAO BANCO: contagem de casos diferente de 28"
    exit 1
fi
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE INTEGRACAO BANCO: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE INTEGRACAO BANCO: REPROVADO"
exit 1
