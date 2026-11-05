#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste de --pull=never (NL-3.0g, revisão de segurança, L-17 e L-36): toda chamada `docker run` dos espelhos
# (tools/espelho/*.sh), de build_local.sh, de portoes_estaticos.sh, das linhas docker run do ci.yml e da sabotagem
# (tools/mutacao/sabotar_portoes.sh) tem --pull=never como PRIMEIRA opção, logo depois do comando. Imagem local ausente
# vira REPROVA, nunca download de um namespace que não é nosso. Comentário no fim da linha não conta (A8, revisão final).
# Piso: zero chamadas encontradas é falha (varredura quebrada), nunca verde. Um único awk para todos os arquivos:
# nenhum processo por item (L-11).
#
# EXCEÇÃO DECLARADA (NL-3.0g, revisão de segurança, ordem do main): sabotar_portoes.sh, linha 149, roda a imagem
# externa "$FLATHUB_KDE" por digest (ghcr.io flathub-infra), que não existe no armazenamento local e precisa de pull,
# por isso o --pull=never não se aplica a ela. A exceção é estreita: só vale para o arquivo sabotar_portoes.sh e só
# para chamada cujo texto (sem comentário) tem "$FLATHUB_KDE"; é exatamente UMA (piso, conferido pelo script).
#
# Antes da varredura, o script roda casos de fixture (L-36): cada caso tem um rc, um número de violações e um número de
# exceções esperados. Um caso que não sai como esperado reprova a varredura, para que a regra não afrouxe em silêncio.
#
# Uso: autoteste_pull_never.sh RAIZ    (RAIZ é a árvore a varrer; sai 0 só com casos verdes e todas as chamadas com a flag)
set -u

RAIZ="${1:?uso: autoteste_pull_never.sh RAIZ}"
TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/pullnever-casos.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT

# Programa do awk. Não usar apóstrofo aqui: o texto vai entre aspas simples do shell.
PROG='
FNR == 1 { acum = "" }
/^[ \t]*#/ { next }
{
    linha = $0
    if (acum == "") ini = FNR
    if (linha ~ /\\[ \t]*$/) {
        sub(/\\[ \t]*$/, "", linha)
        acum = (acum == "" ? linha : acum " " linha)
        next
    }
    acum = (acum == "" ? linha : acum " " linha)
    if (acum ~ /docker run/) {
        total++
        if (acum ~ /--pull=never/) {
            # ok: imagem local com a flag
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
    if (viol + 0 > 0) { print "REPROVA: chamada docker run sem --pull=never"; exit 1 }
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
