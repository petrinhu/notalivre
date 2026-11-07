#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#
# Emissão de teste do líder em Produção Restrita, em um comando só (NL-2.10; decisão do líder de 01/11/2026:
# "Um comando só (Recomendado)"). Faz as seções 3.1 a 3.11 e a 4 de docs/pesquisa/onda2_producao_restrita_roteiro.md:
# confere o programa, escolhe o .pfx numa janela, pede a inscrição (CNPJ ou CPF) e a IM quando não vieram por opção, monta a DPS,
# emite e devolve o resultado.
# A senha é pedida pelo kdialog (janela própria) e entregue à CLI pelo descritor 3 (--senha-certificado-fd 3, NL-2.27). Este
# script não a lê, não a guarda em variável e não a passa por argv nem por env; sem kdialog ele para antes de consumir número.
#
# Para o líder:                  bash tools/producao_restrita/emitir_teste.sh
# Com opções (NL-2.10g):         bash tools/producao_restrita/emitir_teste.sh --im T [--cnpj N | --cpf N] [--ie T]
#   o que faltar (inscrição, IM) a janela pergunta; --opcao=valor vale igual. Com --cpf ou --cnpj, o script avisa do histórico.
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
# Regras que o script cumpre: o caminho do .pfx nunca é impresso, copiado, listado nem gravado em log; ele só é guardado, se o usuário escolher, no arquivo de lembrar (0600, numa pasta 0700 em $XDG_STATE_HOME), e o --esquecer o apaga.
# A inscrição e a IM não vão a arquivo versionado nem a saída (NL-2.10g); nenhuma chamada vai à Produção; nada é apagado por este script; só há tentativa 2
# (base) se a Sefin responder 404, e tentativa 3 (algoritmo rsa-sha256) se ela responder E0714.

set -u
# Rodada 3 (revisor): caminho absoluto do próprio script, antes de qualquer cd do harness (o ensaio roda por caminho relativo também).
SELF_ABS=$(readlink -f -- "$0" 2>/dev/null || printf '%s' "$0")
# CISO (BAIXA-S3): tudo que o script cria sai com permissão só do dono (0600 para arquivos, 0700 para pastas).
umask 077

APP=io.github.petrinhu.notalivre
RUNTIME=org.kde.Platform//6.11
# Pacote do CI: único lugar do script com o run e o SHA-256 do notalivre.flatpak. Fonte: artefato notalivre-bundle do run
# 41575930687 (commit 073bd62, branch onda-2, com o total de tributos J1 e J2), baixado com gh run download (digest do zip d43056d8da4f94e74a91dd12badcbb7637bd7a9451b0710928f5da1fa21b28fc,
# conferido na API do GitHub). O main troca estas duas linhas quando o pacote do CI muda (NL-2.10j J3).
BUNDLE_RUN=41575930687
BUNDLE_SHA=05e7e73cf04e16b6ade13813e821bc9cf195a0f86dc9082787a4e69f12e15f93
BASE=/var/tmp/notalivre-nl210
BUNDLE="$BASE/bundle/run-$BUNDLE_RUN/notalivre.flatpak"
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
    XDG_STATE_HOME="$HOME/.local/state"
    export XDG_STATE_HOME
    BASE="$NL_ENSAIO_DIR/base"
    BUNDLE="$BASE/bundle/run-$BUNDLE_RUN/notalivre.flatpak"
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
# NL-3.0i: o contador de DPS e o estado da instalação ficam numa pasta durável, fora de /var/tmp (o sistema limpa /var/tmp em
# 30 dias sem uso). MÉDIA-S2: o contador nunca se apaga; fora de teste-lider (que se limpa).
DURAVEL="$HOME/.local/share/notalivre/nl210"
NDPS_ARQ="$DURAVEL/ultimo-ndps.txt"
DPS_ARQ="$TESTE_DIR/dps-teste.json"
ESTADO_INST="$DURAVEL/programa-instalado-sha.txt"
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

# Confere um CPF de 11 dígitos pelos dois dígitos verificadores; recusa 11 dígitos iguais (como o Back, NL-2.10g).
cpf_valido() {
    local c="$1" i soma d1 d2
    [[ "$c" =~ ^[0-9]{11}$ ]] || return 1
    [[ "$c" =~ ^(.)\1{10}$ ]] && return 1
    soma=0
    for i in 0 1 2 3 4 5 6 7 8; do soma=$(( soma + ${c:i:1} * (10 - i) )); done
    d1=$(( soma * 10 % 11 )); [ "$d1" -eq 10 ] && d1=0
    soma=0
    for i in 0 1 2 3 4 5 6 7 8 9; do soma=$(( soma + ${c:i:1} * (11 - i) )); done
    d2=$(( soma * 10 % 11 )); [ "$d2" -eq 10 ] && d2=0
    [ "${c:9:1}" -eq "$d1" ] && [ "${c:10:1}" -eq "$d2" ]
}

# Pergunta ao líder pela janela (kdialog, exigido pelo pré-voo). O valor nunca é impresso nem gravado.
perguntar() {
    local texto="$1" modo="$2"
    if [ "$modo" = pfx ]; then
        kdialog --getopenfilename "$HOME" "*.pfx *.p12|Certificado A1 (*.pfx, *.p12)" 2>/dev/null
    else
        kdialog --inputbox "$texto" 2>/dev/null
    fi
}

gravar_config() {
    mkdir -p "$CFG_DIR" || return 1
    if [ -f "$CFG" ] && [ "$BACKUP_FEITO" -eq 0 ]; then
        cp -p -- "$CFG" "$CFG.antes-$RUN_TS" || return 1
        chmod 600 "$CFG.antes-$RUN_TS" || return 1
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
    chmod 600 "$CFG.tmp" || return 1
    mv -f -- "$CFG.tmp" "$CFG" || return 1
    chmod 600 "$CFG"
}

# NL-3.0i: prepara a pasta durável do contador (0700). Migração única: sem contador no durável e com o da pasta antiga ($BASE,
# /var/tmp), copia com 0600 e segue; a pasta antiga fica como está. Com os dois presentes, vale o durável.
preparar_duravel() {
    mkdir -p -- "$DURAVEL" || falha "não consegui criar a pasta do contador da DPS. Nada foi emitido."
    chmod 700 -- "$DURAVEL" || falha "não consegui pôr a pasta do contador da DPS em 0700. Nada foi emitido."
    if [ ! -e "$NDPS_ARQ" ] && [ -f "$BASE/ultimo-ndps.txt" ]; then
        ( umask 077; cp -- "$BASE/ultimo-ndps.txt" "$NDPS_ARQ" ) || falha "não consegui copiar o contador antigo. Nada foi emitido."
    fi
}

# Próximo nDps: lê o último número gravado e soma 1. Não grava nada aqui (NL-2.27, regra do líder): o número só vai ao arquivo
# depois da tentativa, e só se ela chegou à Sefin (ver gravar_ndps).
proximo_ndps() {
    local ultimo=0
    if [ -f "$NDPS_ARQ" ]; then ultimo=$(cat "$NDPS_ARQ"); fi
    case "$ultimo" in
        ''|*[!0-9]*) falha "o arquivo do último número da DPS está inválido. Nada foi emitido." ;;
    esac
    NDPS_ATUAL=$(( ultimo + 1 ))
}

# Grava o número da tentativa. Chamada só quando a classificação não é "local": uma tentativa que parou antes da Sefin não
# consome número (o contador fica como estava).
gravar_ndps() {
    printf '%s\n' "$1" > "$NDPS_ARQ"
}

# DA-2.92: guarda o estado do contador antes da reserva, para devolver o número só na falha local.
NDPS_ANTES_EXISTIA=0
NDPS_ANTES_VALOR=""
reservar_ndps() {
    if [ -f "$NDPS_ARQ" ]; then
        NDPS_ANTES_EXISTIA=1
        NDPS_ANTES_VALOR=$(cat "$NDPS_ARQ")
    else
        NDPS_ANTES_EXISTIA=0
        NDPS_ANTES_VALOR=""
    fi
    gravar_ndps "$1"
}

# Falha local (nada foi à rede): o contador volta ao que era antes da reserva. Caminho literal, nunca glob.
devolver_ndps() {
    if [ "$NDPS_ANTES_EXISTIA" = 1 ]; then
        printf '%s\n' "$NDPS_ANTES_VALOR" > "$NDPS_ARQ"
    else
        rm -f -- "$NDPS_ARQ"
    fi
}

