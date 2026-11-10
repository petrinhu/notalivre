#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do arnês de repetição (NL-3.0j, decisão do CTO de 10/11/2026; L-35, L-36). Duas camadas, nenhuma com contêiner:
#   (a) build_local.sh com docker de mentira (STUB; nenhum contêiner sobe): cotas e argumentos de cada docker run nos modos
#       normal, ensaio e mutante-mtls, e listas inválidas, que não podem chegar a nenhum docker run;
#   (b) o corpo ensaio_no_conteiner.sh rodado direto, com cmake e ctest de mentira que registram o argv: a fixture roda uma
#       vez, fora da repetição; cada slot roda com -FS, --no-tests=error, -j 1, -V e regex ancorada com [.]; contagens e
#       paradas. O RLIMIT_CORE é posto por python3 (prlimit não existe na imagem fedora45, medido em 10/11/2026).
# Vermelho antes do verde: os casos do modo ensaio reprovam enquanto o modo não existir.
# A prova de que cada propriedade morre quando sabotada é feita à parte, em cópias fora da árvore (L-27).
# Uso: autoteste_build_ensaio.sh     (usa a árvore onde o script está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$RAIZ/tools/conteiner/build_local.sh"
BODY="$RAIZ/tools/conteiner/ensaio_no_conteiner.sh"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-ensaio.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin" "$BASE/mock" "$BASE/tmp" "$BASE/trab"
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# ---------- (a) build_local.sh com docker de mentira ----------
cat > "$BASE/bin/docker" <<'STUB'
#!/bin/sh
echo "docker $*" >> "${STUB_LOG:-/dev/null}"
exit 0
STUB
chmod +x "$BASE/bin/docker"

# Fixture com os caminhos de LISTA_FONTE e os três arquivos que os portões leem do SHA (como em autoteste_build_sha.sh).
F="$BASE/fixture"
for d in src apps tests third_party tools/conteiner tools/tabelas tools/servidor_falso tools/portoes tools/mutacao flatpak .github/workflows; do
    mkdir -p "$F/$d"
    printf 'x\n' > "$F/$d/conteudo.txt"
done
printf 'project(x)\n' > "$F/CMakeLists.txt"
printf '{}\n' > "$F/CMakePresets.json"
printf "name: ci\n" > "$F/.github/workflows/ci.yml"
for arq in portoes_estaticos.sh piso_portoes.txt lista_portoes.txt; do
    cp "$RAIZ/tools/conteiner/$arq" "$F/tools/conteiner/$arq" || { echo "ABORTA: falta $arq na árvore"; exit 1; }
done
# Arquivos reais que o modo mutante-mtls usa dentro de /src (o pacote é tirado do SHA pelo LISTA_FONTE).
cp "$RAIZ/tools/mutacao/mutante_mtls.sh" "$F/tools/mutacao/mutante_mtls.sh" || { echo "ABORTA: falta mutante_mtls.sh"; exit 1; }
cp "$RAIZ/tools/servidor_falso/servidor.py" "$F/tools/servidor_falso/servidor.py" || { echo "ABORTA: falta servidor.py"; exit 1; }
git -C "$F" init -q
git -C "$F" add -A
git -C "$F" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: fixture ensaio" \
    || { echo "ABORTA: commit da fixture"; exit 1; }
SHA="$(git -C "$F" rev-parse HEAD)" || { echo "ABORTA: rev-parse"; exit 1; }

# Roda build_local.sh com o docker de mentira. Uso: build ARQ_LOG [VAR=valor ...] -- ARGS...
# Ambiente do arnês é sempre limpo (NL_SEM_CORE, NL_ENSAIO, NL_MODO_ARNES); só o que vem antes do -- entra.
build() {
    local log="$1" envx=() a
    shift
    while [ "$#" -gt 0 ] && [ "$1" != -- ]; do envx+=("$1"); shift; done
    shift
    : > "$log.docker"
    env -u NL_FONTE -u NL_THREADS -u NL_MEMORIA -u NL_SEM_CORE -u NL_ENSAIO -u NL_MODO_ARNES \
        NL_FATIA=autoteste-ensaio NL_REPO="$F" NL_SHA="$SHA" \
        NL_TMP_BASE="$BASE/tmp" STUB_LOG="$log.docker" NL_LOG="$log" PATH="$BASE/bin:$PATH" \
        "${envx[@]}" timeout 600 bash "$BUILD" "$@" > "$log.saida" 2>&1
}

