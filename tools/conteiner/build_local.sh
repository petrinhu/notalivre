#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Build e teste local do NotaLivre dentro de contêiner (L-50: nada de teste dinâmico na sessão do líder).
#
# Uso:
#   build_local.sh imagem <distro>        constrói a imagem de build da distro (pesado, sob flock)
#   build_local.sh qt                     instala Qt 6.12 no volume notalivre-qt-6120 (só se ainda não estiver)
#   build_local.sh preparar               imagem fedora45 + Qt (forma antiga da NL-0.2)
#   build_local.sh <distro> <preset>      configure + build + ctest do preset na distro (exige NL_FATIA)
#   build_local.sh <preset>               atalho: fedora45 <preset> (forma antiga, mantida; exige NL_FATIA)
#   build_local.sh ensaio <distro> <preset>   ensaio de repetição (NL-3.0j, CTO): etapa 2 = ensaio_no_conteiner.sh;
#                                         exige NL_ENSAIO; core=1 e --pids-limit 512 em todo docker run
#   build_local.sh mutante-mtls <distro>  mutante do E6 (tools/mutacao/mutante_mtls.sh) no SHA de NL_SHA; core=1
#   Leitura do ensaio: ENSAIO: VERDE (NL-RC 0), VERMELHO (1, slot reprovado) ou INDETERMINADO (7, não é reprovação).
#   distro: fedora45 | ubuntu26.04 | manjaro | mint22.3
#
# Variáveis de ambiente:
#   NL_FATIA          OBRIGATÓRIA nos builds: identificador da fatia (ex.: NL-2.0), letras, dígitos, . _ e -.
#                     Dá nome ao diretório de fonte e ao de build. Sem default: um default silencioso faria dois
#                     executores voltarem a compartilhar diretório (achado A1-07).
#   NL_SHA            commit a construir (padrão: HEAD do repositório NL_REPO). É resolvido para 40 hex antes de tudo.
#   NL_REPO           repositório git de onde sai o git archive (padrão: a raiz do repositório).
#   NL_FONTE          modo compatível para revisores com snapshot: copia esta pasta em vez de fazer o archive.
#                     O log grava "SHA: <sha>+copia", e o build PARA antes do contêiner se o manifesto da cópia
#                     diverge do commit (arquivo fora do commit). Sem NL_SHA, usa o HEAD da pasta, se ela for git.
#   NL_HIGIENE_IMAGEM imagem dos portões estáticos rodados antes do configure (padrão notalivre-higiene:ci; DA-2.55)
#   NL_TMP_BASE       base de temporários e de build no host (padrão: /var/tmp; o preset exige a variável)
#   NL_LOG            log do run (padrão: $NL_TMP_BASE/notalivre-logs/NL-0.4-<distro>-<preset>.log;
#                     imagem: $NL_TMP_BASE/notalivre-logs/NL-2.0-imagem-<distro>.log)
#   NL_BUILD_ROOT     diretório de build no host (padrão: $NL_TMP_BASE/notalivre-build-<distro>-<fatia>)
#   NL_SEM_CORE       1: todo docker run leva --ulimit core=1 (rodada normal; ensaio e mutante sempre levam)
#   NL_ENSAIO         ensaio: lista "nome:N" separada por espaço; nome em [A-Za-z0-9_.], N de 1 a 9999
#   NL_CPUSET         lista de CPUs para --cpuset-cpus em todo docker run (ex.: 4-15). Padrão calculado pelo átomo
#                     cota_nucleos.sh: as 4 primeiras CPUs ficam livres, em máquina com 8 CPUs ou mais; em máquina menor, ou
#                     com NL_CPUSET=todas, a opção não é posta (AVISO). Inválida: PARADA, rc 2, antes de qualquer docker run.
#                     A linha NL-COTAS do log grava o que valeu. Só o contêiner de build leva a opção (L-17: os gêmeos
#                     portoes_estaticos.sh, jobs do espelho, sabotar_portoes.sh e docker build ficam na INBOX).
#   NL_CPUS_ONLINE_ARQ  só dos autotestes: arquivo no lugar de /sys/devices/system/cpu/online (nunca vai ao contêiner).
#   NL_XSD_ZIP_CACHE  diretório no host com o ZIP do XSD (padrão: $NL_TMP_BASE/notalivre-xsd-cache). É montado no
#                     contêiner em /xsd-cache e exportado como NL_XSD_ZIP_CACHE; o CMake do third_party lê a variável.
#
# Evidência por SHA (NL-2.0): a linha 1 do log é "SHA: <40 hex> ...", e NL-MANIFESTO-SHA256 traz o hash da fonte
# que o contêiner viu. tools/conteiner/evidencia_sha.sh verificar <log> confere o manifesto contra o commit.
#
# Regras aplicadas (L-11, L-36, L-45, L-49, L-53):
#   - um trabalho pesado por vez: cópia da fonte E docker run ficam DENTRO da mesma trava flock $TRAVA;
#   - cada fatia tem a própria fonte (notalivre-src-<SHA>-<fatia>) e o próprio build; nada compartilhado;
#   - cota de CPU e RAM do contêiner: NL_THREADS (padrão 7) e NL_MEMORIA (padrão 12g), por ordem do líder de 30/10/2026;
#   - reserva de CPUs, por ordem do líder de 11/11/2026: as 4 primeiras CPUs ficam livres para a sessão dele (NL_CPUSET);
#   - build e temporários em /var/tmp (TMPDIR exportado abaixo), nunca em /tmp (tmpfs);
#   - o status é o do contêiner: o log é gravado por redirecionamento, nunca por pipe em tail;
#   - o contêiner roda com --user 1000:1000 e sem --privileged.
set -euo pipefail

