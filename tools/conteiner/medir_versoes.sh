#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Tabela de versões do ambiente de build (NL-0.4). Um só script, usado pelo build local e pelo
# $GITHUB_STEP_SUMMARY do CI (L-17): a mesma tabela em todos os jobs.
# Roda DENTRO do contêiner. Só lê versões; atualiza o índice de pacotes para ler o candidato do repositório.
#
# Uso:
#   medir_versoes.sh              tabela em texto (colunas separadas por tabulação)
#   medir_versoes.sh --markdown   tabela em markdown (para anexar ao $GITHUB_STEP_SUMMARY)
#
# Coluna "Instalado": o que a imagem traz agora, ou "ausente".
# Coluna "Candidato no repositório": a versão que o gerenciador instalaria, ou "ausente".
# A segunda coluna é a que decide a questão Argon2id no OpenSSL 3.0 (plano, NL-0.4).
set -u

QT_PREFIXO="${QT_PREFIXO:-/opt/qt/6.12.0/gcc_64}"
FORMATO="${1:-texto}"

# Nomes de pacote por família de distro, para o candidato do repositório.
declare -A PACOTE_DEB=( [openssl]=libssl-dev [libsodium]=libsodium-dev [xmlsec1]=libxmlsec1-dev \
                        [libxml2]=libxml2-dev [zlib]=zlib1g-dev [qt6]=qt6-base-dev )
declare -A PACOTE_ARCH=( [openssl]=openssl [libsodium]=libsodium [xmlsec1]=xmlsec \
                         [libxml2]=libxml2 [zlib]=zlib [qt6]=qt6-base )
# Fedora 44 não tem zlib-devel: o zlib é o zlib-ng-compat (medido em 25/10/2026, dnf info).
declare -A PACOTE_DNF=( [openssl]=openssl-devel [libsodium]=libsodium-devel [xmlsec1]=xmlsec1-devel \
                        [libxml2]=libxml2-devel [zlib]=zlib-ng-compat-devel [qt6]=qt6-qtbase-devel )

# Sistema de pacotes da imagem, pelo os-release.
famiglia_distro()
{
    local ids
    ids="$(. /etc/os-release && echo "$ID ${ID_LIKE:-}")"
    case " $ids " in
        *" debian "*|*" ubuntu "*) echo deb ;;
        *" arch "*|*" manjaro "*)  echo arch ;;
        *" fedora "*|*" rhel "*)   echo dnf ;;
        *)                          echo desconhecida ;;
    esac
}

# Primeira linha da saída de um comando; "ausente" se o comando não existe.
primeira_linha()
{
    local cmd="$1"; shift
    if ! command -v "$cmd" > /dev/null 2>&1; then echo ausente; return; fi
    "$cmd" "$@" 2>&1 | sed -n '1p'
}

# Versão de um módulo pkg-config; "ausente" se o módulo não existe.
modulo_instalado()
{
    if command -v pkg-config > /dev/null 2>&1 && pkg-config --exists "$1" 2>/dev/null; then
        pkg-config --modversion "$1"
    else
        echo ausente
    fi
}

# Atualiza o índice do gerenciador (leitura de metadados; não instala nada).
# Falha não é silenciosa (L-36): marca INDICE_OK=0 e avisa; a coluna de candidato vira "sem-indice",
# para não confundir índice que não atualizou com pacote que não existe ("ausente").
INDICE_OK=1
atualizar_indice()
{
    local rc=0
    case "$(famiglia_distro)" in
        deb)  apt-get update -qq > ${TMPDIR:-/var/tmp}/medir_indice.$$ 2>&1 || rc=$? ;;
        arch) pacman -Sy --noconfirm > ${TMPDIR:-/var/tmp}/medir_indice.$$ 2>&1 || rc=$? ;;
        dnf)  dnf -q makecache > ${TMPDIR:-/var/tmp}/medir_indice.$$ 2>&1 || rc=$? ;;
        *)    rc=1 ;;
    esac
    if [ "$rc" -ne 0 ]; then
        INDICE_OK=0
        echo "AVISO medir_versoes: falha ao atualizar o índice de pacotes (rc=$rc); candidatos marcados sem-indice"
        sed -n '1,5p' ${TMPDIR:-/var/tmp}/medir_indice.$$ 2>/dev/null | sed 's/^/  indice: /'
    fi
    rm -f ${TMPDIR:-/var/tmp}/medir_indice.$$
}

