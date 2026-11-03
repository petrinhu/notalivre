#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Medição da senha por descritor dentro do Flatpak, no job smoke-host do CI (NL-2.27, passo 6; DA-2.16, DA-2.68, DA-2.71). Mede o que
# a máquina do líder não pode ser usada para medir: a versão do flatpak instalado, o tipo do descritor 3 herdado pelo job, o grupo
# de processo da CLI dentro do sandbox, o repasse do descritor 3 pelo `flatpak run` e a CLI sem descritor sob um pseudo-terminal
# real. Só executa o programa já instalado em --user. Não lê nenhum segredo: o descritor da prova é um arquivo sintético, recusado
# antes de qualquer leitura. Antes das medidas, grava no sandbox um config.ini mínimo de Produção Restrita, um certificado de faz de
# conta (arquivo sintético, nunca um A1 real) e uma DPS vazia `{}`: sem isso a CLI cai na configuração antes do canal da senha.
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

# Tipo do descritor 3 que o processo herdou (DA-2.71): lido do /proc, sem abrir nem ler o descritor. Só o tipo sai, nunca conteúdo.
tipo_bruto_fd3() {
    command -v stat >/dev/null 2>&1 || { echo indisponivel; return 0; }
    stat -L -c %F /proc/self/fd/3 2>/dev/null || echo fechado
}

# Traduz o %F do stat para o nome do tipo que a medida registra. Um pipe aparece como fifo no stat do Linux.
classifica_fd3() {
    case "$1" in
        ""|indisponivel|fechado) echo "${1:-sem_resposta}" ;;
        fifo) echo fifo ;;
        socket) echo socket ;;
        "regular file"|"regular empty file") echo arquivo_comum ;;
        "character special file"|"block special file") echo dispositivo ;;
        directory) echo diretorio ;;
        *) echo outro ;;
    esac
}

# (a) versão do flatpak instalado: a armadilha medida depende dela (INFERÊNCIA A e B do ordem).
VERSAO=$("$FLATPAK" --version 2>&1) || VERSAO="indisponivel"
echo "MEDIDA: flatpak_versao=$VERSAO"

# (a2) tipo do descritor 3 herdado pelo job (DA-2.71), medido antes de qualquer redirecionamento e sem fechá-lo: fora do sandbox e
# dentro, pelo mesmo `flatpak run` que a medida (c) usa, sem redirecionar nada.
FD3_FORA=$(classifica_fd3 "$(tipo_bruto_fd3)")
FD3_DENTRO=$(classifica_fd3 "$(com_teto "$FLATPAK" run --user --command=sh "$APP" -c 'command -v stat >/dev/null 2>&1 || { echo indisponivel; exit 0; }; stat -L -c %F /proc/self/fd/3 2>/dev/null || echo fechado')")
echo "MEDIDA: fd3_herdado_tipo_fora=$FD3_FORA tipo_dentro_sandbox=$FD3_DENTRO"

# (a3) preparo do sandbox: config.ini mínimo de Produção Restrita, certificado de faz de conta e DPS `{}` no XDG_CONFIG_HOME do app.
# Tudo sintético: o caminho do certificado não existe de verdade e nada aponta para dado do líder. O config.ini sai em 0600 (BAIXA-S3).
preparar_sandbox() {
    com_teto "$FLATPAK" run --user --command=sh "$APP" -s <<'PREP'
umask 077
d="${XDG_CONFIG_HOME:-$HOME/.config}/notalivre"
mkdir -p "$d" || exit 1
printf 'sintetico\n' > "$d/faz-de-conta.txt" || exit 1
printf '{}\n' > "$d/dps-faz-de-conta.json" || exit 1
cat > "$d/config.ini" <<CFG || exit 1
[sefin]
url = https://sefin.producaorestrita.nfse.gov.br
prefixo = /SefinNacional

[certificado]
caminho = $d/faz-de-conta.txt

[assinatura]
algoritmo = rsa-sha1

[emissao]
producao_liberada = false
CFG
chmod 600 "$d/config.ini" || exit 1
printf '%s\n' "$d"
PREP
}
PREP_DIR=$(preparar_sandbox)
if [ -n "$PREP_DIR" ]; then
    echo "MEDIDA: sandbox_preparado=sim"
else
    falha "não consegui preparar o sandbox (config.ini, certificado de faz de conta e DPS): a CLI não chega à conferência da senha"
fi
DPS="${PREP_DIR:-/sem-preparo}/dps-faz-de-conta.json"

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
# com cofre.descritor_legivel_por_outros. Sem descritor (3<&- fecha o 3 herdado só para este processo), a CLI recusa com
# cofre.descritor_invalido, antes de ler qualquer configuração.
ARQUIVO="$TMP/descritor-0644.txt"
printf 'sintetico\n' > "$ARQUIVO" && chmod 0644 "$ARQUIVO" || falha "não consegui criar o arquivo sintético do descritor"
EMITIR=(run --user --command=notalivre-cli "$APP" emitir "$DPS" --serie 1 --numero 1 --ambiente producao-restrita --senha-certificado-fd 3)
COM=$(com_teto "$FLATPAK" "${EMITIR[@]}" 3< "$ARQUIVO" 2>&1); RC_COM=$?
echo "MEDIDA: repasse_com_fd3_rc=$RC_COM saida=[$(printf '%s' "$COM" | tr '\n' ' ')]"
case "$COM" in
    *cofre.descritor_legivel_por_outros*) [ "$RC_COM" -eq 2 ] || falha "o descritor chegou, mas a saída não é 2 (rc=$RC_COM)" ;;
    *) falha "o descritor 3 não chegou à CLI: o flatpak não repassou o descritor (saída: $COM)" ;;
esac
SEM=$(com_teto "$FLATPAK" "${EMITIR[@]}" < /dev/null 3<&- 2>&1); RC_SEM=$?
echo "MEDIDA: repasse_sem_fd3_rc=$RC_SEM saida=[$(printf '%s' "$SEM" | tr '\n' ' ')]"
case "$SEM" in
    *cofre.descritor_invalido*) [ "$RC_SEM" -eq 2 ] || falha "sem descritor 3, a saída não é 2 (rc=$RC_SEM)" ;;
    *) falha "sem redirecionamento, a CLI não recusou o descritor 3 como inválido (saída: $SEM)" ;;
esac

# (d) CLI sem descritor sob um pseudo-terminal real: nunca pode parar. O timeout é --foreground: sem ele o timeout põe o filho
# em grupo próprio e reproduz o defeito por conta própria (passo 6, item d).
timeout --foreground "$TEMPO" script -qec "$FLATPAK run --user --command=notalivre-cli $APP emitir $DPS --serie 1 --numero 1 --ambiente producao-restrita" /dev/null > "$TMP/pty.txt" 2>&1
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
        echo "| Descritor 3 herdado pelo job (fora / dentro do sandbox) | $FD3_FORA / $FD3_DENTRO |"
        echo "| Grupos, dentro do sandbox | $DENTRO |"
        echo "| Repasse com fd 3 (rc $RC_COM) | $(printf '%s' "$COM" | tr '\n' ' ') |"
        echo "| Sem fd 3 (rc $RC_SEM) | $(printf '%s' "$SEM" | tr '\n' ' ') |"
        echo "| Sem descritor, pseudo-terminal (rc $RC_PTY) | $(printf '%s' "$PTY" | tr '\n' ' ') |"
        echo "| Resultado | $RESULTADO |"
    } >> "$GITHUB_STEP_SUMMARY"
fi

[ "$FALHAS" -eq 0 ]