# DA-2.89 (CISO, rodada de revisão B1): a exportação da DPS sai do portal (document-unexport) e o id passa pelo caminho do
# documento exportado. Usado antes de cada nova exportação e pelo trap de saída.
desexportar_dps() {
    local id_doc
    if [ -n "$DOC_DPS" ]; then
        id_doc=${DOC_DPS#/run/user/*/doc/}
        id_doc=${id_doc%%/*}
        flatpak document-unexport --app="$APP" "$id_doc" >/dev/null 2>&1 || true
        DOC_DPS=""
    fi
}

# DA-2.89: ao sair (sucesso, falha ou interrupção) a DPS sai do portal e é apagada pelo caminho literal conferido, nunca por glob.
# SIGKILL não roda trap: nesse caso a tentativa seguinte sobrescreve e limpa (montar_dps).
limpar_dps() {
    desexportar_dps
    if [ "$DPS_ARQ" = "$TESTE_DIR/dps-teste.json" ] && [ -e "$DPS_ARQ" ]; then
        rm -f -- "$TESTE_DIR/dps-teste.json"
    fi
}

# Monta a DPS do roteiro (3.8) com o número dado e libera o arquivo para o programa.
# NL-2.10j, J2 (DA-2.99): a forma do totTrib segue o regime. MEI: indTotTrib 0. ME/EPP: pTotTribSN com --ptottribsn; sem ele, pTotTrib 0,00.
# Não optante: pTotTrib 0,00 nos três percentuais. O domínio (NL-2.10j, J1) recusa qualquer outra combinação antes do número.
totTrib_da_dps() {
    if [ "$OPSIMP" = 2 ]; then
        TOTTRIB_JSON='{ "indTotTrib": "0" }'
    elif [ "$OPSIMP" = 3 ] && [ -n "$PTOTTRIBSN" ]; then
        TOTTRIB_JSON="{ \"pTotTribSN\": \"$PTOTTRIBSN\" }"
    else
        TOTTRIB_JSON='{ "pTotTribFed": "0.00", "pTotTribEst": "0.00", "pTotTribMun": "0.00" }'
    fi
}

montar_dps() {
    local n="$1" dh dc prest_chave im_linha="" reg_apt_linha="" ctribmun_linha=""
    totTrib_da_dps
    # NL-2.10g: o prestador leva CNPJ ou CPF (nunca os dois) e a IM. Os valores são fechados por validação, sem aspas nem barras.
    if [ "$INSC_TIPO" = cpf ]; then
        prest_chave="\"cpf\": \"$INSC_VALOR\","
    else
        prest_chave="\"cnpj\": \"$INSC_VALOR\","
    fi
    # G1c (C3): regApTribSN só com opSimpNac 3 (RN 245 e 246); sem IM quando o emissor não tem registro (--sem-im).
    if [ "$OPSIMP" = 3 ]; then reg_apt_linha="\"regApTribSN\": \"$REG_APT\", "; fi
    if [ "$SEM_IM" = 0 ]; then im_linha="\"im\": \"$IM\","; fi
    # NL-2.10h (H1): cTribMun só entra quando há valor (nunca 000, nunca para MEI: validado antes do número).
    if [ -n "$CTRIBMUN" ]; then ctribmun_linha="\"cTribMun\": \"$CTRIBMUN\", "; fi
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
  "cLocEmi": "$MUN_EMI",
  "prestador": {
    $prest_chave
    $im_linha
    "regime": { "opSimpNac": "$OPSIMP", ${reg_apt_linha}"regEspTrib": "$REG_ESP" }
  },
  "servico": {
    "cLocPrestacao": "$LOC_PREST",
    "cTribNac": "$CTRIBNAC",
    ${ctribmun_linha}"xDescServ": "$DESC"
  },
  "valores": {
    "vServ": "$VALOR",
    "tribMun": { "tribISSQN": "$TRIB_ISSQN", "tpRetISSQN": "$RET_ISSQN" },
    "totTrib": ${TOTTRIB_JSON}
  }
}
EOF
    ) || falha "não consegui escrever a DPS de teste. Nada foi emitido."
    chmod 600 "$DPS_ARQ"
    grep -q "\"nDps\": $n," "$DPS_ARQ" && grep -q "\"serie\": $SERIE," "$DPS_ARQ" \
        || falha "a DPS não tem o --serie e o --numero desta tentativa. Nada foi emitido."
    desexportar_dps
    DOC_DPS=$(flatpak document-export --app="$APP" --forbid-write "$DPS_ARQ" 2>/dev/null) || DOC_DPS=""
    case "$DOC_DPS" in
        /run/user/*) ;;
        *) falha "o programa não liberou a DPS de teste. Nada foi emitido." ;;
    esac
}

# Uma tentativa de emissão (3.9). A saída inteira vai para a pasta da execução. A senha vem do kdialog por substituição de
# processo no descritor 3 (NL-2.27, Emenda 1, item 3): não passa por variável, por argv nem por env, e o shell não a lê.
# O texto da janela é genérico: sem nome de arquivo e sem CNPJ.
emitir_tentativa() {
    local k="$1" n="$2"
    SAIDA_ATUAL="$RUN_DIR/saida-tentativa-$k.txt"
    flatpak run --command=notalivre-cli "$APP" emitir "$DOC_DPS" \
        --serie "$SERIE" --numero "$n" --ambiente producao-restrita --senha-certificado-fd 3 \
        > "$SAIDA_ATUAL" 2>&1 3< <(kdialog --password "Senha do certificado digital, para a emissão de teste.")
    EMITIU_RC=$?
}

# Classifica o resultado: aceita, 404 (rota), e0714 (algoritmo), sefin (outra recusa da Sefin) ou local (antes da Sefin).
classificar() {
    if [ "$1" -eq 0 ]; then RESULTADO=aceita; return; fi
    # MÉDIA-S1: só o POST de emissão com 404 diz que a rota de envio não existe; GET de consulta não conta.
    if awk '/"evento":"sefin.requisicao"/ && /"metodo":"POST"/ && /"status":"404"/ { achou = 1 } END { exit !achou }' "$2"; then
        RESULTADO=404
        return
    fi
    if grep -q 'sefin\.E0714' "$2"; then RESULTADO=e0714; return; fi
    # NL-2.10e (E7, DA-2.80): "A Sefin recusou" só com prova de resposta: um código E ou um evento sefin.requisicao. Um "sefin.falha"
    # sem nenhum dos dois não prova que a Sefin respondeu, então é incerta.
    # NL-2.10e (E4, E7): falha de transporte com zero bytes escritos é antes da Sefin: local. Com bytes escritos, o POST pode ter
    # chegado: incerta (o sinal é o evento sefin.transporte_falhou, antes de qualquer texto de recusa).
    if grep -q 'sefin\.transporte_falhou' "$2"; then
        # R-B1 (revisão, NL-2.10e): qualquer evento com bytes escritos, em qualquer etapa, torna a tentativa incerta (o POST pode ter
        # chegado). Local só com a etapa POST e zero bytes escritos.
        if awk '/"evento":"sefin.transporte_falhou"/ && !/"bytesEscritos":"0"/ { achou = 1 } END { exit !achou }' "$2"; then
            RESULTADO=incerta
        elif awk '/"evento":"sefin.transporte_falhou"/ && /"etapa":"POST"/ && /"bytesEscritos":"0"/ { achou = 1 } END { exit !achou }' "$2"; then
            RESULTADO=local
        else
            RESULTADO=incerta
        fi
        return
    fi
    if grep -q 'emissão recusada (sefin\.E[0-9]' "$2"; then RESULTADO=sefin; return; fi
    if grep -q 'emissão recusada (sefin\.' "$2"; then
        if awk '/"evento":"sefin.requisicao"/ { achou = 1 } END { exit !achou }' "$2"; then
            RESULTADO=sefin
        else
            RESULTADO=incerta
        fi
        return
    fi
    # NL-2.27 (B1, revisão de segurança): processo morto (rc >= 128) ou POST registrado na saída: a Sefin pode ter recebido a DPS.
    # Incerta grava o número e manda confirmar antes de reemitir. Só "local" prova que a Sefin não foi chamada.
    if [ "$1" -ge 128 ] || awk '/"evento":"sefin.requisicao"/ && /"metodo":"POST"/ { achou = 1 } END { exit !achou }' "$2"; then
        RESULTADO=incerta
        return
    fi
    RESULTADO=local
}

# ---------------------------------------------------------------------------------------------------------------------
# Opções (NL-2.10g, DA-2.86 e DA-2.87): --cnpj, --cpf, --im e --ie, na forma "--opcao valor" ou "--opcao=valor". Tudo é conferido
# em ler_opcoes, antes de qualquer instalação, pergunta ou número. Nenhum valor é impresso em saída, log ou erro.
# ---------------------------------------------------------------------------------------------------------------------
INSC_TIPO=""
INSC_VALOR=""
IM=""
declare -A OPT=()
# G1c: valores dos campos e sua origem (opcao, lembrado, consulta ou padrao). A origem decide o que o lembrado guarda.
declare -A ORIG=() LEM=()
OPSIMP=3
REG_APT=1
REG_ESP=0
VALOR="10,00"
LOC_PREST=""
TRIB_ISSQN=1
RET_ISSQN=1
DESC="Medicina: teste de emissão em Produção Restrita"
MUN_EMI="$MUNICIPIO"
SEM_IM=0
BA_OK=0
BA_OPSIMP=""
BA_MUN=""
BA_ATIVA=""
LEMBRAR_EXCLUIR=""
PFX_USADO=""
CTRIBMUN=""
PTOTTRIBSN=""
TOTTRIB_JSON=""
PEDIR_CTRIBMUN=0
PEDIR_NOVO=0
LEMBRAR_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/notalivre-teste"
LEMBRAR="$LEMBRAR_DIR/lembrar"
AVISO_HISTORICO='Aviso: o número de CPF ou CNPJ digitado na linha de comando fica gravado no histórico do terminal e, enquanto o script roda, aparece para outros usuários desta máquina na lista de processos. Para não deixar esse rastro, rode sem --cpf e sem --cnpj: a janela pede o número. Começar o comando com um espaço só tira a linha do histórico se o seu terminal estiver configurado para isso.'

# Tira espaços das pontas.
aparar() {
    local v="$1"
    v="${v#"${v%%[![:space:]]*}"}"
    v="${v%"${v##*[![:space:]]}"}"
    printf '%s' "$v"
}

# Conjunto fechado (I2, D3, D4): só [0-9A-Za-z./-], de 1 até MAX caracteres. LC_ALL=C: nada fora do ASCII passa.
valida_texto_fechado() {
    local max="$1" v="$2" re
    local LC_ALL=C
    re="^[0-9A-Za-z./-]{1,$max}$"
    [[ "$v" =~ $re ]]
}

# CNPJ (14 dígitos, DV) ou CPF (11 dígitos, DV). Sem tipo, decide pelo tamanho (janela). Define INSC_TIPO e INSC_VALOR.
definir_inscricao() {
    local valor tipo="$2"
    valor=${1//[[:space:]]/}
    if [ -z "$tipo" ]; then
        case "${#valor}" in
            14) tipo=cnpj ;;
            11) tipo=cpf ;;
            *) falha "o documento do emissor precisa ter 14 dígitos (CNPJ) ou 11 (CPF). Nada foi emitido." ;;
        esac
    fi
    case "$tipo" in
        cnpj) cnpj_valido "$valor" || falha "o CNPJ não tem 14 dígitos ou o dígito verificador não confere. Nada foi emitido." ;;
        cpf) cpf_valido "$valor" || falha "o CPF não tem 11 dígitos ou o dígito verificador não confere. Nada foi emitido." ;;
    esac
    INSC_TIPO=$tipo
    INSC_VALOR=$valor
}

# IM: de 1 a 15 caracteres do conjunto fechado; vazia (ou cancelada na janela) para antes de consumir número (D3).
definir_im() {
    local v
    v=$(aparar "$1")
    valida_texto_fechado 15 "$v" || falha "a inscrição municipal precisa ter de 1 a 15 caracteres: letras, números, ponto, barra ou traço, e não pode ser vazia; se a empresa não tem inscrição municipal, use --sem-im. Nada foi emitido."
    IM=$v
}

# IE (D4): só confere o tamanho (1 a 20) e o conjunto fechado; não vai na DPS nem é guardada.
validar_ie() {
    local v
    v=$(aparar "$1")
    valida_texto_fechado 20 "$v" || falha "a inscrição estadual precisa ter de 1 a 20 caracteres: letras, números, ponto, barra ou traço. Nada foi emitido."
    printf '%s\n' 'A inscrição estadual não vai na NFS-e e este script não a guarda; a guarda no cofre vem com a interface (spec 4.5).'
}

# Coordenação (G1c, item 1; seção C2 e C9 da G1b): local da prestação, tipo de tributação do ISSQN e retenção (travada em 1).
definir_local_prestacao() {
    [[ "$1" =~ ^([1-5][0-9]{6}|0000000)$ ]] || falha "o local da prestação precisa ser um código IBGE de 7 dígitos (começando de 1 a 5) ou 0000000. Nada foi emitido."
    LOC_PREST=$1; ORIG[localprestacao]=opcao
}
definir_trib_issqn() {
    [[ "$1" =~ ^[1-4]$ ]] || falha "o tipo de tributação do ISSQN precisa ser 1, 2, 3 ou 4. Nada foi emitido."
    TRIB_ISSQN=$1; ORIG[trib_issqn]=opcao
}
definir_ret_issqn() {
    [[ "$1" = 1 ]] || falha "a retenção do ISSQN só aceita 1 neste script (não há tomador). Nada foi emitido."
    RET_ISSQN=$1; ORIG[ret_issqn]=opcao
}
# Sem opção, o local da prestação é o do município emissor (C2).
resolver_local_prestacao() {
    if [ "${ORIG[localprestacao]:-}" != opcao ]; then LOC_PREST=$MUN_EMI; ORIG[localprestacao]=padrao; fi
}

# G1c (validadores sem janela): cada valor de opção é conferido na hora e fica com origem "opcao".
valida_municipio() { [[ "$1" =~ ^[1-5][0-9]{6}$ ]]; }
valida_ctribnac() { [[ "$1" =~ ^[0-9]{6}$ && "$1" != 9903* && "$1" != 990401 ]]; }
# NL-2.10h (H1): cTribMun de 3 dígitos, nunca 000 (regra E0315). A regra de MEI (E0313) é conferida em validar_ctribmun.
valida_ctribmun() { [[ "$1" =~ ^[0-9]{3}$ && "$1" != 000 ]]; }
definir_municipio() {
    valida_municipio "$1" || falha "o código IBGE do município precisa ter 7 dígitos e começar de 1 a 5. Nada foi emitido."
    MUN_EMI=$1; ORIG[municipio]=opcao
}
definir_ctribnac() {
    valida_ctribnac "$1" || falha "o código de tributação nacional precisa ter 6 dígitos e não pode começar por 9903 nem ser 990401. Nada foi emitido."
    CTRIBNAC=$1; ORIG[ctribnac]=opcao
}
definir_ctribmun() {
    valida_ctribmun "$1" || falha "o código de tributação municipal precisa ter 3 dígitos e não pode ser 000. Nada foi emitido."
    CTRIBMUN=$1; ORIG[ctribmun]=opcao
}
# Código digitado na janela (depois de uma recusa): recusa vazia ou inválida para antes de consumir número.
definir_ctribmun_janela() {
    valida_ctribmun "$1" || falha "o código complementar municipal precisa ter 3 dígitos e não pode ser 000. Nada foi emitido."
    CTRIBMUN=$1; ORIG[ctribmun]=janela
}
# NL-2.10j, J2: --ptottribsn é o percentual aproximado do Simples, de 0,00 a 99,99, com até duas casas (vírgula ou ponto). Sai com ponto e duas casas.
valida_ptottribsn() { [[ "$1" =~ ^(0|[1-9][0-9]?)([.,][0-9]{1,2})?$ ]]; }
definir_ptottribsn() {
    valida_ptottribsn "$1" || falha "o percentual do Simples (--ptottribsn) precisa estar entre 0,00 e 99,99, com até duas casas. Nada foi emitido."
    local v=${1/,/.} inteiro casas
    case "$v" in *.*) ;; *) v="$v.00" ;; esac
    inteiro=${v%%.*}
    casas=${v#*.}
    case ${#casas} in 1) casas="${casas}0" ;; esac
    PTOTTRIBSN="$inteiro.$casas"; ORIG[ptottribsn]=opcao
}
# Só ME/EPP (opSimpNac 3) manda o percentual do Simples. Opção com MEI ou não optante para antes do número; o lembrado é descartado.
validar_ptottribsn() {
    [ -n "$PTOTTRIBSN" ] || return 0
    [ "$OPSIMP" = 3 ] && return 0
    if [ "${ORIG[ptottribsn]:-}" = opcao ]; then
        falha "o percentual do Simples (--ptottribsn) só vale para ME/EPP. Com MEI ou não optante, tire --ptottribsn. Nada foi emitido."
    fi
    PTOTTRIBSN=""
}
# Aviso para ME/EPP sem o percentual: a nota de teste vai com 0,00, e o aviso fala como ao usuário, sem valor.
avisar_ptottrib() {
    if [ "$OPSIMP" = 3 ] && [ -z "$PTOTTRIBSN" ]; then
        printf 'Aviso: o percentual aproximado dos tributos do Simples não foi informado. A nota de teste vai com 0,00 nos três percentuais. Para a nota real, use --ptottribsn com a alíquota efetiva do seu DAS.\n'
    fi
}

definir_opsimpnac() {
    [[ "$1" =~ ^[123]$ ]] || falha "o regime do Simples precisa ser 1 (não optante), 2 (MEI) ou 3 (ME/EPP). Nada foi emitido."
    OPSIMP=$1; ORIG[opsimpnac]=opcao
}
definir_regaptribsn() {
    [[ "$1" =~ ^[123]$ ]] || falha "o regime de apuração (regApTribSN) precisa ser 1, 2 ou 3. Nada foi emitido."
    REG_APT=$1; ORIG[regaptribsn]=opcao
}
definir_regesptrib() {
    [[ "$1" =~ ^(0|1|2|3|4|5|6|9)$ ]] || falha "o regime especial precisa ser 0, 1, 2, 3, 4, 5, 6 ou 9. Nada foi emitido."
    REG_ESP=$1; ORIG[regesptrib]=opcao
}
definir_valor() {
    local v=$1
    { [[ "$v" =~ ^[0-9]{1,13}[,.][0-9]{2}$ ]] && [[ "${v//[^1-9]/}" ]]; } || falha "o valor precisa estar no formato 10,00 (até 13 dígitos e 2 casas, com ponto ou vírgula) e ser maior que zero. Nada foi emitido."
    VALOR=${v/./,}; ORIG[valor]=opcao
}
definir_descricao() {
    local v=$1 re
    local LC_ALL=C.UTF-8
    re='^[A-Za-z0-9áàâãéêíóôõúüçÁÀÂÃÉÊÍÓÔÕÚÜÇ .,;:/()%-]{1,2000}$'
    [[ "$v" =~ $re ]] || falha "a descrição precisa ter de 1 a 2000 caracteres: letras (com ou sem acento), números, espaço e . , ; : - / ( ) %. Nada foi emitido."
    DESC=$v; ORIG[descricao]=opcao
}

ler_opcoes() {
    local nome valor
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --sem-im) nome=sem-im; valor=1; shift ;;
            --cnpj=*|--cpf=*|--im=*|--ie=*|--municipio=*|--ctribnac=*|--ctribmun=*|--ptottribsn=*|--opsimpnac=*|--regaptribsn=*|--regesptrib=*|--valor=*|--descricao=*|--local-prestacao=*|--trib-issqn=*|--ret-issqn=*)
                nome="${1%%=*}"; valor="${1#*=}"; shift ;;
            --cnpj|--cpf|--im|--ie|--municipio|--ctribnac|--ctribmun|--ptottribsn|--opsimpnac|--regaptribsn|--regesptrib|--valor|--descricao|--local-prestacao|--trib-issqn|--ret-issqn)
                [ "$#" -ge 2 ] || falha "a opção $1 pede um valor. Nada foi emitido."
                nome="$1"; valor="$2"; shift 2 ;;
            *) falha "opção desconhecida (use --help para ver as opções). Nada foi emitido." ;;
        esac
        nome="${nome#--}"
        [ -z "${OPT[$nome]+x}" ] || falha "a opção --$nome foi dada mais de uma vez. Nada foi emitido."
        OPT[$nome]=$valor
    done
    if [ -n "${OPT[cnpj]+x}" ] && [ -n "${OPT[cpf]+x}" ]; then
        falha "use só um: --cnpj ou --cpf, não os dois. Nada foi emitido."
    fi
    if [ -n "${OPT[cnpj]+x}" ] || [ -n "${OPT[cpf]+x}" ]; then
        printf '%s\n' "$AVISO_HISTORICO"
    fi
    if [ -n "${OPT[cnpj]+x}" ]; then definir_inscricao "${OPT[cnpj]}" cnpj; fi
    if [ -n "${OPT[cpf]+x}" ]; then definir_inscricao "${OPT[cpf]}" cpf; fi
    if [ -n "${OPT[im]+x}" ]; then definir_im "${OPT[im]}"; fi
    if [ -n "${OPT[ie]+x}" ]; then validar_ie "${OPT[ie]}"; fi
    if [ -n "${OPT[sem-im]+x}" ] && [ -n "${OPT[im]+x}" ]; then falha "--sem-im e --im não podem vir juntos. Nada foi emitido."; fi
    if [ -n "${OPT[sem-im]+x}" ]; then SEM_IM=1; ORIG[sem_im]=opcao; fi
    if [ -n "${OPT[municipio]+x}" ]; then definir_municipio "${OPT[municipio]}"; fi
    if [ -n "${OPT[ctribnac]+x}" ]; then definir_ctribnac "${OPT[ctribnac]}"; fi
    if [ -n "${OPT[ctribmun]+x}" ]; then definir_ctribmun "${OPT[ctribmun]}"; fi
    if [ -n "${OPT[ptottribsn]+x}" ]; then definir_ptottribsn "${OPT[ptottribsn]}"; fi
    if [ -n "${OPT[opsimpnac]+x}" ]; then definir_opsimpnac "${OPT[opsimpnac]}"; fi
    if [ -n "${OPT[regaptribsn]+x}" ]; then definir_regaptribsn "${OPT[regaptribsn]}"; fi
    if [ -n "${OPT[regesptrib]+x}" ]; then definir_regesptrib "${OPT[regesptrib]}"; fi
    if [ -n "${OPT[valor]+x}" ]; then definir_valor "${OPT[valor]}"; fi
    if [ -n "${OPT[descricao]+x}" ]; then definir_descricao "${OPT[descricao]}"; fi
    if [ -n "${OPT[local-prestacao]+x}" ]; then definir_local_prestacao "${OPT[local-prestacao]}"; fi
    if [ -n "${OPT[trib-issqn]+x}" ]; then definir_trib_issqn "${OPT[trib-issqn]}"; fi
    if [ -n "${OPT[ret-issqn]+x}" ]; then definir_ret_issqn "${OPT[ret-issqn]}"; fi
    if [ -n "${OPT[regaptribsn]+x}" ] && [ -n "${OPT[opsimpnac]+x}" ] && [ "$OPSIMP" != 3 ]; then
        falha "--regaptribsn só vale com a opção 3 do Simples (ME/EPP). Nada foi emitido."
    fi
    return 0
}

# ---------------------------------------------------------------------------------------------------------------------
# G1c: consulta pública (BrasilAPI) só com CNPJ, sem --opsimpnac e sem regime lembrado. Só o regime e o município saem dela,
# e o JSON é lido por python3 -I na entrada padrão: a saída é só o regime, o município e a situação (nunca nome, nunca sócio).
PARSER_BRASILAPI='import json, sys
try:
    d = json.load(sys.stdin)
    mei = d["opcao_pelo_mei"] is True and d["data_exclusao_do_mei"] in (None, "")
    sn = d["opcao_pelo_simples"] is True and d["data_exclusao_do_simples"] in (None, "")
    mun = str(d["codigo_municipio_ibge"])
    if len(mun) != 7 or not mun.isdigit():
        raise ValueError
    ativa = 1 if str(d["situacao_cadastral"]) in ("2", "ATIVA") else 0
    print(2 if mei else 3 if sn else 1, mun, ativa)
except Exception:
    sys.exit(1)
'

# Sem curl ou python3, sem resposta, ou resposta que não serve: avisa em uma linha e segue com os padrões (nunca para).
consultar_brasilapi() {
    local json saida
    BA_OK=0
    if ! command -v curl >/dev/null 2>&1 || ! command -v python3 >/dev/null 2>&1; then
        printf 'AVISO: curl ou python3 ausente: a consulta pública não foi feita; uso os padrões.\n'
        return 0
    fi
    json=$(curl -fsS --max-time 10 --proto =https "https://brasilapi.com.br/api/cnpj/v1/$INSC_VALOR" 2>/dev/null) || json=""
    if [ -z "$json" ]; then
        printf 'AVISO: a consulta pública não respondeu; uso os padrões.\n'
        return 0
    fi
    saida=$(printf '%s' "$json" | python3 -I -c "$PARSER_BRASILAPI" 2>/dev/null) || saida=""
    if [ -z "$saida" ]; then
        printf 'AVISO: a resposta da consulta pública não serve; uso os padrões.\n'
        return 0
    fi
    read -r BA_OPSIMP BA_MUN BA_ATIVA <<< "$saida"
    BA_OK=1
    if [ "$BA_ATIVA" != 1 ]; then
        printf 'AVISO: a situação cadastral do CNPJ na consulta pública não é ativa; a Sefin decide.\n'
    fi
}

# Precedência de cada campo (C1): opção, depois lembrado, depois consulta (regime e município), depois padrão.
resolver_campos() {
    if [ "${ORIG[sem_im]:-}" != opcao ]; then
        if [ -z "${OPT[im]+x}" ] && [ -n "${LEM[sem_im]+x}" ]; then SEM_IM=${LEM[sem_im]}; ORIG[sem_im]=lembrado; fi
    fi
    if [ "${ORIG[opsimpnac]:-}" != opcao ]; then
        if [ -n "${LEM[opsimpnac]+x}" ]; then OPSIMP=${LEM[opsimpnac]}; ORIG[opsimpnac]=lembrado
        elif [ "$BA_OK" = 1 ]; then OPSIMP=$BA_OPSIMP; ORIG[opsimpnac]=consulta
        elif [ "$INSC_TIPO" = cpf ]; then OPSIMP=1; ORIG[opsimpnac]=padrao
        else OPSIMP=3; ORIG[opsimpnac]=padrao; fi
    fi
    if [ "${ORIG[municipio]:-}" != opcao ]; then
        if [ -n "${LEM[municipio]+x}" ]; then MUN_EMI=${LEM[municipio]}; ORIG[municipio]=lembrado
        elif [ "$BA_OK" = 1 ]; then MUN_EMI=$BA_MUN; ORIG[municipio]=consulta
        else MUN_EMI=$MUNICIPIO; ORIG[municipio]=padrao; fi
    fi
    if [ "${ORIG[regaptribsn]:-}" != opcao ]; then
        if [ -n "${LEM[regaptribsn]+x}" ]; then REG_APT=${LEM[regaptribsn]}; ORIG[regaptribsn]=lembrado
        else REG_APT=1; ORIG[regaptribsn]=padrao; fi
    fi
    if [ "${ORIG[regesptrib]:-}" != opcao ]; then
        if [ -n "${LEM[regesptrib]+x}" ]; then REG_ESP=${LEM[regesptrib]}; ORIG[regesptrib]=lembrado
        else REG_ESP=0; ORIG[regesptrib]=padrao; fi
    fi
    if [ "${ORIG[ctribnac]:-}" != opcao ]; then
        if [ -n "${LEM[ctribnac]+x}" ]; then CTRIBNAC=${LEM[ctribnac]}; ORIG[ctribnac]=lembrado
        else CTRIBNAC=040101; ORIG[ctribnac]=padrao; fi
    fi
    # NL-2.10h: cTribMun só vem por opção ou lembrado; sem nenhum dos dois não há padrão (a janela pergunta, se for o caso).
    if [ "${ORIG[ctribmun]:-}" != opcao ] && [ -n "${LEM[ctribmun]+x}" ]; then CTRIBMUN=${LEM[ctribmun]}; ORIG[ctribmun]=lembrado; fi
    if [ "${ORIG[ptottribsn]:-}" != opcao ] && [ -n "${LEM[ptottribsn]+x}" ]; then PTOTTRIBSN=${LEM[ptottribsn]}; ORIG[ptottribsn]=lembrado; fi
}

# NL-2.10h (H1, regra E0313): MEI não pode ter cTribMun. Opção para MEI para antes de consumir número; lembrado ou janela não
# valem para MEI e são descartados sem pergunta.
validar_ctribmun() {
    [ "$OPSIMP" = 2 ] || return 0
    if [ "${ORIG[ctribmun]:-}" = opcao ]; then
        falha "o código de tributação municipal não vale para MEI (regra E0313): tire --ctribmun. Nada foi emitido."
    fi
    CTRIBMUN=""
}

# NL-2.10h (H1): a janela do cTribMun abre só depois de uma recusa E0312, E0313 ou E0314 na execução anterior (flag pedir_ctribmun
# no arquivo de lembrar), sem valor por opção nem lembrado, e nunca para MEI. Cancelada ou vazia para antes de consumir número.
janela_ctribmun() {
    [ "$PEDIR_CTRIBMUN" = 1 ] || return 0
    [ -z "$CTRIBMUN" ] || return 0
    [ "$OPSIMP" != 2 ] || return 0
    definir_ctribmun_janela "$(perguntar "Código complementar municipal do serviço (3 dígitos, o final do código que aparece no Emissor Nacional, por exemplo 501 em 04.01.01.501)." ctribmun)"
}

# Confere a combinação de regime com os valores finais (RN 245, 248 e 249): antes de consumir número.
validar_regime() {
    if [ "$OPSIMP" = 2 ] && [ "$REG_ESP" != 0 ]; then
        falha "o regime especial de MEI (regEspTrib) só pode ser 0. Nada foi emitido."
    fi
    if [ "$OPSIMP" = 3 ] && [ "$REG_APT" = 1 ] && [ "$REG_ESP" != 0 ]; then
        falha "com a opção 3 e regApTribSN 1, o regime especial (regEspTrib) precisa ser 0. Nada foi emitido."
    fi
}

texto_origem() {
    case "$1" in
        opcao) printf 'da linha de comando' ;;
        lembrado) printf 'lembrado' ;;
        consulta) printf 'da consulta pública' ;;
        janela) printf 'digitado na janela' ;;
        *) printf 'padrão' ;;
    esac
}

# Diz de onde veio o regime e o município (C1). Nada de CNPJ, CPF, IM nem nome.
anunciar_origens() {
    local regime
    case "$OPSIMP" in
        1) regime="não optante" ;;
        2) regime=MEI ;;
        *) regime="ME/EPP" ;;
    esac
    printf 'regime: %s, %s\n' "$regime" "$(texto_origem "${ORIG[opsimpnac]:-padrao}")"
    printf 'município: %s, %s\n' "$MUN_EMI" "$(texto_origem "${ORIG[municipio]:-padrao}")"
    if [ -n "$CTRIBMUN" ]; then printf 'código complementar municipal: %s, %s\n' "$CTRIBMUN" "$(texto_origem "${ORIG[ctribmun]:-padrao}")"; fi
}

# DA-2.94 (CISO): confere o caminho guardado do .pfx sem abri-lo (lstat, por testes de bash): absoluto, arquivo comum,
# não é link simbólico e é do usuário. Qualquer falha manda à janela; nada disso imprime o caminho.
pfx_guardado_ok() {
    local p="$1"
    case "$p" in
        /*) ;;
        *) return 1 ;;
    esac
    [ ! -L "$p" ] && [ -f "$p" ] && [ -O "$p" ]
}

# Arquivo de lembrar (C5, DA-2.91): $XDG_STATE_HOME/notalivre-teste/lembrar, 0600 numa pasta 0700. O CPF nunca entra.
lembrar_valido() {
    local chave="$1" v="$2"
    case "$chave" in
        opsimpnac|regaptribsn) [[ "$v" =~ ^[123]$ ]] ;;
        regesptrib) [[ "$v" =~ ^(0|1|2|3|4|5|6|9)$ ]] ;;
        municipio) valida_municipio "$v" ;;
        ctribnac) valida_ctribnac "$v" ;;
        ctribmun) valida_ctribmun "$v" ;;
        pedir_ctribmun) [[ "$v" =~ ^[01]$ ]] ;;
        ptottribsn) [[ "$v" =~ ^(0|[1-9][0-9]?)\.[0-9]{2}$ ]] ;;
        sem_im) [[ "$v" =~ ^[01]$ ]] ;;
        cnpj) cnpj_valido "$v" ;;
        im) valida_texto_fechado 15 "$v" ;;
        data) [[ "$v" =~ ^[0-9-]{8,10}$ ]] ;;
        pfx) [[ "$v" == /* && ${#v} -le 4096 && "$v" != *[[:cntrl:]]* ]] ;;
        origem_*) [[ "$v" =~ ^(consulta|opcao|lembrado|padrao|janela)$ ]] ;;
        *) return 1 ;;
    esac
}

# Lê o lembrado. Permissão aberta, outro dono ou linha fora do conjunto: ignora tudo e avisa.
lembrar_carregar() {
    local linha chave valor modo dono
    LEM=()
    [ -e "$LEMBRAR" ] || return 0
    modo=$(stat -c %a -- "$LEMBRAR" 2>/dev/null)
    dono=$(stat -c %u -- "$LEMBRAR" 2>/dev/null)
    if [ "$modo" != 600 ] || [ "$dono" != "$(id -u)" ]; then
        printf 'AVISO: o arquivo de lembrar tem permissão aberta ou outro dono: ignorado.\n'
        return 0
    fi
    while IFS= read -r linha || [ -n "$linha" ]; do
        chave=${linha%%=*}
        valor=${linha#*=}
        if [ "$chave" = "$linha" ] || ! lembrar_valido "$chave" "$valor"; then
            printf 'AVISO: o arquivo de lembrar tem uma linha fora do conjunto: ignorado.\n'
            LEM=()
            return 0
        fi
        LEM[$chave]=$valor
    done < "$LEMBRAR"
    return 0
}

# Grava o que vale para a próxima vez (depois da resposta da Sefin). Valor padrão não é gravado; campo recusado não entra.
gravar_lembrar() {
    local tmp="$LEMBRAR.tmp" chave
    local -A novo=()
    novo[opsimpnac]=$OPSIMP
    if [ "$OPSIMP" = 3 ]; then novo[regaptribsn]=$REG_APT; fi
    novo[regesptrib]=$REG_ESP
    novo[municipio]=$MUN_EMI
    novo[ctribnac]=$CTRIBNAC
    if [ -n "$CTRIBMUN" ]; then novo[ctribmun]=$CTRIBMUN; fi
    if [ -n "$PTOTTRIBSN" ]; then novo[ptottribsn]=$PTOTTRIBSN; fi
    # NL-2.10h: a flag da janela vale só depois da resposta da Sefin que a pediu; qualquer outra resposta a limpa (não é escrita).
    if [ "$PEDIR_NOVO" = 1 ]; then novo[pedir_ctribmun]=1; fi
    novo[sem_im]=$SEM_IM
    if [ "$INSC_TIPO" = cnpj ]; then novo[cnpj]=$INSC_VALOR; fi
    if [ "$SEM_IM" = 0 ] && [ -n "$IM" ]; then novo[im]=$IM; fi
    # DA-2.94: o caminho do .pfx usado vai só para o arquivo de lembrar (0600, pasta 0700); nunca é impresso.
    if [ -n "${PFX_USADO:-}" ]; then novo[pfx]=$PFX_USADO; fi
    novo[data]=$(date +%Y-%m-%d)
    mkdir -p -- "$LEMBRAR_DIR" && chmod 700 "$LEMBRAR_DIR" || return 1
    for chave in opsimpnac regaptribsn regesptrib municipio ctribnac ctribmun ptottribsn pedir_ctribmun sem_im cnpj im pfx data; do
        [ -n "${novo[$chave]+x}" ] || continue
        case " $LEMBRAR_EXCLUIR " in *" $chave "*) continue ;; esac
        case "$chave" in
            opsimpnac|regaptribsn|regesptrib|municipio|ctribnac|ctribmun|ptottribsn|sem_im)
                [ "${ORIG[$chave]:-padrao}" != padrao ] || continue
                printf '%s=%s\n' "$chave" "${novo[$chave]}"
                printf 'origem_%s=%s\n' "$chave" "${ORIG[$chave]}" ;;
            *) printf '%s=%s\n' "$chave" "${novo[$chave]}" ;;
        esac
    done > "$tmp" || return 1
    chmod 600 "$tmp" && mv -f -- "$tmp" "$LEMBRAR"
}

campo_recusado() {
    LEMBRAR_EXCLUIR="$LEMBRAR_EXCLUIR $1"
    printf 'na próxima tentativa, %s.\n' "$2"
}

# Recusa da Sefin ligada a um campo (C6): uma linha de dica e o campo sai do lembrado.
dica_recusa() {
    local codigo
    codigo=$(grep -o 'sefin\.E[0-9][0-9][0-9][0-9]' "$SAIDA_ATUAL" | tail -n 1 | cut -d. -f2)
    case "$codigo" in
        E0160) campo_recusado opsimpnac "use --opsimpnac 1, 2 ou 3, conforme o cadastro do Simples na competência" ;;
        E0162|E0166) campo_recusado regaptribsn "use --regaptribsn 1, 2 ou 3 (com a opção 3 do Simples)" ;;
        E0174|E0175|E0177) campo_recusado regesptrib "use --regesptrib 0" ;;
        E0116|E0119|E0120|E0125) campo_recusado im "use --im com a inscrição municipal certa, ou --sem-im se a empresa não tem registro no CNC" ;;
        E0037|E0038|E0084) campo_recusado municipio "use --municipio com o código IBGE do município emissor" ;;
        E0310) campo_recusado ctribnac "use --ctribnac com um código de tributação do município" ;;
        # NL-2.10h (H1, D3): E0312 cobre o código complementar e o nacional; E0313 e E0314 pedem o código municipal; a janela abre depois.
        E0312) LEMBRAR_EXCLUIR="$LEMBRAR_EXCLUIR ctribnac"; PEDIR_NOVO=1
               campo_recusado ctribmun "use --ctribmun, o município pode exigir o código complementar, ou --ctribnac" ;;
        # NL-2.10h (H1, QA A2, decisão do main): com MEI, a dica manda NÃO informar --ctribmun (o script não envia para MEI).
        E0313) PEDIR_NOVO=1
               if [ "$OPSIMP" = 2 ]; then campo_recusado ctribmun "com MEI, não informe --ctribmun"
               else campo_recusado ctribmun "a Sefin trata o emitente como MEI: confira o regime e não informe --ctribmun"; fi ;;
        E0314) PEDIR_NOVO=1; campo_recusado ctribmun "use --ctribmun" ;;
        E0315) campo_recusado ctribmun "use --ctribmun" ;;
        # NL-2.10j, J2 (DA-2.99): E0710 (MEI), E0712 e E0713 apontam para --ptottribsn (só ME/EPP) ou para o regime certo.
        E0710|E0712|E0713) campo_recusado ptottribsn "use --ptottribsn com o percentual do seu DAS (só com ME/EPP), ou confira o regime com --opsimpnac" ;;
    esac
}

# --esquecer: apaga o arquivo de lembrar pelo caminho conferido e sai com 0, sem emitir, sem janela e sem flatpak.
esquecer_lembrar() {
    if [ "$LEMBRAR" = "$LEMBRAR_DIR/lembrar" ] && [ -e "$LEMBRAR" ]; then
        rm -f -- "$LEMBRAR"
        printf 'Lembrança apagada: %s\n' "$LEMBRAR"
    else
        printf 'Não havia nada lembrado.\n'
    fi
}

# Procura uma opção sem valor (--help, -h, --esquecer) fora das posições de valor.
procura_flag() {
    local alvo="$1" a
    shift
    while [ "$#" -gt 0 ]; do
        a=$1
        shift
        [ "$a" = "$alvo" ] && return 0
        case "$a" in
            --cnpj|--cpf|--im|--ie|--municipio|--ctribnac|--ctribmun|--ptottribsn|--opsimpnac|--regaptribsn|--regesptrib|--valor|--descricao|--local-prestacao|--trib-issqn|--ret-issqn)
                [ "$#" -gt 0 ] && shift ;;
        esac
    done
    return 1
}

ajuda() {
    cat <<'AJUDA'
Uso: emitir_teste.sh [opções]. Sem opção, o script usa o que foi lembrado e a consulta pública, e pergunta só o que falta.
Cada opção vale como --opcao valor ou --opcao=valor.

  --cnpj N        CNPJ do emissor (14 dígitos com DV). Sem ele, e sem CNPJ lembrado, a janela pede CNPJ ou CPF.
  --cpf N         CPF do emissor (11 dígitos com DV). O CPF nunca fica lembrado.
  --im T          inscrição municipal: de 1 a 15 letras, números, ponto, barra ou traço.
  --sem-im        a empresa não tem inscrição municipal no CNC: a DPS sai sem IM.
  --ie T          inscrição estadual (de 1 a 20 caracteres): só confere; não vai na NFS-e e não é guardada.
  --municipio N   código IBGE do município emissor (7 dígitos, começa de 1 a 5). Padrão: o da consulta, ou 2611606.
  --ctribnac N    código de tributação nacional (6 dígitos; não 9903xx nem 990401). Padrão 040101.
  --ptottribsn P  percentual aproximado do Simples, de 0,00 a 99,99 (vírgula ou ponto, até duas casas); só com ME/EPP (opção 3).
                  Sem ele, a nota de teste vai com 0,00 nos três percentuais e um aviso.
  --ctribmun N    código complementar municipal (3 dígitos, nunca 000; não vale para MEI). Sem ele, a janela só pergunta
                  depois de uma recusa E0312, E0313 ou E0314 da execução anterior.
  --opsimpnac N   regime do Simples: 1 não optante, 2 MEI, 3 ME/EPP. Padrão: o da consulta, ou 3.
  --regaptribsn N regime de apuração (1, 2 ou 3), só com a opção 3. Padrão 1.
  --regesptrib N  regime especial (0, 1, 2, 3, 4, 5, 6 ou 9). Padrão 0.
  --local-prestacao N  código IBGE do local da prestação (7 dígitos) ou 0000000. Padrão: o do município emissor.
  --trib-issqn N  tipo de tributação do ISSQN (1, 2, 3 ou 4). Padrão 1.
  --ret-issqn N   retenção do ISSQN: só 1 neste script (não há tomador). Padrão 1.
  --valor V       valor do serviço, como 10,00 (ponto ou vírgula). Padrão 10,00.
  --descricao T   descrição do serviço, de 1 a 2000 caracteres: letras, números, espaço e . , ; : - / ( ) %.
  --esquecer      apaga tudo o que o script lembrou (CNPJ, IM, regime e o caminho do certificado) e sai, sem emitir.

O caminho do certificado (.pfx) fica guardado no lembrado, nunca o certificado nem a senha; --esquecer apaga. Nas vezes seguintes só a senha é
pedida. Se o caminho guardado não existir mais, for link simbólico ou não for seu, a janela pergunta de novo.
O caminho nunca é impresso.
  --help, -h      mostra esta ajuda e sai com 0, sem perguntar nada.

Precedência de cada campo: opção na linha de comando, depois o lembrado, depois a consulta pública (regime e município,
só com CNPJ), depois o padrão. O lembrado fica em $XDG_STATE_HOME/notalivre-teste/lembrar (ou ~/.local/state), 0600.

AJUDA
    printf '%s\n' "$AVISO_HISTORICO"
}

# Fluxo do líder
# ---------------------------------------------------------------------------------------------------------------------
fluxo() {
    # DA-2.89: o trap de saída limpa a DPS em qualquer saída; interrupção (INT, TERM) vira saída com 130 e passa pelo trap.
    trap limpar_dps EXIT
    trap 'exit 130' INT TERM
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

    # NL-2.27 (Emenda 1, item 3): pré-voo do canal de senha, antes de qualquer pergunta e antes de consumir número. Sem kdialog a
    # senha não é pedida; sem o repasse do descritor 3 pelo flatpak instalado, a CLI não receberia a senha. Nos dois casos, para.
    [ "$TEM_KDIALOG" -eq 1 ] || falha "o kdialog não está instalado: sem ele a senha não é pedida. Nada foi emitido."
    flatpak --version > "$RUN_DIR/flatpak-versao.txt" 2>&1 || true
    local repasse
    repasse=$(flatpak run --command=sh "$APP" -c 'cat <&3' 3< <(printf ok) 2>/dev/null) || repasse=""
    [ "$repasse" = ok ] || falha "o Flatpak instalado não repassa descritores ao programa (versão em $RUN_DIR/flatpak-versao.txt). Nada foi emitido."

    # G1c: o lembrado vem antes de qualquer pergunta. Inscrição: opção, depois o CNPJ lembrado; sem nenhum dos dois, a janela
    # de inscrição (reserva da G1, decisão do coordenador). Nada disso consome número.
    lembrar_carregar
    if [ -z "$INSC_TIPO" ] && [ -n "${LEM[cnpj]+x}" ]; then definir_inscricao "${LEM[cnpj]}" cnpj; fi
    if [ -z "$INSC_TIPO" ]; then
        definir_inscricao "$(perguntar "Digite o CNPJ (14 dígitos) ou o CPF (11 dígitos) do emissor, sem pontos, barras ou traços." inscricao)" ""
    fi
    # Consulta pública (C2): só com CNPJ, sem --opsimpnac e sem regime lembrado. Falha só avisa e segue com os padrões.
    if [ "$INSC_TIPO" = cnpj ] && [ -z "${OPT[opsimpnac]+x}" ] && [ -z "${LEM[opsimpnac]+x}" ]; then
        consultar_brasilapi
    fi
    resolver_campos
    resolver_local_prestacao
    PEDIR_CTRIBMUN=${LEM[pedir_ctribmun]:-0}
    validar_ctribmun
    validar_ptottribsn
    # IM (C4): só a IM abre a janela, e só se não veio por --im, não está lembrada e não há --sem-im.
    if [ "$SEM_IM" = 0 ] && [ -z "$IM" ] && [ -n "${LEM[im]+x}" ]; then definir_im "${LEM[im]}"; fi
    if [ "$SEM_IM" = 0 ] && [ -z "$IM" ]; then
        definir_im "$(perguntar "Digite a inscrição municipal (IM) do emissor, no cadastro da Prefeitura: de 1 a 15 letras, números, ponto, barra ou traço." im)"
    fi
    janela_ctribmun
    validar_regime
    anunciar_origens
    avisar_ptottrib

    # 3.3: o .pfx é escolhido pelo líder e só passa ao document-export. Nunca é impresso nem listado.
    # DA-2.94: o caminho guardado no lembrar é usado sem janela se passar na conferência; se não passar, a janela pergunta de novo.
    local pfx=""
    if [ -n "${LEM[pfx]+x}" ]; then
        if pfx_guardado_ok "${LEM[pfx]}"; then
            pfx=${LEM[pfx]}
        else
            printf 'AVISO: o certificado guardado não pode ser usado; escolha o arquivo de novo.\n'
        fi
    fi
    if [ -z "$pfx" ]; then
        pfx=$(perguntar "Escolha o arquivo .pfx do certificado" pfx)
    fi
    # INFO-S8: caminho relativo (ou que começa por "-") não entra; o flatpak o leria como opção ou fora do lugar.
    # CISO (BAIXA-S3, condição 1): a mensagem fala só do "certificado escolhido"; o caminho nunca é impresso.
    [ -n "$pfx" ] || falha "certificado escolhido não pôde ser lido. Nada foi emitido."
    case "$pfx" in
        /*) ;;
        *) falha "o certificado escolhido precisa de caminho absoluto (começar por /). Nada foi emitido." ;;
    esac
    [ -f "$pfx" ] && [ -r "$pfx" ] || falha "certificado escolhido não pôde ser lido. Nada foi emitido."
    DOC_PFX=$(flatpak document-export --app="$APP" --forbid-write "$pfx" 2>/dev/null) || DOC_PFX=""
    PFX_USADO=$pfx
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
    chmod 600 "$CFG" || falha "não consegui fechar a permissão do config.ini. Nada foi emitido."
    DOC_PFX=""

    # 3.8 a 3.11: número novo, DPS, emissão e as tentativas 2 e 3 só pelas condições do roteiro.
    local tentativa=0 base_trocada=0 alg_trocado=0
    while :; do
        tentativa=$(( tentativa + 1 ))
        proximo_ndps
        montar_dps "$NDPS_ATUAL"
        # DA-2.92 (CTO): o número é reservado antes da chamada à CLI, depois de todos os pré-voos. Interrupção, morte por
        # sinal e incerta gastam o número; só a falha local (nada foi à rede) devolve o valor anterior.
        reservar_ndps "$NDPS_ATUAL" || falha "não consegui reservar o número $NDPS_ATUAL. Nada foi emitido."
        printf 'Tentativa %s: enviando a DPS de número %s.\n' "$tentativa" "$NDPS_ATUAL"
        emitir_tentativa "$tentativa" "$NDPS_ATUAL"
        classificar "$EMITIU_RC" "$SAIDA_ATUAL"
        if [ "$RESULTADO" = local ]; then
            devolver_ndps
        fi
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

    # G1c (C5, C6): depois da resposta da Sefin (aceita ou recusa), grava o que vale para a próxima vez; a recusa ligada a um
    # campo tira esse campo do lembrado e imprime a dica. Local e incerta não gravam.
    if [ "$RESULTADO" = aceita ] || [ "$RESULTADO" = sefin ]; then
        LEMBRAR_EXCLUIR=""
        if [ "$RESULTADO" = sefin ]; then dica_recusa; fi
        gravar_lembrar || printf 'AVISO: não consegui gravar o que foi lembrado.\n'
    fi

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
    elif [ "$RESULTADO" = incerta ]; then
        printf 'Não foi possível confirmar se a nota chegou à Sefin; avise o Claude antes de tentar de novo.\n'
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
    # Cópia da DPS que o programa leu (a DPS real some ao fim do script, DA-2.89): o ensaio confere o conteúdo por aqui.
    cp "$D/base/teste-lider/dps-teste.json" "$D/dps-emitida.json" 2>/dev/null || true
    n=$(( $(cat "$D/emitidas.txt" 2>/dev/null || echo 0) + 1 ))
    printf '%s' "$n" > "$D/emitidas.txt"
    # Senha pelo descritor (NL-2.27): confere só que o fd 3 está aberto e que a linha tem --senha-certificado-fd 3. Não lê o fd.
    if [ -e /proc/self/fd/3 ] && printf '%s' "$*" | grep -q -e '--senha-certificado-fd 3'; then
        echo "senha-fd3=ok" >> "$D/chamadas.txt"
    fi
    # Cancelado no kdialog: o pipe fecha sem bytes, e a CLI real sai com cofre.senha_vazia (teste e2e da CLI prova isso).
    if [ -f "$D/senha-cancelada" ]; then
        echo "cofre.senha_vazia: a senha do descritor está vazia"
        exit 2
    fi
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
        consulta404E0714)
            echo '{"level":"info","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"GET","status":"404"},"descartados":0}'
            echo '{"level":"info","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"POST","status":"200"},"descartados":0}'
            echo "emissão recusada (sefin.E0714): assinatura recusada"
            exit 1 ;;
        E0714)
            echo '{"level":"info","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"POST","status":"200"},"descartados":0}'
            echo "emissão recusada (sefin.E0714): assinatura recusada"
            exit 1 ;;
        sefin_falha_sem_requisicao)
            # NL-2.10e (E7, DA-2.80): "sefin.falha" sem nenhum evento sefin.requisicao não é prova de recusa da Sefin: a classificação é incerta.
            echo "emissão recusada (sefin.falha): "
            exit 1 ;;
        post5_get0)
            # NL-2.10e (R-B1, sonda SP do revisor): o POST escreveu bytes e a consulta GET falhou com zero bytes. O POST pode ter chegado:
            # incerta, e nunca local. O evento da etapa POST tem bytes maiores que zero.
            echo '{"level":"aviso","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.transporte_falhou","campos":{"causa":"conexao","codigoQt":"2","handshake":"nao","bytesEscritos":"5","etapa":"POST"},"descartados":0}'
            echo '{"level":"aviso","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.transporte_falhou","campos":{"causa":"conexao","codigoQt":"2","handshake":"nao","bytesEscritos":"0","etapa":"GET"},"descartados":0}'
            echo "emissão recusada (sefin.falha): "
            exit 1 ;;
        transporte_bytes0)
            # NL-2.10e (E7, E4): falha de transporte com zero bytes escritos é antes da Sefin: local.
            echo '{"level":"aviso","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.transporte_falhou","campos":{"causa":"tls","codigoQt":"4","handshake":"sim","bytesEscritos":"0","etapa":"POST"},"descartados":0}'
            echo "emissão recusada (sefin.falha): "
            exit 1 ;;
        transporte_bytes5)
            # NL-2.10e (E7, E4): com bytes escritos, o POST pode ter chegado: incerta.
            echo '{"level":"aviso","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.transporte_falhou","campos":{"causa":"conexao","codigoQt":"2","handshake":"nao","bytesEscritos":"5","etapa":"POST"},"descartados":0}'
            echo "emissão recusada (sefin.falha): "
            exit 1 ;;
        morto139)
            # NL-2.27 (B1, revisão de segurança): POST registrado e processo morto. A Sefin pode ter recebido a DPS: a classificação é incerta.
            echo '{"level":"info","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"POST","status":"200"},"descartados":0}'
            exit 139 ;;
        post_sem_texto)
            # NL-2.27 (R2-B2): POST registrado e saída comum (rc 1), sem texto sefin. Só a regra POST torna isso incerta.
            echo '{"level":"info","ts":"2026-11-01T00:00:00Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"POST","status":"200"},"descartados":0}'
            exit 1 ;;
        morto137)
            # Processo morto sem nenhum POST na saída: também incerta, porque o corte pode ter sido depois do envio.
            exit 137 ;;
        local)
            echo "certificado.caminho_ilegivel: ensaio"
            exit 2 ;;
        interrompe)
            # DA-2.89: o usuário interrompe o script enquanto o programa roda (SIGINT ao pai, que é o script).
            kill -INT "$PPID"
            exit 130 ;;
        E0[0-9][0-9][0-9])
            # Recusa com código da Sefin ligado a um campo (NL-2.10g, rodada G1c, item 14).
            echo "emissão recusada (sefin.$resultado): ensaio"
            exit 1 ;;
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
    --version) echo "Flatpak 1.18.4 (ensaio)"; exit 0 ;;
    remote-add) exit 0 ;;
    install)
        case "$*" in
            *org.kde.Platform*) : > "$D/runtime-ok" ;;
            *) : > "$D/app-ok" ;;
        esac
        exit 0 ;;
    document-export)
        # DA-2.89: o portal lista cada exportação (id por arquivo: a DPS é ab12, o resto cd34); document-unexport a tira da lista.
        for ultimo in "$@"; do :; done
        base_doc=$(basename "$ultimo")
        case "$base_doc" in
            dps-teste.json) id_doc=ab12 ;;
            *) id_doc=cd34 ;;
        esac
        printf '%s %s\n' "$id_doc" "$base_doc" >> "$D/portal.txt"
        printf '/run/user/1000/doc/%s/%s\n' "$id_doc" "$base_doc"
        exit 0 ;;
    document-unexport)
        for ultimo in "$@"; do :; done
        if [ -f "$D/portal.txt" ]; then
            grep -v -F "$ultimo " "$D/portal.txt" > "$D/portal.tmp"
            mv -f "$D/portal.tmp" "$D/portal.txt"
        fi
        exit 0 ;;
    run)
        case "$*" in
            *--command=ls*)
                [ "${NL_ENSAIO_CA:-ok}" = faltando ] && exit 1
                exit 0 ;;
            *"cat <&3"*)
                # Pré-voo do repasse do descritor (NL-2.27): o stub devolve o que chegou ao fd 3, ou nada se o repasse está desligado.
                [ "${NL_ENSAIO_FD_NAO_REPASSA:-}" = 1 ] && exit 0
                cat <&3
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
# Stub do kdialog para o ensaio: devolve um .pfx de faz de conta e uma inscrição (CNPJ ou CPF) e uma IM de teste.
case "$1" in
    --getopenfilename)
        # DA-2.94: registra que a janela do .pfx abriu (arquivo à parte, para não mexer nas listas de perguntas).
        echo pfx >> "$NL_ENSAIO_DIR/pfx-perguntado.txt"
        [ "${NL_ENSAIO_PFX_VAZIO:-}" = 1 ] && exit 1
        if [ "${NL_ENSAIO_PFX_RELATIVO:-}" = 1 ]; then
            printf '%s\n' "pfxsecreto-ensaio.p12"
            exit 0
        fi
        printf '%s\n' "$NL_ENSAIO_DIR/${NL_ENSAIO_PFX_NOME:-pfxdir-ensaio/pfxsecreto-ensaio.p12}"
        exit 0 ;;
    --inputbox)
        # NL-2.10g: a janela da inscrição e a da IM são distinguidas pelo texto; cada pergunta fica registrada (só o tipo).
        case "$2" in
            *"inscrição municipal"*)
                echo im >> "$NL_ENSAIO_DIR/perguntas.txt"
                [ -n "${NL_ENSAIO_IM_CANCELA:-}" ] && exit 1
                printf '%s\n' "${NL_ENSAIO_IM_JANELA-IMENS-7731}"
                exit 0 ;;
            *"complementar municipal"*)
                # NL-2.10h (H1): janela do cTribMun; cancelada ou com valor de ensaio (501 por padrão).
                echo ctribmun >> "$NL_ENSAIO_DIR/perguntas.txt"
                [ -n "${NL_ENSAIO_CTRIBMUN_CANCELA:-}" ] && exit 1
                printf '%s\n' "${NL_ENSAIO_CTRIBMUN_JANELA-501}"
                exit 0 ;;
            *)
                echo inscricao >> "$NL_ENSAIO_DIR/perguntas.txt"
                printf '%s\n' "${NL_ENSAIO_INSCRICAO:-${NL_ENSAIO_CNPJ:-11222333000181}}"
                exit 0 ;;
        esac ;;
    --password)
        # NL-2.27 (I9): guarda o texto que o script mostrou, para o ensaio conferir que é o genérico.
        printf '%s\n' "$2" >> "$NL_ENSAIO_DIR/kdialog-texto.txt"
        # Cancelado: sai com 1 e não escreve nada, e marca o cancelamento para o stub do flatpak (ver emitir_stub).
        if [ -n "${NL_ENSAIO_KDIALOG_CANCELA:-}" ]; then
            : > "$NL_ENSAIO_DIR/senha-cancelada"
            exit 1
        fi
        printf '%s\n' "${NL_ENSAIO_SENHA:-NL227-senha-do-ensaio}"
        exit 0 ;;
esac
exit 1
STUB
    # NL-2.10g (G1c): curl de faz de conta. Nunca toca a rede; responde pelo modo em NL_ENSAIO_BRASILAPI. O registro guarda só o
    # nome do comando, sem a URL (que leva o CNPJ).
    cat > "$dir/bin/curl" <<'STUB'
#!/bin/sh
D="$NL_ENSAIO_DIR"
echo "curl consulta" >> "$D/chamadas.txt"
# Rodada 2 (revisor, mutante 11): os argumentos ficam num arquivo à parte, para o ensaio conferir --proto =https.
printf '%s\n' "$*" >> "$D/curl-argumentos.txt"
# NL-2.10h (H4): com NL_ENSAIO_URL_ESPERADA, o curl só responde se a URL traz exatamente esse caminho (CNPJ com zeros, sem conversão
# numérica); qualquer outra URL falha como rede, e a consulta cai nos padrões.
if [ -n "${NL_ENSAIO_URL_ESPERADA:-}" ]; then
    case "$*" in *"$NL_ENSAIO_URL_ESPERADA"*) ;; *) exit 22 ;; esac
fi
CORPO='"razao_social":"RAZAO-FICTICIA","qsa":[{"nome_socio":"SOCIO-FICTICIO"}]'
SN='"opcao_pelo_mei":false,"data_exclusao_do_mei":null,"opcao_pelo_simples":true,"data_exclusao_do_simples":null'
case "${NL_ENSAIO_BRASILAPI:-simples}" in
    mei) echo "{\"opcao_pelo_mei\":true,\"data_exclusao_do_mei\":null,\"opcao_pelo_simples\":false,\"data_exclusao_do_simples\":null,\"codigo_municipio_ibge\":2611606,\"situacao_cadastral\":2,$CORPO}" ;;
    simples) echo "{$SN,\"codigo_municipio_ibge\":2611606,\"situacao_cadastral\":2,$CORPO}" ;;
    nulo) echo "{\"opcao_pelo_mei\":null,\"data_exclusao_do_mei\":null,\"opcao_pelo_simples\":null,\"data_exclusao_do_simples\":null,\"codigo_municipio_ibge\":2611606,\"situacao_cadastral\":2,$CORPO}" ;;
    exclusao) echo "{\"opcao_pelo_mei\":false,\"data_exclusao_do_mei\":null,\"opcao_pelo_simples\":true,\"data_exclusao_do_simples\":\"2020-01-01\",\"codigo_municipio_ibge\":2611606,\"situacao_cadastral\":2,$CORPO}" ;;
    municipio) echo "{$SN,\"codigo_municipio_ibge\":3550308,\"situacao_cadastral\":2,$CORPO}" ;;
    situacao8) echo "{$SN,\"codigo_municipio_ibge\":2611606,\"situacao_cadastral\":8,$CORPO}" ;;
    rede) exit 6 ;;
    json) echo '{nao e json' ;;
    campo) echo "{\"opcao_pelo_simples\":true,\"data_exclusao_do_simples\":null,\"situacao_cadastral\":2,$CORPO}" ;;
esac
exit 0
STUB
    chmod 755 "$dir/bin/flatpak" "$dir/bin/kdialog" "$dir/bin/curl"
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
    local dir="$RAIZ/$nome" entrada rc saidas emit problema="" pfx_nome="pfxdir-ensaio/pfxsecreto-ensaio.p12" sem_kd=0 par ndps_esp=""
    local semente="" perg_esp="" dps_esp="" dps_ausente="" saida_esp="" segredo="" opcoes_txt="" arq_ausente="" portal_ausente="" cham_esp=""
    local lem_seed="" lem_modo="" lem_esp="" lem_aus="" cham_aus="" curl_esp="" varrer_disco="" plantar="" pfx_janela="" lem="$dir/home/.local/state/notalivre-teste/lembrar"
    mkdir -p "$dir"
    for par in "$@"; do
        case "$par" in
            NL_ENSAIO_SEM_KDIALOG=1) sem_kd=1 ;;
            NL_ENSAIO_PFX_NOME=*) pfx_nome="${par#NL_ENSAIO_PFX_NOME=}" ;;
            NL_ENSAIO_NDPS_ESPERADO=*) ndps_esp="${par#NL_ENSAIO_NDPS_ESPERADO=}" ;;
            NL_ENSAIO_SEMENTE=*) semente="${par#NL_ENSAIO_SEMENTE=}" ;;
            NL_ENSAIO_PERGUNTAS=*) perg_esp="${par#NL_ENSAIO_PERGUNTAS=}" ;;
            NL_ENSAIO_DPS_ESPERADA=*) dps_esp="${par#NL_ENSAIO_DPS_ESPERADA=}" ;;
            NL_ENSAIO_DPS_AUSENTE=*) dps_ausente="${par#NL_ENSAIO_DPS_AUSENTE=}" ;;
            NL_ENSAIO_SAIDA_ESPERADA=*) saida_esp="${par#NL_ENSAIO_SAIDA_ESPERADA=}" ;;
            NL_ENSAIO_SEGREDO=*) segredo="${par#NL_ENSAIO_SEGREDO=}" ;;
            NL_ENSAIO_OPCOES=*) opcoes_txt="${par#NL_ENSAIO_OPCOES=}" ;;
            NL_ENSAIO_ARQUIVO_AUSENTE=*) arq_ausente="${par#NL_ENSAIO_ARQUIVO_AUSENTE=}" ;;
            NL_ENSAIO_PORTAL_AUSENTE=*) portal_ausente="${par#NL_ENSAIO_PORTAL_AUSENTE=}" ;;
            NL_ENSAIO_CHAMADA_ESPERADA=*) cham_esp="${par#NL_ENSAIO_CHAMADA_ESPERADA=}" ;;
            NL_ENSAIO_CHAMADA_AUSENTE=*) cham_aus="${par#NL_ENSAIO_CHAMADA_AUSENTE=}" ;;
            NL_ENSAIO_LEMBRAR=*) lem_seed="${par#NL_ENSAIO_LEMBRAR=}" ;;
            NL_ENSAIO_LEMBRAR_MODO=*) lem_modo="${par#NL_ENSAIO_LEMBRAR_MODO=}" ;;
            NL_ENSAIO_LEMBRAR_ESPERADO=*) lem_esp="${par#NL_ENSAIO_LEMBRAR_ESPERADO=}" ;;
            NL_ENSAIO_LEMBRAR_AUSENTE=*) lem_aus="${par#NL_ENSAIO_LEMBRAR_AUSENTE=}" ;;
            NL_ENSAIO_CURL_ESPERADO=*) curl_esp="${par#NL_ENSAIO_CURL_ESPERADO=}" ;;
            NL_ENSAIO_VARRER_DISCO=*) varrer_disco="${par#NL_ENSAIO_VARRER_DISCO=}" ;;
            NL_ENSAIO_PLANTAR_JSON=*) plantar="${par#NL_ENSAIO_PLANTAR_JSON=}" ;;
            NL_ENSAIO_PFX_JANELA=*) pfx_janela="${par#NL_ENSAIO_PFX_JANELA=}" ;;
        esac
    done
    # NL-2.10g: opções da linha de comando, separadas por | (o valor pode ter espaço ou aspas). Vazio = nenhuma opção.
    # NL-2.10g (G1c): lembrado semeado (linhas separadas por |), com o modo do arquivo pedido pelo caso.
    if [ -n "$lem_seed" ]; then
        mkdir -p "$(dirname "$lem")"
        printf '%s\n' "$lem_seed" | tr '|' '\n' > "$lem"
        chmod "${lem_modo:-600}" "$lem"
    fi
    local -a opcoes=()
    [ -z "$opcoes_txt" ] || IFS='|' read -r -a opcoes <<< "$opcoes_txt"
    mkdir -p "$(dirname "$dir/$pfx_nome")"; printf 'pfxsecreto-ensaio\n' > "$dir/$pfx_nome"
    # NL-2.10g: semente do contador, para provar que a parada antes da Sefin não o mexe.
    if [ -n "$semente" ]; then
        mkdir -p "$dir/home/.local/share/notalivre/nl210"
        printf '%s\n' "$semente" > "$dir/home/.local/share/notalivre/nl210/ultimo-ndps.txt"
    fi
    entrada=/dev/null
    if [ "$sem_kd" -eq 1 ]; then
        escrever_stubs_sem_kdialog "$dir"
        entrada="$dir/entrada.txt"
        printf '11222333000181\n%s\n' "$dir/$pfx_nome" > "$entrada"
    else
        escrever_stubs "$dir"
    fi
    # Rodada 2 (revisor, mutante 12): com NL_ENSAIO_PLANTAR_JSON=1, um json.py falso fica no diretório de trabalho do parser
    # (o python3 sem -I o importaria). O caso exige que a consulta não mude.
    if [ "$plantar" = 1 ]; then
        printf '%s\n' 'def load(f):' '    return {"opcao_pelo_mei": False, "data_exclusao_do_mei": None, "opcao_pelo_simples": False,' '            "data_exclusao_do_simples": None, "codigo_municipio_ibge": 1234567, "situacao_cadastral": 2}' > "$dir/json.py"
        (cd "$dir" && env "$@" NL_ENSAIO_DIR="$dir" bash "$SELF_ABS" --ensaio-caso "${opcoes[@]}" < "$entrada" > "$dir/saida-do-script.txt" 2>&1)
    else
        env "$@" NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso "${opcoes[@]}" < "$entrada" > "$dir/saida-do-script.txt" 2>&1
    fi
    rc=$?
    saidas=$(ls "$dir"/base/teste-lider/execucao-*/saida-tentativa-*.txt 2>/dev/null | wc -l | tr -d ' ')
    emit=$(grep -c ' emitir ' "$dir/chamadas.txt" 2>/dev/null || true)
    [ -n "$emit" ] || emit=0
    [ "$rc" -eq "$rc_esp" ] || problema="$problema rc=$rc(esperado $rc_esp)"
    grep -q -F -- "$txt_esp" "$dir/saida-do-script.txt" || problema="$problema texto_ausente('$txt_esp')"
    [ "$saidas" -eq "$saidas_esp" ] || problema="$problema saidas=$saidas(esperado $saidas_esp)"
    [ "$emit" -eq "$emit_esp" ] || problema="$problema emitir=$emit(esperado $emit_esp)"
    # NL-2.27: cada emissão tem de receber a senha pelo descritor 3, e o contador só anda quando a tentativa chegou à Sefin.
    local fd3 ndps_atual=ausente
    fd3=$(grep -c 'senha-fd3=ok' "$dir/chamadas.txt" 2>/dev/null || true)
    [ -n "$fd3" ] || fd3=0
    [ "$emit" -eq 0 ] || [ "$fd3" -eq "$emit" ] || problema="$problema senha_fora_do_descritor(fd3=$fd3,emitir=$emit)"
    [ -f "$dir/home/.local/share/notalivre/nl210/ultimo-ndps.txt" ] && ndps_atual=$(cat "$dir/home/.local/share/notalivre/nl210/ultimo-ndps.txt")
    [ -z "$ndps_esp" ] || [ "$ndps_atual" = "$ndps_esp" ] || problema="$problema ultimo_ndps=$ndps_atual(esperado $ndps_esp)"
    local cfg_arq="$dir/home/.var/app/$APP/config/notalivre/config.ini" padrao
    if [ -n "$cfg_esp" ]; then
        printf '%s\n' "$cfg_esp" | tr '|' '\n' > "$dir/cfg-esperados.txt"
        while IFS= read -r padrao; do
            grep -q -F -- "$padrao" "$cfg_arq" 2>/dev/null || problema="$problema config('$padrao')"
        done < "$dir/cfg-esperados.txt"
    fi
    # CISO (BAIXA-S3, condição 2): o config.ini que o script escreve tem de ficar em 0600.
    if [ -f "$cfg_arq" ]; then
        [ "$(stat -c %a "$cfg_arq")" = 600 ] || problema="$problema config.ini_modo_$(stat -c %a "$cfg_arq")(esperado 600)"
    fi
    if grep -r -l -F -e pfxsecreto -e 11222333000181 -e 1122233300018A "$dir/saida-do-script.txt" "$dir/base/devolucao" \
        "$dir"/base/teste-lider/execucao-* >/dev/null 2>&1; then
        problema="$problema vazamento(pfx ou CNPJ em saída, devolução ou execução)"
    fi
    # NL-2.10g (k, L-17): CPF, IM e IE de fixture também não podem sair da saída, da devolução, da execução nem do registro de chamadas.
    # O registro de chamadas guarda o caminho do .pfx (argumento do document-export); por isso o pfx não entra nesta varredura.
    if grep -r -l -F -e 12345678909 -e IMENS-7731 -e IEENS-5520 -e DESCSEG-ENSAIO -e RAZAO-FICTICIA -e SOCIO-FICTICIO "$dir/saida-do-script.txt" "$dir/chamadas.txt" \
        "$dir/base/devolucao" "$dir"/base/teste-lider/execucao-* >/dev/null 2>&1; then
        problema="$problema vazamento(CPF, IM ou IE em saída, devolução, execução ou chamadas)"
    fi
    # NL-2.27 (I9, revisão de segurança): o texto da janela do kdialog é o genérico, sem CNPJ nem nome de arquivo.
    if [ -f "$dir/kdialog-texto.txt" ]; then
        grep -q -F -- "Senha do certificado digital, para a emissão de teste." "$dir/kdialog-texto.txt" || problema="$problema texto_kdialog_fora_do_padrao"
        grep -q -F -e pfxsecreto -e 11222333000181 -e 1122233300018A "$dir/kdialog-texto.txt" && problema="$problema texto_kdialog_com_dado_pessoal"
    fi
    # NL-2.27: a senha do kdialog nunca aparece na saída do script, na devolução nem na linha de comando do flatpak (chamadas).
    if grep -r -l -F -e NL227-senha-do-ensaio "$dir/saida-do-script.txt" "$dir/base/devolucao" "$dir/chamadas.txt" \
        "$dir"/base/teste-lider/execucao-* >/dev/null 2>&1; then
        problema="$problema vazamento_da_senha"
    fi
    # NL-2.10g: perguntas da janela, trechos da DPS e da saída, e o segredo da variante (valor inválido) fora da DPS.
    if [ -n "$perg_esp" ]; then
        local perg
        perg=$(paste -sd, "$dir/perguntas.txt" 2>/dev/null) || true
        [ -n "$perg" ] || perg=nenhuma
        [ "$perg" = "$perg_esp" ] || problema="$problema perguntas=$perg(esperado $perg_esp)"
    fi
    local r
    r=$(trechos "$dir/dps-emitida.json" presente "$dps_esp")
    [ -z "$r" ] || problema="$problema dps$r"
    r=$(trechos "$dir/dps-emitida.json" ausente "$dps_ausente")
    [ -z "$r" ] || problema="$problema dps$r"
    r=$(trechos "$dir/saida-do-script.txt" presente "$saida_esp")
    [ -z "$r" ] || problema="$problema saida$r"
    if [ -n "$segredo" ] && grep -r -l -F -e "$segredo" "$dir/saida-do-script.txt" "$dir/chamadas.txt" \
        "$dir/base/devolucao" "$dir"/base/teste-lider/execucao-* >/dev/null 2>&1; then
        problema="$problema vazamento_do_valor_da_variante"
    fi
    # DA-2.94: a janela do .pfx abre (sim) ou não abre (nao) nesta execução.
    if [ -n "$pfx_janela" ]; then
        local pfx_got=nao
        [ -e "$dir/pfx-perguntado.txt" ] && pfx_got=sim
        [ "$pfx_got" = "$pfx_janela" ] || problema="$problema janela_pfx=$pfx_got(esperado $pfx_janela)"
    fi
    # Rodada 3 (revisor, R3-B2): o caminho do .pfx não fica em nenhum arquivo de base nem de home (XDG_STATE_HOME está dentro de home),
    # exceto o arquivo de lembrar (o único lugar permitido). O document-export fica em chamadas.txt e não entra aqui.
    # Rodada 9 (revisor, LEAK_DIR): a varredura procura DUAS marcas, sem trocar uma pela outra: o nome do pfx em qualquer pasta
    # ($dir/pfxsecreto) e a pasta que o contém ($dir/pfxdir). DA-2.97 (CISO): esta varredura cobre o nome do .pfx e a pasta que o
    # contém, não os ancestrais de $dir. O V_PAI (diretório pai do caso no argv) é residual declarado e vai para a INBOX, com a NL-3.13.
    achados_pfx=$(grep -r -l -F -e "$dir/pfxsecreto" -e "$dir/pfxdir" "$dir/base" "$dir/home" 2>/dev/null | grep -v -x -F -- "$lem" || true)
    [ -z "$achados_pfx" ] || problema="$problema caminho_do_pfx_fora_do_lembrar"
    # DA-2.94: o caminho do .pfx não aparece nas chamadas, salvo no document-export, que é o caminho já aceito.
    # Rodada 4 (revisor, R4-B1): a exceção é só a linha exata do document-export ("flatpak document-export --app=..." com o caminho
    # como último argumento). Qualquer outra linha de chamadas que tenha o caminho reprova, mesmo que tenha a palavra document-export.
    # Rodada 5 (revisor, R4-B1 e inferência fechada): a única linha de chamadas que pode ter o caminho é, por igualdade da linha
    # inteira, "flatpak document-export --app=<APP> --forbid-write <caminho>". O caminho é o da janela ($dir/<pfx_nome>) ou o guardado
    # que o caso semeou (pfx=...). Nenhuma outra forma passa.
    # Rodada 6 (revisor, R5-B1 e R5-B2): a checagem procura o MARCADOR do nome do arquivo (pfxsecreto, o nome do pfx da janela e o nome
    # do pfx semeado, por exemplo hosts), não o caminho inteiro. Qualquer linha de chamadas.txt com marcador tem de ser exatamente a
    # linha do document-export; as outras reprovam, mesmo com o caminho partido em dois argumentos ou só o nome do arquivo.
    if [ -f "$dir/chamadas.txt" ]; then
        local pfx_semeado="" prefixo_doc="flatpak document-export --app=$APP --forbid-write " pfx_base_semeado="" linha_pfx m
        local -a marcas=(pfxsecreto pfxdir) args_grep=()
        pfx_semeado=$(printf '%s' "$lem_seed" | tr '|' '\n' | sed -n 's/^pfx=//p')
        [ -n "$pfx_nome" ] && marcas+=("$(basename -- "$pfx_nome")")
        if [ -n "$pfx_semeado" ]; then
            pfx_base_semeado=$(basename -- "$pfx_semeado")
            [ -n "$pfx_base_semeado" ] && marcas+=("$pfx_base_semeado")
        fi
        for m in "${marcas[@]}"; do args_grep+=(-e "$m"); done
        # DA-2.95 (CISO, opção B): a varredura aceita o marcador do pfx em exatamente DUAS linhas, por igualdade da linha inteira:
        # o document-export (caminho do host) e o config certificado (caminho do portal, /run/user/1000/doc/cd34/<nome>, que o stub
        # gera). A opção (C), tirar o nome do argv do notalivre-cli, fica na INBOX, com destino na NL-4.4. Qualquer outra linha reprova.
        local prefixo_cli="flatpak run --command=notalivre-cli $APP config certificado /run/user/1000/doc/cd34/" ok
        while IFS= read -r linha_pfx; do
            ok=0
            [ "$linha_pfx" = "$prefixo_doc$dir/$pfx_nome" ] && ok=1
            [ "$linha_pfx" = "$prefixo_cli$(basename -- "$pfx_nome")" ] && ok=1
            if [ -n "$pfx_semeado" ]; then
                [ "$linha_pfx" = "$prefixo_doc$pfx_semeado" ] && ok=1
                [ "$linha_pfx" = "$prefixo_cli$pfx_base_semeado" ] && ok=1
            fi
            [ "$ok" = 1 ] || problema="$problema marcador_do_pfx_fora_das_duas_linhas"
        done < <(grep -F "${args_grep[@]}" -- "$dir/chamadas.txt" 2>/dev/null || true)
    fi
    # Rodada 2 (revisor, mutante 3a): a resposta crua da BrasilAPI não pode ir a disco. Varre toda a árvore de base e de
    # XDG_STATE_HOME atrás das marcas do stub (razão e sócio fictícios).
    if [ "$varrer_disco" = 1 ] && grep -r -l -F -e RAZAO-FICTICIA -e SOCIO-FICTICIO "$dir/base" "$dir/home" >/dev/null 2>&1; then
        problema="$problema resposta_da_consulta_em_disco"
    fi
    # Rodada 2 (revisor, mutante 11): a chamada ao curl tem de ser com --proto =https.
    r=$(trechos "$dir/curl-argumentos.txt" presente "$curl_esp")
    [ -z "$r" ] || problema="$problema curl$r"
    # NL-2.10g (G1c): o arquivo de lembrar, quando existe, é 0600 numa pasta 0700; confere as chaves pedidas e as que não podem estar.
    if [ -e "$lem" ]; then
        [ "$(stat -c %a "$lem")" = 600 ] || problema="$problema lembrar_modo_$(stat -c %a "$lem")(esperado 600)"
        [ "$(stat -c %a "$(dirname "$lem")")" = 700 ] || problema="$problema lembrar_pasta_modo(esperado 700)"
    fi
    r=$(trechos "$lem" presente "$lem_esp" 2>/dev/null)
    [ -z "$r" ] || problema="$problema lembrar$r"
    r=$(trechos "$lem" ausente "$lem_aus" 2>/dev/null)
    [ -z "$r" ] || problema="$problema lembrar$r"
    if [ -n "$cham_aus" ] && grep -q -F -- "$cham_aus" "$dir/chamadas.txt" 2>/dev/null; then
        problema="$problema chamada_proibida($cham_aus)"
    fi
    # DA-2.89 (B1): a DPS temporária não existe depois da execução, e o portal não lista o documento.
    if [ -n "$arq_ausente" ] && [ -e "$dir/$arq_ausente" ]; then
        problema="$problema arquivo_ainda_existe($arq_ausente)"
    fi
    if [ -n "$portal_ausente" ] && grep -q -F -- "$portal_ausente" "$dir/portal.txt" 2>/dev/null; then
        problema="$problema portal_ainda_lista($portal_ausente)"
    fi
    r=$(trechos "$dir/chamadas.txt" presente "$cham_esp")
    [ -z "$r" ] || problema="$problema chamadas$r"
    if [ -z "$problema" ]; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s:%s\n' "$nome" "$problema"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# Trechos separados por | que têm de estar (presente) ou não estar (ausente) num arquivo. Imprime " falta(...)" ou " sobra(...)"
# por trecho que falhou; saída vazia = ok. Os trechos são fixtures de ensaio, nunca dado real.
trechos() {
    local arq="$1" modo="$2" lista="$3" trecho
    local -a itens=()
    [ -n "$lista" ] || return 0
    IFS='|' read -r -a itens <<< "$lista"
    for trecho in "${itens[@]}"; do
        if [ "$modo" = presente ]; then
            grep -q -F -- "$trecho" "$arq" 2>/dev/null || printf ' falta(%s)' "$trecho"
        else
            grep -q -F -- "$trecho" "$arq" 2>/dev/null && printf ' sobra(%s)' "$trecho"
        fi
    done
    return 0
}

# Grupo de variantes: cada variante imprime a sua linha; o grupo conta como um caso só (contagem da ordem NL-2.10g).
grupo_abrir() {
    G_CASOS=$CASOS
    G_FALHAS=$FALHAS
}

grupo_fechar() {
    local nome="$1" falhas_grupo=$(( FALHAS - G_FALHAS ))
    CASOS=$(( G_CASOS + 1 ))
    if [ "$falhas_grupo" -eq 0 ]; then
        printf 'ok grupo %s\n' "$nome"
        FALHAS=$G_FALHAS
    else
        printf 'FALHA grupo %s: %s variante(s)\n' "$nome" "$falhas_grupo"
        FALHAS=$(( G_FALHAS + 1 ))
    fi
}

# DA-2.94 (líder, "Guardar o local"): a primeira execução abre a janela do .pfx e guarda o caminho; a segunda não abre.
ensaio_segunda_sem_pfx() {
    local nome=pfx_segunda_execucao_sem_janela dir r1 r2 problema=""
    dir="$RAIZ/$nome"
    mkdir -p "$dir"
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    mv -f -- "$dir/pfx-perguntado.txt" "$dir/pfx-perguntado-1.txt" 2>/dev/null || true
    env NL_ENSAIO_SEQ=aceita,aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    [ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] || problema="$problema rc=$r1/$r2"
    [ -e "$dir/pfx-perguntado.txt" ] && problema="$problema janela_do_pfx_abriu_na_segunda"
    grep -q '"nDps": 2,' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ndps_2_nao_e_2"
    if [ -z "$problema" ]; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s:%s\n' "$nome" "$problema"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# DA-2.92 (CTO): duas execuções seguidas na mesma pasta. Confere o número depois da primeira (rc1 e ndps1) e o nDps da segunda.
ensaio_sequencia() {
    local nome="$1" seq1="$2" seq2="$3" rc1_esp="$4" ndps1_esp="$5" ndps2_esp="$6" dir r1 r2 ndps1 problema=""
    dir="$RAIZ/$nome"
    mkdir -p "$dir"
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ="$seq1" NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    ndps1=$(cat "$dir/home/.local/share/notalivre/nl210/ultimo-ndps.txt" 2>/dev/null || echo ausente)
    env NL_ENSAIO_SEQ="$seq2" NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    [ "$r1" -eq "$rc1_esp" ] || problema="$problema rc1=$r1(esperado $rc1_esp)"
    [ "$ndps1" = "$ndps1_esp" ] || problema="$problema ndps_apos_1=$ndps1(esperado $ndps1_esp)"
    grep -q "\"nDps\": $ndps2_esp," "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ndps_2_nao_e_$ndps2_esp"
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
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    env NL_ENSAIO_SEQ=aceita,aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    ndps=$(cat "$dir/home/.local/share/notalivre/nl210/ultimo-ndps.txt" 2>/dev/null)
    if [ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] && [ "$ndps" = 2 ] && grep -q '"nDps": 2,' "$dir/dps-emitida.json"; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s: rc=%s/%s ultimo_ndps=%s\n' "$nome" "$r1" "$r2" "$ndps"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# NL-2.10h (H1): janela do cTribMun só depois de uma recusa E0312, E0313 ou E0314 na execução anterior, sem valor por opção
# nem lembrado. Duas execuções na mesma pasta: a primeira com $2; a segunda com a sequência "$2,$3" (o stub conta as emissões),
# mais a variável de ambiente $4, se houver. $5 = sim ou nao (janela aberta na segunda). $6 = rc esperado da segunda. $7 = nDps
# que fica gravado depois da segunda (prova de que a parada antes da Sefin não gasta número).
ensaio_ctribmun_janela() {
    local nome="$1" seq1="$2" seq2="$3" extra="$4" janela_esp="$5" rc2_esp="$6" ndps2_esp="$7" dir r2 janela ndps2 problema=""
    dir="$RAIZ/$nome"
    mkdir -p "$dir"
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ="$seq1" NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    mv -f -- "$dir/perguntas.txt" "$dir/perguntas-1.txt" 2>/dev/null || true
    env NL_ENSAIO_SEQ="$seq1,$seq2" NL_ENSAIO_DIR="$dir" $extra bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    if grep -q '^ctribmun$' "$dir/perguntas.txt" 2>/dev/null; then janela=sim; else janela=nao; fi
    ndps2=$(cat "$dir/home/.local/share/notalivre/nl210/ultimo-ndps.txt" 2>/dev/null || echo ausente)
    [ "$r2" -eq "$rc2_esp" ] || problema="$problema rc2=$r2(esperado $rc2_esp)"
    [ "$janela" = "$janela_esp" ] || problema="$problema janela=$janela(esperada $janela_esp)"
    [ "$ndps2" = "$ndps2_esp" ] || problema="$problema ndps_apos_2=$ndps2(esperado $ndps2_esp)"
    if [ "$rc2_esp" -eq 0 ] && [ "$janela_esp" = sim ]; then
        grep -q '"cTribMun": "501"' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ctribmun_501_ausente_na_dps"
    fi
    if [ "$rc2_esp" -eq 0 ] && [ "$janela_esp" = nao ]; then
        grep -q '"cTribMun"' "$dir/dps-emitida.json" 2>/dev/null && problema="$problema ctribmun_sem_janela"
    fi
    if [ -z "$problema" ]; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s:%s\n' "$nome" "$problema"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# NL-2.10h (H1, QA A3/M5): o cTribMun dado por opção grava no lembrar, e a execução seguinte, sem opção nem janela, usa o valor gravado.
ensaio_ctribmun_gravado() {
    local nome="$1" dir r1 r2 problema=""
    dir="$RAIZ/$nome"
    mkdir -p "$dir"
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso --ctribmun 501 < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    mv -f -- "$dir/perguntas.txt" "$dir/perguntas-1.txt" 2>/dev/null || true
    env NL_ENSAIO_SEQ=aceita,aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    [ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] || problema="$problema rc=$r1/$r2"
    grep -q '"cTribMun": "501"' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ctribmun_501_nao_lido_do_lembrar"
    grep -q '^ctribmun$' "$dir/perguntas.txt" 2>/dev/null && problema="$problema janela_abriu_sem_recusa"
    if [ -z "$problema" ]; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s:%s\n' "$nome" "$problema"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# NL-2.10j, J2: --ptottribsn dado por opção grava no lembrar, e a execução seguinte (sem opção) manda o mesmo valor em pTotTribSN.
ensaio_ptottribsn_gravado() {
    local nome="$1" valor="$2" dir r1 r2 problema=""
    dir="$RAIZ/$nome"
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso --opsimpnac 3 --ptottribsn "$valor" < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    mv -f -- "$dir/perguntas.txt" "$dir/perguntas-1.txt" 2>/dev/null || true
    env NL_ENSAIO_SEQ=aceita,aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    [ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] || problema="$problema rc=$r1/$r2"
    grep -q '"pTotTribSN": "7.35"' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ptottribsn_nao_lido_do_lembrar"
    if [ -z "$problema" ]; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s:%s\n' "$nome" "$problema"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# NL-2.10j, J2 (DA-2.99, "o percentual não sai em log fora da DPS"): o valor digitado não aparece em nenhum arquivo do caso, salvo a DPS.
ensaio_ptottribsn_sem_vazamento() {
    local nome="$1" dir r problema="" achados
    dir="$RAIZ/$nome"
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso --opsimpnac 3 --ptottribsn 7,35 < /dev/null > "$dir/saida-1.txt" 2>&1
    r=$?
    [ "$r" -eq 0 ] || problema="$problema rc=$r"
    # D3: o lembrar guarda o --ptottribsn (arquivo 0600, o único lugar permitido fora da DPS); a varredura cobre o resto do caso.
    achados=$(grep -r -l -F -e '7,35' -e '7.35' "$dir" 2>/dev/null | grep -v -x -F -e "$dir/dps-emitida.json" -e "$dir/base/teste-lider/dps-teste.json" -e "$dir/home/.local/state/notalivre-teste/lembrar" || true)
    [ -z "$achados" ] || problema="$problema percentual_fora_da_dps"
    if [ -z "$problema" ]; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s:%s\n' "$nome" "$problema"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# NL-3.0i: fecha um caso de contador durável (nome e problema vazio = ok).
ensaio_duravel_fecha() {
    if [ -z "$2" ]; then
        printf 'ok %s\n' "$1"
    else
        printf 'FALHA %s:%s\n' "$1" "$2"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

# NL-3.0i: o durável sobrevive a apagar a pasta antiga. Corrida 1 grava 1 no durável; a pasta $dir/base é movida (mv, não rm) e
# a corrida 2 segue do durável: nDps 2.
ensaio_duravel_sobrevive_a_apagar_base() {
    local nome=duravel_sobrevive_a_apagar_base dir d r1 r2 problema=""
    dir="$RAIZ/$nome"; d="$dir/home/.local/share/notalivre/nl210"
    mkdir -p "$dir/pfxdir-ensaio"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    [ "$r1" -eq 0 ] || problema="$problema rc1=$r1(esperado 0)"
    [ "$(cat "$d/ultimo-ndps.txt" 2>/dev/null)" = 1 ] || problema="$problema duravel_apos_1=$(cat "$d/ultimo-ndps.txt" 2>/dev/null || echo ausente)(esperado 1)"
    if [ -d "$dir/base" ]; then
        mv "$dir/base" "$dir/base-apagada" || problema="$problema mv_base_falhou"
    else
        problema="$problema sem_pasta_base_antes_de_apagar"
    fi
    env NL_ENSAIO_SEQ=aceita,aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    [ "$r2" -eq 0 ] || problema="$problema rc2=$r2(esperado 0)"
    grep -q '"nDps": 2,' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ndps_2_nao_e_2"
    [ "$(cat "$d/ultimo-ndps.txt" 2>/dev/null)" = 2 ] || problema="$problema duravel_apos_2(esperado 2)"
    ensaio_duravel_fecha "$nome" "$problema"
}

# NL-3.0i: a pasta antiga com contador e o durável ausente: copia uma vez, com 0600, e segue (corrida 1 -> nDps 8, corrida 2 -> 9).
# A pasta antiga fica intacta (cópia, não mudança de lugar). A pasta do durável sai com 0700.
ensaio_duravel_copia_uma_vez() {
    local nome=duravel_copia_uma_vez dir d r1 r2 problema=""
    dir="$RAIZ/$nome"; d="$dir/home/.local/share/notalivre/nl210"
    mkdir -p "$dir/pfxdir-ensaio" "$dir/base"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    printf '7\n' > "$dir/base/ultimo-ndps.txt"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    [ "$r1" -eq 0 ] || problema="$problema rc1=$r1(esperado 0)"
    grep -q '"nDps": 8,' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ndps_1_nao_e_8"
    [ "$(stat -c %a "$d/ultimo-ndps.txt" 2>/dev/null)" = 600 ] || problema="$problema modo_do_duravel_nao_600"
    [ "$(cat "$dir/base/ultimo-ndps.txt" 2>/dev/null)" = 7 ] || problema="$problema pasta_antiga_mudou"
    ensaio_duravel_fecha copia_uma_vez_0600 "$problema"
    problema=""
    [ "$(stat -c %a "$d" 2>/dev/null)" = 700 ] || problema="$problema modo_da_pasta_duravel_nao_700"
    ensaio_duravel_fecha pasta_duravel_0700 "$problema"
    problema=""
    env NL_ENSAIO_SEQ=aceita,aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-2.txt" 2>&1
    r2=$?
    [ "$r2" -eq 0 ] || problema="$problema rc2=$r2(esperado 0)"
    grep -q '"nDps": 9,' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ndps_2_nao_e_9"
    [ "$(cat "$dir/base/ultimo-ndps.txt" 2>/dev/null)" = 7 ] || problema="$problema copia_repetida_da_pasta_antiga"
    ensaio_duravel_fecha copia_uma_vez_segue "$problema"
}

# NL-3.0i: com o durável e a pasta antiga presentes, vale o durável: corrida grava 4 (durável tinha 3); a pasta antiga (90) fica.
ensaio_duravel_vale_o_duravel() {
    local nome=duravel_vale_o_duravel dir d r1 problema=""
    dir="$RAIZ/$nome"; d="$dir/home/.local/share/notalivre/nl210"
    mkdir -p "$dir/pfxdir-ensaio" "$dir/base" "$d"; printf 'pfxsecreto-ensaio' > "$dir/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    printf '90\n' > "$dir/base/ultimo-ndps.txt"
    printf '3\n' > "$d/ultimo-ndps.txt"; chmod 600 "$d/ultimo-ndps.txt"
    escrever_stubs "$dir"
    env NL_ENSAIO_SEQ=aceita NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso < /dev/null > "$dir/saida-1.txt" 2>&1
    r1=$?
    [ "$r1" -eq 0 ] || problema="$problema rc1=$r1(esperado 0)"
    grep -q '"nDps": 4,' "$dir/dps-emitida.json" 2>/dev/null || problema="$problema ndps_nao_e_4_vale_o_legado"
    [ "$(cat "$d/ultimo-ndps.txt" 2>/dev/null)" = 4 ] || problema="$problema duravel_nao_ficou_4"
    [ "$(cat "$dir/base/ultimo-ndps.txt" 2>/dev/null)" = 90 ] || problema="$problema pasta_antiga_mudou"
    ensaio_duravel_fecha vale_o_duravel_com_os_dois "$problema"
}

ensaio() {
    # Coordenação (G1c, item 3): o ensaio exige python3 (o parser da consulta à BrasilAPI). Sem ele, para no começo, sem pular caso.
    if ! command -v python3 >/dev/null 2>&1; then
        echo "PARADA: o ensaio exige python3 (consulta à BrasilAPI)"
        exit 1
    fi
    # INFO-S6: o ensaio escreve só dentro da base do programa, numa pasta datada que fica no lugar (nada é apagado).
    RAIZ="$BASE/ensaio-$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$RAIZ" || { echo "PARADA: não consegui criar a pasta do ensaio"; exit 1; }
    CASOS=0
    FALHAS=0
    ensaio_caso aceita 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1
    ensaio_dois_envios
    ensaio_caso rota_404_preserva_certificado 0 "Nota de teste aceita" 2 2 "[certificado]" NL_ENSAIO_SEQ=404,aceita NL_ENSAIO_NDPS_ESPERADO=2
    ensaio_caso rota_404_depois_aceita 0 "Nota de teste aceita" 2 2 "prefixo = /API/SefinNacional" NL_ENSAIO_SEQ=404,aceita NL_ENSAIO_NDPS_ESPERADO=2
    ensaio_caso algoritmo_e0714_depois_aceita 0 "Nota de teste aceita" 2 2 "algoritmo = rsa-sha256" NL_ENSAIO_SEQ=E0714,aceita NL_ENSAIO_NDPS_ESPERADO=2
    ensaio_caso e0714_duas_vezes_para 1 "A Sefin recusou" 2 2 "" NL_ENSAIO_SEQ=E0714,E0714 NL_ENSAIO_NDPS_ESPERADO=2
    ensaio_caso rota_404_duas_vezes_para 1 "A Sefin recusou" 2 2 "" NL_ENSAIO_SEQ=404,404 NL_ENSAIO_NDPS_ESPERADO=2
    ensaio_caso outra_recusa_para_sem_tentar 1 "A Sefin recusou" 1 1 "prefixo = /SefinNacional" NL_ENSAIO_SEQ=outro NL_ENSAIO_NDPS_ESPERADO=1
    # NL-2.27: parar antes da Sefin não consome número. Antes de qualquer envio, o contador fica ausente (nada foi gravado).
    ensaio_caso antes_da_sefin_para 1 "antes de chegar à Sefin" 1 1 "" NL_ENSAIO_SEQ=local NL_ENSAIO_NDPS_ESPERADO=ausente
    # NL-2.27 (B1, revisão de segurança): processo morto depois do POST não é "antes da Sefin". Caso permanente: o número é gravado
    # e a mensagem manda confirmar na Sefin antes de reemitir. Morte sem POST na saída também é incerta (o corte pode ser depois do envio).
    ensaio_caso morto_depois_do_post_gera_incerta 1 "Não foi possível confirmar" 1 1 "" NL_ENSAIO_SEQ=morto139 NL_ENSAIO_NDPS_ESPERADO=1
    # NL-2.10e (E7): "sefin.falha" sem sefin.requisicao não pode sair como "A Sefin recusou"; a mensagem é de confirmação.
    # NL-2.10e (E7, E4): transporte com zero bytes é local; com bytes escritos, o POST pode ter chegado: incerta.
    # NL-2.10e (R-B1, sonda SP): POST com bytes e GET com zero bytes: incerta, nunca local (a regra local exige a etapa POST com zero bytes).
    ensaio_caso post_bytes5_get0_gera_incerta 1 "Não foi possível confirmar" 1 1 "" NL_ENSAIO_SEQ=post5_get0 NL_ENSAIO_NDPS_ESPERADO=1
    ensaio_caso transporte_sem_bytes_e_local 1 "antes de chegar à Sefin" 1 1 "" NL_ENSAIO_SEQ=transporte_bytes0 NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso transporte_com_bytes_e_incerta 1 "Não foi possível confirmar" 1 1 "" NL_ENSAIO_SEQ=transporte_bytes5 NL_ENSAIO_NDPS_ESPERADO=1
    ensaio_caso sefin_falha_sem_requisicao_gera_incerta 1 "Não foi possível confirmar" 1 1 "" NL_ENSAIO_SEQ=sefin_falha_sem_requisicao NL_ENSAIO_NDPS_ESPERADO=1
    ensaio_caso morto_sem_post_gera_incerta 1 "Não foi possível confirmar" 1 1 "" NL_ENSAIO_SEQ=morto137 NL_ENSAIO_NDPS_ESPERADO=1
    # NL-2.27 (R2-B2, SB1b): POST 200 registrado e saída 1, sem texto sefin. Só a regra POST torna isso incerta; sem ela, cairia em local.
    ensaio_caso post_sem_texto_sefin_gera_incerta 1 "Não foi possível confirmar" 1 1 "" NL_ENSAIO_SEQ=post_sem_texto NL_ENSAIO_NDPS_ESPERADO=1
    # NL-2.27 (Emenda 1, item 5): kdialog cancelado = senha vazia na CLI (saída 2, local): o número não é consumido.
    ensaio_caso senha_cancelada_nao_consome_numero 1 "antes de chegar à Sefin" 1 1 "" NL_ENSAIO_KDIALOG_CANCELA=1 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso bundle_errado_nao_instala 1 "SHA-256" 0 0 "" NL_ENSAIO_BUNDLE=errado NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso cnpj_invalido_para_antes_de_emitir 1 "CNPJ" 0 0 "" NL_ENSAIO_CNPJ=11222333000180 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso pfx_cancelado_para_antes_de_emitir 1 "não pôde ser lido" 0 0 "" NL_ENSAIO_PFX_VAZIO=1 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso ca_faltando_para_antes_de_emitir 1 "arquivo de CAs" 0 0 "" NL_ENSAIO_CA=faltando NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso pasta_config_errada_para_antes_de_emitir 1 "pasta de configuração" 0 0 "" NL_ENSAIO_XDG=errado NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    # NL-2.27 (Emenda 1, item 3): sem kdialog o script para antes de qualquer pergunta e antes de consumir número de DPS.
    ensaio_caso sem_kdialog_para_antes_do_numero 1 "kdialog" 0 0 "" NL_ENSAIO_SEM_KDIALOG=1 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    # NL-2.27 (DA-2.16): o Flatpak que não repassa o fd 3 faz o script parar antes do kdialog, com a versão do flatpak gravada.
    ensaio_caso fd_nao_repassado_para_antes_do_numero 1 "não repassa descritores" 0 0 "" NL_ENSAIO_FD_NAO_REPASSA=1 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    # MÉDIA-S1: 404 de consulta (GET) seguido de E0714 não troca a base; só o algoritmo muda.
    ensaio_caso consulta_404_depois_e0714_nao_troca_base 0 "Nota de teste aceita" 2 2 "prefixo = /SefinNacional|algoritmo = rsa-sha256" NL_ENSAIO_SEQ=consulta404E0714,aceita NL_ENSAIO_NDPS_ESPERADO=2
    # BAIXA-S4: caminho com espaço, aspas e cifrão, e o .pfx entra sem vazar; caminho relativo para antes de emitir.
    ensaio_caso pfx_com_espaco_aspas_cifrao 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 "NL_ENSAIO_PFX_NOME=pfxdir-ensaio/pfxsecreto com 'aspas' e \$HOME.p12"
    ensaio_caso pfx_relativo_recusado_antes_de_emitir 1 "absoluto" 0 0 "" NL_ENSAIO_PFX_RELATIVO=1 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso cnpj_com_letra_para_antes_de_emitir 1 "CNPJ" 0 0 "" NL_ENSAIO_CNPJ=1122233300018A NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    # NL-2.10g (G1, casos a a l): inscrição municipal (IM), CPF ou CNPJ por opção, e a janela para o que faltar. As fixtures são
    # fictícias: CNPJ 11222333000181, CPF 12345678909 (DV válido, o de 123456789), IM IMENS-7731, IE IEENS-5520.
    local AVISO_CPF='fica gravado no histórico do terminal'
    # a. sem opções: a janela pede a inscrição e a IM; a DPS tem "im" no prestador; o número só é consumido depois.
    ensaio_caso a_sem_opcao_pede_im_e_numero_depois 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=inscricao,im NL_ENSAIO_DPS_ESPERADA='"cnpj": "11222333000181"|"im": "IMENS-7731"'
    # b. --im por opção: a IM não é perguntada; "im" na DPS.
    ensaio_caso b_im_por_opcao_nao_pergunta 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=inscricao NL_ENSAIO_DPS_ESPERADA='"im": "IMENS-7731"'
    # c. IM vazia ou cancelada: para antes de consumir número (contador semeado em 7 e intacto), nada enviado.
    grupo_abrir
    ensaio_caso c_im_vazia_ou_cancelada.vazia 1 "inscrição municipal" 0 0 "" NL_ENSAIO_IM_JANELA= NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=inscricao,im
    ensaio_caso c_im_vazia_ou_cancelada.cancelada 1 "inscrição municipal" 0 0 "" NL_ENSAIO_IM_CANCELA=1 NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=inscricao,im
    grupo_fechar c_im_vazia_ou_cancelada
    # d. IM com 16 caracteres, com aspas, com barra invertida ou com espaço no meio: para, sem repetir o valor.
    grupo_abrir
    ensaio_caso d_im_fora_do_conjunto.dezesseis 1 "inscrição municipal" 0 0 "" NL_ENSAIO_IM_JANELA=ABCDEFGHIJKLMNOP NL_ENSAIO_SEGREDO=ABCDEFGHIJKLMNOP NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=inscricao,im
    ensaio_caso d_im_fora_do_conjunto.aspas 1 "inscrição municipal" 0 0 "" NL_ENSAIO_OPCOES='--im|IM"1' NL_ENSAIO_SEGREDO='IM"1' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso d_im_fora_do_conjunto.barra 1 "inscrição municipal" 0 0 "" NL_ENSAIO_IM_JANELA='IM\1' NL_ENSAIO_SEGREDO='IM\1' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=inscricao,im
    ensaio_caso d_im_fora_do_conjunto.espaco 1 "inscrição municipal" 0 0 "" NL_ENSAIO_OPCOES='--im=IM 1' NL_ENSAIO_SEGREDO='IM 1' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    grupo_fechar d_im_fora_do_conjunto
    # e. --cpf com DV certo: DPS com "cpf" e sem "cnpj"; aviso impresso (texto do CISO, DA-2.87).
    ensaio_caso e_cpf_dv_certo_dps_cpf_e_aviso 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cpf|12345678909' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=im NL_ENSAIO_DPS_ESPERADA='"cpf": "12345678909"|"im": "IMENS-7731"' NL_ENSAIO_DPS_AUSENTE='"cnpj"' NL_ENSAIO_SAIDA_ESPERADA="$AVISO_CPF"
    # DA-2.89 (CISO, rodada de revisão B1): a DPS temporária (com CPF, CNPJ e IM) some ao fim, depois de sucesso, falha e
    # interrupção; o portal não lista mais o documento e o unexport aparece antes. Vermelho no script anterior.
    grupo_abrir
    ensaio_caso b1_dps_some_depois_sucesso.sucesso 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_ARQUIVO_AUSENTE=base/teste-lider/dps-teste.json NL_ENSAIO_PORTAL_AUSENTE=dps-teste.json NL_ENSAIO_CHAMADA_ESPERADA=document-unexport
    ensaio_caso b1_dps_some_depois_falha.falha 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_SEQ=outro NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_ARQUIVO_AUSENTE=base/teste-lider/dps-teste.json NL_ENSAIO_PORTAL_AUSENTE=dps-teste.json NL_ENSAIO_CHAMADA_ESPERADA=document-unexport
    ensaio_caso b1_dps_some_depois_interrompida.interrompida 130 "" 1 1 "" NL_ENSAIO_SEQ=interrompe NL_ENSAIO_ARQUIVO_AUSENTE=base/teste-lider/dps-teste.json NL_ENSAIO_PORTAL_AUSENTE=dps-teste.json NL_ENSAIO_CHAMADA_ESPERADA=document-unexport
    grupo_fechar b1_dps_temporaria_some
    # Rodada de revisão B2: o texto completo da DA-2.87 sai com --cnpj e com --cpf (o aviso vale para os dois ramos).
    local AVISO_TEXTO='Aviso: o número de CPF ou CNPJ digitado na linha de comando fica gravado no histórico do terminal e, enquanto o script roda, aparece para outros usuários desta máquina na lista de processos. Para não deixar esse rastro, rode sem --cpf e sem --cnpj: a janela pede o número. Começar o comando com um espaço só tira a linha do histórico se o seu terminal estiver configurado para isso.'
    ensaio_caso b2_aviso_completo_com_cnpj 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA="$AVISO_TEXTO"
    ensaio_caso b2_aviso_completo_com_cpf 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cpf|12345678909|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA="$AVISO_TEXTO"
    # f. --cpf com DV errado e com 11 dígitos iguais: para antes de consumir número.
    grupo_abrir
    ensaio_caso f_cpf_dv_errado.dv 1 "CPF" 0 0 "" NL_ENSAIO_OPCOES='--cpf|12345678900' NL_ENSAIO_SEGREDO=12345678900 NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso f_cpf_11_digitos_iguais.iguais 1 "CPF" 0 0 "" NL_ENSAIO_OPCOES='--cpf|11111111111' NL_ENSAIO_SEGREDO=11111111111 NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    grupo_fechar f_cpf_invalido
    # g. --cnpj e --cpf juntos: para antes de consumir número.
    ensaio_caso g_cnpj_e_cpf_juntos_para_sem_numero 1 "use só um" 0 0 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--cpf|12345678909' NL_ENSAIO_SEGREDO=12345678909 NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    # h. janela: 11 dígitos vira CPF, 14 vira CNPJ, outro tamanho para antes do número.
    grupo_abrir
    ensaio_caso h_janela_11_digitos_vira_cpf.onze 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_INSCRICAO=12345678909 NL_ENSAIO_SEQ=aceita NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=8 NL_ENSAIO_PERGUNTAS=inscricao,im NL_ENSAIO_DPS_ESPERADA='"cpf": "12345678909"|"im": "IMENS-7731"' NL_ENSAIO_DPS_AUSENTE='"cnpj"'
    ensaio_caso h_janela_14_digitos_vira_cnpj.catorze 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=8 NL_ENSAIO_PERGUNTAS=inscricao,im NL_ENSAIO_DPS_ESPERADA='"cnpj": "11222333000181"' NL_ENSAIO_DPS_AUSENTE='"cpf"'
    ensaio_caso h_janela_outro_tamanho_para.treze 1 "ou 11 (CPF)" 0 0 "" NL_ENSAIO_INSCRICAO=1234567890123 NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=inscricao
    grupo_fechar h_janela_por_tamanho
    # i. --ie: aceita, não aparece na DPS nem em saída nenhuma, aviso impresso (texto da ordem, D4).
    ensaio_caso i_ie_aceita_sem_dps_com_aviso 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--ie|IEENS-5520|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=inscricao NL_ENSAIO_DPS_AUSENTE='"ie"|IEENS-5520' NL_ENSAIO_SAIDA_ESPERADA='A inscrição estadual não vai na NFS-e'
    # Rodada de revisão L1: a --ie recusa com mais de 20 caracteres e com caractere fora do conjunto; para antes do número e sem repetir.
    grupo_abrir
    ensaio_caso l1_ie_mais_de_20.vinte_e_um 1 "inscrição estadual precisa ter de 1 a 20" 0 0 "" NL_ENSAIO_OPCOES='--ie|IEENSAIO-0123456789AB' NL_ENSAIO_SEGREDO='IEENSAIO-0123456789AB' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso l1_ie_fora_do_conjunto.arroba 1 "inscrição estadual precisa ter de 1 a 20" 0 0 "" NL_ENSAIO_OPCOES='--ie|IE@ENSAIO-1' NL_ENSAIO_SEGREDO='IE@ENSAIO-1' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    grupo_fechar l1_ie_recusada
    # j. opção desconhecida: para sem repetir o valor.
    ensaio_caso j_opcao_desconhecida_para_sem_repetir 1 "opção desconhecida" 0 0 "" NL_ENSAIO_OPCOES='--cpfx|12345678909' NL_ENSAIO_SEGREDO=12345678909 NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    # k. varredura: CPF, IM e IE de fixture só aparecem na DPS (a saída, a devolução, a execução e as chamadas ficam limpas).
    ensaio_caso k_varredura_fixtures_so_na_dps 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cpf|12345678909|--im|IMENS-7731|--ie|IEENS-5520' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_DPS_ESPERADA='"cpf": "12345678909"|"im": "IMENS-7731"'
    # l. --opcao=valor e --opcao valor dão a mesma DPS (sem contar dhEmi, que é a hora da execução).
    grupo_abrir
    ensaio_caso l_forma_separada_ou_igual.separada 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_DPS_ESPERADA='"cnpj": "11222333000181"|"im": "IMENS-7731"'
    ensaio_caso l_forma_separada_ou_igual.igual 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj=11222333000181|--im=IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_DPS_ESPERADA='"cnpj": "11222333000181"|"im": "IMENS-7731"'
    # Rodada de revisão B3: comparação em bash, sem cmp (fedora45 não traz cmp nem diff: medido em 05/11/2026 na imagem notalivre/build-fedora45:nl-2.0; manjaro: afirmação anterior, não remedida nesta fatia). Arquivo ausente dá vazio e falha.
    local dps_a dps_b
    dps_a=$(grep -v dhEmi "$RAIZ/l_forma_separada_ou_igual.separada/dps-emitida.json" 2>/dev/null)
    dps_b=$(grep -v dhEmi "$RAIZ/l_forma_separada_ou_igual.igual/dps-emitida.json" 2>/dev/null)
    if [ -z "$dps_a" ] || [ "$dps_a" != "$dps_b" ]; then
        printf 'FALHA l_forma_separada_ou_igual: as duas DPS diferem\n'
        FALHAS=$(( FALHAS + 1 ))
    fi
    grupo_fechar l_forma_separada_ou_igual
    # NL-2.10g (G1c, rodada): BrasilAPI, arquivo de lembrar, --esquecer, --help e -h, --sem-im, validadores sem janela.
    # Vermelhos no script anterior; as fixtures são fictícias (RAZAO-FICTICIA e SOCIO-FICTICIO vêm só do stub da BrasilAPI).
    ensaio_caso r1_consulta_mei 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=mei NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "2"|"regEspTrib": "0"' NL_ENSAIO_DPS_AUSENTE='"regApTribSN"'
    ensaio_caso r2_consulta_simples 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=simples NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "3"|"regApTribSN": "1"' NL_ENSAIO_CURL_ESPERADO='--proto =https|https://brasilapi.com.br/api/cnpj/v1/'
    ensaio_caso r3_consulta_nula 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=nulo NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "1"' NL_ENSAIO_DPS_AUSENTE='"regApTribSN"'
    ensaio_caso r4_consulta_exclusao_simples 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=exclusao NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "1"'
    ensaio_caso r5_consulta_rede_falha 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=rede NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "3"' NL_ENSAIO_SAIDA_ESPERADA='consulta pública não respondeu'
    grupo_abrir
    ensaio_caso r6_consulta_invalida.json 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=json NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "3"' NL_ENSAIO_SAIDA_ESPERADA='não serve'
    ensaio_caso r6_consulta_invalida.campo_ausente 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=campo NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "3"' NL_ENSAIO_SAIDA_ESPERADA='não serve'
    grupo_fechar r6_consulta_invalida
    ensaio_caso r7_situacao_8_avisa_e_segue 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=situacao8 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "3"' NL_ENSAIO_SAIDA_ESPERADA='não é ativa'
    ensaio_caso r8_municipio_da_consulta 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=municipio NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cLocEmi": "3550308"|"cLocPrestacao": "3550308"'
    ensaio_caso r9_cpf_sem_consulta 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cpf|12345678909' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "1"' NL_ENSAIO_CHAMADA_AUSENTE=curl NL_ENSAIO_LEMBRAR_AUSENTE='cpf=|cpf=12345678909|cnpj='
    grupo_abrir
    ensaio_caso r10_opcao_vence_lembrado.opcao 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--opsimpnac|1' NL_ENSAIO_LEMBRAR='opsimpnac=2|origem_opsimpnac=consulta|cnpj=11222333000181|im=IMENS-7731|sem_im=0' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "1"'
    ensaio_caso r10_lembrado_vence_consulta.lembrado 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=simples NL_ENSAIO_LEMBRAR='opsimpnac=2|origem_opsimpnac=consulta|cnpj=11222333000181|im=IMENS-7731|sem_im=0' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "2"' NL_ENSAIO_CHAMADA_AUSENTE=curl
    grupo_fechar r10_precedencia
    grupo_abrir
    ensaio_caso r11_so_im_abre.com_cnpj 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=im
    ensaio_caso r11_so_im_abre.com_im 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso r11_so_im_abre.sem_im 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--sem-im' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso r11_so_im_abre.lembrado 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR='cnpj=11222333000181|im=IMENS-7731|opsimpnac=3|origem_opsimpnac=consulta' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma
    grupo_fechar r11_so_im_abre
    ensaio_caso r12_im_vazia_para_com_dica 1 "use --sem-im" 0 0 "" NL_ENSAIO_IM_JANELA= NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=inscricao,im
    grupo_abrir
    ensaio_caso r13_lembrar_grava_0600.gravado 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_LEMBRAR_ESPERADO='cnpj=11222333000181|im=IMENS-7731|opsimpnac=3|origem_opsimpnac=consulta' NL_ENSAIO_LEMBRAR_AUSENTE='cpf='
    ensaio_caso r13_lembrar_644_ignorado.modo 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR='opsimpnac=2|origem_opsimpnac=lembrado' NL_ENSAIO_LEMBRAR_MODO=644 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "3"' NL_ENSAIO_SAIDA_ESPERADA='permissão aberta'
    ensaio_caso r13_lembrar_linha_fora_ignorado.linha 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR='opsimpnac=3|cpf=12345678909' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "3"' NL_ENSAIO_SAIDA_ESPERADA='linha fora do conjunto' NL_ENSAIO_LEMBRAR_AUSENTE='cpf='
    grupo_fechar r13_lembrar
    ensaio_caso r14_recusa_e0160_tira_opsimpnac 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_BRASILAPI=simples NL_ENSAIO_SEQ=E0160 NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA='use --opsimpnac' NL_ENSAIO_LEMBRAR_AUSENTE='opsimpnac=3'
    ensaio_caso r14_recusa_e0166_tira_regaptribsn 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_OPCOES='--regaptribsn|2' NL_ENSAIO_SEQ=E0166 NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA='use --regaptribsn' NL_ENSAIO_LEMBRAR_AUSENTE='regaptribsn=2'
    ensaio_caso r14_recusa_e0116_tira_im 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_SEQ=E0116 NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA='--sem-im' NL_ENSAIO_LEMBRAR_AUSENTE='im=IMENS-7731'
    ensaio_caso r15_esquecer_apaga 0 "Lembrança apagada" 0 0 "" NL_ENSAIO_OPCOES='--esquecer' NL_ENSAIO_LEMBRAR='cnpj=11222333000181|im=IMENS-7731|opsimpnac=3|origem_opsimpnac=consulta|pfx=/caminho/guardado/pfxsecreto-ensaio.p12' NL_ENSAIO_ARQUIVO_AUSENTE=home/.local/state/notalivre-teste/lembrar NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_CHAMADA_AUSENTE=flatpak
    ensaio_caso r16_ajuda_help 0 "--opsimpnac" 0 0 "" NL_ENSAIO_OPCOES='--help' NL_ENSAIO_SAIDA_ESPERADA='fica gravado no histórico do terminal' NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_CHAMADA_AUSENTE=flatpak NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7
    ensaio_caso r16_ajuda_h 0 "--opsimpnac" 0 0 "" NL_ENSAIO_OPCOES='-h' NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_CHAMADA_AUSENTE=flatpak NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7
    ensaio_caso r17_varredura_descricao_e_json_so_na_dps 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=simples NL_ENSAIO_OPCOES='--cnpj|11222333000181|--im|IMENS-7731|--descricao|DESCSEG-ENSAIO teste' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"xDescServ": "DESCSEG-ENSAIO teste"'
    ensaio_caso v_municipio_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--municipio|3550308' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cLocEmi": "3550308"|"cLocPrestacao": "3550308"'
    ensaio_caso v_municipio_invalido 1 "começar de 1 a 5" 0 0 "" NL_ENSAIO_OPCOES='--municipio|0611606' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso v_ctribnac_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--ctribnac|040102' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cTribNac": "040102"'
    ensaio_caso v_ctribnac_invalido 1 "9903" 0 0 "" NL_ENSAIO_OPCOES='--ctribnac|990401' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    # NL-2.10h (H1): --ctribmun NNN (3 dígitos, nunca 000, proibido com MEI). A recusa acontece antes de consumir número.
    ensaio_caso h1_ctribmun_opcao_vai_na_dps 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--ctribmun|501' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cTribMun": "501"'
    ensaio_caso h1_ctribmun_zero_para_antes_do_numero 1 "não pode ser 000" 0 0 "" NL_ENSAIO_OPCOES='--ctribmun|000' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso h1_ctribmun_mei_para_antes_do_numero 1 "MEI" 0 0 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--opsimpnac|2|--ctribmun|501' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso h1_ctribmun_lembrado_vence_ausencia 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR='cnpj=11222333000181|im=IMENS-7731|opsimpnac=3|origem_opsimpnac=consulta|ctribmun=501|origem_ctribmun=lembrado' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cTribMun": "501"'
    ensaio_caso h1_ctribmun_opcao_vence_lembrado 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--ctribmun|502' NL_ENSAIO_LEMBRAR='cnpj=11222333000181|im=IMENS-7731|opsimpnac=3|origem_opsimpnac=consulta|ctribmun=501|origem_ctribmun=lembrado' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cTribMun": "502"'
    ensaio_caso h1_ctribmun_ausente_nao_gera_chave 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_AUSENTE='"cTribMun"'
    ensaio_caso h1_e0312_dica_cita_ctribmun 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_SEQ=E0312 NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA='use --ctribmun'
    ensaio_caso h1_e0314_dica_cita_ctribmun 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_SEQ=E0314 NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA='use --ctribmun'
    # NL-2.10h (H1, QA A2, decisão do main): com MEI, a dica do E0313 manda NÃO informar --ctribmun.
    ensaio_caso h1_e0313_mei_dica_nao_informe 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--opsimpnac|2' NL_ENSAIO_SEQ=E0313 NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA='com MEI, não informe --ctribmun'
    # NL-2.10h (H1, QA A3/M5): o cTribMun gravado no lembrar volta na execução seguinte, sem opção.
    ensaio_ctribmun_gravado h1_ctribmun_gravado_lido_na_seguinte
    # NL-2.10j, J2 (DA-2.99): totTrib por regime. ME/EPP manda pTotTribSN com --ptottribsn; sem ele, pTotTrib 0,00 e aviso; MEI manda
    # indTotTrib 0; não optante manda pTotTrib; --ptottribsn com 1 ou 2 e valor fora do formato recusado antes do número.
    ensaio_caso h2_me_sem_opcao_ptottrib_zero_e_aviso 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--opsimpnac|3' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"pTotTribFed": "0.00"|"pTotTribMun": "0.00"' NL_ENSAIO_DPS_AUSENTE='"indTotTrib"|"pTotTribSN"' NL_ENSAIO_SAIDA_ESPERADA='não foi informado'
    ensaio_caso h2_me_com_ptottribsn_manda_pTotTribSN 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--opsimpnac|3|--ptottribsn|7,35' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"pTotTribSN": "7.35"' NL_ENSAIO_DPS_AUSENTE='"pTotTribFed"|"indTotTrib"'
    ensaio_ptottribsn_gravado h2_ptottribsn_gravado_lido_na_seguinte 7,35
    ensaio_ptottribsn_sem_vazamento h2_percentual_nao_vaza_fora_da_dps
    # NL-2.10j, J2-r2 (revisão de segurança, achado F): um --ptottribsn adulterado no arquivo de lembrar faz o arquivo inteiro ser
    # descartado, com aviso; nada vai à DPS. O valor 7.3 tem uma casa a menos e não passa no formato do lembrar.
    ensaio_caso j2_lembrar_ptottribsn_adulterado_ignorado 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--opsimpnac|3' NL_ENSAIO_LEMBRAR='cnpj=11222333000181|im=IMENS-7731|opsimpnac=3|origem_opsimpnac=lembrado|ptottribsn=7.3|origem_ptottribsn=lembrado' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"pTotTribFed": "0.00"' NL_ENSAIO_DPS_AUSENTE='"pTotTribSN"' NL_ENSAIO_SAIDA_ESPERADA='linha fora do conjunto'
    ensaio_caso h2_mei_manda_indtottrib_zero 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--opsimpnac|2' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"indTotTrib": "0"' NL_ENSAIO_DPS_AUSENTE='"pTotTribFed"|"pTotTribSN"'
    ensaio_caso h2_nao_optante_manda_pTotTrib 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--opsimpnac|1' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"pTotTribFed": "0.00"' NL_ENSAIO_DPS_AUSENTE='"indTotTrib"|"pTotTribSN"'
    ensaio_caso h2_nao_optante_com_ptottribsn_recusado 1 "--ptottribsn" 0 0 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--opsimpnac|1|--ptottribsn|7,35' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso h2_mei_com_ptottribsn_recusado 1 "--ptottribsn" 0 0 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--opsimpnac|2|--ptottribsn|7,35' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso h2_ptottribsn_acima_da_faixa_recusado 1 "0,00 e 99,99" 0 0 "" NL_ENSAIO_OPCOES='--opsimpnac|3|--ptottribsn|100' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso h2_ptottribsn_tres_casas_recusado 1 "0,00 e 99,99" 0 0 "" NL_ENSAIO_OPCOES='--opsimpnac|3|--ptottribsn|7,345' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso h2_e0712_simulado_imprime_dica 1 "A Sefin recusou" 1 1 "" NL_ENSAIO_OPCOES='--opsimpnac|3' NL_ENSAIO_SEQ=E0712 NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_SAIDA_ESPERADA='--ptottribsn'
    ensaio_ctribmun_janela h1_janela_depois_de_e0312 E0312 aceita "" sim 0 2
    ensaio_ctribmun_janela h1_janela_sem_recusa_nao_abre aceita aceita "" nao 0 2
    ensaio_ctribmun_janela h1_janela_cancelada_para_antes_do_numero E0312 aceita NL_ENSAIO_CTRIBMUN_CANCELA=1 sim 1 1
    # NL-2.10h (H4): CNPJ e CPF com zero à esquerda. O zero é dígito: não some no envio, no lembrar, na consulta nem na janela.
    # Fixtures fictícias com DV pelo módulo 11 (calculados no contêiner). A consulta (BrasilAPI) só responde para a URL com os zeros.
    ensaio_caso h4_cnpj_zero_inicial_consulta_url 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=mei NL_ENSAIO_OPCOES='--cnpj|01234567890107|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cnpj": "01234567890107"|"opSimpNac": "2"' NL_ENSAIO_DPS_AUSENTE='"cpf"' NL_ENSAIO_URL_ESPERADA=/api/cnpj/v1/01234567890107 NL_ENSAIO_CURL_ESPERADO='--proto =https|https://brasilapi.com.br/api/cnpj/v1/01234567890107'
    ensaio_caso h4_cnpj_dois_zeros_consulta_url 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=mei NL_ENSAIO_OPCOES='--cnpj|00012345678978|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cnpj": "00012345678978"|"opSimpNac": "2"' NL_ENSAIO_DPS_AUSENTE='"cpf"' NL_ENSAIO_URL_ESPERADA=/api/cnpj/v1/00012345678978 NL_ENSAIO_CURL_ESPERADO='--proto =https|https://brasilapi.com.br/api/cnpj/v1/00012345678978'
    ensaio_caso h4_cpf_zero_inicial_aceito 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cpf|01234567890|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cpf": "01234567890"' NL_ENSAIO_DPS_AUSENTE='"cnpj"'
    ensaio_caso h4_cnpj_zero_dv_errado_para_antes 1 "CNPJ" 0 0 "" NL_ENSAIO_INSCRICAO=01234567890108 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso h4_cpf_zero_dv_errado_para_antes 1 "CPF" 0 0 "" NL_ENSAIO_INSCRICAO=01234567891 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=ausente
    ensaio_caso h4_janela_cnpj_zero_inicial.catorze 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_INSCRICAO=01234567890107 NL_ENSAIO_SEQ=aceita NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=8 NL_ENSAIO_PERGUNTAS=inscricao,im NL_ENSAIO_DPS_ESPERADA='"cnpj": "01234567890107"' NL_ENSAIO_DPS_AUSENTE='"cpf"'
    ensaio_caso h4_janela_cpf_zero_inicial.onze 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_INSCRICAO=01234567890 NL_ENSAIO_SEQ=aceita NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=8 NL_ENSAIO_PERGUNTAS=inscricao,im NL_ENSAIO_DPS_ESPERADA='"cpf": "01234567890"' NL_ENSAIO_DPS_AUSENTE='"cnpj"'
    ensaio_caso h4_lembrar_grava_cnpj_com_zero_inicial.gravado 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|01234567890107|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_LEMBRAR_ESPERADO='cnpj=01234567890107|im=IMENS-7731' NL_ENSAIO_LEMBRAR_AUSENTE='cnpj=1234567890107'
    ensaio_caso h4_lembrar_le_cnpj_com_zero_inicial 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR='cnpj=01234567890107|im=IMENS-7731|opsimpnac=3|origem_opsimpnac=consulta' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cnpj": "01234567890107"' NL_ENSAIO_DPS_AUSENTE='"cpf"'
    ensaio_caso v_opsimpnac_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--opsimpnac|1' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "1"' NL_ENSAIO_DPS_AUSENTE='"regApTribSN"'
    ensaio_caso v_opsimpnac_invalido 1 "1 (não optante), 2 (MEI) ou 3" 0 0 "" NL_ENSAIO_OPCOES='--opsimpnac|4' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso v_regaptribsn_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--regaptribsn|2' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"regApTribSN": "2"'
    ensaio_caso v_regaptribsn_invalido 1 "só vale com a opção 3" 0 0 "" NL_ENSAIO_OPCOES='--opsimpnac|1|--regaptribsn|2' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7
    ensaio_caso v_regesptrib_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--regaptribsn|2|--regesptrib|1' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"regEspTrib": "1"'
    ensaio_caso v_regesptrib_invalido 1 "precisa ser 0" 0 0 "" NL_ENSAIO_OPCOES='--regaptribsn|1|--regesptrib|1' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7
    ensaio_caso v_valor_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--valor|25.50' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"vServ": "25,50"'
    ensaio_caso v_valor_invalido 1 "maior que zero" 0 0 "" NL_ENSAIO_OPCOES='--valor|0,00' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso v_descricao_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--descricao|Consulta médica (retorno): 50% de desconto' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"xDescServ": "Consulta médica (retorno): 50% de desconto"'
    ensaio_caso v_descricao_invalido 1 "de 1 a 2000 caracteres" 0 0 "" NL_ENSAIO_OPCOES='--descricao|Teste "aspas"' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    # Rodada 2 (revisor, mutante 12): o parser da consulta lê com python3 -I; um json.py plantado no diretório de trabalho
    # não pode mudar o regime nem o município (o mei tem de sair opSimpNac 2 e cLocEmi 2611606).
    ensaio_caso r19_json_plantado_nao_muda_a_consulta 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=mei NL_ENSAIO_PLANTAR_JSON=1 NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"opSimpNac": "2"|"cLocEmi": "2611606"'
    # DA-2.94 (líder, "Guardar o local"): a segunda execução não abre a janela do .pfx; o caminho guardado que não passa na
    # conferência (link simbólico, arquivo sumido, dono de outro usuário) volta à janela e é substituído. Os preparos ficam na pasta do caso.
    ensaio_segunda_sem_pfx
    mkdir -p "$RAIZ/p2_caminho_guardado_link_simbolico"
    ln -s "$RAIZ/p2_caminho_guardado_link_simbolico/pfxdir-ensaio/pfxsecreto-ensaio.p12" "$RAIZ/p2_caminho_guardado_link_simbolico/pfxsecreto-link.p12"
    ensaio_caso p2_caminho_guardado_link_simbolico 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR="pfx=$RAIZ/p2_caminho_guardado_link_simbolico/pfxsecreto-link.p12|cnpj=11222333000181|im=IMENS-7731" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PFX_JANELA=sim NL_ENSAIO_LEMBRAR_ESPERADO="pfx=$RAIZ/p2_caminho_guardado_link_simbolico/pfxdir-ensaio/pfxsecreto-ensaio.p12" NL_ENSAIO_LEMBRAR_AUSENTE='pfxsecreto-link.p12'
    mkdir -p "$RAIZ/p3_caminho_guardado_sumiu"
    ensaio_caso p3_caminho_guardado_sumiu 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR="pfx=$RAIZ/p3_caminho_guardado_sumiu/pfxsecreto-sumiu.p12|cnpj=11222333000181|im=IMENS-7731" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PFX_JANELA=sim NL_ENSAIO_LEMBRAR_ESPERADO="pfx=$RAIZ/p3_caminho_guardado_sumiu/pfxdir-ensaio/pfxsecreto-ensaio.p12"
    # Rodada 3 (revisor, R3-B3): o dono diferente é /etc/hosts (root, legível); funciona como uid 1000, sem chown. O caminho guardado não pode sair na saída.
    ensaio_caso p4_caminho_guardado_outro_dono 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_LEMBRAR="pfx=/etc/hosts|cnpj=11222333000181|im=IMENS-7731" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PFX_JANELA=sim NL_ENSAIO_LEMBRAR_ESPERADO="pfx=$RAIZ/p4_caminho_guardado_outro_dono/pfxdir-ensaio/pfxsecreto-ensaio.p12" NL_ENSAIO_SEGREDO='/etc/hosts'
    # Rodada 3 (revisor, R3-B1): falha de senha e falha da CLI com o .pfx guardado válido. O marcador pfxsecreto não pode sair em saída, devolução ou execução.
    mkdir -p "$RAIZ/r20_falha_de_senha_com_pfx_guardado"
    mkdir -p "$RAIZ/r20_falha_de_senha_com_pfx_guardado/pfxdir-guardado"; printf 'pfxsecreto-guardado' > "$RAIZ/r20_falha_de_senha_com_pfx_guardado/pfxdir-guardado/pfxsecreto-guardado.p12"
    ensaio_caso r20_falha_de_senha_com_pfx_guardado 1 "antes de chegar à Sefin" 1 1 "" NL_ENSAIO_LEMBRAR="pfx=$RAIZ/r20_falha_de_senha_com_pfx_guardado/pfxdir-guardado/pfxsecreto-guardado.p12|cnpj=11222333000181|im=IMENS-7731" NL_ENSAIO_KDIALOG_CANCELA=1 NL_ENSAIO_SEQ=aceita NL_ENSAIO_PFX_JANELA=nao NL_ENSAIO_NDPS_ESPERADO=ausente
    mkdir -p "$RAIZ/r21_falha_da_cli_com_pfx_guardado"
    mkdir -p "$RAIZ/r21_falha_da_cli_com_pfx_guardado/pfxdir-guardado"; printf 'pfxsecreto-guardado' > "$RAIZ/r21_falha_da_cli_com_pfx_guardado/pfxdir-guardado/pfxsecreto-guardado.p12"
    ensaio_caso r21_falha_da_cli_com_pfx_guardado 1 "antes de chegar à Sefin" 1 1 "" NL_ENSAIO_LEMBRAR="pfx=$RAIZ/r21_falha_da_cli_com_pfx_guardado/pfxdir-guardado/pfxsecreto-guardado.p12|cnpj=11222333000181|im=IMENS-7731" NL_ENSAIO_SEQ=local NL_ENSAIO_PFX_JANELA=nao NL_ENSAIO_NDPS_ESPERADO=ausente
    # Rodada 2 (revisor, mutante 3a): depois da consulta, nenhuma marca da resposta crua fica em base nem em home.
    ensaio_caso r18_resposta_crua_fora_do_disco 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_BRASILAPI=simples NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_VARRER_DISCO=1
    # Rodada 2 (revisor, mutante 10b): barra invertida na descrição tem de parar antes do número, sem repetir o valor.
    ensaio_caso r10b_descricao_com_barra_invertida 1 "de 1 a 2000 caracteres" 0 0 "" NL_ENSAIO_OPCOES='--descricao|Teste \ barra' NL_ENSAIO_SEGREDO='Teste \ barra' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso v_sem_im_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--sem-im|--cnpj|11222333000181' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_AUSENTE='"im"' NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso v_sem_im_invalido 1 "não podem vir juntos" 0 0 "" NL_ENSAIO_OPCOES='--sem-im|--im|IMENS-7731' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    # Coordenação (G1c, item 1): --local-prestacao e --trib-issqn entram; --ret-issqn só aceita 1 (retenção travada em 1, C9).
    ensaio_caso s1_local_prestacao_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--local-prestacao|0000000' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"cLocPrestacao": "0000000"|"cLocEmi": "2611606"'
    ensaio_caso s2_local_prestacao_invalido 1 "ou 0000000" 0 0 "" NL_ENSAIO_OPCOES='--local-prestacao|261160' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso s3_trib_issqn_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--trib-issqn|2' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"tribISSQN": "2"|"tpRetISSQN": "1"'
    ensaio_caso s4_trib_issqn_invalido 1 "1, 2, 3 ou 4" 0 0 "" NL_ENSAIO_OPCOES='--trib-issqn|5' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    ensaio_caso s5_ret_issqn_valido 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--ret-issqn|1' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_DPS_ESPERADA='"tpRetISSQN": "1"'
    ensaio_caso s6_ret_issqn_invalido 1 "só aceita 1" 0 0 "" NL_ENSAIO_OPCOES='--ret-issqn|2' NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    # DA-2.92 (CTO): o número é reservado antes da chamada. Ctrl+C no meio do POST (INT, saída 130) gasta o número, e a
    # execução seguinte usa o próximo. Falha local devolve o número. O primeiro caso fica vermelho antes da implementação.
    ensaio_sequencia ndps_ctrlc_gasta_o_numero interrompe aceita 130 1 2
    ensaio_sequencia ndps_falha_local_devolve_o_numero local aceita 1 ausente 1
    # NL-3.0i: o contador de DPS mora em ~/.local/share/notalivre/nl210 (dentro de $dir/home, no ensaio), e não em $BASE, que o
    # sistema limpa em 30 dias sem uso. Os casos conferem o durável; $dir/base é a pasta antiga (/var/tmp).
    ensaio_duravel_sobrevive_a_apagar_base
    ensaio_duravel_copia_uma_vez
    ensaio_duravel_vale_o_duravel
    # CISO (BAIXA-S3, condição 1): o script não liga o rastreio do shell.
    local marca_xtrace marca_bash
    marca_xtrace="set -$(printf x)"
    marca_bash="bash -$(printf x)"
    if grep -q -F -e "$marca_xtrace" -e "$marca_bash" "$0"; then
        printf 'FALHA sem_rastreio_do_shell: o script liga o rastreio do shell\n'
        FALHAS=$(( FALHAS + 1 ))
    else
        printf 'ok sem_rastreio_do_shell\n'
    fi
    CASOS=$(( CASOS + 1 ))
    echo "ensaio: casos=$CASOS falhas=$FALHAS (pasta: $RAIZ)"
    [ "$FALHAS" -eq 0 ]
}

case "$MODO" in
    ensaio) ensaio; exit $? ;;
    caso|normal)
        # NL-2.10g: as opções são lidas e conferidas antes de qualquer passo do fluxo (o modo caso passa o próprio flag antes).
        [ "$MODO" = caso ] && shift
        # G1c: --help e -h vêm antes de tudo (sem pré-voo, sem janela, sem número); --esquecer sai logo depois.
        if procura_flag --help "$@" || procura_flag -h "$@"; then ajuda; exit 0; fi
        if procura_flag --esquecer "$@"; then esquecer_lembrar; exit 0; fi
        ler_opcoes "$@"
        preparar_duravel
        fluxo; exit $? ;;
esac
