#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Autoteste dos portões de higiene do NotaLivre (NL-0.6): segredos (gitleaks, com regra
# senha-literal) e travessão/meia-risca (grupos UI e DOCS).
# Nenhuma chave ou senha de teste é versionada: são geradas ou escritas na hora, em WORK.
# Saída: uma linha [VERDE] ou [VERMELHO] por controle. Sai com 1 se algum controle falhar.
#
# Uso: autoteste_higiene.sh
# Variável HIGIENE_PORTOES: diretório com os portões a testar (padrão: este diretório).
set -u

DIR_PORTOES="${HIGIENE_PORTOES:-$(cd "$(dirname "$0")" && pwd)}"
RAIZ_REPO="$(cd "$(dirname "$0")/../.." && pwd)"
FIXTURES="$RAIZ_REPO/tests/fixtures/higiene"
WORK="$(mktemp -d "${TMPDIR:-/var/tmp}/higiene-autoteste.XXXXXX")" || exit 1
trap 'rm -rf -- "$WORK"' EXIT
FALHAS=0
RC=0

# veredito NOME RC_ESPERADO RC_OBTIDO FALHA_EXTRA(0 ok, 1 falhou) DETALHE
veredito() {
    if [ "$3" -eq "$2" ] && [ "$4" -eq 0 ]; then
        echo "[VERDE] $1: rc=$3 (esperado $2) $5"
    else
        echo "[VERMELHO] $1: rc=$3 (esperado $2) $5"
        FALHAS=$((FALHAS + 1))
    fi
}

# campo SAIDA NOME: valor numérico de NOME=N na saída do portão; -1 se ausente
campo() {
    local v
    v=$(/usr/bin/grep -o "$2=[0-9]*" "$1" 2>/dev/null | /usr/bin/tail -n 1 | cut -d= -f2)
    echo "${v:--1}"
}

# formato_trav SAIDA: a linha de contagem obrigatória existe e está completa
formato_trav() {
    /usr/bin/grep -qE '^encontrados=[0-9]+ analisados=[0-9]+ violacoes=[0-9]+$' "$1"
}

# formato_seg SAIDA: a linha de contagem obrigatória do portão de segredos existe
formato_seg() {
    /usr/bin/grep -qE '^encontrados=[0-9]+ analisados=[0-9]+ segredos=[0-9]+ gitleaks_rc=(-|[0-9]+)' "$1"
}

# roda_trav SAIDA ARGS...: roda o portão de travessão; RC recebe o status
roda_trav() {
    local saida="$1"
    shift
    python3 -I "$DIR_PORTOES/higiene_travessao.py" "$@" > "$saida" 2>&1
    RC=$?
}

# roda_seg SAIDA RAIZ: roda o portão de segredos; RC recebe o status
roda_seg() {
    local saida="$1"
    shift
    bash "$DIR_PORTOES/higiene_segredos.sh" "$@" > "$saida" 2>&1
    RC=$?
}

# nova_arvore NOME: cria $WORK/NOME como cópia da árvore do repositório (sem .git nem build*)
nova_arvore() {
    mkdir -p "$WORK/$1" && tar -C "$WORK/$1" -xf "$WORK/base.tar"
}

# regra_achada ARVORE REGRA: confere, pelo relatório JSON gravado em WORK, se REGRA foi achada.
# O relatório nunca vai para o log: o modo --verbose vaza o valor da senha no contexto da linha.
regra_achada() {
    gitleaks dir "$1" --config "$1/.gitleaks.toml" --redact --no-banner --exit-code 0 \
        --report-format json --report-path "$WORK/relatorio.json" > /dev/null 2>&1
    /usr/bin/grep -q "\"RuleID\": \"$2\"" "$WORK/relatorio.json" 2>/dev/null
}