# Base de temporários e de build no HOST (A0-08). Padrão /var/tmp; trocável pela variável NL_TMP_BASE.
NL_TMP_BASE="${NL_TMP_BASE:-/var/tmp}"
export NL_TMP_BASE
export TMPDIR="$NL_TMP_BASE"
# Modo do arnês (NL-3.0j): vazio na rodada normal; ensaio ou mutante. Só o despacho define (ensaio, mutante-mtls), e o modo
# chega ao subcomando executar como quarto argumento. A variável de ambiente NL_MODO_ARNES não é lida (C3 da revisão).
MODO_ARNES=""
# sha256 do tools/mutacao/mutante_mtls.sh do SHA pedido; preenchido por validar_mutante.
HASH_MUTANTE=""

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SCRIPT="$RAIZ/tools/conteiner/build_local.sh"
NL_REPO="${NL_REPO:-$RAIZ}"
# Funções de evidência por SHA (extrair_do_sha, manifesto_de_dir, manifesto_do_sha) e a lista LISTA_FONTE.
. "$RAIZ/tools/conteiner/evidencia_sha.sh"
# Regra da reserva de CPUs (nl_cpuset_valido) e da cota de núcleos: átomo único, a regex fica lá dentro.
. "$RAIZ/tools/conteiner/cota_nucleos.sh"
VOLUME_QT="notalivre-qt-6120"
QT_PREFIXO="/opt/qt/6.12.0/gcc_64"
# SELinux Enforcing: a fonte (pasta sincronizada com a nuvem) é unlabeled_t e o contêiner não a lê por bind mount.
# Por isso o contêiner recebe uma cópia montada de /var/tmp (rótulo do próprio usuário, :z), refeita a cada fatia.
# Não se relabela a árvore sincronizada do líder.
LOG_DIR="$NL_TMP_BASE/notalivre-logs"
TRAVA="$NL_TMP_BASE/notalivre-heavy.lock"
# Cota dividida com outra sessão desta máquina (ordem do líder de 30/10/2026): 1 núcleo e 4 GiB livres para o sistema.
THREADS="${NL_THREADS:-7}"
NL_MEMORIA="${NL_MEMORIA:-12g}"
# Reserva de CPUs (NL-3.0j): preenchidos por resolver_cpuset, nunca no nível de cima do script.
CPUSET=""
cotas_cpu=()

