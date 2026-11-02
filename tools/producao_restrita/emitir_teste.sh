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
# Regras que o script cumpre: o caminho do .pfx nunca é impresso, copiado, listado nem gravado em log; a inscrição e a IM
# não vão a arquivo versionado nem a saída (NL-2.10g); nenhuma chamada vai à Produção; nada é apagado por este script; só há tentativa 2
# (base) se a Sefin responder 404, e tentativa 3 (algoritmo rsa-sha256) se ela responder E0714.

set -u
# CISO (BAIXA-S3): tudo que o script cria sai com permissão só do dono (0600 para arquivos, 0700 para pastas).
umask 077

APP=io.github.petrinhu.notalivre
RUNTIME=org.kde.Platform//6.11
# Pacote do CI: único lugar do script com o run e o SHA-256 do notalivre.flatpak. Fonte: artefato notalivre-bundle do run
# 41362487894 (commit 22cdb30, branch onda-2), baixado com gh run download (digest do zip bf64796c188e477a490080ce4e5c0fcf30005dd053fb9a2bd83ec84fc246d2a5,
# conferido na API do GitHub). O main troca estas duas linhas quando o pacote do CI muda (NL-2.10f).
BUNDLE_RUN=41362487894
BUNDLE_SHA=2c2372581c392b0088faa6271bb40a9333f658b047ba6398e5acef23a90164c0
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
# MÉDIA-S2: o contador de números de DPS fica fora de teste-lider (que se limpa); nunca se apaga.
NDPS_ARQ="$BASE/ultimo-ndps.txt"
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

# Monta a DPS do roteiro (3.8) com o número dado e libera o arquivo para o programa.
montar_dps() {
    local n="$1" dh dc prest_chave
    # NL-2.10g: o prestador leva CNPJ ou CPF (nunca os dois) e a IM. Os valores são fechados por validação, sem aspas nem barras.
    if [ "$INSC_TIPO" = cpf ]; then
        prest_chave="\"cpf\": \"$INSC_VALOR\","
    else
        prest_chave="\"cnpj\": \"$INSC_VALOR\","
    fi
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
    $prest_chave
    "im": "$IM",
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
    valida_texto_fechado 15 "$v" || falha "a inscrição municipal precisa ter de 1 a 15 caracteres: letras, números, ponto, barra ou traço, e não pode ser vazia. Nada foi emitido."
    IM=$v
}

# IE (D4): só confere o tamanho (1 a 20) e o conjunto fechado; não vai na DPS nem é guardada.
validar_ie() {
    local v
    v=$(aparar "$1")
    valida_texto_fechado 20 "$v" || falha "a inscrição estadual precisa ter de 1 a 20 caracteres: letras, números, ponto, barra ou traço. Nada foi emitido."
    printf '%s\n' 'A inscrição estadual não vai na NFS-e e este script não a guarda; a guarda no cofre vem com a interface (spec 4.5).'
}