total_runs() {
    local n
    n=$(grep -c '^docker run ' "$1" 2>/dev/null) || true
    echo "${n:-0}"
}
# Os docker run do contêiner de build são os que levam a imagem notalivre/build-<distro>. Os do portoes_estaticos.sh
# (imagem notalivre-higiene) ficam de fora: NL_SEM_CORE não os alcança, declarado no build_local.sh.
# Confere que todo docker run de build leva o padrão e que há pelo menos um (varredura vazia reprova, L-36).
todos_levam() {
    local log="$1" padrao="$2" total com
    total=$(grep '^docker run ' "$log.docker" 2>/dev/null | grep -c -F 'notalivre/build-' || true)
    com=$(grep '^docker run ' "$log.docker" 2>/dev/null | grep -F 'notalivre/build-' | grep -c -F -- "$padrao" || true)
    [ "${total:-0}" -gt 0 ] && [ "${com:-0}" = "$total" ]
}
nenhum_leva() {
    local log="$1" padrao="$2" com
    com=$(grep '^docker run ' "$log.docker" 2>/dev/null | grep -F 'notalivre/build-' | grep -c -F -- "$padrao" || true)
    [ "${com:-0}" = 0 ]
}

# O pacote que o executar extrai do SHA (LISTA_FONTE, em evidencia_sha.sh) é o que chega a /src no contêiner. Confere o
# pacote de verdade, não o log: os caminhos que o modo usa precisam estar nele. Chamar logo depois do build do modo.
pacote_tem() {
    local nome="$1" arq faltam=""
    shift
    for arq in "$@"; do
        [ -e "$BASE/tmp/notalivre-src-$SHA-autoteste-ensaio/$arq" ] || faltam="$faltam $arq"
    done
    if [ -z "$faltam" ]; then
        ok "$nome: pacote de /src tem os caminhos do modo"
    else
        falha "$nome: pacote de /src sem:$faltam"
    fi
}

# Caso normal, sem NL_SEM_CORE: nenhuma cota de arnês, e a etapa 2 de executar foi alcançada.
build "$BASE/normal.log" -- fedora45 debug
if [ "$(total_runs "$BASE/normal.log.docker")" -gt 0 ] && nenhum_leva "$BASE/normal.log" "--ulimit " \
    && nenhum_leva "$BASE/normal.log" "--pids-limit " \
    && grep -q -F 'executar_no_conteiner.sh debug' "$BASE/normal.log.docker"; then
    ok "normal sem NL_SEM_CORE: zero --ulimit e zero --pids-limit, etapa 2 alcançada"
else
    falha "normal sem NL_SEM_CORE: esperava zero cota de arnês e a etapa 2 (rc=$?)"
fi

# Caso normal com NL_SEM_CORE=1: todo docker run leva core=1 e nenhum leva pids-limit.
build "$BASE/semcore.log" NL_SEM_CORE=1 -- fedora45 debug
if todos_levam "$BASE/semcore.log" "--ulimit core=1 " && nenhum_leva "$BASE/semcore.log" "--pids-limit "; then
    ok "normal com NL_SEM_CORE=1: todo docker run leva --ulimit core=1"
else
    falha "normal com NL_SEM_CORE=1: esperava --ulimit core=1 em todo docker run e nenhum --pids-limit"
fi

# Caso ensaio: core=1 e pids-limit 512 em todo docker run, mesmo sem NL_SEM_CORE; corpo copiado e com hash no log.
build "$BASE/ensaio.log" NL_ENSAIO='transporte.exemplo:3' -- ensaio fedora45 debug
rc_ensaio=$?
if [ "$rc_ensaio" = 0 ] && todos_levam "$BASE/ensaio.log" "--ulimit core=1 " && todos_levam "$BASE/ensaio.log" "--pids-limit 512 " \
    && grep -q -F '/ensaio/ensaio_no_conteiner.sh' "$BASE/ensaio.log.docker" \
    && grep -q -E '^NL-ENSAIO-CORPO-SHA256=[0-9a-f]{64}$' "$BASE/ensaio.log"; then
    ok "ensaio sem NL_SEM_CORE: todo docker run leva --ulimit core=1 e --pids-limit 512, corpo com hash no log"