# raiz_limpa NOME: raiz de teste com interface limpa e README limpo (piso DOCS satisfeito)
raiz_limpa() {
    rm -rf "$WORK/$1"; mkdir -p "$WORK/$1/apps/gui"
    cp "$FIXTURES/limpo.qml" "$WORK/$1/apps/gui/Main.qml" > /dev/null 2>&1
    printf 'Projeto de teste\n' > "$WORK/$1/README.md"
}

echo "== Ferramentas"
echo "gitleaks: $(gitleaks version 2>&1 || echo AUSENTE)"
echo "python3: $(python3 -V 2>&1 || echo AUSENTE)"
echo "openssl: $(openssl version 2>&1 || echo AUSENTE)"
echo "portoes: $DIR_PORTOES"
echo "raiz do repositorio: $RAIZ_REPO"

tar --exclude=./.git --exclude='./build*' -C "$RAIZ_REPO" -cf "$WORK/base.tar" . > /dev/null 2>&1
openssl genpkey -algorithm ed25519 -out "$WORK/chave_falsa.pem" > "$WORK/openssl.log" 2>&1
CHAVE_RC=$?

echo "== Travessao e meia-risca (higiene_travessao.py, grupos UI e DOCS)"

# T1: interface e README limpos passam
raiz_limpa t1
roda_trav "$WORK/t1.out" "$WORK/t1"
enc=$(campo "$WORK/t1.out" encontrados); vio=$(campo "$WORK/t1.out" violacoes)
ui=$(campo "$WORK/t1.out" encontrados_ui); docs=$(campo "$WORK/t1.out" encontrados_docs)
falha=0; [ "$ui" -eq 1 ] && [ "$docs" -eq 1 ] && [ "$vio" -eq 0 ] && [ "$enc" -eq 2 ] || falha=1; formato_trav "$WORK/t1.out" || falha=1
veredito "T1 travessao positivo (interface e README limpos)" 0 "$RC" "$falha" "encontrados=$enc ui=$ui docs=$docs violacoes=$vio"