ler_opcoes() {
    local nome valor
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --cnpj=*|--cpf=*|--im=*|--ie=*) nome="${1%%=*}"; valor="${1#*=}"; shift ;;
            --cnpj|--cpf|--im|--ie)
                [ "$#" -ge 2 ] || falha "a opção $1 pede um valor. Nada foi emitido."
                nome="$1"; valor="$2"; shift 2 ;;
            *) falha "opção desconhecida (use --cnpj, --cpf, --im ou --ie). Nada foi emitido." ;;
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
    return 0
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

    # NL-2.27 (Emenda 1, item 3): pré-voo do canal de senha, antes de qualquer pergunta e antes de consumir número. Sem kdialog a
    # senha não é pedida; sem o repasse do descritor 3 pelo flatpak instalado, a CLI não receberia a senha. Nos dois casos, para.
    [ "$TEM_KDIALOG" -eq 1 ] || falha "o kdialog não está instalado: sem ele a senha não é pedida. Nada foi emitido."
    flatpak --version > "$RUN_DIR/flatpak-versao.txt" 2>&1 || true
    local repasse
    repasse=$(flatpak run --command=sh "$APP" -c 'cat <&3' 3< <(printf ok) 2>/dev/null) || repasse=""
    [ "$repasse" = ok ] || falha "o Flatpak instalado não repassa descritores ao programa (versão em $RUN_DIR/flatpak-versao.txt). Nada foi emitido."

    # NL-2.10g: inscrição do emissor (CNPJ ou CPF) e IM. O que não veio por opção a janela pergunta. Nada disso consome número.
    if [ -z "$INSC_TIPO" ]; then
        definir_inscricao "$(perguntar "Digite o CNPJ (14 dígitos) ou o CPF (11 dígitos) do emissor, sem pontos, barras ou traços." inscricao)" ""
    fi
    if [ -z "$IM" ]; then
        definir_im "$(perguntar "Digite a inscrição municipal (IM) do emissor, no cadastro da Prefeitura: de 1 a 15 letras, números, ponto, barra ou traço." im)"
    fi

    # 3.3: o .pfx é escolhido pelo líder e só passa ao document-export. Nunca é impresso nem listado.
    local pfx
    pfx=$(perguntar "Escolha o arquivo .pfx do certificado" pfx)
    # INFO-S8: caminho relativo (ou que começa por "-") não entra; o flatpak o leria como opção ou fora do lugar.
    # CISO (BAIXA-S3, condição 1): a mensagem fala só do "certificado escolhido"; o caminho nunca é impresso.
    [ -n "$pfx" ] || falha "certificado escolhido não pôde ser lido. Nada foi emitido."
    case "$pfx" in
        /*) ;;
        *) falha "o certificado escolhido precisa de caminho absoluto (começar por /). Nada foi emitido." ;;
    esac
    [ -f "$pfx" ] && [ -r "$pfx" ] || falha "certificado escolhido não pôde ser lido. Nada foi emitido."
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
    chmod 600 "$CFG" || falha "não consegui fechar a permissão do config.ini. Nada foi emitido."
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
        if [ "$RESULTADO" != local ]; then
            gravar_ndps "$NDPS_ATUAL" || falha "a tentativa chegou à Sefin, mas o número $NDPS_ATUAL não foi gravado: avise o Claude antes de emitir de novo."
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
        for ultimo in "$@"; do :; done
        printf '/run/user/1000/doc/ab12/%s\n' "$(basename "$ultimo")"
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
        [ "${NL_ENSAIO_PFX_VAZIO:-}" = 1 ] && exit 1
        if [ "${NL_ENSAIO_PFX_RELATIVO:-}" = 1 ]; then
            printf '%s\n' "pfxsecreto-ensaio.p12"
            exit 0
        fi
        printf '%s\n' "$NL_ENSAIO_DIR/${NL_ENSAIO_PFX_NOME:-pfxsecreto-ensaio.p12}"
        exit 0 ;;
    --inputbox)
        # NL-2.10g: a janela da inscrição e a da IM são distinguidas pelo texto; cada pergunta fica registrada (só o tipo).
        case "$2" in
            *"inscrição municipal"*)
                echo im >> "$NL_ENSAIO_DIR/perguntas.txt"
                [ -n "${NL_ENSAIO_IM_CANCELA:-}" ] && exit 1
                printf '%s\n' "${NL_ENSAIO_IM_JANELA-IMENS-7731}"
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
    local dir="$RAIZ/$nome" entrada rc saidas emit problema="" pfx_nome="pfxsecreto-ensaio.p12" sem_kd=0 par ndps_esp=""
    local semente="" perg_esp="" dps_esp="" dps_ausente="" saida_esp="" segredo="" opcoes_txt=""
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
        esac
    done
    # NL-2.10g: opções da linha de comando, separadas por | (o valor pode ter espaço ou aspas). Vazio = nenhuma opção.
    local -a opcoes=()
    [ -z "$opcoes_txt" ] || IFS='|' read -r -a opcoes <<< "$opcoes_txt"
    printf 'pfxsecreto-ensaio\n' > "$dir/$pfx_nome"
    # NL-2.10g: semente do contador, para provar que a parada antes da Sefin não o mexe.
    if [ -n "$semente" ]; then
        mkdir -p "$dir/base"
        printf '%s\n' "$semente" > "$dir/base/ultimo-ndps.txt"
    fi
    entrada=/dev/null
    if [ "$sem_kd" -eq 1 ]; then
        escrever_stubs_sem_kdialog "$dir"
        entrada="$dir/entrada.txt"
        printf '11222333000181\n%s\n' "$dir/$pfx_nome" > "$entrada"
    else
        escrever_stubs "$dir"
    fi
    env "$@" NL_ENSAIO_DIR="$dir" bash "$0" --ensaio-caso "${opcoes[@]}" < "$entrada" > "$dir/saida-do-script.txt" 2>&1
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
    [ -f "$dir/base/ultimo-ndps.txt" ] && ndps_atual=$(cat "$dir/base/ultimo-ndps.txt")
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
    if grep -r -l -F -e 12345678909 -e IMENS-7731 -e IEENS-5520 "$dir/saida-do-script.txt" "$dir/chamadas.txt" \
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
    r=$(trechos "$dir/base/teste-lider/dps-teste.json" presente "$dps_esp")
    [ -z "$r" ] || problema="$problema dps$r"
    r=$(trechos "$dir/base/teste-lider/dps-teste.json" ausente "$dps_ausente")
    [ -z "$r" ] || problema="$problema dps$r"
    r=$(trechos "$dir/saida-do-script.txt" presente "$saida_esp")
    [ -z "$r" ] || problema="$problema saida$r"
    if [ -n "$segredo" ] && grep -r -l -F -e "$segredo" "$dir/saida-do-script.txt" "$dir/chamadas.txt" \
        "$dir/base/devolucao" "$dir"/base/teste-lider/execucao-* >/dev/null 2>&1; then
        problema="$problema vazamento_do_valor_da_variante"
    fi
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
    ndps=$(cat "$dir/base/ultimo-ndps.txt" 2>/dev/null)
    if [ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] && [ "$ndps" = 2 ] && grep -q '"nDps": 2,' "$dir/base/teste-lider/dps-teste.json"; then
        printf 'ok %s\n' "$nome"
    else
        printf 'FALHA %s: rc=%s/%s ultimo_ndps=%s\n' "$nome" "$r1" "$r2" "$ndps"
        FALHAS=$(( FALHAS + 1 ))
    fi
    CASOS=$(( CASOS + 1 ))
}

ensaio() {
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
    ensaio_caso pfx_com_espaco_aspas_cifrao 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 "NL_ENSAIO_PFX_NOME=pfxsecreto com 'aspas' e \$HOME.p12"
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
    # j. opção desconhecida: para sem repetir o valor.
    ensaio_caso j_opcao_desconhecida_para_sem_repetir 1 "opção desconhecida" 0 0 "" NL_ENSAIO_OPCOES='--cpfx|12345678909' NL_ENSAIO_SEGREDO=12345678909 NL_ENSAIO_SEMENTE=7 NL_ENSAIO_NDPS_ESPERADO=7 NL_ENSAIO_PERGUNTAS=nenhuma
    # k. varredura: CPF, IM e IE de fixture só aparecem na DPS (a saída, a devolução, a execução e as chamadas ficam limpas).
    ensaio_caso k_varredura_fixtures_so_na_dps 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cpf|12345678909|--im|IMENS-7731|--ie|IEENS-5520' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_DPS_ESPERADA='"cpf": "12345678909"|"im": "IMENS-7731"'
    # l. --opcao=valor e --opcao valor dão a mesma DPS (sem contar dhEmi, que é a hora da execução).
    grupo_abrir
    ensaio_caso l_forma_separada_ou_igual.separada 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj|11222333000181|--im|IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_DPS_ESPERADA='"cnpj": "11222333000181"|"im": "IMENS-7731"'
    ensaio_caso l_forma_separada_ou_igual.igual 0 "Nota de teste aceita" 1 1 "" NL_ENSAIO_OPCOES='--cnpj=11222333000181|--im=IMENS-7731' NL_ENSAIO_SEQ=aceita NL_ENSAIO_NDPS_ESPERADO=1 NL_ENSAIO_PERGUNTAS=nenhuma NL_ENSAIO_DPS_ESPERADA='"cnpj": "11222333000181"|"im": "IMENS-7731"'
    if ! cmp -s <(grep -v dhEmi "$RAIZ/l_forma_separada_ou_igual.separada/base/teste-lider/dps-teste.json") \
                <(grep -v dhEmi "$RAIZ/l_forma_separada_ou_igual.igual/base/teste-lider/dps-teste.json") 2>/dev/null; then
        printf 'FALHA l_forma_separada_ou_igual: as duas DPS diferem\n'
        FALHAS=$(( FALHAS + 1 ))
    fi
    grupo_fechar l_forma_separada_ou_igual
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
        ler_opcoes "$@"
        fluxo; exit $? ;;
esac
