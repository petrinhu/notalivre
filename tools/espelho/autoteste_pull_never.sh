#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste de --pull=never (NL-3.0g, revisão de segurança, L-17 e L-36): toda chamada `docker run` dos espelhos
# (tools/espelho/*.sh), de build_local.sh, de portoes_estaticos.sh, das linhas docker run do ci.yml e da sabotagem
# (tools/mutacao/sabotar_portoes.sh) tem --pull=never. Imagem local ausente vira REPROVA, nunca download de um
# namespace que não é nosso. Piso: zero chamadas encontradas é falha (varredura quebrada), nunca verde. Um único awk
# para todos os arquivos: nenhum processo por item (L-11).
#
# EXCEÇÃO DECLARADA (NL-3.0g, revisão de segurança, ordem do main): sabotar_portoes.sh, linha 149, roda a imagem
# externa "$FLATHUB_KDE" por digest (ghcr.io flathub-infra), que não existe no armazenamento local e precisa de pull,
# por isso o --pull=never não se aplica a ela. A exceção é estreita: só vale para o arquivo sabotar_portoes.sh e só
# para chamada cujo texto tem "$FLATHUB_KDE"; é exatamente UMA (piso); qualquer outra chamada sem a flag reprova.
#
# Uso: autoteste_pull_never.sh RAIZ       (RAIZ é a árvore a varrer; sai 0 só com chamadas encontradas e todas com a flag ou na exceção)
set -u

RAIZ="${1:?uso: autoteste_pull_never.sh RAIZ}"
ARQUIVOS=("$RAIZ/tools/conteiner/build_local.sh" "$RAIZ/tools/conteiner/portoes_estaticos.sh" "$RAIZ/.github/workflows/ci.yml")
for f in "$RAIZ"/tools/espelho/*.sh; do
    case "$f" in
        *autoteste_pull_never.sh) continue ;;
    esac
    ARQUIVOS+=("$f")
done
ARQUIVOS+=("$RAIZ/tools/mutacao/sabotar_portoes.sh")

# Junta as linhas com contra-barra e confere cada comando `docker run`. Comentários (linhas com # no início) são ignorados.
# VIOLACAO e EXCECAO saem com arquivo:linha da PRIMEIRA linha física do comando.
awk -v excecao_arquivo='sabotar_portoes[.]sh$' '
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
    if (excecoes + 0 != 1) { print "REPROVA: exceções declaradas fora do combinado (esperada 1)"; exit 1 }
    if (viol + 0 > 0) { print "REPROVA: chamada docker run sem --pull=never"; exit 1 }
    print "PASSA: todas as chamadas docker run têm --pull=never (exceção declarada conferida)"
}
' "${ARQUIVOS[@]}"
