#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do build e da evidência por SHA (NL-2.0, achado A1-07 da auditoria da Onda 1).
# Sabotagem de estreia (L-36): cada caso tem de REPROVAR contra a sabotagem correspondente (cópia, nunca o arquivo rastreado).
#   C0  controle positivo: build de fixture limpa; linha 1 do log traz "SHA: <40 hex>"; NL-RC=0; o verificador aceita.
#   C1  log sem SHA no topo reprova (sem SHA nenhum; e SHA só na linha 2).
#   C1b log com SHA válido só na linha 2 e manifesto válido reprova, citando a linha 1 (não aceita SHA em qualquer linha).
#   C2  arquivo alheio não commitado NÃO entra na fonte do build (a fonte é o git archive do SHA).
#   C3  log com arquivo alheio reprova: fonte vinda de snapshot sujo (NL_FONTE) é recusada pelo verificador;
#       snapshot limpo do mesmo SHA passa (controle positivo do modo cópia).
#   C3b a cópia divergente para ANTES do contêiner: rc 4, NL-RC=4 no log e o docker não é chamado.
#   C4  duas fatias em paralelo não compartilham diretório de fonte nem de build.
#   C5  NL_XSD_ZIP_CACHE chega ao contêiner (variável de ambiente e montagem no docker run).
#   C6  xmlsec1 declarado nos quatro scripts de instalar/ (estático; a medição real das imagens fica no ci_medicao.md).
#   C7  sem NL_FATIA o build para com rc 2, a mensagem "PARADA: NL_FATIA obrigatória" e sem chamar o docker.
#   C8  build de imagem registra "SHA: <sha> fatia=imagem" na linha 1, NL-RC no fim, e o verificador reprova (sem manifesto).
#   C9  concorrência: dois executar e dois imagem em paralelo nunca têm contêineres sobrepostos (marcas S/E do stub).
# O docker é um STUB que só registra as chamadas (e, em C9, marca início e fim com sleep). Nenhum contêiner sobe aqui.
# Usa NL_TMP_BASE próprio, nunca a trava real /var/tmp/notalivre-heavy.lock. A fixture não dispara hooks do git.
# Uso: autoteste_build_sha.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-sha.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin" "$BASE/tmp"
# Stub: registra cada chamada; com STUB_CONC, marca S (início) e E (fim) com sleep 1 entre as duas marcas.
printf '#!/bin/sh\necho "docker $*" >> "${STUB_LOG:-/dev/null}"\nif [ -n "${STUB_CONC:-}" ]; then echo "S $$" >> "$STUB_CONC"; sleep 1; echo "E $$" >> "$STUB_CONC"; fi\nexit 0\n' > "$BASE/bin/docker"
chmod +x "$BASE/bin/docker"
export PATH="$BASE/bin:$PATH" NL_TMP_BASE="$BASE/tmp"
EVID="$RAIZ/tools/conteiner/evidencia_sha.sh"
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# Valor de uma chave NL-... ou de linha do log (primeira ocorrência). Vazio se ausente.
valor_do_log() { sed -n "s/^$2=//p" "$1" | head -n 1; }

# Roda o build como o CI faz (construir, com log próprio). Remove do ambiente as variáveis de build, para o caso ser
# hermético; os argumentos extras voltam como variáveis explícitas.
RC_BUILD=0
rodar_build() {
    local nome="$1"; shift
    env -u NL_FATIA -u NL_FONTE -u NL_SHA -u NL_REPO -u NL_XSD_ZIP_CACHE -u NL_BUILD_ROOT "$@" \
        STUB_LOG="$BASE/$nome.docker" NL_LOG="$BASE/$nome.log" \
        timeout 180 bash "$RAIZ/tools/conteiner/build_local.sh" fedora44 debug
    RC_BUILD=$?
    return "$RC_BUILD"
}

# Build de imagem (subcomando imagem), mesma higiene de ambiente.
RC_IMG=0
rodar_imagem() {
    local nome="$1"; shift
    env -u NL_FATIA -u NL_FONTE -u NL_SHA -u NL_REPO -u NL_XSD_ZIP_CACHE -u NL_BUILD_ROOT "$@" \
        STUB_LOG="$BASE/$nome.docker" NL_LOG="$BASE/$nome.log" \
        timeout 180 bash "$RAIZ/tools/conteiner/build_local.sh" imagem fedora44
    RC_IMG=$?
    return "$RC_IMG"
}

# Verificador da evidência: rc 99 quando o script não existe (nunca conta como reprovação legítima).
verificar() {
    if [ ! -f "$EVID" ]; then
        echo "verificador ausente: $EVID"
        return 99
    fi
    timeout 120 bash "$EVID" verificar "$1" "$REPO" > "$1.verificador" 2>&1
}

