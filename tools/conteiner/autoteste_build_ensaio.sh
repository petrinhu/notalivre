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
# Arquivo real que o modo mutante-mtls usa dentro de /src (o pacote é tirado do SHA pelo LISTA_FONTE).
cp "$RAIZ/tools/servidor_falso/servidor.py" "$F/tools/servidor_falso/servidor.py" || { echo "ABORTA: falta servidor.py"; exit 1; }
# Dois commits: SHA_SEM não tem tools/mutacao/mutante_mtls.sh; SHA tem, com uma linha a mais no fim ("# fixture"), para o
# arquivo do commit ser DIFERENTE do da árvore (sem isso, trocar o hash do SHA pelo da árvore passaria despercebido).
git -C "$F" init -q
git -C "$F" add -A
git -C "$F" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: fixture ensaio sem mutante" \
    || { echo "ABORTA: commit da fixture sem mutante"; exit 1; }
SHA_SEM="$(git -C "$F" rev-parse HEAD)" || { echo "ABORTA: rev-parse"; exit 1; }
cp "$RAIZ/tools/mutacao/mutante_mtls.sh" "$F/tools/mutacao/mutante_mtls.sh" || { echo "ABORTA: falta mutante_mtls.sh"; exit 1; }
printf '# fixture\n' >> "$F/tools/mutacao/mutante_mtls.sh"
git -C "$F" add -A
git -C "$F" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: fixture ensaio com mutante" \
    || { echo "ABORTA: commit da fixture com mutante"; exit 1; }
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
# Destinos das montagens (-v origem:destino[:opções]) do primeiro docker run que contém MARCA, ordenados, separados por espaço.
# Lista fechada (L-17): uma montagem a mais (a árvore de quem chama, por exemplo) muda a lista e reprova o caso.
montagens_de() {
    grep '^docker run ' "$1.docker" | grep -F -- "$2" | head -n 1 | grep -o -E -- '-v [^ ]+' | cut -d: -f2 | LC_ALL=C sort | tr '\n' ' '
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

# A-I3 (I3 e C6 da revisão): o log grava o comando pedido, o SHA vai ao contêiner, o hash do corpo é o do arquivo da árvore, e
# as montagens da etapa 2 são exatamente as da lista fechada. A costura de pids do autoteste nunca chega ao docker run.
corpo_hash="$(sha256sum < "$BODY" | cut -d' ' -f1)"
if grep -q -x -F 'NL-ENSAIO-PEDIDO=transporte.exemplo:3' "$BASE/ensaio.log" \
    && grep '^docker run ' "$BASE/ensaio.log.docker" | grep -F '/ensaio/ensaio_no_conteiner.sh' | grep -q -F -- "-e NL_ENSAIO_SHA=$SHA" \
    && grep -q -x -F "NL-ENSAIO-CORPO-SHA256=$corpo_hash" "$BASE/ensaio.log" \
    && [ "$(montagens_de "$BASE/ensaio.log" '/ensaio/ensaio_no_conteiner.sh')" = '/ensaio /gitrepo /opt/qt /src /var/tmp/notalivre-build /xsd-cache ' ] \
    && [ "$(grep -c -F 'NL_ENSAIO_ARQ_PIDS_MAX' "$BASE/ensaio.log.docker" || true)" = 0 ]; then
    ok "A-I3 ensaio: pedido no log, SHA no contêiner, hash do corpo igual ao do arquivo, montagens da lista fechada, sem costura de pids"
else
    falha "A-I3 ensaio: esperava pedido, SHA, hash do corpo, montagens fechadas e nenhuma costura de pids"
    montagens_de "$BASE/ensaio.log" '/ensaio/ensaio_no_conteiner.sh' | sed 's/^/    montagens: /'
fi

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
    && grep -q -F 'mutante_no_conteiner.sh /gitrepo' "$BASE/mutante.log.docker" \
    && grep -q -F ':/extrator:ro,z' "$BASE/mutante.log.docker"; then
    ok "mutante-mtls: core=1 em todo docker run, sem pids-limit, corpo do mutante na etapa 2"
else
    falha "mutante-mtls: esperava core=1, nenhum pids-limit e mutante_no_conteiner.sh /gitrepo na etapa 2"
fi
# A-I1a (I1 da revisão): o hash do blob do SHA vai ao log e ao contêiner (igual ao do blob da fixture, que difere do da
# árvore), o hash do corpo do mutante é o do arquivo da árvore, e as montagens da etapa 2 são exatamente a lista fechada.
hash_blob="$(git -C "$F" show "$SHA:tools/mutacao/mutante_mtls.sh" | sha256sum | cut -d' ' -f1)"
hash_corpo_mut="$(sha256sum < "$RAIZ/tools/conteiner/mutante_no_conteiner.sh" | cut -d' ' -f1)"
if grep '^docker run ' "$BASE/mutante.log.docker" | grep -F 'mutante_no_conteiner.sh /gitrepo' | grep -q -F -- "-e NL_MUTANTE_SCRIPT_SHA256=$hash_blob" \
    && grep -q -x -F "NL-MUTANTE-SCRIPT-SHA256-DO-SHA=$hash_blob" "$BASE/mutante.log" \
    && grep -q -x -F "NL-MUTANTE-CORPO-SHA256=$hash_corpo_mut" "$BASE/mutante.log" \
    && [ "$(montagens_de "$BASE/mutante.log" '/extrator/mutante_no_conteiner.sh')" = '/extrator /gitrepo /opt/qt /src /var/tmp/notalivre-build /xsd-cache ' ] \
    && [ "$hash_blob" != "$(sha256sum < "$RAIZ/tools/mutacao/mutante_mtls.sh" | cut -d' ' -f1)" ]; then
    ok "A-I1a mutante-mtls: hash do blob do SHA no log e no contêiner, hash do corpo da árvore, montagens da lista fechada"
else
    falha "A-I1a mutante-mtls: esperava hash do blob do SHA no log e em NL_MUTANTE_SCRIPT_SHA256, hash do corpo e montagens fechadas"
    montagens_de "$BASE/mutante.log" '/extrator/mutante_no_conteiner.sh' | sed 's/^/    montagens: /'
fi
# A-I1b: mutante_mtls.sh ausente no SHA pedido para com PARADA antes de qualquer docker run.
build "$BASE/i1b.log" NL_SHA="$SHA_SEM" -- mutante-mtls fedora45
rc=$?
if [ "$rc" != 0 ] && [ "$(total_runs "$BASE/i1b.log.docker")" = 0 ] && grep -q -F 'PARADA' "$BASE/i1b.log"; then
    ok "A-I1b mutante-mtls com SHA sem tools/mutacao/mutante_mtls.sh: PARADA antes de qualquer docker run (rc=$rc)"
else
    falha "A-I1b mutante-mtls com SHA sem o script: esperava rc != 0, PARADA e zero docker run (rc=$rc, docker run=$(total_runs "$BASE/i1b.log.docker"))"
fi
# A-C3a e A-C3b (C3 da revisão): o modo do arnês vem só do despacho. NL_MODO_ARNES exportado no ambiente não transforma
# uma rodada normal em mutante nem em ensaio: etapa 2 normal, zero cota de arnês, nenhum corpo de arnês montado.
build "$BASE/c3a.log" NL_MODO_ARNES=mutante -- fedora45 debug
if [ "$(total_runs "$BASE/c3a.log.docker")" -gt 0 ] && nenhum_leva "$BASE/c3a.log" "--ulimit " \
    && grep -q -F 'executar_no_conteiner.sh debug' "$BASE/c3a.log.docker" \
    && [ "$(grep -c -F -e '/extrator' -e 'mutante_no_conteiner.sh' "$BASE/c3a.log.docker" || true)" = 0 ]; then
    ok "A-C3a NL_MODO_ARNES=mutante no ambiente: rodada normal, zero --ulimit, nenhum corpo de mutante"
else
    falha "A-C3a NL_MODO_ARNES=mutante no ambiente: esperava a etapa 2 normal, zero --ulimit e nenhum corpo de mutante"
fi
build "$BASE/c3b.log" NL_MODO_ARNES=ensaio NL_ENSAIO='transporte.exemplo:3' -- executar fedora45 debug
if [ "$(total_runs "$BASE/c3b.log.docker")" -gt 0 ] && nenhum_leva "$BASE/c3b.log" "--ulimit " && nenhum_leva "$BASE/c3b.log" "--pids-limit " \
    && grep -q -F 'executar_no_conteiner.sh debug' "$BASE/c3b.log.docker" \
    && [ "$(grep -c -F '/ensaio/ensaio_no_conteiner.sh' "$BASE/c3b.log.docker" || true)" = 0 ]; then
    ok "A-C3b NL_MODO_ARNES=ensaio no ambiente com executar: rodada normal, zero cota de arnês, nenhum corpo de ensaio"
else
    falha "A-C3b NL_MODO_ARNES=ensaio no ambiente com executar: esperava a etapa 2 normal e nenhuma cota de arnês"
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
# A-C4 (C4 da revisão): a segunda extração no mesmo destino para com rc 2, sem misturar resto de rodada anterior.
saida_c4="$(bash "$RAIZ/tools/conteiner/extrair_do_sha.sh" "$F/.git" "$SHA" "$ext" tools/mutacao 2>&1)"
rc=$?
if [ "$rc" = 2 ] && printf '%s' "$saida_c4" | grep -q -F 'já existe'; then
    ok "A-C4 extração em destino que já existe: rc 2 e 'já existe'"
else
    falha "A-C4 extração em destino que já existe: esperava rc 2 e 'já existe' (rc=$rc)"
fi

# Listas e presets inválidos: erro antes de qualquer docker run.
caso_invalido() {
    local nome="$1" lista="$2" preset="$3" id="${4:+$4 }" log="$BASE/invalido-$1.log" rc
    build "$log" NL_ENSAIO="$lista" -- ensaio fedora45 "$preset"
    rc=$?
    if [ "$rc" != 0 ] && [ "$(total_runs "$log.docker")" = 0 ] && grep -q -F 'PARADA' "$log"; then
        ok "${id}rejeita $nome antes de qualquer docker run (rc=$rc)"
    else
        falha "${id}$nome: esperava rc != 0 e zero docker run (rc=$rc, docker run=$(total_runs "$log.docker"))"
    fi
}
caso_invalido "lista vazia" "" debug
caso_invalido "nome sem dois-pontos" "transporte.exemplo" debug
caso_invalido "nome com caractere fora de [A-Za-z0-9_.]" "transporte-x:3" debug
caso_invalido "N zero" "transporte.exemplo:0" debug
caso_invalido "N acima de 9999" "transporte.exemplo:10000" debug
caso_invalido "N com zero à esquerda" "transporte.exemplo:08" debug
caso_invalido "preset fora da lista" "transporte.exemplo:3" pwn
# A-C5a a A-C5e (C5 da revisão): N não numérico e os caracteres de shell e de regex $ ; * [ no nome. Caracterização: o código
# já rejeita; a prova de que os casos distinguem é a sabotagem S11 (aceitar N zero) e o portão do nome, no passo 19.
caso_invalido "N não numérico" 'transporte.exemplo:3a' debug A-C5a
caso_invalido "nome com cifrão" 'transporte.$x:3' debug A-C5b
caso_invalido "nome com ponto e vírgula" 'transporte;x:3' debug A-C5c
caso_invalido "nome com asterisco" 'transporte*:3' debug A-C5d
caso_invalido "nome com colchete" 'transporte[a]:3' debug A-C5e

# ---------- (b) corpo ensaio_no_conteiner.sh, direto, com cmake e ctest de mentira ----------
cat > "$BASE/mock/cmake" <<'MOCK'
#!/bin/bash
{ printf 'cmake|'; printf '%s|' "$@"; printf '\n'; } >> "$MOCK_LOG"
exit 0
MOCK
cat > "$BASE/mock/ctest" <<'MOCK'
#!/bin/bash
{ printf 'ctest|'; printf '%s|' "$@"; printf '\n'; } >> "$MOCK_LOG"
MOCK_TESTES="${MOCK_TESTES:-certificados.gerar transporte.exemplo transporte.a transporte.b transporte.semCertificadoDeClienteHandshakeRecusado}"
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
    [ "$noerr" = 1 ] && exit 8
    exit 0
fi
if [ "$n" = 0 ]; then
    echo "1/1 Test #1: $nome ....   Passed"
    exit 0
fi
if [ -n "${MOCK_SAIDA_ARQUIVO:-}" ]; then
    cat "$MOCK_SAIDA_ARQUIVO"
    exit "${MOCK_RC_FINAL:-0}"
fi
for k in $(seq 1 "$n"); do
    [ "${MOCK_PULA_START:-0}" = 1 ] || echo "1: ********* Start testing of tst_Mock *********"
    if [ "$nome" = "${MOCK_FALHA_TESTE:-}" ] && [ "$k" = "${MOCK_FALHA_EM:-0}" ]; then
        echo "1: FAIL!  : mock falhou na repetição $k"
        exit 8
    fi
    echo "1: QINFO  : mock() medicao tipo=1;repeticao=$k"
    [ "${MOCK_PULA_TOTALS:-0}" = 1 ] || echo "1: Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 1ms"
done
exit "${MOCK_RC_FINAL:-0}"
MOCK
chmod +x "$BASE/mock/cmake" "$BASE/mock/ctest"

# Arquivo que o corpo lê no lugar de /sys/fs/cgroup/pids.max (costura só do autoteste): nenhum caso depende do pids.max do
# contêiner ou do runner. Um caso que queira outro valor passa a variável de novo depois (a última vale).
printf '512\n' > "$BASE/pids512"
printf 'max\n' > "$BASE/pidsmax"

# Roda o corpo com RLIMIT_CORE = CORE (soft e hard). Uso: rodar_corpo CORPO CORE LOG SAIDA LISTA [VAR=valor ...]
rodar_corpo() {
    local corpo="$1" core="$2" log="$3" saida="$4" lista="$5"
    shift 5
    : > "$log"
    (cd "$BASE/trab" && env PATH="$BASE/mock:$PATH" MOCK_LOG="$log" NL_ENSAIO="$lista" TMPDIR="$BASE/tmp" \
        NL_ENSAIO_SHA="$SHA" NL_ENSAIO_ARQ_PIDS_MAX="$BASE/pids512" "$@" \
        python3 -I -c 'import os, resource, sys
m = int(sys.argv[1])
resource.setrlimit(resource.RLIMIT_CORE, (m, m))
os.execvp(sys.argv[2], sys.argv[2:])' "$core" bash "$corpo" debug) > "$saida" 2>&1
}

# Confere o argv registrado pelo ctest de mentira contra as regras do arnês. Retorna 1 se alguma regra falha.
checar_chamadas() {
    local log="$1" total sem_noerr fixtures fixture_repete fora_fs sem_j sem_v regex_ruim rep_total rep_ruim ln_fix ln_rep bad=0
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
    rep_ruim=$(grep -F '|--repeat|' "$log" | grep -c -v -E '[|]--repeat[|]until-fail:([1-9][0-9]{0,3})[|]' || true)
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
    [ "${rep_ruim:-0}" = 0 ] || { echo "  checagem: $rep_ruim repetição(ões) sem --repeat until-fail:N (N de 1 a 9999)"; bad=1; }
    [ "${regex_ruim:-0}" = 0 ] || { echo "  checagem: $regex_ruim regex de slot sem ancora ou sem [.]"; bad=1; }
    if [ -n "$ln_rep" ] && { [ -z "$ln_fix" ] || [ "$ln_fix" -gt "$ln_rep" ]; }; then
        echo "  checagem: fixture depois da primeira repetição"
        bad=1
    fi
    return "$bad"
}

# Confere que a saída ou o registro contém todas as cadeias dadas (grep -F). Uso: tem_todas ARQUIVO CADEIA...
tem_todas() {
    local arq="$1" s
    shift
    for s in "$@"; do
        grep -q -F -- "$s" "$arq" || { echo "  falta em ${arq##*/}: $s"; return 1; }
    done
}
# Zero chamadas de cmake e de ctest no registro (o corpo parou antes de qualquer build).
zero_chamadas() {
    [ "$(grep -c -E '^(ctest|cmake)\|' "$1" || true)" = 0 ]
}
# Conta as chamadas de ctest do registro que trazem a cadeia dada.
chamadas_com() {
    local n
    n=$(grep '^ctest|' "$1" | grep -c -F -- "$2") || true
    echo "${n:-0}"
}

# B1: verde. Fixture uma vez, slot com N=3, três processos, três medições, resumo sai; a abertura grava o comando recebido.
rodar_corpo "$BODY" 1 "$BASE/b1.log" "$BASE/b1.saida" 'transporte.exemplo:3'
rc=$?
if [ "$rc" = 0 ] && checar_chamadas "$BASE/b1.log" \
    && tem_todas "$BASE/b1.saida" \
        'ensaio teste=transporte.exemplo pedidas=3 processos=3 aprovadas=3 medicao=3 rc=0 classe=aprovado motivo=nenhum' \
        'ensaio resumo slots_pedidos=1 slots=1 reprovados=0 indeterminados=0' 'ENSAIO: VERDE' \
        'NL-ENSAIO-PRESET=debug' "NL-ENSAIO-SHA=$SHA" 'NL-ENSAIO-LISTA=transporte.exemplo:3' 'Max core file size' \
        'pids.max: 512 (esperado 512)'; then
    ok "B1 corpo verde: fixture uma vez, -FS, --no-tests=error, -j 1, -V, regex ancorada, 3 de 3, comando na abertura"
else
    falha "B1 corpo verde: esperava rc 0, checagens e abertura com preset, SHA, lista, core cru e pids.max (rc=$rc)"
    sed 's/^/    /' "$BASE/b1.saida" | tail -n 12
fi

# B2: core diferente de 1/1 aborta antes de qualquer ctest e de qualquer cmake.
rodar_corpo "$BODY" 0 "$BASE/b2.log" "$BASE/b2.saida" 'transporte.exemplo:3'
rc=$?
if [ "$rc" = 2 ] && grep -q -F 'ENSAIO PARADA' "$BASE/b2.saida" && zero_chamadas "$BASE/b2.log"; then
    ok "B2 core 0/0: aborta com rc 2 antes de cmake e ctest"
else
    falha "B2 core 0/0: esperava rc 2, PARADA e zero chamadas (rc=$rc)"
fi

# B2p-a e B2p-b: pids.max diferente de 512 (a exceção do líder à L-11 depende dele) aborta antes de qualquer build.
rodar_corpo "$BODY" 1 "$BASE/b2pa.log" "$BASE/b2pa.saida" 'transporte.exemplo:3' NL_ENSAIO_ARQ_PIDS_MAX="$BASE/pidsmax"
rc=$?
if [ "$rc" = 2 ] && grep -q -F 'ENSAIO PARADA: pids.max' "$BASE/b2pa.saida" && zero_chamadas "$BASE/b2pa.log"; then
    ok "B2p-a pids.max max: aborta com rc 2 antes de cmake e ctest"
else
    falha "B2p-a pids.max max: esperava rc 2, PARADA de pids.max e zero chamadas (rc=$rc)"
fi
rodar_corpo "$BODY" 1 "$BASE/b2pb.log" "$BASE/b2pb.saida" 'transporte.exemplo:3' NL_ENSAIO_ARQ_PIDS_MAX="$BASE/naoexiste"
rc=$?
if [ "$rc" = 2 ] && grep -q -F 'pids.max: ausente' "$BASE/b2pb.saida" && zero_chamadas "$BASE/b2pb.log"; then
    ok "B2p-b pids.max ausente: aborta com rc 2 antes de cmake e ctest"
else
    falha "B2p-b pids.max ausente: esperava rc 2, 'pids.max: ausente' e zero chamadas (rc=$rc)"
fi

# B3: nome que não casa com teste nenhum: ctest com --no-tests=error sai com erro e nenhum processo de teste inicia. Não é
# reprovação do slot, é indeterminado (nao_rodou); o resumo sai.
rodar_corpo "$BODY" 1 "$BASE/b3.log" "$BASE/b3.saida" 'transporte.inexistente:2'
rc=$?
if [ "$rc" = 7 ] && tem_todas "$BASE/b3.saida" \
    'ensaio teste=transporte.inexistente pedidas=2 processos=0 aprovadas=0 medicao=0 rc=8 classe=indeterminado motivo=nao_rodou' \
    'ensaio resumo slots_pedidos=1 slots=1 reprovados=0 indeterminados=1' 'ENSAIO: INDETERMINADO'; then
    ok "B3 slot sem teste correspondente: indeterminado nao_rodou, rc 7, resumo sai"
else
    falha "B3 slot sem teste correspondente: esperava rc 7 e classe=indeterminado motivo=nao_rodou (rc=$rc)"
fi

# B4: falha na primeira repetição do slot A para a lista; o slot B não roda; o slot é reprovado (rc 8 com processo iniciado).
rodar_corpo "$BODY" 1 "$BASE/b4.log" "$BASE/b4.saida" 'transporte.a:2 transporte.b:2' MOCK_FALHA_TESTE=transporte.a MOCK_FALHA_EM=1
rc=$?
if [ "$rc" = 1 ] && [ "$(chamadas_com "$BASE/b4.log" '|-R|^transporte[.]b$|')" = 0 ] \
    && tem_todas "$BASE/b4.saida" \
        'ensaio teste=transporte.a pedidas=2 processos=1 aprovadas=0 medicao=0 rc=8 classe=reprovado motivo=ctest' \
        'ensaio resumo slots_pedidos=2 slots=1 reprovados=1 indeterminados=0' 'ENSAIO: VERMELHO'; then
    ok "B4 falha num slot: reprovado, rc 1, o slot seguinte não roda"
else
    falha "B4 falha num slot: esperava rc 1, classe=reprovado e zero chamadas do slot B (rc=$rc)"
fi

# B5: contagem de processos que não bate com N: indeterminado (contagem), nunca aprovado nem reprovado.
rodar_corpo "$BODY" 1 "$BASE/b5.log" "$BASE/b5.saida" 'transporte.exemplo:3' MOCK_PULA_START=1
rc=$?
if [ "$rc" = 7 ] && tem_todas "$BASE/b5.saida" \
    'processos=0 aprovadas=3 medicao=3 rc=0 classe=indeterminado motivo=contagem' 'ENSAIO: INDETERMINADO'; then
    ok "B5 contagem de processos diferente de N: indeterminado, rc 7"
else
    falha "B5 contagem de processos diferente de N: esperava rc 7 e motivo=contagem (rc=$rc)"
fi

# B5b: a outra metade do portão de contagem: processos certos e aprovadas diferente de N.
rodar_corpo "$BODY" 1 "$BASE/b5b.log" "$BASE/b5b.saida" 'transporte.exemplo:3' MOCK_PULA_TOTALS=1
rc=$?
if [ "$rc" = 7 ] && tem_todas "$BASE/b5b.saida" \
    'processos=3 aprovadas=0 medicao=3 rc=0 classe=indeterminado motivo=contagem'; then
    ok "B5b aprovadas diferente de N: indeterminado, rc 7"
else
    falha "B5b aprovadas diferente de N: esperava rc 7 e motivo=contagem (rc=$rc)"
fi

# B6 (I2): rc do ctest diferente de 0 com processos e aprovadas iguais a N reprova o slot. É o caso do rc que a contagem não vê.
rodar_corpo "$BODY" 1 "$BASE/b6.log" "$BASE/b6.saida" 'transporte.exemplo:3' MOCK_RC_FINAL=8
rc=$?
if [ "$rc" = 1 ] && tem_todas "$BASE/b6.saida" \
    'processos=3 aprovadas=3 medicao=3 rc=8 classe=reprovado motivo=ctest' 'ENSAIO: VERMELHO'; then
    ok "B6 rc 8 com contagem perfeita: reprovado, rc 1"
else
    falha "B6 rc 8 com contagem perfeita: esperava rc 1 e classe=reprovado (rc=$rc)"
fi

# B7, B7b, B7c: rc 126 ou mais é a ferramenta (L-49), não reprovação; 125 é o último reprovado (um passo antes da fronteira).
rodar_corpo "$BODY" 1 "$BASE/b7.log" "$BASE/b7.saida" 'transporte.a:2 transporte.b:2' MOCK_RC_FINAL=137
rc=$?
if [ "$rc" = 7 ] && [ "$(chamadas_com "$BASE/b7.log" '|-R|^transporte[.]b$|')" = 0 ] && tem_todas "$BASE/b7.saida" \
    'rc=137 classe=indeterminado motivo=ferramenta' 'ENSAIO: INDETERMINADO'; then
    ok "B7 rc 137: indeterminado ferramenta, rc 7, o slot seguinte não roda"
else
    falha "B7 rc 137: esperava rc 7, motivo=ferramenta e zero chamadas do slot B (rc=$rc)"
fi
rodar_corpo "$BODY" 1 "$BASE/b7b.log" "$BASE/b7b.saida" 'transporte.exemplo:3' MOCK_RC_FINAL=126
rc=$?
if [ "$rc" = 7 ] && tem_todas "$BASE/b7b.saida" 'rc=126 classe=indeterminado motivo=ferramenta'; then
    ok "B7b rc 126: indeterminado ferramenta, rc 7"
else
    falha "B7b rc 126: esperava rc 7 e motivo=ferramenta (rc=$rc)"
fi
rodar_corpo "$BODY" 1 "$BASE/b7c.log" "$BASE/b7c.saida" 'transporte.exemplo:3' MOCK_RC_FINAL=125
rc=$?
if [ "$rc" = 1 ] && tem_todas "$BASE/b7c.saida" 'rc=125 classe=reprovado motivo=ctest'; then
    ok "B7c rc 125: reprovado, rc 1 (um passo antes da fronteira)"
else
    falha "B7c rc 125: esperava rc 1 e classe=reprovado (rc=$rc)"
fi

# B8 (C7): lista vazia no corpo é uso inválido, rc 5, antes de qualquer build.
rodar_corpo "$BODY" 1 "$BASE/b8.log" "$BASE/b8.saida" ''
rc=$?
if [ "$rc" = 5 ] && grep -q -F 'ENSAIO PARADA: NL_ENSAIO vazia' "$BASE/b8.saida" && zero_chamadas "$BASE/b8.log"; then
    ok "B8 lista vazia no corpo: rc 5 antes de cmake e ctest"
else
    falha "B8 lista vazia no corpo: esperava rc 5, PARADA e zero chamadas (rc=$rc)"
fi

# B10 (I3): sem NL_ENSAIO_SHA o corpo não roda, porque o log não provaria de qual commit era.
rodar_corpo "$BODY" 1 "$BASE/b10.log" "$BASE/b10.saida" 'transporte.exemplo:3' NL_ENSAIO_SHA=
rc=$?
if [ "$rc" = 5 ] && grep -q -F 'ENSAIO PARADA: NL_ENSAIO_SHA ausente ou fora de 40 hexadecimais' "$BASE/b10.saida" \
    && zero_chamadas "$BASE/b10.log"; then
    ok "B10 NL_ENSAIO_SHA vazio: rc 5 antes de cmake e ctest"
else
    falha "B10 NL_ENSAIO_SHA vazio: esperava rc 5, PARADA e zero chamadas (rc=$rc)"
fi

# B11: dois slots aprovados; cada slot roda uma vez e o laço só para no fim da lista.
rodar_corpo "$BODY" 1 "$BASE/b11.log" "$BASE/b11.saida" 'transporte.a:2 transporte.b:2'
rc=$?
if [ "$rc" = 0 ] && [ "$(grep -c -F 'classe=aprovado' "$BASE/b11.saida" || true)" = 2 ] \
    && [ "$(chamadas_com "$BASE/b11.log" '|-R|^transporte[.]a$|')" = 1 ] \
    && [ "$(chamadas_com "$BASE/b11.log" '|-R|^transporte[.]b$|')" = 1 ] \
    && tem_todas "$BASE/b11.saida" 'ensaio resumo slots_pedidos=2 slots=2 reprovados=0 indeterminados=0' 'ENSAIO: VERDE'; then
    ok "B11 dois slots aprovados: uma chamada de cada, resumo 2 de 2, verde"
else
    falha "B11 dois slots aprovados: esperava rc 0, duas linhas aprovado e uma chamada de cada slot (rc=$rc)"
fi

# G1 e G2: saída REAL do CTest 4.3.0 guardada em arquivo (tira a circularidade de o ctest de mentira imprimir o que o corpo lê).
DADOS="$RAIZ/tools/conteiner/dados_autoteste_ensaio"
rodar_corpo "$BODY" 1 "$BASE/g1.log" "$BASE/g1.saida" 'transporte.semCertificadoDeClienteHandshakeRecusado:3' \
    MOCK_SAIDA_ARQUIVO="$DADOS/ctest_verde_n3.txt"
rc=$?
if [ "$rc" = 0 ] && tem_todas "$BASE/g1.saida" \
    'pedidas=3 processos=3 aprovadas=3 medicao=3 rc=0 classe=aprovado motivo=nenhum'; then
    ok "G1 saída real do CTest (verde, 3 repetições): processos 3, aprovadas 3, medicao 3, aprovado"
else
    falha "G1 saída real do CTest (verde): esperava rc 0 e 3/3/3 aprovado (rc=$rc)"
fi
rodar_corpo "$BODY" 1 "$BASE/g2.log" "$BASE/g2.saida" 'transporte.semCertificadoDeClienteHandshakeRecusado:3' \
    MOCK_SAIDA_ARQUIVO="$DADOS/ctest_vermelho_rep1.txt" MOCK_RC_FINAL=8
rc=$?
if [ "$rc" = 1 ] && tem_todas "$BASE/g2.saida" \
    'pedidas=3 processos=1 aprovadas=0 medicao=0 rc=8 classe=reprovado motivo=ctest'; then
    ok "G2 saída real do CTest (falha na repetição 1): processos 1 (o bloco repetido sem prefixo não conta), reprovado"
else
    falha "G2 saída real do CTest (falha): esperava rc 1, processos=1 e reprovado (rc=$rc)"
fi

# S1 e S2: sabotagens do corpo, em cópias. O checador tem de reprovar cada uma (a fixture dentro da repetição; o ctest
# sem --no-tests=error). Sem a aplicação da sabotagem, o caso não prova nada e reprova.
# Uso: sabotar ORIGEM SAIDA TROCAR POR (a origem é o corpo do ensaio ou o do mutante; a troca tem de casar).
sabotar() {
    local origem="$1" saida="$2" trocar="$3" por="$4"
    python3 -I -c 'import sys
origem, destino, velho, novo = sys.argv[1:5]
texto = open(origem, encoding="utf-8").read()
if velho not in texto:
    sys.exit(3)
open(destino, "w", encoding="utf-8").write(texto.replace(velho, novo))' "$origem" "$saida" "$trocar" "$por"
}
if sabotar "$BODY" "$BASE/corpo-sem-fs.sh" "-FS '^certificados_teste\$' " "" ; then
    rodar_corpo "$BASE/corpo-sem-fs.sh" 1 "$BASE/s1.log" "$BASE/s1.saida" 'transporte.exemplo:2'
    if checar_chamadas "$BASE/s1.log"; then
        falha "sabotagem sem -FS: o checador NÃO reprovou (fixture entrou na repetição)"
    else
        ok "sabotagem sem -FS: o checador reprova"
    fi
else
    falha "sabotagem sem -FS: trecho não encontrado no corpo, sabotagem não aplicada"
fi
if sabotar "$BODY" "$BASE/corpo-sem-noerr.sh" " --no-tests=error" ""; then
    rodar_corpo "$BASE/corpo-sem-noerr.sh" 1 "$BASE/s2.log" "$BASE/s2.saida" 'transporte.exemplo:2'
    if checar_chamadas "$BASE/s2.log"; then
        falha "sabotagem sem --no-tests=error: o checador NÃO reprovou"
    else
        ok "sabotagem sem --no-tests=error: o checador reprova"
    fi
else
    falha "sabotagem sem --no-tests=error: trecho não encontrado no corpo, sabotagem não aplicada"
fi

# SB-rc: sem o gate de rc do ctest o B6 (rc 8 com contagem perfeita) sairia verde. Prova que o B6 distingue.
if sabotar "$BODY" "$BASE/corpo-sem-gate-rc.sh" '    elif [ "$rc" -ne 0 ]; then' '    elif false; then'; then
    rodar_corpo "$BASE/corpo-sem-gate-rc.sh" 1 "$BASE/sbrc.log" "$BASE/sbrc.saida" 'transporte.exemplo:3' MOCK_RC_FINAL=8
    rc=$?
    if [ "$rc" = 0 ] && grep -q -F 'ENSAIO: VERDE' "$BASE/sbrc.saida"; then
        ok "SB-rc: sem o gate de rc o corpo diz VERDE com rc 8 (o B6 distingue)"
    else
        falha "SB-rc: a sabotagem sem o gate de rc não produziu VERDE (rc=$rc): o B6 não prova o gate"
    fi
else
    falha "SB-rc: trecho do gate de rc não encontrado no corpo, sabotagem não aplicada"
fi

# SB-vazia: sem a validação de lista vazia o corpo nunca pode dizer VERDE com zero slot (rc 7, slots_pedidos=0).
if sabotar "$BODY" "$BASE/corpo-sem-lista-vazia.sh" 'if [ "${#itens[@]}" -eq 0 ]; then' 'if false; then'; then
    rodar_corpo "$BASE/corpo-sem-lista-vazia.sh" 1 "$BASE/sbvazia.log" "$BASE/sbvazia.saida" ''
    rc=$?
    if [ "$rc" = 7 ] && grep -q -F 'slots_pedidos=0' "$BASE/sbvazia.saida" \
        && [ "$(grep -c -F 'ENSAIO: VERDE' "$BASE/sbvazia.saida" || true)" = 0 ]; then
        ok "SB-vazia: sem a validação da lista vazia o corpo sai com rc 7 e nunca diz VERDE"
    else
        falha "SB-vazia: esperava rc 7, slots_pedidos=0 e nenhuma linha VERDE (rc=$rc)"
    fi
else
    falha "SB-vazia: trecho da lista vazia não encontrado no corpo, sabotagem não aplicada"
fi

# ---------- (c) corpo mutante_no_conteiner.sh, direto (I1 da revisão) ----------
# Fixture F2: SHA2_SEM não tem tools/mutacao; SHA2 tem um mutante_mtls.sh falso que imprime MARCA-DO-SHA e sai com
# FALSO_RC; depois, sem commit, o arquivo da ÁRVORE de F2 vira outro que imprime MARCA-DA-ARVORE. O corpo só pode executar
# o arquivo do commit, e só se o sha256 dele for o esperado.
CORPO_MUT="$RAIZ/tools/conteiner/mutante_no_conteiner.sh"
F2="$BASE/f2"
mkdir -p "$F2"
git -C "$F2" init -q
printf 'x\n' > "$F2/leia.txt"
git -C "$F2" add -A
git -C "$F2" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: f2 sem mutacao" \
    || { echo "ABORTA: commit F2 sem mutacao"; exit 1; }
SHA2_SEM="$(git -C "$F2" rev-parse HEAD)" || { echo "ABORTA: rev-parse F2"; exit 1; }
mkdir -p "$F2/tools/mutacao"
printf '#!/bin/bash\necho MARCA-DO-SHA\nexit "${FALSO_RC:-0}"\n' > "$F2/tools/mutacao/mutante_mtls.sh"
git -C "$F2" add -A
git -C "$F2" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: f2 com mutante falso" \
    || { echo "ABORTA: commit F2 com mutante"; exit 1; }
SHA2="$(git -C "$F2" rev-parse HEAD)" || { echo "ABORTA: rev-parse F2"; exit 1; }
ESP2="$(git -C "$F2" show "$SHA2:tools/mutacao/mutante_mtls.sh" | sha256sum | cut -d' ' -f1)"
printf '#!/bin/bash\necho MARCA-DA-ARVORE\n' > "$F2/tools/mutacao/mutante_mtls.sh"
ESP_ARV="$(sha256sum < "$F2/tools/mutacao/mutante_mtls.sh" | cut -d' ' -f1)"

# Roda o corpo do mutante com RLIMIT_CORE = CORE (soft e hard).
# Uso: rodar_mutante CORPO CORE SAIDA REPO SHA BASEDIR HASH [VAR=valor ...]
rodar_mutante() {
    local corpo="$1" core="$2" saida="$3" repo="$4" sha="$5" basedir="$6" hash="$7"
    shift 7
    (cd "$BASE/trab" && env TMPDIR="$BASE/tmp" NL_MUTANTE_SCRIPT_SHA256="$hash" "$@" \
        python3 -I -c 'import os, resource, sys
m = int(sys.argv[1])
resource.setrlimit(resource.RLIMIT_CORE, (m, m))
os.execvp(sys.argv[2], sys.argv[2:])' "$core" bash "$corpo" "$repo" "$sha" "$basedir") > "$saida" 2>&1
}
nenhuma_marca() {
    [ "$(grep -c -F 'MARCA-' "$1" || true)" = 0 ]
}

# MU1: caminho feliz. Executa o arquivo do SHA (MARCA-DO-SHA), nunca o da árvore; o hash extraído é o esperado.
rodar_mutante "$CORPO_MUT" 1 "$BASE/mu1.saida" "$F2/.git" "$SHA2" "$BASE/mu1" "$ESP2"
rc=$?
if [ "$rc" = 0 ] && tem_todas "$BASE/mu1.saida" 'MARCA-DO-SHA' "NL-MUTANTE-SCRIPT-SHA256-EXTRAIDO=$ESP2" \
    'NL-MUTANTE-SCRIPT-EXECUTADO=' && [ "$(grep -c -F 'MARCA-DA-ARVORE' "$BASE/mu1.saida" || true)" = 0 ]; then
    ok "MU1 corpo do mutante: executa o script do SHA, hash extraído igual ao esperado, nunca o da árvore"
else
    falha "MU1 corpo do mutante: esperava rc 0, MARCA-DO-SHA, hash extraído igual ao esperado e nenhuma MARCA-DA-ARVORE (rc=$rc)"
    sed 's/^/    /' "$BASE/mu1.saida" | tail -n 10
fi
# MU2: o código de saída do script do mutante sai do corpo.
rodar_mutante "$CORPO_MUT" 1 "$BASE/mu2.saida" "$F2/.git" "$SHA2" "$BASE/mu2" "$ESP2" FALSO_RC=9
rc=$?
if [ "$rc" = 9 ]; then
    ok "MU2 corpo do mutante: o rc do script do mutante (9) é o do corpo"
else
    falha "MU2 corpo do mutante: esperava rc 9 (rc=$rc)"
fi
# MU3: hash esperado diferente do do arquivo extraído (hash do arquivo da árvore): nada é executado, rc 24.
rodar_mutante "$CORPO_MUT" 1 "$BASE/mu3.saida" "$F2/.git" "$SHA2" "$BASE/mu3" "$ESP_ARV"
rc=$?
if [ "$rc" = 24 ] && grep -q -F 'MUTANTE PARADA' "$BASE/mu3.saida" && nenhuma_marca "$BASE/mu3.saida"; then
    ok "MU3 corpo do mutante: hash esperado diferente do extraído, rc 24, nada executado"
else
    falha "MU3 corpo do mutante: esperava rc 24, MUTANTE PARADA e nenhuma linha MARCA- (rc=$rc)"
fi
# MU4: caminho ausente no SHA: rc 23, nada executado.
rodar_mutante "$CORPO_MUT" 1 "$BASE/mu4.saida" "$F2/.git" "$SHA2_SEM" "$BASE/mu4" "$ESP2"
rc=$?
if [ "$rc" = 23 ] && grep -q -F 'MUTANTE PARADA' "$BASE/mu4.saida" && nenhuma_marca "$BASE/mu4.saida"; then
    ok "MU4 corpo do mutante: tools/mutacao ausente no SHA, rc 23, nada executado"
else
    falha "MU4 corpo do mutante: esperava rc 23, MUTANTE PARADA e nenhuma linha MARCA- (rc=$rc)"
fi
# MU5: segunda rodada na base do MU1 (mutante-fonte já existe): rc 23, 'já existe', nada executado.
rodar_mutante "$CORPO_MUT" 1 "$BASE/mu5.saida" "$F2/.git" "$SHA2" "$BASE/mu1" "$ESP2"
rc=$?
if [ "$rc" = 23 ] && grep -q -F 'já existe' "$BASE/mu5.saida" && nenhuma_marca "$BASE/mu5.saida"; then
    ok "MU5 corpo do mutante: base de rodada anterior, rc 23, 'já existe', nada executado"
else
    falha "MU5 corpo do mutante: esperava rc 23, 'já existe' e nenhuma linha MARCA- (rc=$rc)"
fi
# MU6: core 0 aborta com rc 21 antes de extrair qualquer coisa (o mutante derruba teste de propósito).
rodar_mutante "$CORPO_MUT" 0 "$BASE/mu6.saida" "$F2/.git" "$SHA2" "$BASE/mu6" "$ESP2"
rc=$?
if [ "$rc" = 21 ] && nenhuma_marca "$BASE/mu6.saida" && [ ! -e "$BASE/mu6/mutante-fonte" ]; then
    ok "MU6 corpo do mutante: core 0/0, rc 21, nada extraído nem executado"
else
    falha "MU6 corpo do mutante: esperava rc 21, nenhuma linha MARCA- e mutante-fonte não criada (rc=$rc)"
fi
# MU7: hash esperado vazio: rc 20.
rodar_mutante "$CORPO_MUT" 1 "$BASE/mu7.saida" "$F2/.git" "$SHA2" "$BASE/mu7" ""
rc=$?
if [ "$rc" = 20 ] && nenhuma_marca "$BASE/mu7.saida"; then
    ok "MU7 corpo do mutante: hash esperado vazio, rc 20, nada executado"
else
    falha "MU7 corpo do mutante: esperava rc 20 e nenhuma linha MARCA- (rc=$rc)"
fi
# MS1: sem a comparação dos hashes o MU3 sairia executando o script (MARCA-DO-SHA, rc 0). Prova que o MU3 distingue.
mkdir -p "$BASE/ms1"
cp "$RAIZ/tools/conteiner/extrair_do_sha.sh" "$BASE/ms1/extrair_do_sha.sh"
if sabotar "$CORPO_MUT" "$BASE/ms1/mutante_no_conteiner.sh" 'if [ "$obtido" != "$esperado" ]; then' 'if false; then'; then
    rodar_mutante "$BASE/ms1/mutante_no_conteiner.sh" 1 "$BASE/ms1.saida" "$F2/.git" "$SHA2" "$BASE/ms1base" "$ESP_ARV"
    rc=$?
    if [ "$rc" = 0 ] && grep -q -F 'MARCA-DO-SHA' "$BASE/ms1.saida"; then
        ok "MS1 sem a comparação de hash o corpo do mutante executa o script (o MU3 distingue)"
    else
        falha "MS1 a sabotagem sem a comparação de hash não executou o script (rc=$rc): o MU3 não prova a comparação"
    fi
else
    falha "MS1 trecho da comparação de hash não encontrado no corpo do mutante, sabotagem não aplicada"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE BUILD ENSAIO: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE BUILD ENSAIO: REPROVADO"
exit 1