else
    falha "ensaio sem NL_SEM_CORE: esperava rc 0, core=1 e pids-limit 512 em todo docker run, corpo com hash (rc=$rc_ensaio)"
fi
pacote_tem "ensaio" CMakePresets.json CMakeLists.txt tests

# Caso ensaio com NL_SEM_CORE=0 exportado: o modo ensaio força core=1 de qualquer jeito.
build "$BASE/ensaio0.log" NL_SEM_CORE=0 NL_ENSAIO='transporte.exemplo:3' -- ensaio fedora45 debug
if todos_levam "$BASE/ensaio0.log" "--ulimit core=1 " && todos_levam "$BASE/ensaio0.log" "--pids-limit 512 "; then
    ok "ensaio com NL_SEM_CORE=0: core=1 forçado"
else
    falha "ensaio com NL_SEM_CORE=0: esperava core=1 forçado em todo docker run"
fi

# Caso mutante-mtls: core=1 em todo docker run, sem pids-limit, e o comando de mutante na etapa 2.
build "$BASE/mutante.log" NL_SHA="$SHA" -- mutante-mtls fedora45
if todos_levam "$BASE/mutante.log" "--ulimit core=1 " && nenhum_leva "$BASE/mutante.log" "--pids-limit " \
    && grep -q -F 'mutante_mtls.sh /gitrepo' "$BASE/mutante.log.docker" \
    && grep -q -F 'extrair_do_sha.sh /gitrepo' "$BASE/mutante.log.docker" \
    && grep -q -F ':/extrator:ro,z' "$BASE/mutante.log.docker"; then
    ok "mutante-mtls: core=1 em todo docker run, sem pids-limit, comando de mutante na etapa 2"
else
    falha "mutante-mtls: esperava core=1, nenhum pids-limit e mutante_mtls.sh /gitrepo na etapa 2"
fi
# O modo mutante-mtls tira o script do SHA dentro do contêiner, com o helper extrair_do_sha.sh sobre /gitrepo (o pacote
# de /src não traz tools/mutacao, LISTA_FONTE fica como está: mudar a lista muda o manifesto de todo SHA, e o log de build
# antigo deixa de verificar). Aqui o helper roda sobre o .git da fixture, do mesmo tipo que o /gitrepo do contêiner.
ext="$BASE/extraido"
if bash "$RAIZ/tools/conteiner/extrair_do_sha.sh" "$F/.git" "$SHA" "$ext" tools/mutacao \
    && [ "$(sha256sum < "$ext/tools/mutacao/mutante_mtls.sh")" = "$(sha256sum < "$F/tools/mutacao/mutante_mtls.sh")" ]; then
    ok "mutante-mtls: extração do SHA traz tools/mutacao/mutante_mtls.sh igual ao do commit"
else
    falha "mutante-mtls: extração do SHA sem tools/mutacao/mutante_mtls.sh igual ao do commit"
fi

# Listas e presets inválidos: erro antes de qualquer docker run.
caso_invalido() {
    local nome="$1" lista="$2" preset="$3" log="$BASE/invalido-$1.log" rc
    build "$log" NL_ENSAIO="$lista" -- ensaio fedora45 "$preset"
    rc=$?
    if [ "$rc" != 0 ] && [ "$(total_runs "$log.docker")" = 0 ] && grep -q -F 'PARADA' "$log"; then
        ok "rejeita $nome antes de qualquer docker run (rc=$rc)"
    else
        falha "$nome: esperava rc != 0 e zero docker run (rc=$rc, docker run=$(total_runs "$log.docker"))"
    fi
}
caso_invalido "lista vazia" "" debug
caso_invalido "nome sem dois-pontos" "transporte.exemplo" debug
caso_invalido "nome com caractere fora de [A-Za-z0-9_.]" "transporte-x:3" debug
caso_invalido "N zero" "transporte.exemplo:0" debug
caso_invalido "N acima de 9999" "transporte.exemplo:10000" debug
caso_invalido "N com zero à esquerda" "transporte.exemplo:08" debug
caso_invalido "preset fora da lista" "transporte.exemplo:3" pwn