# Decide a lista de CPUs do contêiner e monta cotas_cpu (vazio com "todas", senão --cpuset-cpus <lista>). A regra mora no átomo
# cota_nucleos.sh. Só roda dentro de executar e de qt_instalar: o AVISO tem de sair depois da linha "SHA:" do log, que
# evidencia_sha.sh exige como linha 1, e aqui o padrão só avisa em máquina pequena (nesta máquina de 16 CPUs não avisa).
# Arquivo de CPUs ausente não derruba o script (set -e): texto vazio, e o átomo responde "todas" com AVISO.
resolver_cpuset()
{
    local online="" arq="${NL_CPUS_ONLINE_ARQ:-/sys/devices/system/cpu/online}"
    if [ -r "$arq" ]; then
        read -r online < "$arq" || true
    fi
    if ! CPUSET="$(nl_cpuset_valido "${NL_CPUSET:-}" "$online")"; then
        echo "PARADA: NL_CPUSET inválida; build não iniciado" >&2
        return 2
    fi
    cotas_cpu=()
    if [ "$CPUSET" != todas ]; then
        cotas_cpu=(--cpuset-cpus "$CPUSET")
    fi
}

# Distros aceitas: cada uma tem <distro>-build.Dockerfile e instalar/<distro>.sh (ver ci.yml).
checar_distro()
{
    case "$1" in
        fedora45|ubuntu26.04|manjaro|mint22.3) ;;
        *) echo "distro desconhecida: $1 (aceitas: fedora45 ubuntu26.04 manjaro mint22.3)" >&2; exit 2 ;;
    esac
}

# Tag NL-2.0: as imagens ganharam xmlsec1 (oráculo da DA-2.1). A tag nl-0.4 antiga fica no Docker, sem apagar.
imagem_de()
{
    echo "notalivre/build-$1:nl-2.0"
}

# Executa um comando pesado sob a trava compartilhada e com prioridade reduzida.
# Se o chamador já detém a trava (NL_TRAVA_DETIDA=1), não pede de novo: flock aninhado no mesmo arquivo
# trava o próprio processo (medido em 25/10/2026: um flock órfão esperou a própria trava por 18 min).
pesado()
{
    if [ "${NL_TRAVA_DETIDA:-}" = 1 ]; then
        nice -n 5 "$@"
    else
        flock "$TRAVA" nice -n 5 "$@"
    fi
}

# Reconstrói a imagem de uma distro. O log é gravado por redirecionamento (NL_LOG ou padrão).
# Exceção documentada à evidência por SHA: o contexto da imagem (tools/conteiner) vem da ÁRVORE DE TRABALHO, porque
# as mudanças de instalar/ ainda não estão commitadas. A linha 1 traz o SHA de HEAD, mas declara origem=arvore-de-trabalho
# e o log NÃO traz NL-MANIFESTO-SHA256: o verificador reprova de propósito (imagem não é evidência por SHA até o commit).
construir_imagem()
{
    local distro="$1" log rc=0 sha
    checar_distro "$distro"
    mkdir -p "$LOG_DIR"
    log="${NL_LOG:-$LOG_DIR/NL-2.0-imagem-$distro.log}"
    sha="$(resolver_sha)" || exit 2
    printf 'SHA: %s fatia=imagem distro=%s origem=arvore-de-trabalho data=%s\n' \
        "$sha" "$distro" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" > "$log"
    pesado docker build -t "$(imagem_de "$distro")" \
        -f "$RAIZ/tools/conteiner/$distro-build.Dockerfile" "$RAIZ/tools/conteiner" \
        >> "$log" 2>&1 || rc=$?
    echo "NL-RC=$rc" >> "$log"
    echo "log: $log rc=$rc"
    return "$rc"
}

# Instala Qt 6.12 no volume, uma única vez. Se o volume já tem o Qt, não reinstala (aqt reinstalaria tudo).
qt_instalar()
{
    resolver_cpuset || return 2
    if pesado docker run --pull=never --rm "${cotas_cpu[@]}" --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --user 1000:1000 -v "$VOLUME_QT:/opt/qt:ro" "$(imagem_de ubuntu26.04)" \
        test -f "$QT_PREFIXO/lib/cmake/Qt6/Qt6Config.cmake"; then
        echo "Qt 6.12 já presente no volume $VOLUME_QT: nada a instalar"
        return 0
    fi
    docker volume create "$VOLUME_QT" > /dev/null
    # O volume nasce com dono root; ajusta uma única vez para o usuário 1000 (dono do host).
    pesado docker run --pull=never --rm "${cotas_cpu[@]}" --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" -u 0:0 -v "$VOLUME_QT:/opt/qt" "$(imagem_de ubuntu26.04)" chown 1000:1000 /opt/qt
    # HOME em /tmp/home (gravável pelo uid 1000 no contêiner; /var/tmp da imagem é do root e aqt falha ali).
    # aqt pode terminar com status 0 mesmo após erro: a conferência final mede o artefato no volume.
    pesado docker run --pull=never --rm "${cotas_cpu[@]}" --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -e NL_TMP_BASE=/var/tmp \
        -v "$VOLUME_QT:/opt/qt" "$(imagem_de ubuntu26.04)" \
        sh -c 'mkdir -p "$HOME" && cd "$HOME" && aqt install-qt -O /opt/qt linux desktop 6.12.0 linux_gcc_64 -m qtnetworkauth qthttpserver qtimageformats qtshadertools qtwebsockets && test -f /opt/qt/6.12.0/gcc_64/lib/cmake/Qt6/Qt6Config.cmake'
}

