#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Portão de registro Qt Test (NL-2.14, revisão devops ALTA-1 e MÉDIA-2; L-36). Confere por igualdade de conjuntos que os testes
# do CTest que rodam um executável Qt Test são exatamente os do manifesto junit/esperados.txt, o que nl_registra_teste_qt
# registrou. Um Qt Test que entra por add_test direto aparece aqui como "fora do registro único".
# Fontes: CTest = <build>/**/CTestTestfile.cmake, gerado pelo CMake a partir de add_test (o ctest lê esse arquivo);
# executáveis Qt = junit/executaveis_qt.txt, gravado pelo CMake para todo alvo que linka Qt6::Test.
# Também confere os testes obrigatórios da distro (linhas "distro teste motivo" de tests/testes_obrigatorios_por_distro.txt).
# Sem laço por teste: cada conferência é um grep -f sobre listas, numa passagem.
#
# Uso: registro_qt.sh <dir_build> <distro> <obrigatorios>
set -u

BUILD="${1:?uso: registro_qt.sh <dir_build> <distro> <obrigatorios>}"
DISTRO="${2:-}"
OBRIG="${3:?uso: registro_qt.sh <dir_build> <distro> <obrigatorios>}"
J="$BUILD/junit"
ruins=0
parada() {
    echo "PARADA: $*"
    ruins=$((ruins + 1))
}

if [ -z "$DISTRO" ]; then
    echo "PARADA: distro ausente no registro Qt Test"
    exit 1
fi
if [ ! -r "$J/esperados.txt" ]; then
    echo "PARADA: manifesto ausente ($J/esperados.txt): o configure grava o manifesto"
    exit 1
fi
if [ ! -r "$J/executaveis_qt.txt" ]; then
    echo "PARADA: lista de executáveis Qt ausente ($J/executaveis_qt.txt): o configure grava a lista"
    exit 1
fi
if [ ! -r "$OBRIG" ]; then
    echo "PARADA: lista de testes obrigatórios ilegível: $OBRIG"
    exit 1
fi

# Pares "NOME BASE" do CTest: NOME é o nome do teste, BASE o nome do executável (última pasta do caminho).
CT_PARES="$J/.ctest_pares.txt"
# Dois formatos, porque a versão do CMake muda a escrita: add_test([=[NOME]=] "EXE" ...) (fedora44, ubuntu26.04, mint22.3) e
# add_test("NOME" "EXE" ...) (manjaro, rodada de 31/10 06:57). Qualquer outra linha add_test fica sem leitura e reprova.
find "$BUILD" -name CTestTestfile.cmake -exec grep -h '^add_test(' {} + > "$J/.ctest_linhas.txt"
n_linhas=$(wc -l < "$J/.ctest_linhas.txt" | tr -d ' ')
sed -n -e 's/^add_test(\[=\[\(.*\)\]=\] "\([^"]*\)".*/\1 \2/p' \
       -e 's/^add_test("\([^"]*\)" "\([^"]*\)".*/\1 \2/p' "$J/.ctest_linhas.txt" \
    | sed 's|^\([^ ]*\) \(.*\)/\([^/ ]*\)$|\1 \3|' > "$CT_PARES"
cut -d' ' -f1 "$CT_PARES" > "$J/.ctest_nomes.txt"
n_ctest=$(wc -l < "$CT_PARES" | tr -d ' ')
n_manifesto=$(wc -l < "$J/esperados.txt" | tr -d ' ')
n_executaveis=$(wc -l < "$J/executaveis_qt.txt" | tr -d ' ')
if [ "$n_linhas" -ne "$n_ctest" ]; then
    parada "formato do CTestTestfile.cmake não reconhecido: $n_linhas linhas add_test, $n_ctest lidas (o portão lê [=[NOME]=] e \"NOME\")"
fi
if [ "$n_ctest" -eq 0 ]; then
    parada "CTest sem testes registrados em $BUILD (zero testes não é varredura)"
fi
if [ "$n_executaveis" -eq 0 ]; then
    parada "nenhum executável Qt Test na lista (zero executáveis não é varredura)"
fi

# Testes do CTest cujo executável está na lista de Qt Test (um só grep, padrão montado a partir da lista).
PADRAO=$(paste -sd'|' "$J/executaveis_qt.txt")
QT_NOMES="$J/.ctest_qt.txt"
if [ -n "$PADRAO" ]; then
    grep -E "^[^ ]* ($PADRAO)\$" "$CT_PARES" | cut -d' ' -f1 > "$QT_NOMES"
else
    : > "$QT_NOMES"
fi
n_qt=$(wc -l < "$QT_NOMES" | tr -d ' ')

# Conferência nas duas direções, por igualdade de conjuntos.
grep -vxF -f "$J/esperados.txt" "$QT_NOMES" > "$J/.fora_do_registro.txt"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    parada "teste Qt Test fora do registro único (nl_registra_teste_qt): $nome"
done < "$J/.fora_do_registro.txt"
grep -vxF -f "$J/.ctest_nomes.txt" "$J/esperados.txt" > "$J/.registrado_fora_ctest.txt"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    parada "registrado no manifesto e fora do CTest: $nome"
done < "$J/.registrado_fora_ctest.txt"

# Testes obrigatórios da distro: presentes no CTest. A prova de rede (xmldsig.contextoDoParserBloqueiaRede) entra aqui.
grep -E "^$DISTRO [^ ]+" "$OBRIG" | cut -d' ' -f2 > "$J/.obrig_nomes.txt"
n_obrig=$(wc -l < "$J/.obrig_nomes.txt" | tr -d ' ')
grep -vxF -f "$J/.ctest_nomes.txt" "$J/.obrig_nomes.txt" > "$J/.obrig_ausentes.txt"
while IFS= read -r nome; do
    [ -n "$nome" ] || continue
    parada "teste obrigatório ausente no CTest (distro $DISTRO): $nome"
done < "$J/.obrig_ausentes.txt"

echo "registro_qt: distro=$DISTRO ctest=$n_ctest qt_ctest=$n_qt manifesto=$n_manifesto executaveis_qt=$n_executaveis obrigatorios=$n_obrig"
if [ "$ruins" -ne 0 ]; then
    echo "registro_qt: REPROVADO ($ruins motivo(s))"
    exit 1
fi
echo "registro_qt: VERDE, todo teste Qt Test do CTest está no manifesto"
exit 0