# T2: U+2014 em .qml reprova
raiz_limpa t2; cp "$FIXTURES/com_em_dash.qml" "$WORK/t2/apps/gui/Main.qml" > /dev/null 2>&1
roda_trav "$WORK/t2.out" "$WORK/t2"
vio=$(campo "$WORK/t2.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t2.out" || falha=1
veredito "T2 travessao negativo (U+2014 em .qml)" 1 "$RC" "$falha" "violacoes=$vio"

# T3: U+2013 em .qml reprova
raiz_limpa t3; cp "$FIXTURES/com_en_dash.qml" "$WORK/t3/apps/gui/Main.qml" > /dev/null 2>&1
roda_trav "$WORK/t3.out" "$WORK/t3"
vio=$(campo "$WORK/t3.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t3.out" || falha=1
veredito "T3 travessao negativo (U+2013 em .qml)" 1 "$RC" "$falha" "violacoes=$vio"

# T4: varredura vazia reprova (piso DOCS zerado)
rm -rf "$WORK/t4"; mkdir -p "$WORK/t4"
roda_trav "$WORK/t4.out" "$WORK/t4"
enc=$(campo "$WORK/t4.out" encontrados)
falha=0; [ "$enc" -eq 0 ] || falha=1; formato_trav "$WORK/t4.out" || falha=1
veredito "T4 travessao varredura vazia" 1 "$RC" "$falha" "encontrados=$enc"

# T5: U+2014 DENTRO de tests/fixtures/higiene (prefixo da raiz) passa pela allowlist
raiz_limpa t5; mkdir -p "$WORK/t5/tests/fixtures/higiene"
cp "$FIXTURES/com_em_dash.qml" "$WORK/t5/tests/fixtures/higiene/" > /dev/null 2>&1
roda_trav "$WORK/t5.out" "$WORK/t5"
vio=$(campo "$WORK/t5.out" violacoes); ign=$(campo "$WORK/t5.out" ignorados_allowlist)
falha=0; [ "$vio" -eq 0 ] && [ "$ign" -ge 1 ] || falha=1; formato_trav "$WORK/t5.out" || falha=1
veredito "T5 travessao allowlist (prefixo tests/fixtures/higiene na raiz)" 0 "$RC" "$falha" "violacoes=$vio ignorados=$ign"

# T6: o mesmo U+2014 FORA da allowlist (tests/fixtures/outro) reprova
raiz_limpa t6; mkdir -p "$WORK/t6/tests/fixtures/outro"
cp "$FIXTURES/com_em_dash.qml" "$WORK/t6/tests/fixtures/outro/" > /dev/null 2>&1
roda_trav "$WORK/t6.out" "$WORK/t6"
vio=$(campo "$WORK/t6.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t6.out" || falha=1
veredito "T6 travessao allowlist estreita (fora de tests/fixtures/higiene)" 1 "$RC" "$falha" "violacoes=$vio"

# T7: arquivo encontrado e não lido (symlink quebrado) reprova: analisados != encontrados
raiz_limpa t7; ln -s "$WORK/nao_existe.qml" "$WORK/t7/apps/gui/quebrado.qml"
roda_trav "$WORK/t7.out" "$WORK/t7"
enc=$(campo "$WORK/t7.out" encontrados); ana=$(campo "$WORK/t7.out" analisados)
falha=0; [ "$enc" -ge 3 ] && [ "$ana" -lt "$enc" ] || falha=1; formato_trav "$WORK/t7.out" || falha=1
veredito "T7 travessao arquivo nao analisado" 1 "$RC" "$falha" "encontrados=$enc analisados=$ana"

# T8: ancoragem: o prefixo tests/fixtures/higiene NÃO vale em subdiretório (substring proibida)
raiz_limpa t8; mkdir -p "$WORK/t8/apps/tests/fixtures/higiene"
cp "$FIXTURES/com_em_dash.qml" "$WORK/t8/apps/tests/fixtures/higiene/" > /dev/null 2>&1
roda_trav "$WORK/t8.out" "$WORK/t8"
vio=$(campo "$WORK/t8.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t8.out" || falha=1
veredito "T8 travessao allowlist ancorada (apps/tests/fixtures/higiene NAO isenta)" 1 "$RC" "$falha" "violacoes=$vio"

# T9: tr("...") com U+2014 em .cpp reprova
raiz_limpa t9; mkdir -p "$WORK/t9/apps/cli"
printf 'QString m = tr("Falha \xe2\x80\x94 x");\n' > "$WORK/t9/apps/cli/main.cpp"
roda_trav "$WORK/t9.out" "$WORK/t9"
vio=$(campo "$WORK/t9.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t9.out" || falha=1
veredito "T9 travessao negativo (tr com U+2014 em .cpp)" 1 "$RC" "$falha" "violacoes=$vio"

# T10: tr("...") limpo em .cpp passa e conta no grupo UI
raiz_limpa t10; mkdir -p "$WORK/t10/apps/cli"
printf 'QString m = tr("Falha de rede");\n' > "$WORK/t10/apps/cli/main.cpp"
roda_trav "$WORK/t10.out" "$WORK/t10"
vio=$(campo "$WORK/t10.out" violacoes); ui=$(campo "$WORK/t10.out" encontrados_ui)
falha=0; [ "$vio" -eq 0 ] && [ "$ui" -eq 2 ] || falha=1; formato_trav "$WORK/t10.out" || falha=1
veredito "T10 travessao tr limpo em .cpp passa" 0 "$RC" "$falha" "violacoes=$vio ui=$ui"

# T11: README com U+2014 reprova (grupo DOCS)
raiz_limpa t11; printf 'Projeto \xe2\x80\x94 teste\n' > "$WORK/t11/README.md"
roda_trav "$WORK/t11.out" "$WORK/t11"
vio=$(campo "$WORK/t11.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t11.out" || falha=1
veredito "T11 travessao negativo (README com U+2014)" 1 "$RC" "$falha" "violacoes=$vio"

# T12: TODO.md com U+2014 PASSA (documento interno, fora do grupo DOCS)
raiz_limpa t12; printf 'Tarefa \xe2\x80\x94 pendente\n' > "$WORK/t12/TODO.md"
roda_trav "$WORK/t12.out" "$WORK/t12"
vio=$(campo "$WORK/t12.out" violacoes)
falha=0; [ "$vio" -eq 0 ] || falha=1; formato_trav "$WORK/t12.out" || falha=1
veredito "T12 travessao TODO.md com U+2014 passa (interno)" 0 "$RC" "$falha" "violacoes=$vio"

# T13: docs internos (docs/superpowers e docs/pesquisa) com U+2014 passam
raiz_limpa t13; mkdir -p "$WORK/t13/docs/superpowers" "$WORK/t13/docs/pesquisa"
printf 'Plano \xe2\x80\x94 interno\n' > "$WORK/t13/docs/superpowers/plano.md"
printf 'Fonte \xe2\x80\x94 interna\n' > "$WORK/t13/docs/pesquisa/fonte.md"
roda_trav "$WORK/t13.out" "$WORK/t13"
vio=$(campo "$WORK/t13.out" violacoes)
falha=0; [ "$vio" -eq 0 ] || falha=1; formato_trav "$WORK/t13.out" || falha=1
veredito "T13 travessao docs internos passam (superpowers e pesquisa)" 0 "$RC" "$falha" "violacoes=$vio"

# T18: docs/design (documentação interna de desenvolvimento) com U+2014 PASSA
raiz_limpa t18; mkdir -p "$WORK/t18/docs/design"
printf 'Spec \xe2\x80\x94 interna\n' > "$WORK/t18/docs/design/x.md"
roda_trav "$WORK/t18.out" "$WORK/t18"
vio=$(campo "$WORK/t18.out" violacoes); docs=$(campo "$WORK/t18.out" encontrados_docs)
falha=0; [ "$vio" -eq 0 ] && [ "$docs" -eq 1 ] || falha=1; formato_trav "$WORK/t18.out" || falha=1
veredito "T18 travessao docs/design (interno) com U+2014 passa" 0 "$RC" "$falha" "violacoes=$vio encontrados_docs=$docs"

# T19: documentação interna de origem, auditoria e ADR (docs/origem, docs/auditoria, docs/adr) com U+2014 PASSA
raiz_limpa t19; mkdir -p "$WORK/t19/docs/origem" "$WORK/t19/docs/auditoria" "$WORK/t19/docs/adr"
printf 'Brief \xe2\x80\x94 texto de origem\n' > "$WORK/t19/docs/origem/x.md"
printf 'Achado \xe2\x80\x94 auditoria\n' > "$WORK/t19/docs/auditoria/y.md"
printf 'Decisao \xe2\x80\x94 ADR\n' > "$WORK/t19/docs/adr/z.md"
roda_trav "$WORK/t19.out" "$WORK/t19"
vio=$(campo "$WORK/t19.out" violacoes)
falha=0; [ "$vio" -eq 0 ] || falha=1; formato_trav "$WORK/t19.out" || falha=1
veredito "T19 travessao docs/origem, docs/auditoria e docs/adr (internos) passam" 0 "$RC" "$falha" "violacoes=$vio"

# T20: a exclusão não vaza para o README: README.md com U+2014 reprova mesmo com docs/origem presente
raiz_limpa t20; mkdir -p "$WORK/t20/docs/origem"
printf 'Brief \xe2\x80\x94 texto de origem\n' > "$WORK/t20/docs/origem/x.md"
printf 'Projeto \xe2\x80\x94 README\n' >> "$WORK/t20/README.md"
roda_trav "$WORK/t20.out" "$WORK/t20"
vio=$(campo "$WORK/t20.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t20.out" || falha=1
veredito "T20 travessao README com U+2014 reprova com docs/origem presente" 1 "$RC" "$falha" "violacoes=$vio"

# T14: docs voltados ao usuário com U+2014 reprovam
raiz_limpa t14; mkdir -p "$WORK/t14/docs"
printf 'Guia \xe2\x80\x94 usuario\n' > "$WORK/t14/docs/guia.md"
roda_trav "$WORK/t14.out" "$WORK/t14"
vio=$(campo "$WORK/t14.out" violacoes)
falha=0; [ "$vio" -ge 1 ] || falha=1; formato_trav "$WORK/t14.out" || falha=1
veredito "T14 travessao negativo (docs/guia.md com U+2014)" 1 "$RC" "$falha" "violacoes=$vio"

# T15: piso DOCS: raiz sem README e sem docs reprova
rm -rf "$WORK/t15"; mkdir -p "$WORK/t15/apps/gui"
cp "$FIXTURES/limpo.qml" "$WORK/t15/apps/gui/Main.qml" > /dev/null 2>&1
roda_trav "$WORK/t15.out" "$WORK/t15"
docs=$(campo "$WORK/t15.out" encontrados_docs)
falha=0; [ "$docs" -eq 0 ] || falha=1; formato_trav "$WORK/t15.out" || falha=1
veredito "T15 travessao piso DOCS (sem README, so UI)" 1 "$RC" "$falha" "encontrados_docs=$docs"

# T16: piso UI desligado explicitamente (--piso-ui=off): raiz só com README passa
rm -rf "$WORK/t16"; mkdir -p "$WORK/t16"; printf 'Projeto de teste\n' > "$WORK/t16/README.md"
roda_trav "$WORK/t16.out" --piso-ui=off "$WORK/t16"
ui=$(campo "$WORK/t16.out" encontrados_ui)
falha=0; [ "$ui" -eq 0 ] || falha=1; formato_trav "$WORK/t16.out" || falha=1
/usr/bin/grep -q 'piso_ui_ligado=False' "$WORK/t16.out" || falha=1
veredito "T16 travessao piso UI desligado explicitamente (--piso-ui=off, so README)" 0 "$RC" "$falha" "encontrados_ui=$ui piso=off-explicito"

# T16b: a configuração real (piso_ui_ligado=true desde a NL-1.8) reprova raiz só com README: o piso vem da configuração
roda_trav "$WORK/t16b.out" "$WORK/t16"
ui=$(campo "$WORK/t16b.out" encontrados_ui)
falha=0; [ "$ui" -eq 0 ] || falha=1; formato_trav "$WORK/t16b.out" || falha=1
/usr/bin/grep -q 'piso_ui_ligado=True' "$WORK/t16b.out" || falha=1
veredito "T16b travessao piso UI ligado pela configuracao (so README)" 1 "$RC" "$falha" "encontrados_ui=$ui piso=configuracao"

# T17: o mesmo caso com --piso-ui=on reprova: o interruptor da configuração é real
rm -rf "$WORK/t17"; mkdir -p "$WORK/t17"; printf 'Projeto de teste\n' > "$WORK/t17/README.md"
roda_trav "$WORK/t17.out" --piso-ui=on "$WORK/t17"
ui=$(campo "$WORK/t17.out" encontrados_ui)
falha=0; [ "$ui" -eq 0 ] || falha=1; formato_trav "$WORK/t17.out" || falha=1
veredito "T17 travessao piso UI ligado (--piso-ui=on, so README)" 1 "$RC" "$falha" "encontrados_ui=$ui"

echo "== Segredos (higiene_segredos.sh, gitleaks dir, regra senha-literal)"

# S1: árvore limpa do repositório passa
nova_arvore s1
roda_seg "$WORK/s1.out" "$WORK/s1"
enc=$(campo "$WORK/s1.out" encontrados)
glrc=$(campo "$WORK/s1.out" gitleaks_rc)
falha=0; [ "$enc" -ge 1 ] && [ "$glrc" -eq 0 ] || falha=1; formato_seg "$WORK/s1.out" || falha=1
veredito "S1 segredos positivo (arvore limpa, com senha-literal)" 0 "$RC" "$falha" "encontrados=$enc gitleaks_rc=$glrc"
echo "   avisos do gitleaks sobre a config (linhas com WRN/warn/deprec, se houver):"
/usr/bin/grep -iE 'wrn|warn|deprec' "$WORK/s1.out" | sed 's/^/   | /' || true

# S2: chave privada falsa em apps/ reprova
nova_arvore s2; cp "$WORK/chave_falsa.pem" "$WORK/s2/apps/gui/" > /dev/null 2>&1
roda_seg "$WORK/s2.out" "$WORK/s2"
glrc=$(campo "$WORK/s2.out" gitleaks_rc)
falha=0; [ "$CHAVE_RC" -eq 0 ] && [ "$glrc" -eq 1 ] || falha=1; formato_seg "$WORK/s2.out" || falha=1
veredito "S2 segredos negativo (chave privada falsa em apps/)" 1 "$RC" "$falha" "gitleaks_rc=$glrc (1 = vazamento achado, nao erro)"
echo "   saida do gitleaks no S2, redigida (primeiras linhas):"
sed -n '1,4p' "$WORK/s2.out" | sed 's/^/   | /'

# S3: chave NOVA gerada na hora, em tests/fixtures/higiene, REPROVA (sem allowlist de caminho)
nova_arvore s3; mkdir -p "$WORK/s3/tests/fixtures/higiene"
cp "$WORK/chave_falsa.pem" "$WORK/s3/tests/fixtures/higiene/" > /dev/null 2>&1
roda_seg "$WORK/s3.out" "$WORK/s3"
glrc=$(campo "$WORK/s3.out" gitleaks_rc)
falha=0; [ "$CHAVE_RC" -eq 0 ] && [ "$glrc" -eq 1 ] || falha=1; formato_seg "$WORK/s3.out" || falha=1
veredito "S3 chave nova em tests/fixtures/higiene REPROVA (sem allowlist)" 1 "$RC" "$falha" "gitleaks_rc=$glrc"

# S4: senha-literal em CMake (set) dentro de tests/fixtures/higiene reprova
nova_arvore s4; mkdir -p "$WORK/s4/tests/fixtures/higiene"
printf 'set(SENHA_BANCO "%s")\n' segredo123 > "$WORK/s4/tests/fixtures/higiene/CMakeLists.txt"
roda_seg "$WORK/s4.out" "$WORK/s4"
glrc=$(campo "$WORK/s4.out" gitleaks_rc)
falha=0; [ "$glrc" -eq 1 ] || falha=1; formato_seg "$WORK/s4.out" || falha=1
regra_achada "$WORK/s4" senha-literal || falha=1
veredito "S4 senha-literal em CMake set() dentro de fixture REPROVA" 1 "$RC" "$falha" "gitleaks_rc=$glrc regra=senha-literal"

# S5: senha-literal em CMake set() em apps/ reprova (controle vermelho da regra nova)
nova_arvore s5; mkdir -p "$WORK/s5/apps/cli"
printf 'set(SENHA_BANCO "%s")\n' segredo123 > "$WORK/s5/apps/cli/CMakeLists.txt"
roda_seg "$WORK/s5.out" "$WORK/s5"
glrc=$(campo "$WORK/s5.out" gitleaks_rc)
falha=0; [ "$glrc" -eq 1 ] || falha=1; formato_seg "$WORK/s5.out" || falha=1
regra_achada "$WORK/s5" senha-literal || falha=1
veredito "S5 senha-literal em CMake set() em apps/ REPROVA" 1 "$RC" "$falha" "gitleaks_rc=$glrc regra=senha-literal"

# S6: senha-literal em C++ (password = "...") reprova
nova_arvore s6; mkdir -p "$WORK/s6/apps/cli"
printf 'const char* password = "%s";\n' hunter2hunter2 > "$WORK/s6/apps/cli/main.cpp"
roda_seg "$WORK/s6.out" "$WORK/s6"
glrc=$(campo "$WORK/s6.out" gitleaks_rc)
falha=0; [ "$glrc" -eq 1 ] || falha=1; formato_seg "$WORK/s6.out" || falha=1
regra_achada "$WORK/s6" senha-literal || falha=1
veredito "S6 senha-literal em C++ REPROVA" 1 "$RC" "$falha" "gitleaks_rc=$glrc regra=senha-literal"

# S7: varredura vazia (zero arquivos) reprova, sem chamar o gitleaks
mkdir -p "$WORK/s7"
roda_seg "$WORK/s7.out" "$WORK/s7"
enc=$(campo "$WORK/s7.out" encontrados)
falha=0; [ "$enc" -eq 0 ] || falha=1; formato_seg "$WORK/s7.out" || falha=1
veredito "S7 segredos varredura vazia" 1 "$RC" "$falha" "encontrados=$enc"

echo "== Chave privada commitada (test_certificados.py, gitleaks git num repositorio temporario)"

# roda_s8 ARQUIVO_TESTE SAIDA: roda o teste de chave privada commitada e decide o veredito DESTE wrapper.
# Passa (VEREDITO_S8=0) só se: RC 0, a linha OK e exatamente um caso rodado ("Ran 1 test"). O RC sozinho não basta:
# um teste que falha e sai com 0, ou uma suíte vazia (Ran 0), não podem passar (L-45).
roda_s8() {
    local saida="$WORK/$2.out" falha=0
    python3 -I "$1" nenhumaChavePrivadaRastreadaNoGit > "$saida" 2>&1
    RC_S8=$?
    [ "$RC_S8" -eq 0 ] || falha=1
    grep -qx 'OK' "$saida" || falha=1
    grep -q '^Ran 1 test' "$saida" || falha=1
    VEREDITO_S8=$falha
    DETALHE_S8="$(tail -n 1 "$saida")"
}

# S8: o teste real, na árvore, tem de passar pelo wrapper
TESTE_SEG="$RAIZ_REPO/tests/fixtures/certificados/test_certificados.py"
roda_s8 "$TESTE_SEG" s8
veredito "S8 chave privada commitada reprovada pelo gitleaks git (teste de certificados)" 0 "$VEREDITO_S8" 0 "$DETALHE_S8"

# S9a (controle negativo do wrapper): fixture cujo teste FALHA (espera segredo onde o portão acha, invertido).
# O wrapper tem de reprovar. A guarda garante que a fixture foi de fato alterada.
nova_arvore s9a
ARQ_S9A="$WORK/s9a/tests/fixtures/certificados/test_certificados.py"
sed -i 's/self.assertEqual(portao(com_chave), 1,/self.assertEqual(portao(com_chave), 0,/' "$ARQ_S9A"
[ "$(/usr/bin/grep -c 'portao(com_chave), 0,' "$ARQ_S9A")" -eq 1 ] || VEREDITO_S8=99
roda_s8 "$ARQ_S9A" s9a
veredito "S9a wrapper reprova teste que falha (controle negativo)" 1 "$VEREDITO_S8" 0 "$DETALHE_S8"

# S9b (controle negativo do wrapper): fixture com suíte VAZIA (Ran 0 tests, OK, RC 0). O wrapper tem de reprovar.
nova_arvore s9b
ARQ_S9B="$WORK/s9b/tests/fixtures/certificados/test_certificados.py"
sed -i 's/    suite = unittest.TestSuite(\[classe(nome)\])/    suite = unittest.TestSuite([])/' "$ARQ_S9B"
[ "$(/usr/bin/grep -c 'TestSuite(\[\])' "$ARQ_S9B")" -eq 1 ] || VEREDITO_S8=99
roda_s8 "$ARQ_S9B" s9b
veredito "S9b wrapper reprova suite vazia Ran 0 (controle negativo)" 1 "$VEREDITO_S8" 0 "$DETALHE_S8"

echo "FALHAS=$FALHAS"
[ "$FALHAS" -eq 0 ] || exit 1
echo "AUTOTESTE: todos os controles conferem"
exit 0
