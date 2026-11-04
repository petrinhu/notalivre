#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da conferência do repositório OSTree que o flatpak-builder gera e de onde sai o bundle (NL-3.2a, revisão A2).
# TDD do portão (L-35, L-36): cada caso foi visto VERMELHO antes da implementação. Os repositórios são de fixture,
# criados aqui com ostree init e commit em TMPDIR e apagados no fim. Roda no contêiner notalivre/flatpak-builder-fedora44:nl-0.5,
# que traz o ostree; o autoteste não executa nada do app.
#   R1 repositório sem o app (files/bin/notalivre-gui): reprova e cita o app.
#   R2 repositório sem files/lib/libsodium.so.26: reprova e cita a biblioteca.
#   R3 repositório sem a licença da libsodium (libsodium/LICENSE): reprova e cita a licença.
#   R4 ref que não existe: reprova (rc diferente de zero).
#   R5 commit sem arquivo nenhum: a varredura acha zero; reprova com analisados=0.
#   G1 controle positivo: app, biblioteca e licença presentes; aprova com analisados=3.
# Uso: autoteste_conferencia_repo_flatpak.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONFERE="$RAIZ/tools/conteiner/conferencia_repo_flatpak.sh"
ID="io.github.petrinhu.notalivre"
REF="app/$ID/x86_64/master"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-repo.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# montar_repo <nome> <app 0|1> <lib 0|1> <lic 0|1>: monta a árvore de fixture e faz o commit na ref.
montar_repo() {
    local nome="$1" arv="$BASE/$1/arvore" repo="$BASE/$1/repo"
    mkdir -p "$arv/files/bin" "$arv/files/lib" "$arv/files/share/licenses/$ID/libsodium"
    if [ "$2" = 1 ]; then echo "app de teste" > "$arv/files/bin/notalivre-gui"; fi
    if [ "$3" = 1 ]; then echo "lib de teste" > "$arv/files/lib/libsodium.so.26"; fi
    if [ "$4" = 1 ]; then echo "licenca de teste" > "$arv/files/share/licenses/$ID/libsodium/LICENSE"; fi
    ostree --repo="$repo" init --mode=archive > /dev/null 2>&1 || return 1
    ostree --repo="$repo" commit --branch="$REF" --tree=dir="$arv" > /dev/null 2>&1
}

# montar_link <nome> <alvo 0|1>: o layout medido no build real (r2, fixture do revisor). files/lib/libsodium.so.26 é um
# link relativo para libsodium.so.26.4.0. Com alvo 0 o alvo não existe e o link fica pendurado.
montar_link() {
    local nome="$1" arv="$BASE/$1/arvore" repo="$BASE/$1/repo"
    mkdir -p "$arv/files/bin" "$arv/files/lib" "$arv/files/share/licenses/$ID/libsodium"
    echo "app de teste" > "$arv/files/bin/notalivre-gui"
    if [ "$2" = 1 ]; then echo "lib de teste" > "$arv/files/lib/libsodium.so.26.4.0"; fi
    ln -s libsodium.so.26.4.0 "$arv/files/lib/libsodium.so.26"
    echo "licenca de teste" > "$arv/files/share/licenses/$ID/libsodium/LICENSE"
    ostree --repo="$repo" init --mode=archive > /dev/null 2>&1 || return 1
    ostree --repo="$repo" commit --branch="$REF" --tree=dir="$arv" > /dev/null 2>&1
}

if [ ! -f "$CONFERE" ]; then
    for c in R1 R2 R3 R4 R5 R6 G1 G2; do falha "$c conferencia_repo_flatpak.sh ausente"; done