# Conta pares de marcas S/E e as sobreposições (um S com outro S ainda aberto). Saída: "pares=N sobreposicoes=K".
conc_resumo() {
    local marca _pid abertos=0 pares=0 sobre=0
    while read -r marca _pid; do
        case "$marca" in
            S) [ "$abertos" -eq 0 ] || sobre=$((sobre + 1)); abertos=1; pares=$((pares + 1)) ;;
            E) abertos=0 ;;
        esac
    done < "$1"
    echo "pares=$pares sobreposicoes=$sobre"
}

# --- fixture: repositório git de brincadeira, com os caminhos que a fonte do build copia ---
# Sem -q nas etapas do git (falha não se esconde) e com core.hooksPath=/dev/null só no commit da fixture,
# para não disparar o hook global de travessão na fixture.
REPO="$BASE/repo"
mkdir -p "$REPO/src" "$REPO/apps" "$REPO/tests" "$REPO/third_party" "$REPO/tools/conteiner" "$REPO/tools/tabelas"
printf 'project(fixture)\n' > "$REPO/CMakeLists.txt"
printf '{}\n' > "$REPO/CMakePresets.json"
printf 'int comprometido = 1;\n' > "$REPO/src/comprometido.cpp"
for d in apps tests third_party tools/conteiner tools/tabelas; do printf 'fixture\n' > "$REPO/$d/conteudo.txt"; done
git -C "$REPO" init || { echo "ABORTA: git init da fixture"; exit 1; }
git -C "$REPO" add -A || { echo "ABORTA: git add da fixture"; exit 1; }
git -C "$REPO" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -m "test: fixture" \
    || { echo "ABORTA: commit da fixture"; exit 1; }
SHA_FIXTURE="$(git -C "$REPO" rev-parse HEAD)" || { echo "ABORTA: rev-parse da fixture"; exit 1; }
cp -a "$REPO" "$BASE/snapshot_limpo"
printf 'arquivo alheio, nao commitado\n' > "$REPO/src/alheio.cpp"
cp -a "$REPO" "$BASE/snapshot_sujo"

# --- C0 e C2: build limpo do SHA, com arquivo alheio não commitado na árvore de trabalho ---
rodar_build t0 NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" NL_FATIA=NL-T0
rc0=$?
l1="$(head -n 1 "$BASE/t0.log" 2>/dev/null)"
if [ "$rc0" -eq 0 ] && [[ "$l1" == "SHA: $SHA_FIXTURE"* ]] && grep -qx 'NL-RC=0' "$BASE/t0.log"; then
    ok "C0 build do SHA com linha 1 'SHA: <sha>' e NL-RC=0 (rc=$rc0)"
else
    falha "C0 build sem SHA na linha 1, NL-RC=0 ausente ou rc=$rc0 (linha 1: '${l1:-vazia}')"
fi
verificar "$BASE/t0.log"; rv=$?
if [ "$rv" -eq 0 ]; then ok "C0 verificador aceita o log limpo (rc=0)"; else falha "C0 verificador rejeitou o log limpo (rc=$rv)"; fi

src0="$(valor_do_log "$BASE/t0.log" NL-SRC)"
if [ -n "$src0" ] && [ -f "$src0/src/comprometido.cpp" ] && [ ! -e "$src0/src/alheio.cpp" ]; then
    ok "C2 fonte do build sem o arquivo alheio, com o arquivo commitado"
else
    falha "C2 fonte do build não provada (NL-SRC='${src0:-ausente}'; alheio presente ou commitado ausente)"
fi

# --- C1 e C1b: log sem SHA no topo reprova ---
printf 'build qualquer, sem cabecalho\n' > "$BASE/c1_sem_sha.log"
printf 'linha 1 sem sha\nSHA: %s fatia=x\n' "$SHA_FIXTURE" > "$BASE/c1_sha_na_linha2.log"
for log in c1_sem_sha c1_sha_na_linha2; do
    verificar "$BASE/$log.log"; rv=$?
    if [ "$rv" -eq 99 ]; then falha "C1 $log: verificador ausente, nada reprovou"
    elif [ "$rv" -ne 0 ]; then ok "C1 $log reprovado pelo verificador (rc=$rv)"
    else falha "C1 $log APROVADO sem SHA no topo"; fi
