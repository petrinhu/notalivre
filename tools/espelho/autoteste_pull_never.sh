#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste de --pull=never (NL-3.0g, revisão de segurança, L-17 e L-36).
#
# REGRA (fechada; critério fixado antes do resultado, L-43):
#   - Coberto: toda chamada literal `docker` + um ou mais espaços ou tabulações + `run` em tools/espelho/*.sh,
#     tools/conteiner/build_local.sh, tools/conteiner/portoes_estaticos.sh, tools/mutacao/sabotar_portoes.sh e
#     .github/workflows/ci.yml, inclusive dentro de blocos `run: |` do ci.yml.
#   - Em cada ocorrência de `docker run` da chamada, --pull=never tem de ser a PRIMEIRA opção, logo depois do comando.
#     Imagem local ausente vira REPROVA, nunca download de um namespace que não é nosso.
#   - Comentário no fim da linha (# precedido de espaço ou tabulação, fora de aspas) não conta.
#   - Rótulo isento: só em arquivo .yml, só fora de bloco `run: |`, e só linha cujo início é `name:` ou `- name:`.
#     Em .sh não existe isenção. Dentro de bloco `run: |` (linhas mais indentadas que a chave run), `name:` é comando.
#   - Exceção declarada: sabotar_portoes.sh, linha 149, roda a imagem externa "$FLATHUB_KDE" por digest
#     (ghcr.io flathub-infra), que precisa de pull; é exatamente UMA (piso conferido pelo script), e só nesse arquivo,
#     só com "$FLATHUB_KDE" fora de comentário.
#   - Piso: zero chamadas encontradas é falha (varredura quebrada), nunca verde. Um único awk para todos os arquivos
#     (L-11). Antes da varredura, casos de fixture (L-36): cada um tem rc, violações e exceções esperados.
#
# LIMITE DECLARADO (não é bloqueio): o portão pega omissão por descuido, não quem tem escrita no repositório. Comando
# montado em variável ou por eval (por exemplo `D=docker; $D run`) fica FORA da varredura, porque ela lê texto e não
# executa o shell. Chamada assim precisa de revisão humana.
#
# Uso: autoteste_pull_never.sh RAIZ    (RAIZ é a árvore a varrer; sai 0 só com casos verdes e todas as chamadas com a flag ou na exceção)
set -u

RAIZ="${1:?uso: autoteste_pull_never.sh RAIZ}"
TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/pullnever-casos.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT

# Programa do awk. Não usar apóstrofo aqui: o texto vai entre aspas simples do shell.
PROG='
# Corta o comentário do fim da linha: # precedido de espaço ou tabulação, fora de aspas simples e duplas.
function sem_comentario(s,    i, c, q, n) {
    q = ""; n = length(s)
    for (i = 1; i <= n; i++) {
        c = substr(s, i, 1)
        if (q != "") { if (c == q) q = ""; continue }
        if (c == "\"" || c == "\047") { q = c; continue }
        if (c == "#" && (i == 1 || substr(s, i - 1, 1) ~ /[ \t]/)) return substr(s, 1, i - 1)
    }
    return s
}
# Largura da indentação (espaços e tabulações, um caractere cada).
function indentacao(s) { match(s, /^[ \t]*/); return RLENGTH }
# Cada docker run do comando tem --pull=never como PRIMEIRA opção, logo depois do comando.
function confere_pull(s,    rest, ok) {
    ok = 1
    while (match(s, /docker[ \t]+run/)) {
        rest = substr(s, RSTART + RLENGTH)
        if (rest !~ /^[ \t]+--pull=never([ \t]|$)/) ok = 0
        s = rest
    }
    return ok
}
FNR == 1 { acum = ""; em_bloco = 0 }
/^[ \t]*#/ { next }
{
    linha = sem_comentario($0)
    if (em_bloco && linha !~ /^[ \t]*$/ && indentacao(linha) <= bloco_ind) em_bloco = 0
    if (!em_bloco && linha ~ /^[ \t]*(- )?run:[ \t]*[|>]/) { em_bloco = 1; bloco_ind = indentacao(linha); next }
    if (!em_bloco && FILENAME ~ /[.]yml$/ && linha ~ /^[ \t]*(-[ \t]+)?name:/) next
    if (acum == "") ini = FNR
    if (linha ~ /\\[ \t]*$/) {
        sub(/\\[ \t]*$/, "", linha)
        acum = (acum == "" ? linha : acum " " linha)
        next
    }
    acum = (acum == "" ? linha : acum " " linha)
    if (acum ~ /docker[ \t]+run/) {
        total++
        if (confere_pull(acum)) {
            # ok: todo docker run do comando traz a flag como primeira opção
        } else if (FILENAME ~ excecao_arquivo && acum ~ /"[$]FLATHUB_KDE"/) {
            excecoes++
            print "EXCECAO " FILENAME ":" ini ": imagem externa por digest (cabeçalho deste autoteste)"
        } else {
            viol++
            print "VIOLACAO " FILENAME ":" ini ": " substr(acum, 1, 110)
        }
    }
    acum = ""
}
END {
    print "encontrados=" total + 0 " sem_pull_never=" viol + 0 " excecoes=" excecoes + 0
    if (total + 0 == 0) { print "REPROVA: varredura vazia, zero chamadas docker run (piso, L-36)"; exit 2 }
    if (viol + 0 > 0) { print "REPROVA: chamada docker run sem --pull=never como primeira opção"; exit 1 }
    exit 0
}
'