else
    montar_repo r1 0 1 1 && montar_repo r2 1 0 1 && montar_repo r3 1 1 0 && montar_repo r5 0 0 0 && montar_repo g1 1 1 1 \
        && montar_link g2 1 && montar_link r6 0 \
        || { echo "FIXTURE: falha ao criar os repositórios com ostree"; FALHAS=$((FALHAS + 1)); }

    # R1: sem o app, rc diferente de zero e o app citado.
    saida="$(bash "$CONFERE" "$BASE/r1/repo" "$REF" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"/files/bin/notalivre-gui"* ]] && [[ "$saida" == *"AUSENTE"* ]]; then
        ok "R1 sem o app: reprova (rc=$rc) e cita /files/bin/notalivre-gui"
    else
        falha "R1 sem o app NÃO reprovou pelo motivo certo (rc=$rc): $saida"
    fi

    # R2: sem a biblioteca, rc diferente de zero e a biblioteca citada.
    saida="$(bash "$CONFERE" "$BASE/r2/repo" "$REF" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"/files/lib/libsodium.so.26"* ]] && [[ "$saida" == *"AUSENTE"* ]]; then
        ok "R2 sem a biblioteca: reprova (rc=$rc) e cita libsodium.so.26"
    else
        falha "R2 sem a biblioteca NÃO reprovou pelo motivo certo (rc=$rc): $saida"
    fi

    # R3: sem a licença da libsodium, rc diferente de zero e libsodium/LICENSE citada.
    saida="$(bash "$CONFERE" "$BASE/r3/repo" "$REF" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"libsodium/LICENSE"* ]] && [[ "$saida" == *"AUSENTE"* ]]; then
        ok "R3 sem a licença da libsodium: reprova (rc=$rc) e cita libsodium/LICENSE"
    else
        falha "R3 sem a licença da libsodium NÃO reprovou pelo motivo certo (rc=$rc): $saida"
    fi

    # R4: ref inexistente, rc diferente de zero.
    saida="$(bash "$CONFERE" "$BASE/g1/repo" "app/nao.existe/x86_64/master" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ]; then ok "R4 ref inexistente: reprova (rc=$rc)"
    else falha "R4 ref inexistente NÃO reprovou (rc=$rc): $saida"; fi

    # R5: commit sem arquivo, a varredura acha zero e reprova com analisados=0.
    saida="$(bash "$CONFERE" "$BASE/r5/repo" "$REF" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"analisados=0"* ]]; then
        ok "R5 repositório vazio: reprova (rc=$rc) com analisados=0"
    else
        falha "R5 repositório vazio NÃO reprovou com analisados=0 (rc=$rc): $saida"
    fi

    # R6: a biblioteca é um link pendurado (o alvo não existe na ref): reprova e diz que o link está pendurado.
    saida="$(bash "$CONFERE" "$BASE/r6/repo" "$REF" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"link pendurado"* ]] && [[ "$saida" == *"/files/lib/libsodium.so.26"* ]]; then
        ok "R6 link pendurado: reprova (rc=$rc) e diz link pendurado"
    else
        falha "R6 link pendurado NÃO reprovou pelo motivo certo (rc=$rc): $saida"
    fi

    # G2: o layout real do build (link válido para o alvo da mesma árvore), rc zero e analisados=3.
    saida="$(bash "$CONFERE" "$BASE/g2/repo" "$REF" 2>&1)"; rc=$?
    if [ "$rc" -eq 0 ] && [[ "$saida" == *"analisados=3 falharam=0"* ]]; then
        ok "G2 link válido para o alvo da mesma árvore: aprova (rc=$rc) com analisados=3"
    else
        falha "G2 link válido reprovou (rc=$rc): $saida"
    fi

    # G1: controle positivo, rc zero, analisados=3 e falharam=0.
    saida="$(bash "$CONFERE" "$BASE/g1/repo" "$REF" 2>&1)"; rc=$?
    if [ "$rc" -eq 0 ] && [[ "$saida" == *"analisados=3 falharam=0"* ]]; then
        ok "G1 repositório completo: aprova (rc=$rc) com analisados=3"
    else
        falha "G1 repositório completo reprovou (rc=$rc): $saida"
    fi
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CONFERENCIA REPO FLATPAK: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CONFERENCIA REPO FLATPAK: REPROVADO"
exit 1
