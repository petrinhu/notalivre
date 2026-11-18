# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Resultado junitxml do teste do banco (NL-3.1h): limpar antes de cada par e contar depois. Carregado por ponto.
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "PARADA: arquivo de banco/ é carregado por ponto, não executável"
    exit 1
fi

# nl_banco_limpar_resultado DIR_JUNIT: apaga só os integracao.banco.*.xml, antes de cada par (um par não herda o verde do anterior).
nl_banco_limpar_resultado() {
    case "$1" in
        */junit) ;;
        *) echo "PARADA: pasta de junit inesperada: $1"; return 1 ;;
    esac
    ls -d -- "$1" >/dev/null || return 1
    rm -f -- "$1"/integracao.banco.*.xml
}

# nl_banco_contar_resultado DIR_JUNIT: imprime "analisados pulados falharam", uma chamada de grep por métrica (L-11).
nl_banco_contar_resultado() {
    local analisados pulados falharam
    analisados=$(/usr/bin/grep -lF '</testsuite>' "$1"/integracao.banco.*.xml 2>/dev/null | wc -l)
    pulados=$(/usr/bin/grep -hE '<skipped| result="skip"' "$1"/integracao.banco.*.xml 2>/dev/null | wc -l)
    falharam=$(/usr/bin/grep -hF '<failure' "$1"/integracao.banco.*.xml 2>/dev/null | wc -l)
    echo "$analisados $pulados $falharam"
}
