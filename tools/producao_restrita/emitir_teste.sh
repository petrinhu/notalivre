#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Emissão de teste do líder em Produção Restrita, em um comando só (NL-2.10; decisão do líder de 01/11/2026:
# "Um comando só (Recomendado)"). Faz as seções 3.1 a 3.11 e a 4 de docs/pesquisa/onda2_producao_restrita_roteiro.md:
# confere o programa, escolhe o .pfx numa janela, pede o CNPJ, monta a DPS, emite e devolve o resultado.
# A senha é digitada só no prompt da própria CLI; este script não a lê, não a guarda e não a passa por argv nem por env.
#
# Para o líder:                  bash tools/producao_restrita/emitir_teste.sh
# Ensaio (contêiner, sem flatpak e sem Sefin):   bash tools/producao_restrita/emitir_teste.sh --ensaio
#
# Fontes dos dois códigos fixos (conferidas em 01/11/2026):
#   - município de Recife, código IBGE 2611606 (UF PE). Anexo A "Município IBGE e Países ISO2", v1.00 de 10/12/2025:
#     https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_a-municipio_ibge-paises_iso2-v1-00-snnfse-20251210.xlsx
#     (SHA-256 238b715ab2dcc2c9e0857c44d69048e0af806b45ec8499568342de4a37f4419d, aba TAB.MUN_IBGE). Confirmado também em
#     https://www.ruacep.com.br/pe/recife/ibge/
#   - serviço de medicina, cTribNac 040101 ("Medicina.", item 04, subitem 01, desdobro 01). Anexo B "Lista de Serviço
#     Nacional", v1.01 de 22/01/2026:
#     https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_b-nbs2-lista_servico_nacional-snnfse-v1-01-20260122.xlsx
#     (SHA-256 e74b0be8ad204e458932efb1716225a3fb5703a84e5475ce5c4c6789d1c61b1a, aba LISTA.SERV.NAC.).
#
# Regras que o script cumpre: o caminho do .pfx nunca é impresso, copiado, listado nem gravado em log; o CNPJ não vai a
# arquivo versionado; nenhuma chamada vai à Produção; nada é apagado por este script; só há tentativa 2
# (base) se a Sefin responder 404, e tentativa 3 (algoritmo rsa-sha256) se ela responder E0714.

set -u

APP=io.github.petrinhu.notalivre
RUNTIME=org.kde.Platform//6.11
BUNDLE_SHA=8314ce15d190807f23c4b42917a4725d313329924763611fc1b16d3f669dae18
BASE=/var/tmp/notalivre-nl210
BUNDLE="$BASE/bundle/run-41279001563/extraido/notalivre.flatpak"
MUNICIPIO=2611606
CTRIBNAC=040101
SERIE=1
MODO=normal
if [ "${1:-}" = "--ensaio" ]; then MODO=ensaio; fi
if [ "${1:-}" = "--ensaio-caso" ]; then MODO=caso; fi

falha() {
    printf 'PARADA: %s\n' "$1"
    exit 1
}

# ---------------------------------------------------------------------------------------------------------------------
# Ensaio: nos casos, o script roda com HOME, BASE, bundle e PATH de faz de conta (stubs). Nunca chama o flatpak real.
# ---------------------------------------------------------------------------------------------------------------------
if [ "$MODO" = caso ]; then
    : "${NL_ENSAIO_DIR:?ensaio sem NL_ENSAIO_DIR}"
    HOME="$NL_ENSAIO_DIR/home"
    BASE="$NL_ENSAIO_DIR/base"
    BUNDLE="$BASE/bundle/run-41279001563/extraido/notalivre.flatpak"
    PATH="$NL_ENSAIO_DIR/bin:$PATH"
    export HOME PATH
    mkdir -p "$HOME" "$(dirname "$BUNDLE")"
    [ -f "$BUNDLE" ] || printf 'programa de ensaio\n' > "$BUNDLE"
    if [ "${NL_ENSAIO_BUNDLE:-}" = errado ]; then
        BUNDLE_SHA=0000000000000000000000000000000000000000000000000000000000000000
    else
        BUNDLE_SHA=$(sha256sum "$BUNDLE" | cut -d' ' -f1)
    fi
fi

