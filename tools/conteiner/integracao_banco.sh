#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Harness de integração com servidor de banco (NL-3.1h, passo 8). Uso: integracao_banco.sh <distro> <nome|todos>.
# Um par por vez: credencial 0700, servidor com rede none, conta de teste, executor na rede do servidor, resultado por junitxml.
# Só orquestra: cada assunto mora num átomo de banco/. Trava única para a execução inteira (flock do passo 8).
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NL_BANCO_DIR="$RAIZ/tools/conteiner"
LOGS=/var/tmp/notalivre-logs
DISTRO="${1:-}"
ALVO="${2:-}"
[ -n "$DISTRO" ] && [ -n "$ALVO" ] || { echo "uso: integracao_banco.sh <distro> <nome|todos>"; exit 2; }
[ -n "${NL_SHA:-}" ] || { echo "PARADA: NL_SHA é obrigatório"; exit 2; }
[ -n "${NL_FATIA:-}" ] || { echo "PARADA: NL_FATIA é obrigatório"; exit 2; }
if [ "${NL_TRAVA_DETIDA:-}" != 1 ]; then
    exec flock /var/tmp/notalivre-heavy.lock env NL_TRAVA_DETIDA=1 nice -n 5 "$0" "$@"
fi
echo "SHA: $NL_SHA fatia=$NL_FATIA distro=$DISTRO"
: "${NL_TMP_BASE:?defina NL_TMP_BASE (no host, /var/tmp)}"
mkdir -p "$LOGS"

. "$NL_BANCO_DIR/cota_conteiner.sh"
. "$NL_BANCO_DIR/banco/listas.sh"
. "$NL_BANCO_DIR/banco/credencial.sh"
. "$NL_BANCO_DIR/banco/servidor.sh"
. "$NL_BANCO_DIR/banco/sql_root.sh"
. "$NL_BANCO_DIR/banco/executor.sh"
. "$NL_BANCO_DIR/banco/resultado.sh"
. "$NL_BANCO_DIR/banco/linhas.sh"

FONTE="$NL_TMP_BASE/notalivre-src-$NL_SHA-$NL_FATIA"
BUILD="$NL_TMP_BASE/notalivre-build-$DISTRO-$NL_FATIA"
if [ ! -d "$FONTE" ] || [ ! -d "$BUILD" ]; then
    echo "PARADA: fonte ou build ausente ($FONTE, $BUILD)"; echo "NL-RC=2"; exit 2
fi
opcao="$(/usr/bin/grep -c 'NL_TESTES_BANCO:BOOL=ON' "$BUILD/banco/CMakeCache.txt" 2>/dev/null || true)"
if [ "$opcao" != 1 ]; then
    echo "PARADA: build banco sem NL_TESTES_BANCO:BOOL=ON"; echo "NL-RC=2"; exit 2
fi

nl_banco_rodar_portao_funcoes "$DISTRO" "$FONTE" "$BUILD" > "$LOGS/NL-3.1h-funcoes-$DISTRO-$NL_SHA.log" 2>&1
grep -h '^funcoes_no_ctest: ' "$LOGS/NL-3.1h-funcoes-$DISTRO-$NL_SHA.log" || echo "funcoes_no_ctest: ausente (PARADA)"

if [ "$ALVO" = todos ]; then
    PARES="$(nl_banco_pares_de "$DISTRO")"
else
    PARES="$(nl_banco_pares_de "$DISTRO" | /usr/bin/grep -x -- "$ALVO" || true)"
fi
if [ -z "$PARES" ]; then
    echo "PARADA: par $DISTRO/$ALVO fora de matriz_banco.txt"; echo "NL-RC=2"; exit 2
fi