done
# C1b: o log tem SHA válido e manifesto válido, mas a linha 1 é outra. Só a regra da linha 1 pode reprovar.
{ printf 'cabecalho\n'; cat "$BASE/t0.log"; } > "$BASE/c1b_sha_na_linha2_manifesto_valido.log"
verificar "$BASE/c1b_sha_na_linha2_manifesto_valido.log"; rv=$?
if [ "$rv" -eq 99 ]; then falha "C1b verificador ausente, nada reprovou"
elif [ "$rv" -ne 0 ] && grep -q 'a linha 1 do log não traz' "$BASE/c1b_sha_na_linha2_manifesto_valido.log.verificador"; then
    ok "C1b SHA só na linha 2 reprovado citando a linha 1 (rc=$rv)"
else
    falha "C1b SHA na linha 2 com manifesto válido aceito, ou reprovado por outro motivo (rc=$rv)"
fi

# --- C3: log com arquivo alheio reprova; snapshot limpo do mesmo SHA passa ---
rodar_build t3s NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" NL_FATIA=NL-T3S NL_FONTE="$BASE/snapshot_sujo"
rc3s=$?
verificar "$BASE/t3s.log"; rv=$?
if [ "$rv" -eq 99 ]; then falha "C3 suja: verificador ausente, nada reprovou"
elif [ "$rv" -ne 0 ]; then ok "C3 log com arquivo alheio reprovado pelo verificador (rc=$rv, build rc=$rc3s; motivo: $(head -n 1 "$BASE/t3s.log.verificador"))"
else falha "C3 log com arquivo alheio APROVADO (build rc=$rc3s)"; fi

rodar_build t3l NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" NL_FATIA=NL-T3L NL_FONTE="$BASE/snapshot_limpo"
rc3l=$?
verificar "$BASE/t3l.log"; rv=$?
if [ "$rv" -eq 0 ]; then ok "C3 controle: snapshot limpo do SHA aprovado (build rc=$rc3l)"
else falha "C3 controle: snapshot limpo reprovado (rv=$rv, build rc=$rc3l)"; fi

# --- C3b: a cópia divergente para antes do contêiner (rc 4, NL-RC=4, nenhuma chamada ao docker) ---
if [ "$rc3s" -eq 4 ] && grep -qx 'NL-RC=4' "$BASE/t3s.log" && [ ! -e "$BASE/t3s.docker" ]; then
    ok "C3b cópia divergente recusada antes do contêiner (rc=4, NL-RC=4, docker não chamado)"
else
    falha "C3b cópia divergente NÃO parou antes do contêiner (rc=$rc3s, docker chamado: $([ -e "$BASE/t3s.docker" ] && echo sim || echo nao))"
fi

# --- C4: duas fatias em paralelo, cada uma com diretórios próprios ---
rodar_build t4a NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" NL_FATIA=NL-T4A &
pa=$!
rodar_build t4b NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" NL_FATIA=NL-T4B &
pb=$!
wait "$pa"; ra=$?
wait "$pb"; rb=$?
sa="$(valor_do_log "$BASE/t4a.log" NL-SRC)"; sb="$(valor_do_log "$BASE/t4b.log" NL-SRC)"
ba="$(valor_do_log "$BASE/t4a.log" NL-BUILD)"; bb="$(valor_do_log "$BASE/t4b.log" NL-BUILD)"
if [ "$ra" -eq 0 ] && [ "$rb" -eq 0 ] && [ -n "$sa" ] && [ -n "$sb" ] && [ "$sa" != "$sb" ] \
   && [ -n "$ba" ] && [ -n "$bb" ] && [ "$ba" != "$bb" ] && [[ "$sa" == *-NL-T4A ]] && [[ "$sb" == *-NL-T4B ]]; then
    ok "C4 fatias paralelas com fonte e build distintos"
else
    falha "C4 fatias compartilham diretório ou não rodaram (rc=$ra/$rb, fonte='${sa:-ausente}'/'${sb:-ausente}', build='${ba:-ausente}'/'${bb:-ausente}')"
fi

# --- C5: NL_XSD_ZIP_CACHE chega ao contêiner ---
if grep -q -- '-e NL_XSD_ZIP_CACHE=/xsd-cache' "$BASE/t0.docker" 2>/dev/null \
   && grep -q -- ':/xsd-cache:ro' "$BASE/t0.docker" 2>/dev/null; then
    ok "C5 cache do XSD montado e exportado ao contêiner"
else
    falha "C5 NL_XSD_ZIP_CACHE não chegou ao contêiner (docker run sem -e nem -v do cache)"
fi

# --- C6: xmlsec1 nos quatro scripts de instalar/ (piso: 4 analisados) ---
analisados=0; encontrados=0; faltou=0
for d in fedora44 ubuntu26.04 manjaro mint22.3; do
    analisados=$((analisados + 1))
    if grep -q -- 'xmlsec1' "$RAIZ/tools/conteiner/instalar/$d.sh" 2>/dev/null; then
        encontrados=$((encontrados + 1))
    else
        faltou=$((faltou + 1))
    fi