TESTE_DIR="$BASE/teste-lider"
DEVOLUCAO="$BASE/devolucao"
NDPS_ARQ="$TESTE_DIR/ultimo-ndps.txt"
DPS_ARQ="$TESTE_DIR/dps-teste.json"
ESTADO_INST="$BASE/programa-instalado-sha.txt"
CFG_DIR="$HOME/.var/app/$APP/config/notalivre"
CFG="$CFG_DIR/config.ini"
RUN_TS=$(date +%Y%m%d-%H%M%S)
RUN_DIR="$TESTE_DIR/execucao-$RUN_TS"

TEM_KDIALOG=0
if command -v kdialog >/dev/null 2>&1; then TEM_KDIALOG=1; fi

PREF=/SefinNacional
ALG=rsa-sha1
BACKUP_FEITO=0
NDPS_ATUAL=0
DOC_DPS=""
DOC_PFX=""
EMITIU_RC=0
SAIDA_ATUAL=""
RESULTADO=""

# Confere um CNPJ de 14 dígitos pelos dois dígitos verificadores (pesos da Receita Federal).
cnpj_valido() {
    local c="$1" i soma d1 d2
    local -a p1=(5 4 3 2 9 8 7 6 5 4 3 2)
    local -a p2=(6 5 4 3 2 9 8 7 6 5 4 3 2)
    [[ "$c" =~ ^[0-9]{14}$ ]] || return 1
    [[ "$c" =~ ^(.)\1{13}$ ]] && return 1
    soma=0
    for i in 0 1 2 3 4 5 6 7 8 9 10 11; do soma=$(( soma + ${c:i:1} * p1[i] )); done
    d1=$(( soma % 11 ))
    if [ "$d1" -lt 2 ]; then d1=0; else d1=$(( 11 - d1 )); fi
    soma=0
    for i in 0 1 2 3 4 5 6 7 8 9 10 11 12; do soma=$(( soma + ${c:i:1} * p2[i] )); done
    d2=$(( soma % 11 ))
    if [ "$d2" -lt 2 ]; then d2=0; else d2=$(( 11 - d2 )); fi
    [ "${c:12:1}" -eq "$d1" ] && [ "${c:13:1}" -eq "$d2" ]
}

# Pergunta ao líder pela janela (kdialog) ou, sem kdialog, pelo terminal. O valor nunca é impresso nem gravado.
perguntar() {
    local texto="$1" modo="$2" resposta=""
    if [ "$TEM_KDIALOG" -eq 1 ]; then
        if [ "$modo" = pfx ]; then
            kdialog --getopenfilename "$HOME" "*.pfx *.p12|Certificado A1 (*.pfx, *.p12)" 2>/dev/null
        else
            kdialog --inputbox "$texto" 2>/dev/null
        fi
    else
        printf '%s: ' "$texto" >&2
        IFS= read -r resposta
        printf '%s' "$resposta"
    fi
}

gravar_config() {
    mkdir -p "$CFG_DIR" || return 1
    if [ -f "$CFG" ] && [ "$BACKUP_FEITO" -eq 0 ]; then
        cp -p -- "$CFG" "$CFG.antes-$RUN_TS" || return 1
        BACKUP_FEITO=1
    fi
    {
        printf '[sefin]\nurl = https://sefin.producaorestrita.nfse.gov.br\nprefixo = %s\n\n' "$PREF"
        printf '[assinatura]\nalgoritmo = %s\n\n' "$ALG"
        printf '[emissao]\nproducao_liberada = false\n'
        if [ -f "$CFG" ]; then
            awk '/^\[certificado\]/ { f = 1; print ""; print; next } /^\[/ { f = 0 } f' "$CFG"
        fi
    } > "$CFG.tmp" || return 1
    mv -f -- "$CFG.tmp" "$CFG"
}

# Próximo nDps: lê o último usado, soma 1 e grava antes de qualquer envio, para nunca repetir.
proximo_ndps() {
    local ultimo=0
    if [ -f "$NDPS_ARQ" ]; then ultimo=$(cat "$NDPS_ARQ"); fi
    case "$ultimo" in
        ''|*[!0-9]*) falha "o arquivo do último número da DPS está inválido. Nada foi emitido." ;;
    esac
    NDPS_ATUAL=$(( ultimo + 1 ))
    printf '%s\n' "$NDPS_ATUAL" > "$NDPS_ARQ"
}