varre() { awk -v excecao_arquivo='sabotar_portoes[.]sh$' "$PROG" "$@"; }

FALHAS_CASOS=0
# caso NOME ARQUIVO LINHA RC VIOLACOES EXCECOES: escreve a linha num arquivo próprio e confere o que o awk devolve.
caso() {
    local nome="$1" arq="$2" linha="$3" erc="$4" evio="$5" eexc="$6" dir out rc vio exc
    dir="$TMP/$nome"
    mkdir -p "$dir" || { echo "REPROVA: caso $nome sem pasta"; exit 1; }
    printf '%s\n' "$linha" > "$dir/$arq"
    out="$(varre "$dir/$arq")"
    rc=$?
    vio="$(printf '%s\n' "$out" | grep -c '^VIOLACAO' || true)"
    exc="$(printf '%s\n' "$out" | grep -c '^EXCECAO' || true)"
    if [ "$rc" -eq "$erc" ] && [ "$vio" -eq "$evio" ] && [ "$exc" -eq "$eexc" ]; then
        echo "[VERDE] caso $nome: rc=$rc violacoes=$vio excecoes=$exc"
    else
        echo "[VERMELHO] caso $nome: rc=$rc (esperado $erc), violacoes=$vio (esperado $evio), excecoes=$exc (esperado $eexc)"
        FALHAS_CASOS=$((FALHAS_CASOS + 1))
    fi
}

# Casos. Os de controle (rc 0 ou rc 2) provam que a regra não reprova o caso bom nem varredura vazia.
caso controle-flag-primeira fixture.sh 'docker run --pull=never --rm img sh' 0 0 0
caso p5-flag-em-comentario fixture.sh 'docker run --rm img sh # --pull=never' 1 1 0
caso flag-depois-da-imagem fixture.sh 'docker run img --pull=never' 1 1 0
caso comentario-com-outra-chamada fixture.sh 'docker run img # docker run --pull=never' 1 1 0
caso excecao-em-comentario sabotar_portoes.sh 'docker run --rm img # "$FLATHUB_KDE"' 1 1 0
caso excecao-real-controle sabotar_portoes.sh 'docker run --rm --privileged -v x "$FLATHUB_KDE" cmd' 0 0 1
caso excecao-fora-do-arquivo fixture.sh 'docker run --rm -v x "$FLATHUB_KDE" cmd' 1 1 0
caso segunda-ocorrencia-sem-flag fixture.sh 'docker run --pull=never img sh && docker run img sh' 1 1 0
caso sem-docker-run fixture.sh 'echo oi' 2 0 0
caso rotulo-yaml-name-nao-e-chamada ci.yml $'      - name: "Autoteste de --pull=never nas chamadas docker run (NL-3.0g, L-17)"\n        run: docker run --pull=never img sh' 0 0 0
caso run-yaml-sem-flag-reprova ci.yml '        run: docker run img sh' 1 1 0
caso sh-solto-rotulo-com-docker-run fixture.sh 'name: x; docker run alpine true' 1 1 0
caso sh-continuacao-rotulo-com-docker-run fixture.sh $'echo a \\\nname: x; docker run alpine true' 1 1 0
caso yml-bloco-run-rotulo-ponto-virgula ci.yml $'      - name: passo\n        run: |\n          name: x; docker run alpine true' 1 1 0
caso yml-bloco-run-rotulo-sem-ponto-virgula ci.yml $'      - name: passo\n        run: |\n          name: docker run alpine true' 1 1 0
caso sh-docker-dois-espacos fixture.sh 'docker  run alpine true' 1 1 0
caso sh-docker-tabulacao fixture.sh $'docker\trun alpine true' 1 1 0
caso controle-yml-rotulo-fora-de-bloco ci.yml $'      - name: docker run\n        run: docker run --pull=never img sh' 0 0 0
caso controle-flag-dois-espacos fixture.sh 'docker  run --pull=never img sh' 0 0 0
caso controle-flag-tabulacao fixture.sh $'docker\trun --pull=never img sh' 0 0 0

if [ "$FALHAS_CASOS" -ne 0 ]; then
    echo "AUTOTESTE PULL NEVER: CASOS REPROVADOS ($FALHAS_CASOS), a varredura não roda"
    exit 1
fi
echo "casos: todos conferem"

ARQUIVOS=("$RAIZ/tools/conteiner/build_local.sh" "$RAIZ/tools/conteiner/portoes_estaticos.sh" "$RAIZ/.github/workflows/ci.yml")
for f in "$RAIZ"/tools/espelho/*.sh; do
    case "$f" in
        *autoteste_pull_never.sh) continue ;;
    esac
    ARQUIVOS+=("$f")
done
ARQUIVOS+=("$RAIZ/tools/mutacao/sabotar_portoes.sh")

out="$(varre "${ARQUIVOS[@]}")"
rc=$?
printf '%s\n' "$out"
[ "$rc" -eq 0 ] || exit "$rc"
ex="$(printf '%s\n' "$out" | sed -n 's/^encontrados=.* excecoes=\([0-9]*\)$/\1/p')"
if [ "$ex" != "1" ]; then
    echo "REPROVA: exceções declaradas fora do combinado (esperada 1, medido: ${ex:-nenhuma})"
    exit 1
fi
echo "PASSA: todas as chamadas docker run têm --pull=never como primeira opção (exceção declarada conferida)"