# Arnês de repetição (NL-3.0j, CTO): valida o que vai ao contêiner antes de qualquer docker run e antes da trava.
# Ensaio: preset de três nomes; NL_ENSAIO é "nome:N" separados por espaço, nome em [A-Za-z0-9_.], N de 1 a 9999 sem zero
# à esquerda. Os valores vão ao contêiner por argv, nunca dentro de string de shell.
validar_arnes()
{
    local preset="$1" item nome n
    local -a itens
    [ "$MODO_ARNES" = ensaio ] || return 0
    case "$preset" in
        debug|release|asan-ubsan) ;;
        *) echo "PARADA: preset '$preset' fora de debug, release e asan-ubsan (ensaio)" >&2; return 2 ;;
    esac
    read -r -a itens <<< "${NL_ENSAIO:-}"
    if [ "${#itens[@]}" -eq 0 ]; then
        echo "PARADA: NL_ENSAIO vazia (ensaio): informe nome:N" >&2
        return 2
    fi
    for item in "${itens[@]}"; do
        case "$item" in
            *:*) ;;
            *) echo "PARADA: '$item' sem ':' (use nome:N)" >&2; return 2 ;;
        esac
        nome="${item%:*}"
        n="${item##*:}"
        case "$nome" in
            ''|*[!A-Za-z0-9_.]*) echo "PARADA: nome '$nome' fora de [A-Za-z0-9_.]" >&2; return 2 ;;
        esac
        case "$n" in
            [1-9]|[1-9][0-9]|[1-9][0-9][0-9]|[1-9][0-9][0-9][0-9]) ;;
            *) echo "PARADA: N '$n' fora de 1 a 9999 sem zero à esquerda" >&2; return 2 ;;
        esac
    done
    return 0
}

# Mutante (NL-3.0j, I1 da revisão): o script do mutante tem de existir no SHA pedido. O hash do blob vai ao log e ao
# contêiner, que só executa o arquivo extraído se o hash dele for igual. Roda antes de qualquer docker run e antes da trava.
validar_mutante()
{
    local sha="$1"
    [ "$MODO_ARNES" = mutante ] || return 0
    if [ "$(git -C "$NL_REPO" cat-file -t "$sha:tools/mutacao/mutante_mtls.sh" 2>/dev/null || true)" != blob ]; then
        echo "PARADA: tools/mutacao/mutante_mtls.sh não existe no SHA $sha (mutante-mtls); build não iniciado" >&2
        return 2
    fi
    HASH_MUTANTE="$(git -C "$NL_REPO" show "$sha:tools/mutacao/mutante_mtls.sh" | sha256sum | cut -d' ' -f1)"
    return 0
}

# Resolve o SHA a construir para 40 hex, sem efeito colateral. Falha com PARADA (rc 2) se não for commit.
resolver_sha()
{
    local rev="${NL_SHA:-}" sha
    if [ -z "$rev" ]; then
        if [ -n "${NL_FONTE:-}" ]; then
            rev="$(git -C "$NL_FONTE" rev-parse --verify --quiet HEAD 2>/dev/null || true)"
            if [ -z "$rev" ]; then
                echo "PARADA: NL_FONTE não é repositório git: defina NL_SHA com o commit do snapshot" >&2
                return 2
            fi
        else
            rev=HEAD
        fi
    fi
    sha="$(git -C "$NL_REPO" rev-parse --verify --quiet "$rev^{commit}" 2>/dev/null || true)"
    if [ -z "$sha" ]; then
        echo "PARADA: '$rev' não é commit de $NL_REPO" >&2
        return 2
    fi
    printf '%s\n' "$sha"
}

