#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do pré-cheque do cliente MariaDB nas imagens de build (NL-3.1h, V-b e v3.17; L-35, L-36). Molde: autoteste_cabecalhos_sodium.sh.
#   M1 cada instalar/ traz o pacote do cliente MariaDB (fedora45, ubuntu26.04, mint22.3, manjaro).
#   M2 prefixo sem mysql.h: o precheque REPROVA com mariadb_h=AUSENTE.
#   M3 controle positivo, layout DEFAULT e Debian: <prefixo>/include/mariadb/mysql.h e <prefixo>/lib/mariadb/libmariadb.so.
#   M4 precheque_mariadb.sh é chamado pelo precheque_qt.sh (o CI e o build_local rodam precheque_qt.sh).
#   M5 controle positivo, layout Arch e RPM: <prefixo>/include/mysql/mysql.h e <prefixo>/lib/libmariadb.so.
#   M6 prefixo com mysql.h e sem libmariadb.so: REPROVA com libmariadb=AUSENTE (cabeçalho e biblioteca do mesmo prefixo).
#   M7 sem NL_MARIADB_PREFIX e sem NL_DISTRO: REPROVA com prefixo=AUSENTE (falha fechada).
#   M8 a tabela prefixo_mariadb.txt dá /usr/local nas 4 distros (o módulo 3.4.11 em todas; decisão do líder de 19/11), e rc 1 fora dela.
#   M9 os 4 instalar/*.sh chamam modulo_mariadb.sh, e os 4 Dockerfiles de build copiam modulo_mariadb.sh e classifica_configure_mariadb.sh.
# Os prefixos são pastas de sandbox: o autoteste não depende do Connector/C da imagem nem do runner. Não sobe contêiner.
# Uso: autoteste_cabecalhos_mariadb.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONTEINER="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-mariadb.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0
CHECA="$CONTEINER/precheque_mariadb.sh"

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# M1: pacote do cliente MariaDB em cada um dos quatro scripts instalar/ (piso: 4 analisados).
analisados=0; faltou=0
for par in fedora45:mariadb-connector-c-devel ubuntu26.04:libmariadb-dev mint22.3:libmariadb-dev manjaro:mariadb-libs; do
    arq="${par%%:*}"; pac="${par#*:}"
    analisados=$((analisados + 1))
    grep -qw -- "$pac" "$CONTEINER/instalar/$arq.sh" 2>/dev/null || faltou=$((faltou + 1))
done
echo "M1 cliente mariadb em instalar/: analisados=$analisados falharam=$faltou"
if [ "$analisados" -eq 4 ] && [ "$faltou" -eq 0 ]; then ok "M1 pacote do cliente MariaDB nos quatro instalar/"
else falha "M1 pacote do cliente MariaDB ausente em $faltou de $analisados scripts"; fi

# Prefixos de sandbox: um mysql.h de uma linha e uma libmariadb.so vazia, nos dois layouts e nos dois casos de falta.
mkdir -p "$BASE/deb/include/mariadb" "$BASE/deb/lib/mariadb" "$BASE/arch/include/mysql" "$BASE/arch/lib" \
    "$BASE/sem_h/lib/mariadb" "$BASE/sem_lib/include/mariadb"
for h in "$BASE/deb/include/mariadb" "$BASE/arch/include/mysql" "$BASE/sem_lib/include/mariadb"; do
    printf '#define NOTALIVRE_MARIADB_SANDBOX_AUTOTESTE 1\n' > "$h/mysql.h"
done
: > "$BASE/deb/lib/mariadb/libmariadb.so"
: > "$BASE/arch/lib/libmariadb.so"
: > "$BASE/sem_h/lib/mariadb/libmariadb.so"

