#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Harness de integração com servidor de banco (NL-3.1h, passo 8). Uso: integracao_banco.sh <distro> <nome|todos>.
# Um par por vez: credencial 0700, servidor com rede none, conta de teste, classe do certificado conferida, executor na rede
# do servidor, resultado por junitxml e quatro linhas NL-TLS (critério 4 de TLS, C4').
# Só orquestra: cada assunto mora num átomo de banco/. Trava única para a execução inteira (~/.claude/bin/pesado, L-11 item 2).
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NL_BANCO_DIR="$RAIZ/tools/conteiner"
LOGS=/var/tmp/notalivre-logs
DISTRO="${1:-}"
ALVO="${2:-}"
[ -n "$DISTRO" ] && [ -n "$ALVO" ] || { echo "uso: integracao_banco.sh <distro> <nome|todos>"; exit 2; }
[ -n "${NL_SHA:-}" ] || { echo "PARADA: NL_SHA é obrigatório"; exit 2; }
[ -n "${NL_FATIA:-}" ] || { echo "PARADA: NL_FATIA é obrigatório"; exit 2; }
# NL-3.1a (H-G): o preset do build de banco. Só dois valores: banco (padrão) e banco-asan (sanitizer). Outro é PARADA.
NL_PRESET_BANCO="${NL_PRESET_BANCO:-banco}"
case "$NL_PRESET_BANCO" in
    banco|banco-asan) ;;
    *) echo "PARADA: NL_PRESET_BANCO '$NL_PRESET_BANCO' fora de banco e banco-asan"; echo "NL-RC=2"; exit 2 ;;
esac
export NL_PRESET_BANCO
if [ "${NL_TRAVA_DETIDA:-}" != 1 ]; then
    export NL_TRAVA_DETIDA=1
    exec "${NL_PESADO:-$HOME/.claude/bin/pesado}" --teto "${NL_PESADO_TETO:-15}" -- bash "$(readlink -f "$0")" "$@"
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
. "$NL_BANCO_DIR/banco/pids.sh"
. "$NL_BANCO_DIR/banco/certificado.sh"
. "$NL_BANCO_DIR/banco/sonda.sh"

FONTE="$NL_TMP_BASE/notalivre-src-$NL_SHA-$NL_FATIA"
BUILD="$NL_TMP_BASE/notalivre-build-$DISTRO-$NL_FATIA"
if [ ! -d "$FONTE" ] || [ ! -d "$BUILD" ]; then
    echo "PARADA: fonte ou build ausente ($FONTE, $BUILD)"; echo "NL-RC=2"; exit 2
fi
opcao="$(/usr/bin/grep -c 'NL_TESTES_BANCO:BOOL=ON' "$BUILD/$NL_PRESET_BANCO/CMakeCache.txt" 2>/dev/null || true)"
if [ "$opcao" != 1 ]; then
    echo "PARADA: build banco sem NL_TESTES_BANCO:BOOL=ON"; echo "NL-RC=2"; exit 2
fi

# Portão de funções (C15', achado A1): a linha tem de existir e ter faltam, sobram e falharam iguais a 0. Ausente é PARADA.
LOG_FUNCOES="$LOGS/NL-3.1h-funcoes-$DISTRO-$NL_SHA.log"
nl_banco_rodar_portao_funcoes "$DISTRO" "$FONTE" "$BUILD" > "$LOG_FUNCOES" 2>&1
linha_funcoes="$(/usr/bin/grep -h '^funcoes_no_ctest: ' "$LOG_FUNCOES" | tail -n 1 || true)"
if [ -z "$linha_funcoes" ]; then
    echo "funcoes_no_ctest: ausente (PARADA)"
    echo "PARADA: portão de funções"; echo "NL-RC=2"; exit 2
fi
echo "$linha_funcoes"
if ! printf '%s\n' "$linha_funcoes" | /usr/bin/grep -Eq ' faltam=0 sobram=0 .* falharam=0$'; then
    echo "PARADA: portão de funções"; echo "NL-RC=2"; exit 2
fi

# Guarda do volume do Qt (achado de 3.3): um volume nomeado montado com -v que não existe nasce vazio.
if ! docker volume inspect notalivre-qt-6120 >/dev/null 2>&1; then
    echo "PARADA: volume do Qt ausente"; echo "NL-RC=2"; exit 2