# Versão candidata de um pacote no repositório: o que o gerenciador instalaria hoje.
candidato()
{
    local generico="$1" pacote versao=""
    if [ "$INDICE_OK" -ne 1 ]; then echo sem-indice; return; fi
    case "$(famiglia_distro)" in
        deb)
            pacote="${PACOTE_DEB[$generico]:-}"
            [ -n "$pacote" ] && versao="$(apt-cache policy "$pacote" 2>/dev/null | sed -n 's/^ *Candidate: //p')"
            ;;
        arch)
            pacote="${PACOTE_ARCH[$generico]:-}"
            [ -n "$pacote" ] && versao="$(pacman -Si "$pacote" 2>/dev/null | sed -n 's/^Version *: *//p')"
            ;;
        dnf)
            pacote="${PACOTE_DNF[$generico]:-}"
            [ -n "$pacote" ] && versao="$(dnf -q info "$pacote" 2>/dev/null | sed -n 's/^Version *: *//p' | sed -n '1p')"
            ;;
    esac
    if [ -n "$versao" ] && [ "$versao" != "(none)" ]; then echo "$versao"; else echo ausente; fi
}

# Versão do Qt montado no volume (lida do cabeçalho qconfig.h, QT_VERSION_STR; não executa o Qt).
qt_instalado()
{
    local arq="$QT_PREFIXO/include/QtCore/qconfig.h" v
    if [ ! -f "$arq" ]; then echo ausente; return; fi
    v="$(sed -n 's/^#define QT_VERSION_STR "\([^"]*\)".*/\1/p' "$arq")"
    if [ -n "$v" ]; then echo "$v"; else echo ausente; fi
}

# Argon2id no OpenSSL, pela listagem do CLI (plano NL-0.4: o Argon2 só existe no OpenSSL a partir da 3.2).
argon2id_openssl()
{
    if ! command -v openssl > /dev/null 2>&1; then echo "ausente (sem CLI openssl)"; return; fi
    if openssl list -kdf-algorithms 2>/dev/null | grep -qi 'argon2id'; then echo presente; else echo ausente; fi
}

linha()
{
    local componente="$1" instalado="$2" cand="$3"
    if [ "$FORMATO" = "--markdown" ]; then
        printf '| %s | %s | %s |\n' "$componente" "$instalado" "$cand"
    else
        printf '%s\t%s\t%s\n' "$componente" "$instalado" "$cand"
    fi
}

cabecalho()
{
    if [ "$FORMATO" = "--markdown" ]; then
        echo '| Componente | Instalado | Candidato no repositório |'
        echo '| --- | --- | --- |'
    else
        printf 'Componente\tInstalado\tCandidato no repositório\n'
    fi
}

# Só atualiza o índice quando a tabela vai ser impressa (o candidato depende dele).
atualizar_indice

echo "# medir_versoes: $(. /etc/os-release && echo "$PRETTY_NAME") ($(famiglia_distro))"
cabecalho
linha "Sistema" "$(. /etc/os-release && echo "$PRETTY_NAME")" "-"
linha "glibc" "$(ldd --version 2>&1 | sed -n '1p')" "-"
linha "GCC (g++)" "$(g++ -dumpfullversion 2>/dev/null || echo ausente)" "-"
linha "CMake" "$(primeira_linha cmake --version)" "-"
linha "Ninja" "$(primeira_linha ninja --version)" "-"
linha "Qt 6.12 (volume)" "$(qt_instalado)" "-"
linha "OpenSSL" "$(primeira_linha openssl version)" "$(candidato openssl)"
linha "Argon2id no OpenSSL (list -kdf-algorithms)" "$(argon2id_openssl)" "-"
linha "libsodium" "$(modulo_instalado libsodium)" "$(candidato libsodium)"
linha "xmlsec1" "$(modulo_instalado xmlsec1)" "$(candidato xmlsec1)"
linha "libxml2" "$(modulo_instalado libxml-2.0)" "$(candidato libxml2)"
linha "zlib" "$(modulo_instalado zlib)" "$(candidato zlib)"
linha "Qt 6 no repositório da distro" "-" "$(candidato qt6)"