# ---------- (b) corpo ensaio_no_conteiner.sh, direto, com cmake e ctest de mentira ----------
cat > "$BASE/mock/cmake" <<'MOCK'
#!/bin/bash
{ printf 'cmake|'; printf '%s|' "$@"; printf '\n'; } >> "$MOCK_LOG"
exit 0
MOCK
cat > "$BASE/mock/ctest" <<'MOCK'
#!/bin/bash
{ printf 'ctest|'; printf '%s|' "$@"; printf '\n'; } >> "$MOCK_LOG"
MOCK_TESTES="${MOCK_TESTES:-certificados.gerar transporte.exemplo transporte.a transporte.b}"
re=''; n=0; prev=''; noerr=0
for a in "$@"; do
    [ "$prev" = -R ] && re="$a"
    case "$a" in until-fail:*) n="${a#until-fail:}" ;; esac
    [ "$a" = --no-tests=error ] && noerr=1
    prev="$a"
done
nome=''
for t in $MOCK_TESTES; do
    if [[ "$t" =~ $re ]]; then nome="$t"; break; fi
done
if [ -z "$nome" ]; then
    echo "No tests were found!!!"
    [ "$noerr" = 1 ] && exit 1
    exit 0
fi
if [ "$n" = 0 ]; then
    echo "1/1 Test #1: $nome ....   Passed"
    exit 0
fi
for k in $(seq 1 "$n"); do
    [ "${MOCK_PULA_START:-0}" = 1 ] || echo "Start testing of tst_Mock"
    if [ "$nome" = "${MOCK_FALHA_TESTE:-}" ] && [ "$k" = "${MOCK_FALHA_EM:-0}" ]; then
        echo "FAIL!  : mock falhou na repetição $k"
        exit 1
    fi
    echo "medicao tipo=1 repeticao=$k"
    echo "Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 1ms"
done
exit 0
MOCK
chmod +x "$BASE/mock/cmake" "$BASE/mock/ctest"