# Valida NL_FATIA (obrigatória). Ela entra em nome de diretório, então aceita só [A-Za-z0-9._-] e sem ponto inicial.
checar_fatia()
{
    local fatia="${NL_FATIA:-}"
    if [ -z "$fatia" ]; then
        echo "PARADA: NL_FATIA obrigatória (ex.: NL_FATIA=NL-2.0): o nome da fatia dá o diretório de fonte e de build" >&2
        return 2
    fi
    case "$fatia" in
        .*|*..*|*[!A-Za-z0-9._-]*)
            echo "PARADA: NL_FATIA '$fatia' com caractere ou formato fora de [A-Za-z0-9._-]" >&2
            return 2
            ;;
    esac
    printf '%s\n' "$fatia"
}

# Monta a fonte do build no diretório $2, que é só desta fatia (criado aqui; o rm -rf atinge só o caminho que o
# próprio script montou com SHA hex e fatia validada, dentro de NL_TMP_BASE).
# Modo archive (padrão): git archive do SHA $1 em NL_REPO, sem nenhum arquivo fora do commit.
# Modo cópia (NL_FONTE): cópia dos mesmos caminhos de LISTA_FONTE, sem --preserve=timestamps (ver a verificação de SHA).
preparar_fonte()
{
    local sha="$1" destino="$2"
    rm -rf -- "$destino"
    mkdir -p -- "$destino"
    if [ -n "${NL_FONTE:-}" ]; then
        (cd "$NL_FONTE" && cp -a --parents --no-preserve=timestamps -- "${LISTA_FONTE[@]}" "$destino/")
    else
        extrair_do_sha "$NL_REPO" "$sha" "$destino"
    fi
}

# Trava própria do build: o subcomando "executar" a adquire aqui, antes de tocar em qualquer coisa.
# Assim, mesmo chamado direto (sem construir), ele não roda fora da trava (L-11: um trabalho pesado por vez).
# Se o chamador já detém a trava (NL_TRAVA_DETIDA=1), não pega de novo (ver pesado).
adquirir_trava()
{
    if [ "${NL_TRAVA_DETIDA:-}" = 1 ]; then
        return 0
    fi
    exec 9> "$TRAVA"
    flock 9
    # Quem adquire exporta a marca: nenhum filho readquire o mesmo arquivo.
    NL_TRAVA_DETIDA=1
    export NL_TRAVA_DETIDA
}

