#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Medição da senha por descritor dentro do Flatpak, no job smoke-host do CI (NL-2.27, passo 6; DA-2.16, DA-2.68). Mede o que
# a máquina do líder não pode ser usada para medir: a versão do flatpak instalado, o grupo de processo da CLI dentro do sandbox,
# o repasse do descritor 3 pelo `flatpak run` e a CLI sem descritor sob um pseudo-terminal real. Só executa o programa já
# instalado em --user. Não lê nenhum segredo: o descritor da prova é um arquivo sintético, recusado antes de qualquer leitura.
#
# Uso: senha_fd.sh <app-id>
# Variáveis: NL_FLATPAK_BIN (padrão flatpak; o autoteste troca por um stub); NL_SENHA_FD_TIMEOUT (segundos, padrão 60) para cada
# chamada que pode parar; RUNNER_TEMP e GITHUB_STEP_SUMMARY quando existirem.
# Saída: uma linha por medida, com prefixo MEDIDA:, e a linha final RESULTADO. Sai com 1 se alguma afirmação falhar.
set -u

APP="${1:?uso: senha_fd.sh <app-id>}"
# Pré-requisitos com motivo próprio: sem script (util-linux) não há pseudo-terminal para medir, e a medição não pode parecer falha
# do flatpak (L-36: reprovar pelo motivo certo).
for ferramenta in script timeout; do
    command -v "$ferramenta" >/dev/null 2>&1 || { echo "FALHA: falta o utilitário '$ferramenta' (util-linux/coreutils): a medição não roda"; exit 1; }
done
FLATPAK="${NL_FLATPAK_BIN:-flatpak}"
TEMPO="${NL_SENHA_FD_TIMEOUT:-60}"
TMP="${RUNNER_TEMP:-${TMPDIR:-/var/tmp}}/senha-fd"
mkdir -p "$TMP" || { echo "FALHA: não consegui criar $TMP"; exit 1; }
FALHAS=0

falha() {
    echo "FALHA: $*"
    FALHAS=$(( FALHAS + 1 ))
}

# Chamada com teto de tempo: a CLI parada (SIGTTOU) não pode prender o job (passo 6, item d).
com_teto() {
    timeout --foreground "$TEMPO" "$@"
}

# Grupos de processo, lidos de /proc/<pid>/stat dentro do próprio processo medido (campos 1, 4, 5, 6 e 8).
GRUPOS='set -- $(cat /proc/$$/stat); echo "pid=$1 ppid=$4 pgid=$5 sid=$6 tpgid=$8"'

# (a) versão do flatpak instalado: a armadilha medida depende dela (INFERÊNCIA A e B do ordem).
VERSAO=$("$FLATPAK" --version 2>&1) || VERSAO="indisponivel"
echo "MEDIDA: flatpak_versao=$VERSAO"

# (b) grupos de processo: do shell de fora do sandbox e do shell de dentro, sob um pseudo-terminal real (script -qec).
FORA=$(com_teto script -qec "sh -c '$GRUPOS'" /dev/null 2>&1 | tr -d '\r')
echo "MEDIDA: grupos_fora=[$FORA]"
DENTRO=$(com_teto script -qec "$FLATPAK run --user --command=sh $APP -c '$GRUPOS'" /dev/null 2>&1 | tr -d '\r')
echo "MEDIDA: grupos_dentro=[$DENTRO]"
case "$DENTRO" in
    *pid=*tpgid=*) ;;
    *) falha "não consegui ler os grupos de processo dentro do sandbox: [$DENTRO]" ;;
esac

# (c) prova diferencial do repasse do descritor 3. Um arquivo 0644 sintético: se o descritor chega, a CLI o inspeciona e recusa
# com cofre.descritor_legivel_por_outros; sem redirecionamento, o descritor 3 não está aberto (cofre.descritor_invalido).
ARQUIVO="$TMP/descritor-0644.txt"
printf 'sintetico\n' > "$ARQUIVO" && chmod 0644 "$ARQUIVO" || falha "não consegui criar o arquivo sintético do descritor"
EMITIR=(run --user --command=notalivre-cli "$APP" emitir /dev/null --serie 1 --numero 1 --ambiente producao-restrita --senha-certificado-fd 3)
COM=$(com_teto "$FLATPAK" "${EMITIR[@]}" 3< "$ARQUIVO" 2>&1); RC_COM=$?
echo "MEDIDA: repasse_com_fd3_rc=$RC_COM saida=[$(printf '%s' "$COM" | tr '\n' ' ')]"
case "$COM" in
    *cofre.descritor_legivel_por_outros*) [ "$RC_COM" -eq 2 ] || falha "o descritor chegou, mas a saída não é 2 (rc=$RC_COM)" ;;
    *) falha "o descritor 3 não chegou à CLI: o flatpak não repassou o descritor (saída: $COM)" ;;
esac
SEM=$(com_teto "$FLATPAK" "${EMITIR[@]}" < /dev/null 2>&1); RC_SEM=$?
echo "MEDIDA: repasse_sem_fd3_rc=$RC_SEM saida=[$(printf '%s' "$SEM" | tr '\n' ' ')]"
case "$SEM" in
    *cofre.descritor_invalido*) [ "$RC_SEM" -eq 2 ] || falha "sem descritor 3, a saída não é 2 (rc=$RC_SEM)" ;;
    *) falha "sem redirecionamento, a CLI não recusou o descritor 3 como inválido (saída: $SEM)" ;;
esac

# (d) CLI sem descritor sob um pseudo-terminal real: nunca pode parar. O timeout é --foreground: sem ele o timeout põe o filho
# em grupo próprio e reproduz o defeito por conta própria (passo 6, item d).
timeout --foreground "$TEMPO" script -qec "$FLATPAK run --user --command=notalivre-cli $APP emitir /dev/null --serie 1 --numero 1 --ambiente producao-restrita" /dev/null > "$TMP/pty.txt" 2>&1
RC_PTY=$?
PTY=$(tr -d '\r' < "$TMP/pty.txt")
echo "MEDIDA: sem_descritor_pty_rc=$RC_PTY saida=[$(printf '%s' "$PTY" | tr '\n' ' ')]"
[ "$RC_PTY" -ne 124 ] || falha "a CLI sem descritor ficou presa no pseudo-terminal até o teto (rc 124): parou"
case "$PTY" in
    *cofre.*) ;;
    *) falha "a CLI sem descritor não deu um código cofre.* no pseudo-terminal (rc=$RC_PTY)" ;;
esac

if [ "$FALHAS" -eq 0 ]; then
    RESULTADO="ok"
else
    RESULTADO="reprovado ($FALHAS)"
fi
echo "RESULTADO: senha_fd $RESULTADO"

if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
        echo "## Senha por descritor no sandbox (NL-2.27, passo 6)"
        echo "| Medida | Valor |"
        echo "| --- | --- |"
        echo "| Versão do flatpak | $VERSAO |"
        echo "| Grupos, dentro do sandbox | $DENTRO |"
        echo "| Repasse com fd 3 (rc $RC_COM) | $(printf '%s' "$COM" | tr '\n' ' ') |"
        echo "| Sem fd 3 (rc $RC_SEM) | $(printf '%s' "$SEM" | tr '\n' ' ') |"
        echo "| Sem descritor, pseudo-terminal (rc $RC_PTY) | $(printf '%s' "$PTY" | tr '\n' ' ') |"
        echo "| Resultado | $RESULTADO |"
    } >> "$GITHUB_STEP_SUMMARY"
fi

[ "$FALHAS" -eq 0 ]
