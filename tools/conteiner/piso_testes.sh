#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Piso de testes do job nativo (rodada 5, CI da onda-2 run 40804430962). Com NL_FLATPAK_BUILD=ON o ctest passava com 95
# testes e 14 sumiam em silêncio. Esta trava reprova se: a contagem de testes ficar abaixo do piso; o cache não declarar
# NL_FLATPAK_BUILD OFF (build nativo); ou a contagem não for lida. A opção NL_FLATPAK_BUILD só é válida no Flatpak.
#
# Uso:
#   piso_testes.sh verificar <dir_build> <piso>   lê a contagem com ctest --test-dir -N e o CMakeCache.txt do build
#   piso_testes.sh avaliar <total> <piso> <cache>  (usado pelo autoteste_piso_testes.sh)
# Saída: uma linha VERDE ou REPROVA. Sai com 1 em qualquer reprova.

avaliar() {
    local total="$1" piso="$2" cache="$3"
    if ! [[ "$total" =~ ^[0-9]+$ ]]; then
        echo "REPROVA: contagem de testes ilegível ('$total')"
        return 1
    fi
    if ! [[ "$piso" =~ ^[0-9]+$ ]]; then
        echo "REPROVA: piso ilegível ('$piso')"
        return 1
    fi
    if [ ! -f "$cache" ]; then
        echo "REPROVA: CMakeCache.txt ausente ($cache)"
        return 1
    fi
    if grep -q '^NL_FLATPAK_BUILD:BOOL=ON' "$cache"; then
        echo "REPROVA: NL_FLATPAK_BUILD ON fora do Flatpak: testes de ferramenta desligados neste build nativo"
        return 1
    fi
    if ! grep -q '^NL_FLATPAK_BUILD:BOOL=OFF' "$cache"; then
        echo "REPROVA: NL_FLATPAK_BUILD não declarado OFF no cache ($cache)"
        return 1
    fi
    if [ "$total" -lt "$piso" ]; then
        echo "REPROVA: $total testes, abaixo do piso $piso (testes sumiram em silêncio)"
        return 1
    fi
    echo "VERDE: $total testes, piso $piso, NL_FLATPAK_BUILD OFF"
    return 0
}

verificar() {
    local build="$1" piso="$2" total
    total="$(ctest --test-dir "$build" -N 2>/dev/null | sed -n 's/^Total Tests: *\([0-9][0-9]*\).*/\1/p')"
    avaliar "$total" "$piso" "$build/CMakeCache.txt"
}

# Só roda quando executado, não quando é carregado pelo autoteste.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    case "${1:-}" in
        verificar)
            verificar "${2:?uso: piso_testes.sh verificar <dir_build> <piso>}" "${3:?piso ausente}"
            ;;
        avaliar)
            avaliar "${2:?total}" "${3:?piso}" "${4:?cache}"
            ;;
        *)
            echo "uso: piso_testes.sh verificar <dir_build> <piso> | avaliar <total> <piso> <cache>" >&2
            exit 2
            ;;
    esac
fi