# Corpo do build. A linha 1 do log é o SHA (escrita antes de qualquer trava, para que um log interrompido ainda
# diga de qual commit era). O trailer NL-RC é gravado ao sair, com o status real do script.
executar()
{
    local distro="$1" preset="$2"
    local sha fatia origem sufixo fonte build_host xsd_host imagem manifesto esperado

    checar_distro "$distro"
    sha="$(resolver_sha)" || exit 2
    fatia="$(checar_fatia)" || exit 2
    origem=archive
    sufixo=""
    if [ -n "${NL_FONTE:-}" ]; then
        origem=copia
        sufixo="+copia"
    fi
    printf 'SHA: %s%s fatia=%s distro=%s preset=%s origem=%s data=%s\n' \
        "$sha" "$sufixo" "$fatia" "$distro" "$preset" "$origem" "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    trap 'echo "NL-RC=$?"' EXIT
    # Reserva de CPUs (NL-3.0j): depois da linha "SHA:", antes de validar_arnes, da trava e de qualquer docker run.
    resolver_cpuset || return 2
    printf 'NL-COTAS=threads=%s memoria=%s cpuset=%s\n' "$THREADS" "$NL_MEMORIA" "$CPUSET"
    validar_arnes "$preset" || return 2
    validar_mutante "$sha" || return 2
    # Cotas de contêiner do arnês (NL-3.0j): o ensaio força core=1 e --pids-limit 512 em todo docker run; mutante e
    # rodada normal levam só core=1 (o mutante sempre, a rodada normal com NL_SEM_CORE=1).
    local -a extras_arnes=()
    if [ "$MODO_ARNES" = ensaio ]; then
        extras_arnes=(--ulimit core=1 --pids-limit 512)
    elif [ -n "$MODO_ARNES" ] || [ "${NL_SEM_CORE:-}" = 1 ]; then
        extras_arnes=(--ulimit core=1)
    fi

    fonte="$NL_TMP_BASE/notalivre-src-$sha-$fatia"
    build_host="${NL_BUILD_ROOT:-$NL_TMP_BASE/notalivre-build-$distro-$fatia}"
    xsd_host="${NL_XSD_ZIP_CACHE:-$NL_TMP_BASE/notalivre-xsd-cache}"
    imagem="$(imagem_de "$distro")"

    adquirir_trava
    preparar_fonte "$sha" "$fonte"
    manifesto="$(manifesto_de_dir "$fonte")"
    printf 'NL-SRC=%s\nNL-BUILD=%s\nNL-XSD-CACHE=%s\nNL-MANIFESTO-SHA256=%s\n' \
        "$fonte" "$build_host" "$xsd_host" "$manifesto"
    if [ -n "${NL_FONTE:-}" ]; then
        if ! esperado="$(manifesto_do_sha "$NL_REPO" "$sha")"; then
            echo "PARADA: não consegui extrair $sha de $NL_REPO para conferir a cópia de NL_FONTE"
            return 4
        fi
        if [ "$esperado" != "$manifesto" ]; then
            echo "PARADA: a cópia de NL_FONTE diverge do commit $sha (manifesto $manifesto, commit $esperado): arquivo fora do commit"
            return 4
        fi
    fi
    mkdir -p "$build_host" "$xsd_host"

    # Autotestes de estreia (NL-2.11, DA-2.61; L-36): rodam no contêiner de build, sobre o mesmo SHA, antes dos portões.
    # Um trava nova só vale com autoteste que roda. O autoteste de limites (autoteste_build_limites.sh) NÃO entra aqui:
    # ele chama o próprio build_local (recursão) e usa um docker de mentira no PATH, que não existe no contêiner. Roda
    # no job autotestes do ci.yml.
    local autoteste
    for autoteste in tools/conteiner/autoteste_piso_portoes.sh flatpak/autoteste_suite_flatpak.sh tools/portoes/autoteste_pulos_testes.sh tools/portoes/autoteste_fora_do_flatpak.sh; do
        if ! docker run --pull=never "${extras_arnes[@]}" --rm "${cotas_cpu[@]}" --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --network none \
            --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -v "$fonte:/src:ro,z" -w /src "$imagem" \
            sh -c "mkdir -p \"\$HOME\" && bash $autoteste"; then
            echo "PARADA: autoteste $autoteste reprovou (L-36); build não iniciado"
            return 6
        fi
    done

    # Portões estáticos (DA-2.55, NL-2.11): os mesmos que o CI roda nos jobs higiene e camadas, sobre o mesmo SHA, antes do
    # configure (L-36). Pasta própria do SHA e da fatia; o rm -rf atinge só o caminho que este script monta.
    # NL-3.0g (CISO, 05/11/2026): o script roda da árvore de trabalho ($RAIZ), e só se for idêntico, byte a byte, ao do SHA.
    # Lista e piso são lidos do SHA dentro do próprio portão. Nada do SHA executa no host: só o script conferido.
    portoes="$NL_TMP_BASE/notalivre-portoes-$sha-$fatia"
    rm -rf -- "$portoes"
    portoes_script="$RAIZ/tools/conteiner/portoes_estaticos.sh"
    # Comparação por sha256sum (coreutils, presente no host e na imagem; cmp não está na fedora45, medido). Byte a byte.
    if [ "$(git -C "$NL_REPO" show "$sha:tools/conteiner/portoes_estaticos.sh" 2>/dev/null | sha256sum)" \
         != "$(sha256sum < "$portoes_script")" ]; then
        echo "PARADA: o portoes_estaticos.sh do SHA $sha não existe ou difere do da árvore ($portoes_script); build não iniciado"
        return 5
    fi
    echo "NL-PORTOES-SCRIPT=$portoes_script"
    if ! bash "$portoes_script" "$NL_REPO" "$sha" "$portoes"; then
        echo "PARADA: portões estáticos reprovaram (DA-2.55); build não iniciado"
        return 5
    fi

    # Etapa 1, como root e só de leitura do Qt: tabela de versões e pré-cheque (o índice do gerenciador exige root).
    if ! docker run --pull=never "${extras_arnes[@]}" --rm "${cotas_cpu[@]}" --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" --user 0:0 -e QT_PREFIXO="$QT_PREFIXO" \
        -v "$fonte:/src:ro,z" -v "$VOLUME_QT:/opt/qt:ro" \
        -w /src "$imagem" bash tools/conteiner/executar_no_conteiner.sh diagnostico; then
        echo "PARADA: diagnóstico do contêiner reprovou (pré-cheque do Qt ou ambiente); build não iniciado"
        return 3
    fi
    # Etapa 2, como 1000:1000: configure, build e ctest. Arquivos do build ficam com o dono do host.
    # NL_XSD_ZIP_CACHE (gêmeo da INBOX): o ZIP do XSD vem do cache do host, como no CI, e não do gov.br.
    # NL-2.14 (adendo 5, item 2): o diretório git comum do repositório entra só para leitura, para o resumo de revisão
    # (tools/portoes/resumo_listas.sh) imprimir o diff das listas contra o merge-base com a onda-2. Sem git, o resumo diz que
    # o diff está indisponível; o build não para por isso.
    gitopts=()
    gitcomum="$(git -C "$NL_REPO" rev-parse --path-format=absolute --git-common-dir 2>/dev/null || true)"
    if [ -n "$gitcomum" ]; then
        gitopts=(-v "$gitcomum:/gitrepo:ro,z" -e NL_GIT_DIR=/gitrepo -e NL_SHA="$sha")
    fi
    # Etapa 2 por modo (NL-3.0j): rodada normal (executar_no_conteiner.sh), ensaio (cópia do corpo da árvore de quem
    # chama, com o hash no log) ou mutante-mtls (script do próprio SHA, lido de /gitrepo).
    local -a corpo_etapa2=(sh -c "mkdir -p \"\$HOME\" && bash tools/conteiner/executar_no_conteiner.sh $preset")
    local -a montagem_arnes=() env_arnes=()
    local corpo_dir
    case "$MODO_ARNES" in
        ensaio)
            corpo_dir="$NL_TMP_BASE/notalivre-ensaio-corpo-$sha-$fatia"
            mkdir -p -- "$corpo_dir"
            cp -f -- "$RAIZ/tools/conteiner/ensaio_no_conteiner.sh" "$corpo_dir/ensaio_no_conteiner.sh"
            printf 'NL-ENSAIO-CORPO-SHA256=%s\n' "$(sha256sum < "$corpo_dir/ensaio_no_conteiner.sh" | cut -d' ' -f1)"
            printf 'NL-ENSAIO-PEDIDO=%s\n' "$NL_ENSAIO"
            montagem_arnes=(-v "$corpo_dir:/ensaio:ro,z")
            env_arnes=(-e "NL_ENSAIO=$NL_ENSAIO" -e "NL_ENSAIO_SHA=$sha")
            corpo_etapa2=(sh -c "mkdir -p \"\$HOME\" && bash /ensaio/ensaio_no_conteiner.sh \"\$1\"" sh "$preset")
            ;;
        mutante)
            if [ "${#gitopts[@]}" -eq 0 ]; then
                echo "PARADA: mutante-mtls sem diretório git comum para montar em /gitrepo; build não iniciado"
                return 2
            fi
            # O corpo do modo (mutante_no_conteiner.sh) e o helper de extração vêm da árvore de quem chama, copiados e
            # montados só para leitura, com o hash de cada um no log (como o corpo do ensaio). O script do mutante sai do
            # próprio SHA, dentro do contêiner, e só roda se o sha256 do arquivo extraído for igual ao do blob do SHA.
            corpo_dir="$NL_TMP_BASE/notalivre-mutante-corpo-$sha-$fatia"
            mkdir -p -- "$corpo_dir"
            cp -f -- "$RAIZ/tools/conteiner/extrair_do_sha.sh" "$corpo_dir/extrair_do_sha.sh"
            cp -f -- "$RAIZ/tools/conteiner/mutante_no_conteiner.sh" "$corpo_dir/mutante_no_conteiner.sh"
            printf 'NL-EXTRATOR-SHA256=%s\n' "$(sha256sum < "$corpo_dir/extrair_do_sha.sh" | cut -d' ' -f1)"
            printf 'NL-MUTANTE-CORPO-SHA256=%s\n' "$(sha256sum < "$corpo_dir/mutante_no_conteiner.sh" | cut -d' ' -f1)"
            printf 'NL-MUTANTE-SCRIPT-SHA256-DO-SHA=%s\n' "$HASH_MUTANTE"
            montagem_arnes=(-v "$corpo_dir:/extrator:ro,z")
            env_arnes=(-e "NL_MUTANTE_SCRIPT_SHA256=$HASH_MUTANTE")
            corpo_etapa2=(sh -c "mkdir -p \"\$HOME\" && bash /extrator/mutante_no_conteiner.sh /gitrepo \"\$NL_SHA\" /var/tmp/notalivre-build")
            ;;
    esac
    docker run --pull=never "${extras_arnes[@]}" --rm "${cotas_cpu[@]}" --cpus "$THREADS" --memory "$NL_MEMORIA" --memory-swap "$NL_MEMORIA" \
        --user 1000:1000 -e HOME=/tmp/home -e TMPDIR=/var/tmp -e NL_TMP_BASE=/var/tmp -e NL_DISTRO="$distro" \
        -e CMAKE_PREFIX_PATH="$QT_PREFIXO" -e CMAKE_BUILD_PARALLEL_LEVEL="$THREADS" \
        -e NL_XSD_ZIP_CACHE=/xsd-cache \
        "${gitopts[@]}" "${env_arnes[@]}" \
        -v "$fonte:/src:ro,z" -v "$build_host:/var/tmp/notalivre-build:z" -v "$VOLUME_QT:/opt/qt:ro" \
        -v "$xsd_host:/xsd-cache:ro,z" "${montagem_arnes[@]}" \
        -w /src "$imagem" \
        "${corpo_etapa2[@]}"
}