# executar_par DISTRO NOME: um par completo. Devolve 0 só se o par ficou verde; a credencial é conferida antes de sair.
executar_par() {
    local distro="$1" nome="$2" linha fam img metodo cred cont log_par plugin versao enc analisados pulados falharam rc_exec ok=1
    local junit="$BUILD/banco/junit"
    linha="$(nl_banco_servidor_de "$nome")" || { echo "PARADA: $nome fora de servidores_banco.txt"; return 1; }
    read -r _ fam img metodo <<<"$linha"
    if ! docker image inspect "$img" >/dev/null 2>&1; then
        echo "PARADA: servidor $nome ausente no host ($img)"; return 1
    fi
    cred="/var/tmp/notalivre-nl31h-cred-$distro-$nome-$$"
    cont="nl31h-srv-$distro-$nome-$$"
    log_par="$LOGS/NL-3.1h-par-$distro-$nome-$NL_SHA.log"

    nl_banco_credencial_nova "$cred" || { echo "PARADA: credencial de $nome"; return 1; }
    printf 'NL_BANCO_HOST=127.0.0.1\nNL_BANCO_PORTA=3306\nNL_BANCO_USUARIO=notalivre_teste\nNL_BANCO_BASE=notalivre_teste\nNL_BANCO_SENHA_ARQUIVO=/run/nl/senha_app\n' > "$cred/executor.env"

    if nl_banco_subir_servidor "$nome" "$cred" "$cont" && nl_banco_esperar_pronto "$cont" "$fam" \
        && nl_banco_criar_conta_teste "$cont" "$fam" "$metodo" "$cred"; then
        plugin="$(nl_banco_ler_metodo "$cont" "$fam")"
        versao="$(nl_banco_cliente "$cont" "$fam" -N -e 'SELECT VERSION();' 2>/dev/null)"
        nl_banco_limpar_resultado "$junit" || ok=0
        nl_banco_rodar_executor "$distro" "$cont" "$cred" "$FONTE" "$BUILD" > "$log_par" 2>&1
        rc_exec=$?
        enc="$(awk '/tests failed out of/ { n = $NF } END { print n + 0 }' "$log_par")"
        read -r analisados pulados falharam <<<"$(nl_banco_contar_resultado "$junit")"
        if [ "$rc_exec" -ne 0 ] || [ "$pulados" -ne 0 ] || [ "$falharam" -ne 0 ] || [ "$analisados" -eq 0 ] \
            || [ "$enc" != "$analisados" ] || [ "$plugin" != "$metodo" ]; then
            ok=0
        fi
    else
        ok=0; rc_exec=2; enc=0; analisados=0; pulados=0; falharam=0; versao=sem_servidor; plugin=sem_conta
        echo "PARADA: par $distro/$nome não ficou pronto ou não criou a conta de teste"
        docker logs --tail 50 "$cont" 2>&1 | tail -n 50
    fi

    nl_banco_linha_par "$distro" "$nome" "$versao" "$plugin" verificado_pelo_teste verificado_pelo_teste \
        "encontrados=$enc analisados=$analisados pulados=$pulados falharam=$falharam" "$rc_exec"
    nl_banco_parar "$cont"
    if ! nl_banco_conferir_senha_fora "$cred" "$log_par"; then
        echo "PARADA: senha encontrada em log do par $distro/$nome"; ok=0
    fi
    nl_banco_apagar_credencial "$cred" || echo "AVISO: credencial de $nome não apagada"
    if [ "$ok" -ne 1 ]; then
        echo "PARADA: par $distro/$nome vermelho (ver a linha NL-BANCO acima)"
        return 1
    fi
    return 0
}

esperados=0; executados=0; verdes=0; vermelhos=0
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    esperados=$((esperados + 1))
    executados=$((executados + 1))
    if executar_par "$DISTRO" "$nome"; then verdes=$((verdes + 1)); else vermelhos=$((vermelhos + 1)); fi
done <<<"$PARES"

nl_banco_linha_resumo "$esperados" "$executados" "$verdes" "$vermelhos"
if [ "$vermelhos" -eq 0 ] && [ "$executados" -eq "$esperados" ]; then rc=0; else rc=1; fi
echo "NL-RC=$rc"
exit "$rc"