# caso NOME PREFIXO DISTRO RC TRECHO: roda o precheque sem NL_MARIADB_PREFIX e sem NL_DISTRO do ambiente, e com os dados
# ("-" deixa a variável de fora). RC 0 exige rc 0; RC 1 exige rc diferente de 0. O trecho tem de estar na saída.
caso() {
    local nome="$1" prefixo="$2" distro="$3" rc_esp="$4" trecho="$5" saida rc
    local -a amb=(env -u NL_MARIADB_PREFIX -u NL_DISTRO)
    [ "$prefixo" = - ] || amb+=("NL_MARIADB_PREFIX=$prefixo")
    [ "$distro" = - ] || amb+=("NL_DISTRO=$distro")
    if [ ! -f "$CHECA" ]; then falha "$nome: precheque_mariadb.sh ausente"; return; fi
    saida="$("${amb[@]}" bash "$CHECA" 2>&1)"; rc=$?
    if { [ "$rc_esp" = 0 ] && [ "$rc" -eq 0 ]; } || { [ "$rc_esp" != 0 ] && [ "$rc" -ne 0 ]; }; then
        if [[ "$saida" == *"$trecho"* ]]; then ok "$nome (rc=$rc)"; return; fi
    fi
    falha "$nome: rc=$rc, esperado $rc_esp com '$trecho'; saída: $saida"
}

caso "M2 prefixo sem mysql.h reprova" "$BASE/sem_h" - 1 "mariadb_h=AUSENTE"
caso "M3 layout include/mariadb e lib/mariadb aprova" "$BASE/deb" - 0 "mariadb_h=ok libmariadb=ok"
caso "M5 layout include/mysql e lib aprova" "$BASE/arch" - 0 "mariadb_h=ok libmariadb=ok"
caso "M6 prefixo sem libmariadb.so reprova" "$BASE/sem_lib" - 1 "libmariadb=AUSENTE"
caso "M7 sem prefixo e sem distro reprova" - - 1 "prefixo=AUSENTE"

# M4: a etapa que o CI e o build_local já rodam chama a checagem.
if grep -q -- 'precheque_mariadb.sh' "$CONTEINER/precheque_qt.sh" 2>/dev/null; then
    ok "M4 precheque_qt.sh chama precheque_mariadb.sh"
else
    falha "M4 precheque_qt.sh não chama precheque_mariadb.sh"
fi

# M8: a tabela de prefixos, pelo átomo que a lê.
m8="$( . "$CONTEINER/prefixo_mariadb.sh" 2>/dev/null || exit 0
       for d in fedora45 ubuntu26.04 mint22.3 manjaro; do printf '%s=%s ' "$d" "$(nl_prefixo_mariadb "$d")"; done
       nl_prefixo_mariadb xyz > /dev/null 2>&1; echo "xyz_rc=$?" )"
esperado='fedora45=/usr/local ubuntu26.04=/usr/local mint22.3=/usr/local manjaro=/usr/local xyz_rc=1'
if [ "$m8" = "$esperado" ]; then ok "M8 tabela de prefixos ($m8)"
else falha "M8 tabela de prefixos: obtido [$m8], esperado [$esperado]"; fi

# M9: o módulo 3.4.11 nas 4 imagens (piso: 4 analisados em cada lista).
n_inst=0; n_dock=0
for d in fedora45 ubuntu26.04 mint22.3 manjaro; do
    grep -q 'MODULO=/opt/nl/modulo_mariadb.sh' "$CONTEINER/instalar/$d.sh" 2>/dev/null && n_inst=$((n_inst + 1))
    if grep -q '^COPY modulo_mariadb.sh ' "$CONTEINER/$d-build.Dockerfile" 2>/dev/null \
       && grep -q '^COPY classifica_configure_mariadb.sh ' "$CONTEINER/$d-build.Dockerfile" 2>/dev/null; then
        n_dock=$((n_dock + 1))
    fi
done
if [ "$n_inst" -eq 4 ] && [ "$n_dock" -eq 4 ]; then ok "M9 módulo do Connector/C nos 4 instalar/ e nos 4 Dockerfiles"
else falha "M9 módulo do Connector/C: instalar=$n_inst de 4, Dockerfile=$n_dock de 4"; fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CABECALHOS MARIADB: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CABECALHOS MARIADB: REPROVADO"
exit 1