construir()
{
    local distro="$1" preset="$2" modo="${3:-}"
    local log="${NL_LOG:-$LOG_DIR/NL-0.4-$distro-$preset.log}"
    local rc=0

    checar_distro "$distro"
    mkdir -p "$LOG_DIR"
    # A trava é adquirida dentro de executar (cópia da fonte e docker run ficam na mesma seção crítica).
    nice -n 5 "$SCRIPT" executar "$distro" "$preset" "$modo" > "$log" 2>&1 || rc=$?
    echo "log: $log rc=$rc"
    return "$rc"
}

case "${1:-}" in
    imagem)
        construir_imagem "${2:?uso: build_local.sh imagem <distro>}"
        ;;
    qt)
        qt_instalar
        ;;
    preparar)
        construir_imagem fedora45
        qt_instalar
        ;;
    executar)
        case "${4:-}" in
            ''|ensaio|mutante) MODO_ARNES="${4:-}" ;;
            *) echo "PARADA: modo de arnês '${4:-}' desconhecido (aceitos: ensaio e mutante)" >&2; exit 2 ;;
        esac
        executar "${2:?distro}" "${3:?preset}"
        ;;
    fedora45|ubuntu26.04|manjaro|mint22.3)
        construir "$1" "${2:?uso: build_local.sh <distro> <preset>}"
        ;;
    debug|release|asan-ubsan)
        # Forma antiga (NL-0.2): atalho para fedora45.
        construir fedora45 "$1"
        ;;
    ensaio)
        # NL-3.0j: ensaio de repetição (CTO). Mesmo caminho de executar; a etapa 2 vira o corpo do ensaio.
        checar_distro "${2:?uso: build_local.sh ensaio <distro> <preset>}"
        construir "$2" "${3:?uso: build_local.sh ensaio <distro> <preset>}" ensaio
        ;;
    mutante-mtls)
        # NL-3.0j: mutante do E6 (mutante_mtls.sh). Mesmo caminho de executar, com core=1 forçado.
        checar_distro "${2:?uso: build_local.sh mutante-mtls <distro>}"
        construir "$2" debug mutante
        ;;
    *)
        echo "uso: $0 imagem <distro> | qt | preparar | <distro> <preset> | ensaio <distro> <preset> | mutante-mtls <distro> | <preset>" >&2
        echo "distro: fedora45 ubuntu26.04 manjaro mint22.3 (todos os builds exigem NL_FATIA)" >&2
        exit 2
        ;;
esac
