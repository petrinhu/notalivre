#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Pré-cheque do Qt montado no contêiner (NL-0.4). Separa duas causas que pedem correções opostas:
#   - biblioteca de runtime ausente na imagem ("not found"): conserta-se o Dockerfile e o instalar;
#   - glibc incompatível ("GLIBC_... not found"): PARADA da distro (D0.4a, nada de compilar Qt da fonte).
# Um único ldd para os sete alvos (L-11: nada de um processo por item). Os alvos são fixos, não varredura.
# Saída: analisados / faltando_lib / glibc_incompativel. Sai com 1 se qualquer contador for diferente de zero.
set -u

QT_PREFIXO="${QT_PREFIXO:-/opt/qt/6.12.0/gcc_64}"
ALVOS=(
    lib/libQt6Core.so.6
    lib/libQt6Gui.so.6
    lib/libQt6Qml.so.6
    lib/libQt6Quick.so.6
    lib/libQt6Network.so.6
    lib/libQt6Test.so.6
    plugins/platforms/libqoffscreen.so
)

arquivos=()
ausentes=0
for alvo in "${ALVOS[@]}"; do
    if [ -f "$QT_PREFIXO/$alvo" ]; then
        arquivos+=("$QT_PREFIXO/$alvo")
    else
        echo "AUSENTE-NO-VOLUME $QT_PREFIXO/$alvo"
        ausentes=$((ausentes + 1))
    fi
done

analisados=${#arquivos[@]}
faltando=$ausentes
glibc=0
if [ "$analisados" -gt 0 ]; then
    saida="$(ldd "${arquivos[@]}" 2>&1)"
    nf="$(printf '%s\n' "$saida" | grep -c 'not found' || true)"
    ng="$(printf '%s\n' "$saida" | grep -c 'GLIBC_.*not found' || true)"
    faltando=$((faltando + nf))
    glibc=$ng
    if [ "$nf" -gt 0 ] || [ "$ng" -gt 0 ]; then
        printf '%s\n' "$saida" | grep -E 'not found|GLIBC_'
    fi
fi

echo "precheque_qt: analisados=$analisados faltando_lib=$faltando glibc_incompativel=$glibc"
if [ "$analisados" -ne "${#ALVOS[@]}" ] || [ "$faltando" -ne 0 ] || [ "$glibc" -ne 0 ]; then
    exit 1
fi
exit 0
