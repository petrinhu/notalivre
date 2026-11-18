#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do classificador do configure e do resultado do Connector/C (NL-3.1h, passo 6.2, v3.10; L-35, L-36, L-17).
# O classificador (classifica_configure_mariadb.sh) é a única fonte da regra de aceite: aqui só se confere o que ele devolve.
#   C1 só CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT não usada, sem Found GSSAPI: rc 0 (tolerada)
#   C2 cópia literal de NL-2.0-imagem-fedora45.log linhas 520-523 (inclui CLIENT_PLUGIN_REMOTE_IO): rc 1
#   C3 nome com erro de digitação (CLIENT_PLUGIN_PARSEK): rc 1
#   C4 Found GSSAPI com CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT não usada: rc 1 (GSSAPI achada não pode ficar sem registro)
#   C5 sem aviso e com Configuring done: rc 0
#   C6 log vazio: rc 1 (piso, L-36)
#   M1 install_manifest com remote_io.so: rc 1
#   M2 install_manifest só com caching_sha2_password.so: rc 0 (controle positivo)
#   M3 install_manifest vazio: rc 1 (piso)
# Vermelho antes do classificador. Não sobe contêiner. Uso: autoteste_classifica_configure_mariadb.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CLASSIFICA="$RAIZ/tools/conteiner/classifica_configure_mariadb.sh"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-classifica.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0
TOTAL=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

mkdir -p "$BASE/cfg" "$BASE/man"

# Fixtures do configure.
printf -- '-- Configuring done\nManually-specified variables were not used by the project:\n\n    CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT\n\n-- Generating done\n' > "$BASE/cfg/c1.log"
cat > "$BASE/cfg/c2.log" <<'LOG'
#8 45.64 PARADA: opção desconhecida no configure do Connector/C
#8 45.64   Manually-specified variables were not used by the project:
#8 45.64
#8 45.64     CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT
#8 45.64     CLIENT_PLUGIN_REMOTE_IO
#8 45.64
LOG
printf -- '-- Configuring done\nManually-specified variables were not used by the project:\n\n    CLIENT_PLUGIN_PARSEK\n\n-- Generating done\n' > "$BASE/cfg/c3.log"
printf -- '-- Found GSSAPI: /usr/lib64/libgssapi_krb5.so\n-- Configuring done\nManually-specified variables were not used by the project:\n\n    CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT\n\n-- Generating done\n' > "$BASE/cfg/c4.log"
printf -- '-- Configuring done\n-- Generating done\n' > "$BASE/cfg/c5.log"
: > "$BASE/cfg/c6.log"

# Fixtures do resultado instalado.
printf '/usr/local/lib64/mariadb/plugin/remote_io.so\n/usr/local/lib64/mariadb/plugin/caching_sha2_password.so\n' > "$BASE/man/m1.txt"
printf '/usr/local/lib64/mariadb/plugin/caching_sha2_password.so\n' > "$BASE/man/m2.txt"
: > "$BASE/man/m3.txt"

# caso NOME RC_ESPERADO SUBCOMANDO ARQUIVO: roda o classificador e confere o rc.
caso() {
    local nome="$1" esperado="$2" sub="$3" arq="$4" saida rc
    TOTAL=$((TOTAL + 1))
    if [ ! -f "$CLASSIFICA" ]; then
        falha "$nome: classificador ausente ($CLASSIFICA)"
        return
    fi
    saida="$(bash "$CLASSIFICA" "$sub" "$arq" 2>&1)"; rc=$?
    if [ "$rc" -eq "$esperado" ]; then ok "$nome (rc=$rc)"
    else falha "$nome: rc=$rc, esperado $esperado. Saída: $(printf '%s' "$saida" | tr '\n' ' ')"; fi
}

caso "C1 só GSSAPI não usada, sem Found GSSAPI, tolerada" 0 configure "$BASE/cfg/c1.log"
caso "C2 cópia do log da imagem com REMOTE_IO reprova" 1 configure "$BASE/cfg/c2.log"
caso "C3 nome com erro de digitação reprova" 1 configure "$BASE/cfg/c3.log"
caso "C4 Found GSSAPI com GSSAPI não usada reprova" 1 configure "$BASE/cfg/c4.log"
caso "C5 sem aviso e com Configuring done passa" 0 configure "$BASE/cfg/c5.log"
caso "C6 log vazio reprova (piso)" 1 configure "$BASE/cfg/c6.log"
caso "M1 install com remote_io.so reprova" 1 plugins "$BASE/man/m1.txt"
caso "M2 install só com caching_sha2_password.so passa" 0 plugins "$BASE/man/m2.txt"
caso "M3 install vazio reprova (piso)" 1 plugins "$BASE/man/m3.txt"

echo "casos=$TOTAL falharam=$FALHAS"
if [ "$TOTAL" -ne 9 ]; then
    echo "AUTOTESTE CLASSIFICA CONFIGURE MARIADB: contagem de casos diferente de 9"
    exit 1
fi
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CLASSIFICA CONFIGURE MARIADB: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CLASSIFICA CONFIGURE MARIADB: REPROVADO"
exit 1