# Monta a DPS do roteiro (3.8) com o número dado e libera o arquivo para o programa.
montar_dps() {
    local n="$1" dh dc
    dh="$(date -u -d '-3 hours' '+%Y-%m-%dT%H:%M:%S')-03:00"
    dc="$(date -u -d '-3 hours' '+%Y-%m-01')"
    case "$dh" in
        *-03:00) ;;
        *) falha "o relógio não deu o fuso de Recife (-03:00). Nada foi emitido." ;;
    esac
    (
        umask 077
        cat > "$DPS_ARQ" <<EOF
{
  "tpAmb": "2",
  "dhEmi": "$dh",
  "verAplic": "NotaLivre 0.1",
  "serie": $SERIE,
  "nDps": $n,
  "dCompet": "$dc",
  "cLocEmi": "$MUNICIPIO",
  "prestador": {
    "cnpj": "$CNPJ",
    "regime": { "opSimpNac": "3", "regEspTrib": "0" }
  },
  "servico": {
    "cLocPrestacao": "$MUNICIPIO",
    "cTribNac": "$CTRIBNAC",
    "xDescServ": "Medicina: teste de emissão em Produção Restrita"
  },
  "valores": {
    "vServ": "10,00",
    "tribMun": { "tribISSQN": "1", "tpRetISSQN": "1" },
    "totTrib": { "indTotTrib": "0" }
  }
}
EOF
    ) || falha "não consegui escrever a DPS de teste. Nada foi emitido."
    chmod 600 "$DPS_ARQ"
    grep -q "\"nDps\": $n," "$DPS_ARQ" && grep -q "\"serie\": $SERIE," "$DPS_ARQ" \
        || falha "a DPS não tem o --serie e o --numero desta tentativa. Nada foi emitido."
    DOC_DPS=$(flatpak document-export --app="$APP" --forbid-write "$DPS_ARQ" 2>/dev/null) || DOC_DPS=""
    case "$DOC_DPS" in
        /run/user/*) ;;
        *) falha "o programa não liberou a DPS de teste. Nada foi emitido." ;;
    esac
}

# Uma tentativa de emissão (3.9). A saída inteira vai para a pasta da execução.
emitir_tentativa() {
    local k="$1" n="$2"
    SAIDA_ATUAL="$RUN_DIR/saida-tentativa-$k.txt"
    flatpak run --command=notalivre-cli "$APP" emitir "$DOC_DPS" \
        --serie "$SERIE" --numero "$n" --ambiente producao-restrita > "$SAIDA_ATUAL" 2>&1
    EMITIU_RC=$?
}

# Classifica o resultado: aceita, 404 (rota), e0714 (algoritmo), sefin (outra recusa da Sefin) ou local (antes da Sefin).
classificar() {
    if [ "$1" -eq 0 ]; then RESULTADO=aceita; return; fi
    if awk '/"evento":"sefin.requisicao"/ && /"status":"404"/ { achou = 1 } END { exit !achou }' "$2"; then
        RESULTADO=404
        return
    fi
    if grep -q 'sefin\.E0714' "$2"; then RESULTADO=e0714; return; fi
    if grep -q 'emissão recusada (sefin\.' "$2"; then RESULTADO=sefin; return; fi
    RESULTADO=local
}

# ---------------------------------------------------------------------------------------------------------------------
# Fluxo do líder
# ---------------------------------------------------------------------------------------------------------------------
fluxo() {
    mkdir -p "$TESTE_DIR" "$DEVOLUCAO" || falha "não consegui criar a pasta de trabalho em $BASE."
    mkdir -p "$RUN_DIR" && chmod 700 "$RUN_DIR" || falha "não consegui criar a pasta desta execução."
    touch "$RUN_DIR/.inicio"
    local log_inst="$RUN_DIR/instalacao.txt"

    # 3.1: o programa tem de ser exatamente o gerado pelo CI.
    [ -f "$BUNDLE" ] || falha "não encontrei o programa em $BUNDLE. Nada foi instalado nem emitido."
    local obtido
    obtido=$(sha256sum "$BUNDLE" | cut -d' ' -f1)
    [ "$obtido" = "$BUNDLE_SHA" ] || falha "o programa não confere com o SHA-256 esperado. Nada foi instalado nem emitido."

    # 3.2: ambiente de execução e programa, só em --user.
    if ! flatpak info --user "$RUNTIME" >/dev/null 2>&1; then
        flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo >>"$log_inst" 2>&1
        flatpak install --user -y flathub "$RUNTIME" >>"$log_inst" 2>&1 \
            || falha "não consegui instalar o ambiente KDE 6.11 (registro em $log_inst)."
    fi
    if ! flatpak info --user "$APP" >/dev/null 2>&1 || [ "$(cat "$ESTADO_INST" 2>/dev/null)" != "$BUNDLE_SHA" ]; then
        flatpak install --user -y --reinstall --bundle "$BUNDLE" >>"$log_inst" 2>&1 \
            || falha "não consegui instalar o programa (registro em $log_inst)."
        printf '%s\n' "$BUNDLE_SHA" > "$ESTADO_INST"
    fi

    # CNPJ do emissor: pedido ao líder, conferido pelo dígito verificador, nunca impresso nem gravado fora da DPS.
    CNPJ=$(perguntar "Digite o CNPJ do emissor: 14 dígitos, sem pontos, barras ou traços." cnpj)
    CNPJ=${CNPJ//[[:space:]]/}
    cnpj_valido "$CNPJ" || falha "o CNPJ não tem 14 dígitos ou o dígito verificador não confere. Nada foi emitido."

    # 3.3: o .pfx é escolhido pelo líder e só passa ao document-export. Nunca é impresso nem listado.
    local pfx
    pfx=$(perguntar "Escolha o arquivo .pfx do certificado" pfx)
    [ -n "$pfx" ] && [ -f "$pfx" ] && [ -r "$pfx" ] \
        || falha "o arquivo escolhido não pôde ser lido. Nada foi emitido."
    DOC_PFX=$(flatpak document-export --app="$APP" --forbid-write "$pfx" 2>/dev/null) || DOC_PFX=""
    pfx=""
    case "$DOC_PFX" in
        /run/user/*) ;;
        *) falha "o programa não liberou o certificado. Nada foi emitido." ;;
    esac

    # 3.5: pasta de configuração esperada e arquivo de CAs do programa.
    local xdg
    xdg=$(flatpak run --command=sh "$APP" -c 'echo "$XDG_CONFIG_HOME"' 2>/dev/null)
    [ "$xdg" = "$HOME/.var/app/$APP/config" ] \
        || falha "a pasta de configuração do programa não é a esperada. Nada foi gravado nem emitido."
    flatpak run --command=ls "$APP" -l /etc/pki/tls/certs/ca-bundle.crt >/dev/null 2>&1 \
        || falha "o programa não tem o arquivo de CAs (roteiro, 3.5). Nada foi emitido."

    # 3.6 e 3.7: configuração exata e caminho do certificado pela própria CLI.
    gravar_config || falha "não consegui gravar o config.ini. Nada foi emitido."
    flatpak run --command=notalivre-cli "$APP" config certificado "$DOC_PFX" >/dev/null 2>&1 \
        || falha "não consegui gravar o certificado na configuração (roteiro, 3.7). Nada foi emitido."
    DOC_PFX=""

    # 3.8 a 3.11: número novo, DPS, emissão e as tentativas 2 e 3 só pelas condições do roteiro.
    local tentativa=0 base_trocada=0 alg_trocado=0
    while :; do
        tentativa=$(( tentativa + 1 ))
        proximo_ndps
        montar_dps "$NDPS_ATUAL"
        printf 'Tentativa %s: enviando a DPS de número %s.\n' "$tentativa" "$NDPS_ATUAL"
        emitir_tentativa "$tentativa" "$NDPS_ATUAL"
        classificar "$EMITIU_RC" "$SAIDA_ATUAL"
        case "$RESULTADO" in
            aceita)
                break ;;
            404)
                if [ "$base_trocada" -eq 0 ] && [ "$tentativa" -lt 3 ]; then
                    PREF=/API/SefinNacional
                    base_trocada=1
                    gravar_config || falha "não consegui trocar a base no config.ini. Nada mais foi enviado."
                else
                    break
                fi ;;
            e0714)
                if [ "$alg_trocado" -eq 0 ] && [ "$tentativa" -lt 3 ]; then
                    ALG=rsa-sha256
                    alg_trocado=1
                    gravar_config || falha "não consegui trocar o algoritmo no config.ini. Nada mais foi enviado."
                else
                    break
                fi ;;
            *)
                break ;;
        esac
    done

    # Resultado e devolução (seção 4).
    local dev_run="$DEVOLUCAO/execucao-$RUN_TS"
    mkdir -p "$dev_run" || falha "não consegui criar a pasta de devolução."
    cp -p -- "$RUN_DIR"/saida-tentativa-*.txt "$dev_run"/ 2>/dev/null
    find "$HOME/.var/app/$APP" -type f -path '*/respostas/*' -name '*.jsonl' -newer "$RUN_DIR/.inicio" \
        -exec cp -p -- {} "$dev_run/" \; 2>/dev/null
    if [ "$RESULTADO" = aceita ]; then
        local chave
        chave=$(grep -o 'emitida: chave [0-9]*' "$SAIDA_ATUAL" | tail -n 1 | grep -o '[0-9]*$')
        printf 'Nota de teste aceita. Chave de acesso: %s\n' "$chave"
    elif [ "$RESULTADO" = local ]; then
        printf 'A emissão parou antes de chegar à Sefin (saída %s). Avise o Claude.\n' "$EMITIU_RC"
    else
        printf 'A Sefin recusou; avise o Claude\n'
    fi
    printf 'Cópias para devolução: %s\n' "$dev_run"
    [ "$RESULTADO" = aceita ] && return 0
    return 1
}

# ---------------------------------------------------------------------------------------------------------------------
# Ensaio: stubs de flatpak e kdialog, e um driver que roda cada caso e confere o resultado.
# ---------------------------------------------------------------------------------------------------------------------
escrever_stubs() {
    local dir="$1"
    mkdir -p "$dir/bin"
    cat > "$dir/bin/flatpak" <<'STUB'
#!/bin/sh
# Stub do flatpak para o ensaio: não toca o host e não fala com a Sefin.
D="$NL_ENSAIO_DIR"
echo "flatpak $*" >> "$D/chamadas.txt"
emitir_stub() {
    local n resultado
    n=$(( $(cat "$D/emitidas.txt" 2>/dev/null || echo 0) + 1 ))
    printf '%s' "$n" > "$D/emitidas.txt"
    resultado=$(printf '%s' "${NL_ENSAIO_SEQ:-outro}" | awk -F, -v n="$n" '{ print $n }')
    [ -n "$resultado" ] || resultado=outro
    local pasta="$HOME/.var/app/io.github.petrinhu.notalivre/data/notalivre/respostas"
    mkdir -p "$pasta"
    printf '{"resposta":"ensaio %s"}\n' "$n" > "$pasta/ensaio-$n.jsonl"
    case "$resultado" in
        aceita)
            echo "emitida: chave 12345678901234567890123456789012345678901234567890"
            exit 0 ;;
        404)
            echo '{"level":"info","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"POST","status":"404"},"descartados":0}'
            echo "emissão recusada (sefin.falha): "
            exit 1 ;;
        E0714)
            echo '{"level":"info","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"POST","status":"200"},"descartados":0}'
            echo "emissão recusada (sefin.E0714): assinatura recusada"
            exit 1 ;;
        local)
            echo "certificado.caminho_ilegivel: ensaio"
            exit 2 ;;
        *)
            echo "emissão recusada (sefin.E1234): código de ensaio"
            exit 1 ;;
    esac
}
case "$1" in
    info)
        case "$*" in
            *org.kde.Platform*) [ -f "$D/runtime-ok" ] && exit 0 || exit 1 ;;
            *) [ -f "$D/app-ok" ] && exit 0 || exit 1 ;;
        esac ;;
    remote-add) exit 0 ;;
    install)
        case "$*" in
            *org.kde.Platform*) : > "$D/runtime-ok" ;;
            *) : > "$D/app-ok" ;;
        esac
        exit 0 ;;
    document-export)
        for ultimo in "$@"; do :; done
        printf '/run/user/1000/doc/ab12/%s\n' "$(basename "$ultimo")"
        exit 0 ;;
    run)
        case "$*" in
            *--command=ls*)
                [ "${NL_ENSAIO_CA:-ok}" = faltando ] && exit 1
                exit 0 ;;
            *--command=sh*)
                if [ "${NL_ENSAIO_XDG:-ok}" = errado ]; then
                    echo /outro/lugar/config
                else
                    echo "$HOME/.var/app/io.github.petrinhu.notalivre/config"
                fi
                exit 0 ;;
            *" config certificado "*)
                # Simula a CLI real: acrescenta [certificado] caminho ao config.ini do programa.
                cfg="$HOME/.var/app/io.github.petrinhu.notalivre/config/notalivre/config.ini"
                mkdir -p "$(dirname "$cfg")"
                printf '\n[certificado]\ncaminho = /run/user/1000/doc/ab12/pfxsecreto-ensaio.p12\n' >> "$cfg"
                exit 0 ;;
            *" emitir "*) emitir_stub "$@" ;;
        esac
        exit 1 ;;
esac
exit 1
STUB
    cat > "$dir/bin/kdialog" <<'STUB'
#!/bin/sh
# Stub do kdialog para o ensaio: devolve um .pfx de faz de conta e um CNPJ de teste.
case "$1" in
    --getopenfilename)
        [ "${NL_ENSAIO_PFX_VAZIO:-}" = 1 ] && exit 1
        printf '%s\n' "$NL_ENSAIO_DIR/pfxsecreto-ensaio.p12"
        exit 0 ;;
    --inputbox)
        printf '%s\n' "${NL_ENSAIO_CNPJ:-11222333000181}"
        exit 0 ;;
esac
exit 1
STUB
    chmod 755 "$dir/bin/flatpak" "$dir/bin/kdialog"
}

# Versão sem kdialog: escreve só o stub do flatpak; o kdialog fica de fora do PATH do caso.
escrever_stubs_sem_kdialog() {
    local dir="$1"
    escrever_stubs "$dir"
    mv -f -- "$dir/bin/kdialog" "$dir/kdialog-desativado"
}

# caso NOME RC_ESPERADO TEXTO_ESPERADO SAIDAS_ESPERADAS EMITIR_ESPERADOS CONFIG_ESPERADO [VAR=valor ...]
ensaio_caso() {
    local nome="$1" rc_esp="$2" txt_esp="$3" saidas_esp="$4" emit_esp="$5" cfg_esp="$6"
    shift 6
    local dir="$RAIZ/$nome" entrada rc saidas emit problema=""
    mkdir -p "$dir"
    printf 'pfxsecreto-ensaio' > "$dir/pfxsecreto-ensaio.p12"
    entrada=/dev/null
    local sem_kd=0
    for par in "$@"; do
        case "$par" in NL_ENSAIO_SEM_KDIALOG=1) sem_kd=1 ;; esac
    done
    if [ "$sem_kd" -eq 1 ]; then
        escrever_stubs_sem_kdialog "$dir"
        entrada="$dir/entrada.txt"
        printf '11222333000181\n%s\n' "$dir/pfxsecreto-ensaio.p12" > "$entrada"
    else
        escrever_stubs "$dir"
    fi
    env "$@" NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < "$entrada" > "$dir/saida-do-script.txt" 2>&1
    rc=$?
    saidas=$(ls "$dir"/base/teste-lider/execucao-*/saida-tentativa-*.txt 2>/dev/null | wc -l | tr -d ' ')
    emit=$(grep -c ' emitir ' "$dir/chamadas.txt" 2>/dev/null || true)
    [ -n "$emit" ] || emit=0
    [ "$rc" -eq "$rc_esp" ] || problema="$problema rc=$rc(esperado $rc_esp)"
    grep -q -F -- "$txt_esp" "$dir/saida-do-script.txt" || problema="$problema texto_ausente('$txt_esp')"
    [ "$saidas" -eq "$saidas_esp" ] || problema="$problema saidas=$saidas(esperado $saidas_esp)"
    [ "$emit" -eq "$emit_esp" ] || problema="$problema emitir=$emit(esperado $emit_esp)"
    if [ -n "$cfg_esp" ]; then
        grep -q -F -- "$cfg_esp" "$dir/home/.var/app/$APP/config/notalivre/config.ini" 2>/dev/null \
            || problema="$problema config('$cfg_esp')"
    fi
    if grep -r -l -F -e pfxsecreto -e 11222333000181 "$dir/saida-do-script.txt" "$dir/base/devolucao" \
        "$dir"/base/teste-lider/execucao-* >/dev/null 2>&1; then
        problema="$problema vazamento(pfx ou CNPJ em saída, devolução ou execução)"
    fi
    if [ -z "$problema" ]; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s:%s\n' "$nome" "$problema"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# Dois envios seguidos na mesma pasta: o segundo tem de usar o número seguinte (nunca repete).
