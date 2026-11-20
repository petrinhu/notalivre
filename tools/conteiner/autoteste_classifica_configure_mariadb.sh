#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do classificador do configure e do resultado do Connector/C (NL-3.1h, passo 6.2, v3.10 e v3.12; L-35, L-36, L-17).
# O classificador (classifica_configure_mariadb.sh) é a única fonte da regra de aceite: aqui só se confere o que ele devolve.
# Cada caso confere o rc e, quando o rc esperado é 1, também o motivo: o trecho exato do caminho que deve reprovar. Sem o
# motivo, um rc 1 vindo de outro caminho do classificador ficaria verde (v3.12).
#   C1 só CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT não usada, sem Found GSSAPI: rc 0
#   C2 aviso da imagem (REMOTE_IO) depois do Configuring done reprova pelo nome: rc 1, tolerância: CLIENT_PLUGIN_REMOTE_IO
#   C3 nome com erro de digitação (CLIENT_PLUGIN_PARSEK): rc 1, tolerância: CLIENT_PLUGIN_PARSEK
#   C4 Found GSSAPI com CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT não usada: rc 1, tolerância: CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT
#   C5 sem aviso e com Configuring done: rc 0
#   C6 log vazio: rc 1, log do configure ausente ou vazio (piso, L-36)
#   C7 log não vazio sem Configuring done: rc 1, sem Configuring done (piso)
#   M1 install_manifest com remote_io.so: rc 1, plugin proibido instalado
#   M2 install_manifest só com caching_sha2_password.so: rc 0 (controle positivo)
#   M3 install_manifest vazio: rc 1, install_manifest ausente ou vazio (piso)
#   M4 install_manifest não vazio sem caching_sha2_password.so: rc 1, caching_sha2_password.so ausente (piso)
# Não sobe contêiner. Uso: autoteste_classifica_configure_mariadb.sh
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
printf '%s\n' '#8 45.60 -- Configuring done (0.4s)' '#8 45.64 CMake Warning:' '#8 45.64   Manually-specified variables were not used by the project:' '#8 45.64 ' '#8 45.64     CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT' '#8 45.64     CLIENT_PLUGIN_REMOTE_IO' '#8 45.64 ' '#8 45.64 -- Generating done (0.1s)' > "$BASE/cfg/c2.log"
printf -- '-- Configuring done\nManually-specified variables were not used by the project:\n\n    CLIENT_PLUGIN_PARSEK\n\n-- Generating done\n' > "$BASE/cfg/c3.log"
printf -- '-- Found GSSAPI: /usr/lib64/libgssapi_krb5.so\n-- Configuring done\nManually-specified variables were not used by the project:\n\n    CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT\n\n-- Generating done\n' > "$BASE/cfg/c4.log"
printf -- '-- Configuring done\n-- Generating done\n' > "$BASE/cfg/c5.log"
: > "$BASE/cfg/c6.log"
printf '%s\n' '-- The C compiler identification is GNU 15.2.1' 'CMake Error at CMakeLists.txt:10 (message):' '  falha simulada' '-- Configuring incomplete, errors occurred!' > "$BASE/cfg/c7.log"

# Fixtures do resultado instalado.
printf '/usr/local/lib64/mariadb/plugin/remote_io.so\n/usr/local/lib64/mariadb/plugin/caching_sha2_password.so\n' > "$BASE/man/m1.txt"
printf '/usr/local/lib64/mariadb/plugin/caching_sha2_password.so\n' > "$BASE/man/m2.txt"
: > "$BASE/man/m3.txt"
printf '%s\n' '/usr/local/lib64/libmariadb.so.3' '/usr/local/include/mariadb/mysql.h' '/usr/local/lib64/mariadb/plugin/client_ed25519.so' > "$BASE/man/m4.txt"

# caso NOME RC_ESPERADO SUBCOMANDO ARQUIVO MOTIVO: roda o classificador, confere o rc e, se MOTIVO não for vazio, o trecho na saída.
caso() {
    local nome="$1" esperado="$2" sub="$3" arq="$4" motivo="$5" saida rc
    TOTAL=$((TOTAL + 1))
    if [ ! -f "$CLASSIFICA" ]; then
        falha "$nome: classificador ausente ($CLASSIFICA)"
        return
    fi
    saida="$(bash "$CLASSIFICA" "$sub" "$arq" 2>&1)"; rc=$?
    if [ "$rc" -ne "$esperado" ]; then
        falha "$nome: rc=$rc, esperado $esperado. Saída: $(printf '%s' "$saida" | tr '\n' ' ')"
        return
    fi
    if [ -n "$motivo" ] && [[ "$saida" != *"$motivo"* ]]; then
        falha "$nome: motivo ausente: $motivo. Saída: $(printf '%s' "$saida" | tr '\n' ' ')"
        return
    fi
    ok "$nome (rc=$rc)"
}

caso "C1 só GSSAPI não usada, sem Found GSSAPI, tolerada" 0 configure "$BASE/cfg/c1.log" ""
caso "C2 aviso da imagem (REMOTE_IO) depois do Configuring done reprova pelo nome" 1 configure "$BASE/cfg/c2.log" "tolerância: CLIENT_PLUGIN_REMOTE_IO"
caso "C3 nome com erro de digitação reprova" 1 configure "$BASE/cfg/c3.log" "tolerância: CLIENT_PLUGIN_PARSEK"
caso "C4 Found GSSAPI com GSSAPI não usada reprova" 1 configure "$BASE/cfg/c4.log" "tolerância: CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT"
caso "C5 sem aviso e com Configuring done passa" 0 configure "$BASE/cfg/c5.log" ""
caso "C6 log vazio reprova (piso)" 1 configure "$BASE/cfg/c6.log" "log do configure ausente ou vazio"
caso "C7 log não vazio sem Configuring done reprova (piso)" 1 configure "$BASE/cfg/c7.log" "sem Configuring done"
caso "M1 install com remote_io.so reprova" 1 plugins "$BASE/man/m1.txt" "plugin proibido instalado"
caso "M2 install só com caching_sha2_password.so passa" 0 plugins "$BASE/man/m2.txt" ""
caso "M3 install vazio reprova (piso)" 1 plugins "$BASE/man/m3.txt" "install_manifest ausente ou vazio"
caso "M4 install sem caching_sha2_password.so reprova (piso)" 1 plugins "$BASE/man/m4.txt" "caching_sha2_password.so ausente"

echo "casos=$TOTAL falharam=$FALHAS"
if [ "$TOTAL" -ne 11 ]; then
    echo "AUTOTESTE CLASSIFICA CONFIGURE MARIADB: contagem de casos diferente de 11"
    exit 1
fi
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CLASSIFICA CONFIGURE MARIADB: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CLASSIFICA CONFIGURE MARIADB: REPROVADO"
exit 1
