#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de histórico (NL-3.E, L-36: portão só conta depois de visto reprovar).
# Os repositórios são criados na hora, em TMPDIR; nenhuma chave ou fixture de segredo fica versionada.
# Controles:
#   H1 .git-arquivo pendurado (o defeito medido: gitleaks com rc 0 e "0 commits scanned") reprova por piso;
#   H2 repositório sem commit (zero commits lidos) reprova por piso;
#   H3 repositório com 1 commit limpo passa e informa commits_lidos=1;
#   H4 chave privada commitada reprova e o rc 1 do gitleaks chega ao veredito;
#   H5 clone raso (--depth 1) com commits reprova: o piso não pega histórico incompleto (NL-3.E, revisão).
# Uso: autoteste_historico.sh   (sem argumentos; roda no contêiner de higiene)
set -u

DIR="$(cd "$(dirname "$0")" && pwd)"
PORTAO="$DIR/higiene_historico.sh"
TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/historico-autoteste.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT
FALHAS=0
CFG='[extend]
useDefault = true'

# controle NOME RC_ESPERADO REGEX RAIZ: roda o portão e exige rc EXATO e a linha esperada.
controle() {
    local nome="$1" rc_esp="$2" regex="$3" raiz="$4" saida rc
    saida="$TMP/$nome.out"
    sh "$PORTAO" "$raiz" > "$saida" 2>&1
    rc=$?
    if [ "$rc" -eq "$rc_esp" ] && grep -qE "$regex" "$saida"; then
        echo "[VERDE] $nome: rc=$rc (esperado $rc_esp)"
    else
        echo "[VERMELHO] $nome: rc=$rc (esperado $rc_esp), saída:"
        sed 's/^/   | /' "$saida"
        FALHAS=$((FALHAS + 1))
    fi
}

# H1: .git-arquivo apontando para fora da árvore (o que a cópia de worktree deixava para o gitleaks).
H1="$TMP/h1"; mkdir -p "$H1"
printf 'gitdir: /nao/existe/.git/worktrees/x\n' > "$H1/.git"
printf '%s\n' "$CFG" > "$H1/.gitleaks.toml"
echo x > "$H1/a.txt"
controle h1-gitfile-pendurado 1 'REPROVA: piso de varredura' "$H1"

# H2: repositório sem nenhum commit.
H2="$TMP/h2"; git init -q "$H2"
printf '%s\n' "$CFG" > "$H2/.gitleaks.toml"
controle h2-zero-commits 1 'REPROVA: piso de varredura' "$H2"

# H3: repositório com 1 commit limpo.
H3="$TMP/h3"; git init -q "$H3"
printf '%s\n' "$CFG" > "$H3/.gitleaks.toml"
echo limpo > "$H3/a.txt"
git -C "$H3" add a.txt .gitleaks.toml
git -C "$H3" -c user.name=autoteste -c user.email=autoteste@invalid -c core.hooksPath=/dev/null \
    commit -q -m "test: fixture limpa" || { echo "ABORTA: commit da fixture H3"; exit 1; }
controle h3-commit-limpo 0 '^commits_lidos=1 gitleaks_rc=0$' "$H3"

# H4: chave privada commitada. O cabeçalho é montado na hora, para o texto do autoteste não casar o próprio gitleaks.
H4="$TMP/h4"; git init -q "$H4"
printf '%s\n' "$CFG" > "$H4/.gitleaks.toml"
{
    printf '%s\n' "-----BEGIN RSA ""PRIVATE KEY-----"
    printf '%s\n' "MIIBOgIBAAJBAKj34GkxFhD90vcNLYLInFEX6Ppy1tPf9Cnzj4p4WGeKLs1Pt8Qu"
    printf '%s\n' "-----END RSA ""PRIVATE KEY-----"
} > "$H4/chave.txt"
git -C "$H4" add chave.txt .gitleaks.toml
git -C "$H4" -c user.name=autoteste -c user.email=autoteste@invalid -c core.hooksPath=/dev/null \
    commit -q -m "test: fixture com chave" || { echo "ABORTA: commit da fixture H4"; exit 1; }
controle h4-chave-commitada 1 'REPROVA: gitleaks rc=1' "$H4"

# H5: clone raso (--depth 1) com commits. O piso (>= 1 commit) não pega: o histórico é incompleto e o portão tem de reprovar.
H5O="$TMP/h5-origem"; git init -q "$H5O"
echo um > "$H5O/a.txt"; git -C "$H5O" add a.txt
git -C "$H5O" -c user.name=autoteste -c user.email=autoteste@invalid -c core.hooksPath=/dev/null commit -q -m "test: origem 1" || { echo "ABORTA: commit 1 da origem H5"; exit 1; }
echo dois > "$H5O/a.txt"; git -C "$H5O" add a.txt
git -C "$H5O" -c user.name=autoteste -c user.email=autoteste@invalid -c core.hooksPath=/dev/null commit -q -m "test: origem 2" || { echo "ABORTA: commit 2 da origem H5"; exit 1; }
H5="$TMP/h5"; git clone -q --depth 1 "file://$H5O" "$H5" || { echo "ABORTA: clone raso da fixture H5"; exit 1; }
printf '%s\n' "$CFG" > "$H5/.gitleaks.toml"
controle h5-clone-raso 1 'REPROVA: repositório raso' "$H5"

echo "FALHAS=$FALHAS"
[ "$FALHAS" -eq 0 ] || { echo "AUTOTESTE HISTORICO: REPROVADO"; exit 1; }
echo "AUTOTESTE HISTORICO: todos os controles conferem"
exit 0