# Roda o corpo com RLIMIT_CORE = CORE (soft e hard). Uso: rodar_corpo CORPO CORE LOG SAIDA LISTA [VAR=valor ...]
rodar_corpo() {
    local corpo="$1" core="$2" log="$3" saida="$4" lista="$5"
    shift 5
    : > "$log"
    (cd "$BASE/trab" && env PATH="$BASE/mock:$PATH" MOCK_LOG="$log" NL_ENSAIO="$lista" TMPDIR="$BASE/tmp" "$@" \
        python3 -I -c 'import os, resource, sys
m = int(sys.argv[1])
resource.setrlimit(resource.RLIMIT_CORE, (m, m))
os.execvp(sys.argv[2], sys.argv[2:])' "$core" bash "$corpo" debug) > "$saida" 2>&1
}

# Confere o argv registrado pelo ctest de mentira contra as regras do arnês. Retorna 1 se alguma regra falha.
checar_chamadas() {
    local log="$1" total sem_noerr fixtures fixture_repete fora_fs sem_j sem_v regex_ruim rep_total ln_fix ln_rep bad=0
    total=$(grep -c '^ctest|' "$log" 2>/dev/null || true)
    if [ "${total:-0}" -eq 0 ]; then
        echo "  checagem: zero chamadas de ctest (varredura vazia)"
        return 1
    fi
    sem_noerr=$(grep '^ctest|' "$log" | grep -c -v -F '|--no-tests=error|' || true)
    fixtures=$(grep -c -F '|-R|^certificados[.]gerar$|' "$log" || true)
    fixture_repete=$(grep -F '|-R|^certificados[.]gerar$|' "$log" | grep -c -F '|--repeat|' || true)
    rep_total=$(grep -c -F '|--repeat|' "$log" || true)
    fora_fs=$(grep -F '|--repeat|' "$log" | grep -c -v -F '|-FS|^certificados_teste$|' || true)
    sem_j=$(grep -F '|--repeat|' "$log" | grep -c -v -F '|-j|1|' || true)
    sem_v=$(grep -F '|--repeat|' "$log" | grep -c -v -F '|-V|' || true)
    regex_ruim=$(grep -F '|--repeat|' "$log" | grep -c -v -E '[|]-R[|]\^[A-Za-z0-9_]+([[][.][]][A-Za-z0-9_]+)*[$][|]' || true)
    ln_fix=$(grep -n -F '|-R|^certificados[.]gerar$|' "$log" | head -1 | cut -d: -f1)
    ln_rep=$(grep -n -F '|--repeat|' "$log" | head -1 | cut -d: -f1)
    [ "${sem_noerr:-0}" = 0 ] || { echo "  checagem: $sem_noerr chamada(s) de ctest sem --no-tests=error"; bad=1; }
    [ "${fixtures:-0}" = 1 ] || { echo "  checagem: fixture certificados.gerar chamada $fixtures vez(es), esperado 1"; bad=1; }
    [ "${fixture_repete:-0}" = 0 ] || { echo "  checagem: fixture certificados.gerar dentro de --repeat"; bad=1; }
    [ "${rep_total:-0}" -ge 1 ] || { echo "  checagem: nenhuma repetição de slot"; bad=1; }
    [ "${fora_fs:-0}" = 0 ] || { echo "  checagem: $fora_fs repetição(ões) sem -FS da fixture"; bad=1; }
    [ "${sem_j:-0}" = 0 ] || { echo "  checagem: $sem_j repetição(ões) sem -j 1"; bad=1; }
    [ "${sem_v:-0}" = 0 ] || { echo "  checagem: $sem_v repetição(ões) sem -V"; bad=1; }
    [ "${regex_ruim:-0}" = 0 ] || { echo "  checagem: $regex_ruim regex de slot sem ancora ou sem [.]"; bad=1; }
    if [ -n "$ln_rep" ] && { [ -z "$ln_fix" ] || [ "$ln_fix" -gt "$ln_rep" ]; }; then
        echo "  checagem: fixture depois da primeira repetição"
        bad=1
    fi
    return "$bad"
}

# B1: verde. Fixture uma vez, slot com N=3, três processos, três medições, resumo sai.
rodar_corpo "$BODY" 1 "$BASE/b1.log" "$BASE/b1.saida" 'transporte.exemplo:3'
rc=$?
if [ "$rc" = 0 ] && checar_chamadas "$BASE/b1.log" \
    && grep -q -F 'ensaio teste=transporte.exemplo pedidas=3 processos=3 aprovadas=3 medicao=3 rc=0' "$BASE/b1.saida" \
    && grep -q -F 'ensaio resumo slots=1 reprovados=0' "$BASE/b1.saida" && grep -q -F 'ENSAIO: VERDE' "$BASE/b1.saida"; then
    ok "corpo verde: fixture uma vez, -FS, --no-tests=error, -j 1, -V, regex ancorada, 3 de 3"
else
    falha "corpo verde: esperava rc 0 e checagens (rc=$rc)"
    sed 's/^/    /' "$BASE/b1.saida" | tail -n 12
fi

# B2: core diferente de 1/1 aborta antes de qualquer ctest e de qualquer cmake.
rodar_corpo "$BODY" 0 "$BASE/b2.log" "$BASE/b2.saida" 'transporte.exemplo:3'
rc=$?
if [ "$rc" = 2 ] && grep -q -F 'ENSAIO PARADA' "$BASE/b2.saida" && [ "$(grep -c -E '^(ctest|cmake)\|' "$BASE/b2.log")" = 0 ]; then
    ok "core 0/0: aborta com rc 2 antes de cmake e ctest"
else
    falha "core 0/0: esperava rc 2, PARADA e zero chamadas (rc=$rc)"
fi

# B3: nome que não casa com teste nenhum reprova: ctest com --no-tests=error sai com erro; o resumo sai.
rodar_corpo "$BODY" 1 "$BASE/b3.log" "$BASE/b3.saida" 'transporte.inexistente:2'
rc=$?
if [ "$rc" != 0 ] && grep -q -F 'ensaio teste=transporte.inexistente pedidas=2 processos=0 aprovadas=0 medicao=0 rc=1' "$BASE/b3.saida" \
    && grep -q -F 'ensaio resumo slots=1 reprovados=1' "$BASE/b3.saida"; then
    ok "slot sem teste correspondente: reprova, rc $rc, resumo sai"
else
    falha "slot sem teste correspondente: esperava rc != 0 e resumo com reprovados=1 (rc=$rc)"
fi

# B4: falha na primeira repetição do slot A para a lista; o slot B não roda.
rodar_corpo "$BODY" 1 "$BASE/b4.log" "$BASE/b4.saida" 'transporte.a:2 transporte.b:2' MOCK_FALHA_TESTE=transporte.a MOCK_FALHA_EM=1
rc=$?
if [ "$rc" != 0 ] && [ "$(grep -c -F '|-R|^transporte[.]b$|' "$BASE/b4.log" || true)" = 0 ] \
    && grep -q -F 'ensaio teste=transporte.a pedidas=2' "$BASE/b4.saida" && grep -q -F 'ensaio resumo slots=1 reprovados=1' "$BASE/b4.saida"; then
    ok "falha num slot para a lista: o slot seguinte não roda"
else
    falha "falha num slot: esperava rc != 0 e zero chamadas do slot B (rc=$rc)"
fi

# B5: contagem de processos que não bate com N reprova.
rodar_corpo "$BODY" 1 "$BASE/b5.log" "$BASE/b5.saida" 'transporte.exemplo:3' MOCK_PULA_START=1
rc=$?
if [ "$rc" != 0 ] && grep -q -F 'processos=0 aprovadas=3' "$BASE/b5.saida"; then
    ok "contagem de processos diferente de N: reprova"
else
    falha "contagem de processos diferente de N: esperava rc != 0 (rc=$rc)"
fi

# S1 e S2: sabotagens do corpo, em cópias. O checador tem de reprovar cada uma (a fixture dentro da repetição; o ctest
# sem --no-tests=error). Sem a aplicação da sabotagem, o caso não prova nada e reprova.
sabotar() {
    local saida="$1" trocar="$2" por="$3"
    python3 -I -c 'import sys
origem, destino, velho, novo = sys.argv[1:5]
texto = open(origem, encoding="utf-8").read()
if velho not in texto:
    sys.exit(3)
open(destino, "w", encoding="utf-8").write(texto.replace(velho, novo))' "$BODY" "$saida" "$trocar" "$por"
}
if sabotar "$BASE/corpo-sem-fs.sh" "-FS '^certificados_teste\$' " "" ; then
    rodar_corpo "$BASE/corpo-sem-fs.sh" 1 "$BASE/s1.log" "$BASE/s1.saida" 'transporte.exemplo:2'
    if checar_chamadas "$BASE/s1.log"; then
        falha "sabotagem sem -FS: o checador NÃO reprovou (fixture entrou na repetição)"
    else
        ok "sabotagem sem -FS: o checador reprova"
    fi
else
    falha "sabotagem sem -FS: trecho não encontrado no corpo, sabotagem não aplicada"
fi
if sabotar "$BASE/corpo-sem-noerr.sh" " --no-tests=error" ""; then
    rodar_corpo "$BASE/corpo-sem-noerr.sh" 1 "$BASE/s2.log" "$BASE/s2.saida" 'transporte.exemplo:2'
    if checar_chamadas "$BASE/s2.log"; then
        falha "sabotagem sem --no-tests=error: o checador NÃO reprovou"
    else
        ok "sabotagem sem --no-tests=error: o checador reprova"
    fi
else
    falha "sabotagem sem --no-tests=error: trecho não encontrado no corpo, sabotagem não aplicada"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE BUILD ENSAIO: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE BUILD ENSAIO: REPROVADO"
exit 1