ensaio_dois_envios() {
    local nome=dois_envios_numeros_diferentes dir="$RAIZ/dois_envios_numeros_diferentes" r1 r2 ndps
    mkdir -p "$dir"
    printf 'pfxsecreto-ensaio' > "$dir/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    env NL_ENSAIO_SEQ=aceita,aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    ndps=$(cat "$dir/base/teste-lider/ultimo-ndps.txt" 2>/dev/null)
    if [ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] && [ "$ndps" = 2 ] && grep -q '"nDps": 2,' "$dir/base/teste-lider/dps-teste.json"; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s: rc=%s/%s ultimo_ndps=%s\n' "$nome" "$r1" "$r2" "$ndps"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

ensaio() {
    RAIZ=$(mktemp -d /var/tmp/notalivre-ensaio-producao.XXXXXX) || { echo "PARADA: mktemp"; exit 1; }
    CASOS=0
    FALHAS=0
    ensaio_caso aceita 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita
    ensaio_dois_envios
    ensaio_caso rota_404_preserva_certificado 0 "Nota de teste aceita" 2 2 "[certificado]" NL_ENSAIO_SEQ=404,aceita
    ensaio_caso rota_404_depois_aceita 0 "Nota de teste aceita" 2 2 "prefixo = /API/SefinNacional" NL_ENSAIO_SEQ=404,aceita
    ensaio_caso algoritmo_e0714_depois_aceita 0 "Nota de teste aceita" 2 2 "algoritmo = rsa-sha256" NL_ENSAIO_SEQ=E0714,aceita
    ensaio_caso e0714_duas_vezes_para 1 "A Sefin recusou" 2 2 "" NL_ENSAIO_SEQ=E0714,E0714
    ensaio_caso rota_404_duas_vezes_para 1 "A Sefin recusou" 2 2 "" NL_ENSAIO_SEQ=404,404
    ensaio_caso outra_recusa_para_sem_tentar 1 "A Sefin recusou" 1 1 "prefixo = /SefinNacional" NL_ENSAIO_SEQ=outro
    ensaio_caso antes_da_sefin_para 1 "antes de chegar à Sefin" 1 1 "" NL_ENSAIO_SEQ=local
    ensaio_caso bundle_errado_nao_instala 1 "SHA-256" 0 0 "" NL_ENSAIO_BUNDLE=errado NL_ENSAIO_SEQ=aceita
    ensaio_caso cnpj_invalido_para_antes_de_emitir 1 "CNPJ" 0 0 "" NL_ENSAIO_CNPJ=11222333000180 NL_ENSAIO_SEQ=aceita
    ensaio_caso pfx_cancelado_para_antes_de_emitir 1 "não pôde ser lido" 0 0 "" NL_ENSAIO_PFX_VAZIO=1 NL_ENSAIO_SEQ=aceita
    ensaio_caso ca_faltando_para_antes_de_emitir 1 "arquivo de CAs" 0 0 "" NL_ENSAIO_CA=faltando NL_ENSAIO_SEQ=aceita
    ensaio_caso pasta_config_errada_para_antes_de_emitir 1 "pasta de configuração" 0 0 "" NL_ENSAIO_XDG=errado NL_ENSAIO_SEQ=aceita
    ensaio_caso sem_kdialog_pergunta_no_terminal 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEM_KDIALOG=1 NL_ENSAIO_SEQ=aceita
    echo "ensaio: casos=$CASOS falhas=$FALHAS (pasta: $RAIZ)"
    [ "$FALHAS" -eq 0 ]
}

case "$MODO" in
    ensaio) ensaio; exit $? ;;
    caso|normal) fluxo; exit $? ;;
esac
