#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do harness de integração com banco (NL-3.1h, V-a; L-35, L-36). Cada caso exige o trecho específico da PARADA
# (ou da linha de resultado) que o harness precisa dar, e o rc. Um caso só passa com a saída e o rc que o harness deve dar.
# Não sobe contêiner: o docker é um de mentira no PATH. O de mentira lê a entrada do exec -i (como o docker real) e
# responde ao que o harness pede: portão de funções, servidor, prontidão, conta, plugin, versão, sonda e executor.
# Casos (17): servidor ausente; zero testes banco; build sem NL_TESTES_BANCO; junitxml do par anterior; junitxml com
# skipped; limpeza em falha; docker run sem o vetor da cota; par fora da matriz; arquivo de banco/ executado direto;
# portão de funções sem linha; portão de funções com faltam; só três linhas NL-TLS; volume do Qt ausente; classe disco
# sem PEM; controle verde; todos com dois pares; GRANT da conta de teste.
# Uso: autoteste_integracao_banco.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONTEINER="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-banco.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0
TOTAL=0
SHA_FIXO=0000000000000000000000000000000000000000
GRANT_ESPERADO="GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, ALTER, INDEX ON \`notalivre\\_%\`.* TO 'notalivre_teste'@'%';"

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

mkdir -p "$BASE/bin"
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
                    case "$a" in *:/var/tmp/notalivre-build:z) jd="${a%%:*}/banco/junit" ;; esac
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
                i=0
                for slot in sessaoPadrao modoAberto modoFixadoImpressaoCerta modoFixadoImpressaoTrocada; do
                    [ "$i" -lt "${STUB_LINHAS_TLS:-4}" ] || break
                    echo "NL-TLS conexao=$slot resultado=tls cliente=X servidor=X versao=TLSv1.3 status=0 erro=0"
                    i=$((i + 1))
                done
                [ -n "${STUB_SEM_RESUMO:-}" ] || echo "100% tests passed, 0 tests failed out of 4"
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
    unset STUB_SERVIDOR_AUSENTE STUB_VOLUME_AUSENTE STUB_JUNIT_XML STUB_SEM_JUNIT STUB_SEM_RESUMO STUB_LINHAS_TLS \
        STUB_EXEC_RC STUB_SRV_RC STUB_FUNCOES_AUSENTE STUB_FUNCOES_FALTA STUB_PLUGIN STUB_PEM STUB_PEM_VAZIO
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

# Caso 2: zero testes banco reprova (nenhum junitxml de integracao.banco.* e nenhum resumo do executor).
novo_caso zero_testes || { echo "ABORTA: caso 2"; exit 1; }
export STUB_SEM_JUNIT=1 STUB_SEM_RESUMO=1
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

# Caso 7: docker run sem o vetor da cota reprova. Varredura: todo docker run do harness e de banco/ leva o vetor.
# Piso (L-36): zero docker run encontrado é varredura quebrada, e reprova.
TOTAL=$((TOTAL + 1))
total_run=$(grep -hF 'docker run' "$CONTEINER/integracao_banco.sh" "$CONTEINER"/banco/*.sh 2>/dev/null | wc -l)
sem_vetor=$(grep -hF 'docker run' "$CONTEINER/integracao_banco.sh" "$CONTEINER"/banco/*.sh 2>/dev/null \
    | grep -vF 'NL_COTAS_DOCKER[@]' | wc -l)
echo "docker_run_sem_vetor: docker_run=$total_run sem_vetor=$sem_vetor"
if [ "$total_run" -gt 0 ] && [ "$sem_vetor" -eq 0 ]; then
    ok "docker run do harness leva o vetor da cota ($total_run chamadas)"
else
    falha "docker run do harness sem o vetor, ou varredura vazia (docker_run=$total_run sem_vetor=$sem_vetor)"
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
executar_caso "só três linhas NL-TLS reprovam" "PARADA: linhas NL-TLS" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

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
    "NL-BANCO-RESUMO pares_esperados=1 executados=1 verdes=1 vermelhos=0" "NL-BANCO-TLS par=" \
    bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 15: todos na distro ubuntu26.04 (dois pares em matriz_banco.txt): os dois pares rodam e ficam verdes (achado A5).
novo_caso todos_dois_pares || { echo "ABORTA: caso 15"; exit 1; }
executar_verde "todos com dois pares, os dois rodam" \
    "NL-BANCO-RESUMO pares_esperados=2 executados=2 verdes=2 vermelhos=0" "NL-BANCO-TLS par=" \
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

echo "casos=$TOTAL falharam=$FALHAS"
if [ "$TOTAL" -ne 17 ]; then
    echo "AUTOTESTE INTEGRACAO BANCO: contagem de casos diferente de 17"
    exit 1
fi
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE INTEGRACAO BANCO: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE INTEGRACAO BANCO: REPROVADO"
exit 1