fi

if [ "$ALVO" = todos ]; then
    PARES="$(nl_banco_pares_de "$DISTRO")"
    ESPERADOS="$(nl_banco_pares_de "$DISTRO" | /usr/bin/grep -c . || true)"
else
    PARES="$(nl_banco_pares_de "$DISTRO" | /usr/bin/grep -x -- "$ALVO" || true)"
    ESPERADOS=1
fi
if [ -z "$PARES" ]; then
    echo "PARADA: par $DISTRO/$ALVO fora de matriz_banco.txt"; echo "NL-RC=2"; exit 2
fi

# executar_par DISTRO NOME: um par completo. Devolve 0 só se o par ficou verde; a credencial é conferida antes de sair.
# Com o executor, todas as quatro linhas NL-TLS têm de aparecer (C4'); a classe do servidor é conferida antes dele.
N_TLS_LISTA=""
executar_par() {
    local distro="$1" nome="$2" linha fam img metodo tlsc certc cred cont log_par plugin versao rede usada
    local enc analisados pulados falharam rc_exec n_tls linhas_tls ok=1
    local junit="$BUILD/$NL_PRESET_BANCO/junit"
    enc=0; analisados=0; pulados=0; falharam=0; rc_exec=2; n_tls=0; linhas_tls=""
    versao=sem_servidor; plugin=sem_conta; rede=nao_medida
    linha="$(nl_banco_servidor_de "$nome")" || { echo "PARADA: $nome fora de servidores_banco.txt"; return 1; }
    read -r _ fam img metodo tlsc certc <<<"$linha"
    if ! docker image inspect "$img" >/dev/null 2>&1; then
        echo "PARADA: servidor $nome ausente no host ($img)"; return 1
    fi
    cred="/var/tmp/notalivre-nl31h-cred-$distro-$nome-$$"
    cont="nl31h-srv-$distro-$nome-$$"
    log_par="$LOGS/NL-3.1h-par-$distro-$nome-$NL_SHA.log"
    : > "$log_par"

    nl_banco_credencial_nova "$cred" || { echo "PARADA: credencial de $nome"; return 1; }
    printf 'NL_BANCO_HOST=127.0.0.1\nNL_BANCO_PORTA=3306\nNL_BANCO_USUARIO=notalivre_teste\nNL_BANCO_BASE=notalivre_teste\nNL_BANCO_SENHA_ARQUIVO=/run/nl/senha_app\nNL_BANCO_SERVIDOR_TLS=%s\nNL_BANCO_CERTIFICADO=%s\n' \
        "$tlsc" "$certc" > "$cred/executor.env"

    if nl_banco_subir_servidor "$nome" "$cred" "$cont" && nl_banco_esperar_pronto "$cont" "$fam" \
        && nl_banco_criar_conta_teste "$cont" "$fam" "$metodo" "$cred"; then
        plugin="$(nl_banco_ler_metodo "$cont" "$fam")"
        versao="$(nl_banco_cliente "$cont" "$fam" -N -e 'SELECT VERSION();' 2>/dev/null)"
        rede="$(docker inspect -f '{{.HostConfig.NetworkMode}} {{.HostConfig.PortBindings}}' "$cont" 2>/dev/null)"
        if usada="$(nl_banco_preparar_classe "$distro" "$cont" "$fam" "$cred" "$FONTE" "$certc" "$log_par")"; then
            printf 'NL_BANCO_IMPRESSAO=%s\n' "$usada" >> "$cred/executor.env"
            nl_banco_limpar_resultado "$junit" || ok=0
            nl_banco_rodar_executor "$distro" "$cont" "$cred" "$FONTE" "$BUILD" >> "$log_par" 2>&1
            rc_exec=$?
            # O resumo do ctest sai com ou sem ", N tests failed" (manjaro: "100% tests passed out of 4"; NL-3.1h, v3.21).
            enc="$(awk '/ tests passed(, [0-9]+ tests failed)? out of [0-9]+$/ { n = $NF } END { print n + 0 }' "$log_par")"
            read -r analisados pulados falharam <<<"$(nl_banco_contar_resultado "$junit")"
            # NL-TLS e NL-CONECTOR chegam pelo ctest --verbose, com o prefixo "NNN: QINFO  : Classe::slot() " (log real do par,
            # 19/11/2026). A âncora exige o prefixo inteiro: a reimpressão sem número do --output-on-failure, a NL-TLS-MSG e os
            # ecos do harness não contam (NL-3.1h, v3.19). Colchetes em vez de barra invertida: ERE POSIX, gawk e mawk.
            read -r n_tls linhas_tls <<<"$(awk '/^[0-9]+: QINFO  : [A-Za-z_][A-Za-z0-9_]*::[A-Za-z_][A-Za-z0-9_]*[(][)] NL-TLS / { n++; s = s (n > 1 ? " | " : "") substr($0, index($0, "NL-TLS ")) } END { print n + 0, s }' "$log_par")"
            if [ "$n_tls" != "$enc" ]; then
                echo "PARADA: linhas NL-TLS (par $distro/$nome: $n_tls, esperadas $enc)"; ok=0
            fi
            n_conector="$(awk '/^[0-9]+: QINFO  : [A-Za-z_][A-Za-z0-9_]*::[A-Za-z_][A-Za-z0-9_]*[(][)] NL-CONECTOR / { n++ } END { print n + 0 }' "$log_par")"
            if [ "$n_conector" != "$enc" ]; then
                echo "PARADA: linhas NL-CONECTOR (par $distro/$nome: $n_conector, esperadas $enc)"; ok=0
            fi
            nl_banco_conferir_pids "$log_par" "$distro/$nome" || ok=0
            if [ "$rc_exec" -ne 0 ] || [ "$pulados" -ne 0 ] || [ "$falharam" -ne 0 ] || [ "$analisados" -eq 0 ] \
                || [ "$enc" != "$analisados" ] || [ "$plugin" != "$metodo" ]; then
                ok=0
            fi
        else
            ok=0
        fi
    else
        ok=0; rc_exec=2
        echo "PARADA: par $distro/$nome não ficou pronto ou não criou a conta de teste"
        docker logs --tail 50 "$cont" 2>&1 | tail -n 50
    fi
    echo "NL-BANCO-TLS par=$distro/$nome $linhas_tls"
    N_TLS_LISTA="$N_TLS_LISTA $n_tls"
    echo "NL-BANCO-REDE par=$distro/$nome rede=$rede"

    nl_banco_linha_par "$distro" "$nome" "$versao" "$plugin" ver_NL-BANCO-TLS \
        "encontrados=$enc analisados=$analisados pulados=$pulados falharam=$falharam" "$rc_exec"
    nl_banco_parar "$cont"
    if ! nl_banco_conferir_senha_fora "$cred" "$log_par"; then
        echo "PARADA: senha encontrada em log do par $distro/$nome"; ok=0
    fi
    if ! nl_banco_conferir_senha_fora "$cred" "$cred/executor.env"; then
        echo "PARADA: senha encontrada no executor.env de $distro/$nome"; ok=0
    fi
    nl_banco_apagar_credencial "$cred" || echo "AVISO: credencial de $nome não apagada"
    if [ "$ok" -ne 1 ]; then
        echo "PARADA: par $distro/$nome vermelho (ver a linha NL-BANCO acima)"
        return 1
    fi
    return 0
}

executados=0; verdes=0; vermelhos=0
mapfile -t LISTA_PARES <<<"$PARES"
for nome in "${LISTA_PARES[@]}"; do
    [ -n "$nome" ] || continue
    executados=$((executados + 1))
    if executar_par "$DISTRO" "$nome" </dev/null; then verdes=$((verdes + 1)); else vermelhos=$((vermelhos + 1)); fi
done

# Número de linhas NL-TLS por par: um valor só se todos os pares tiverem o mesmo; senão, divergente.
N_POR_PAR="$(printf '%s\n' $N_TLS_LISTA | awk 'NF { if (!init) { v = $1; init = 1 } else if ($1 != v) { bad = 1 } } END { if (!init || bad) print "divergente"; else print v }')"
nl_banco_linha_resumo "$ESPERADOS" "$executados" "$verdes" "$vermelhos" "$N_POR_PAR"
if [ "$vermelhos" -eq 0 ] && [ "$executados" -eq "$ESPERADOS" ]; then rc=0; else rc=1; fi
echo "NL-RC=$rc"
exit "$rc"