done
echo "C6 instalar/: encontrados=$encontrados analisados=$analisados falharam=$faltou"
if [ "$analisados" -eq 4 ] && [ "$encontrados" -eq 4 ]; then ok "C6 xmlsec1 nas quatro imagens"
else falha "C6 xmlsec1 ausente em $faltou de $analisados scripts de instalar/"; fi

# --- C7: sem NL_FATIA, parada com rc 2 e nenhum docker ---
rodar_build t7 NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE"
rc7=$?
if [ "$rc7" -eq 2 ] && grep -q 'PARADA: NL_FATIA obrigatória' "$BASE/t7.log" && [ ! -e "$BASE/t7.docker" ]; then
    ok "C7 sem NL_FATIA: PARADA com rc=2 e docker não chamado"
else
    falha "C7 sem NL_FATIA não parou como deveria (rc=$rc7, docker chamado: $([ -e "$BASE/t7.docker" ] && echo sim || echo nao))"
fi

# --- C8: build de imagem com SHA na linha 1; o verificador reprova (imagem não é evidência por SHA) ---
rodar_imagem c8 NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE"
rc8=$?
l1c8="$(head -n 1 "$BASE/c8.log" 2>/dev/null)"
if [ "$rc8" -eq 0 ] && [[ "$l1c8" == "SHA: $SHA_FIXTURE fatia=imagem"* ]] && grep -qx 'NL-RC=0' "$BASE/c8.log" \
   && grep -q '^docker build -t notalivre/build-fedora44' "$BASE/c8.docker"; then
    ok "C8 imagem com SHA na linha 1 e NL-RC=0 (rc=$rc8)"
else
    falha "C8 imagem sem SHA na linha 1 ou sem NL-RC (rc=$rc8; linha 1: '${l1c8:-vazia}')"
fi
verificar "$BASE/c8.log"; rv=$?
if [ "$rv" -eq 99 ]; then falha "C8 verificador ausente"
elif [ "$rv" -ne 0 ] && grep -q 'NL-MANIFESTO-SHA256' "$BASE/c8.log.verificador"; then
    ok "C8 log de imagem reprovado pelo verificador por falta de manifesto (rc=$rv)"
else
    falha "C8 log de imagem aceito como evidência por SHA (rc=$rv)"
fi

# --- C9: concorrência. Dois executar e dois imagem em paralelo: nenhum contêiner pode sobrepor outro ---
rodar_build c9e1 NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" NL_FATIA=NL-C9A STUB_CONC="$BASE/c9e.conc" &
pe1=$!
rodar_build c9e2 NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" NL_FATIA=NL-C9B STUB_CONC="$BASE/c9e.conc" &
pe2=$!
wait "$pe1"; re1=$?
wait "$pe2"; re2=$?
resumo_e="$(conc_resumo "$BASE/c9e.conc" 2>/dev/null)"
# Piso: 2 processos x 2 chamadas de docker (diagnóstico e build) = 4 pares S/E.
if [ "$re1" -eq 0 ] && [ "$re2" -eq 0 ] && [ "$resumo_e" = "pares=4 sobreposicoes=0" ]; then
    ok "C9 executar em paralelo sem sobreposição de contêiner ($resumo_e)"
else
    falha "C9 executar em paralelo com contêineres sobrepostos ou sem piso (rc=$re1/$re2, $resumo_e)"
fi

rodar_imagem c9i1 NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" STUB_CONC="$BASE/c9i.conc" &
pi1=$!
rodar_imagem c9i2 NL_REPO="$REPO" NL_SHA="$SHA_FIXTURE" STUB_CONC="$BASE/c9i.conc" &
pi2=$!
wait "$pi1"; ri1=$?
wait "$pi2"; ri2=$?
resumo_i="$(conc_resumo "$BASE/c9i.conc" 2>/dev/null)"
# Piso: 2 processos x 1 chamada de docker build = 2 pares S/E.
if [ "$ri1" -eq 0 ] && [ "$ri2" -eq 0 ] && [ "$resumo_i" = "pares=2 sobreposicoes=0" ]; then
    ok "C9 imagem em paralelo sem sobreposição de contêiner ($resumo_i)"
else
    falha "C9 imagem em paralelo com contêineres sobrepostos ou sem piso (rc=$ri1/$ri2, $resumo_i)"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE BUILD SHA: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE BUILD SHA: REPROVADO"
exit 1
