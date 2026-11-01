#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de camadas (NL-0.3, matriz 1.1, L-33 e L-36).
#
# Cada controle roda o portão numa cópia (ou na árvore real) e exige rc EXATO e a linha
# `encontrados=N analisados=M violacoes=K`, além de uma regex sobre a saída.
# Grupos: a-d (base, Front->Back, varredura vazia); e-g (include relativo, resolvido por -I
# ou DESCONHECIDO); h-j (extensões em maiúscula e .ipp/.tpp); k-l (poda só na raiz);
# B/M/F (denylist de terceiros por camada, coluna Proibido da matriz 1.1, forma de módulo e de
# classe); N (negativos: o que é permitido não reprova).
#
# Uso: autoteste_camadas.sh [RAIZ_REAL]
#   RAIZ_REAL: árvore a verificar (padrão: raiz do repositório, dois níveis acima deste script).
#   PORTAO (variável de ambiente): troca o portão testado. Só para sabotagem e para o vermelho
#   com portão real anterior, sempre sobre cópia fora da árvore (L-27).
# Cópias de trabalho em ${TMPDIR:-/var/tmp}, apagadas ao fim (L-53: só o próprio scratch).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RAIZ_REAL="${1:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
PORTAO="${PORTAO:-$SCRIPT_DIR/camadas.py}"

TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/nl-autoteste-camadas.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT

APROVADOS=0
TOTAL=0

# Roda o portão numa única invocação. Saída em arquivo (sem pipe, L-45); devolve o rc.
rodar_portao()
{
    local raiz="$1" saida="$2"
    python3 -I "$PORTAO" "$raiz" > "$saida" 2>&1
    echo $?
}

# Confere um controle: rc exato, contagem presente, regex ERE casando a saída.
controle()
{
    local nome="$1" rc_esperado="$2" raiz="$3" regex="$4"
    local saida="$TMP/saida-$nome.txt" rc ok=1

    TOTAL=$((TOTAL + 1))
    rc="$(rodar_portao "$raiz" "$saida")"
    echo "controle $nome: rc=$rc (esperado $rc_esperado)"
    sed 's/^/    | /' "$saida"

    [ "$rc" = "$rc_esperado" ] || ok=0
    grep -Eq '^encontrados=[0-9]+ analisados=[0-9]+ violacoes=[0-9]+$' "$saida" || ok=0
    grep -Eq -- "$regex" "$saida" || ok=0

    if [ "$ok" = 1 ]; then
        echo "controle $nome: VERDE"
        APROVADOS=$((APROVADOS + 1))
    else
        echo "controle $nome: VERMELHO"
    fi
}

# Cópia de src/ e apps/ da árvore real dentro do scratch. A árvore real não é tocada.
copiar_arvore()
{
    local nome="$1" destino="$TMP/$1"
    mkdir -p "$destino" || { echo "ERRO: não criou $destino"; exit 1; }
    cp -a "$RAIZ_REAL/src" "$RAIZ_REAL/apps" "$destino/" || { echo "ERRO: não copiou para $destino"; exit 1; }
}

# Cópia + uma linha acrescentada ao arquivo relativo ARQ (cria o arquivo e o diretório se faltarem).
controle_inclusao()
{
    local nome="$1" arq="$2" rc_esperado="$3" linha="$4" regex="$5"
    copiar_arvore "$nome"
    mkdir -p "$(dirname "$TMP/$nome/$arq")" || { echo "ERRO: diretório de $arq"; exit 1; }
    printf '%s\n' "$linha" >> "$TMP/$nome/$arq" || { echo "ERRO: sabotagem $nome"; exit 1; }
    controle "$nome" "$rc_esperado" "$TMP/$nome" "$regex"
}

# Denylist: include de terceiro proibido na camada do arquivo. Regex de terceiro no texto da saída.
viol_back='^VIOLACAO src/back/camada\.cpp:[0-9]+: .*terceiro proibido'
viol_mid='^VIOLACAO src/mid/camada\.cpp:[0-9]+: .*terceiro proibido'
viol_cli='^VIOLACAO apps/cli/main\.cpp:[0-9]+: .*terceiro proibido'
viol_gui='^VIOLACAO apps/gui/main\.cpp:[0-9]+: .*terceiro proibido'

echo "portao testado: $PORTAO"
echo "arvore real: $RAIZ_REAL"
echo "scratch: $TMP"

# --- Grupo base ---
controle a 0 "$RAIZ_REAL" '^encontrados=[1-9][0-9]* analisados=[0-9]+ violacoes=0$'
controle_inclusao b src/back/camada.cpp 1 '#include <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
mkdir -p "$TMP/c/src" "$TMP/c/apps"
controle c 1 "$TMP/c" '^encontrados=0 analisados=0 '
controle_inclusao d apps/cli/main.cpp 1 '#include <nl/back/camada.h>' \
    '^VIOLACAO apps/cli/main\.cpp:[0-9]+: front:cli inclui nl/back/camada\.h'

# --- Include relativo: resolvido pelo arquivo ou pelo -I da camada; senão DESCONHECIDO ---
controle_inclusao e src/back/camada.cpp 1 '#include "../mid/include/nl/mid/camada.h"' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: .*proibido pela matriz 1\.1'
controle_inclusao f src/back/camada.cpp 1 '#include <../../mid/include/nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: .*proibido pela matriz 1\.1'
controle_inclusao g apps/cli/main.cpp 1 '#include "../../../nao_existe/x.h"' \
    '^VIOLACAO apps/cli/main\.cpp:[0-9]+: .*nao resolvido'
controle_inclusao h2 apps/cli/main.cpp 1 '#include "../../src/back/include/nl/back/camada.h"' \
    '^VIOLACAO apps/cli/main\.cpp:[0-9]+: .*proibido pela matriz 1\.1'

# --- Extensões: maiúsculas e .ipp/.tpp entram na varredura ---
controle_inclusao x1 src/back/y.H 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/y\.H:1: '
controle_inclusao x2 src/back/y.ipp 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/y\.ipp:1: '
controle_inclusao x3 src/back/y.tpp 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/y\.tpp:1: '

# --- Poda: nada é podado dentro de src/ ou apps/ por nome ---
controle_inclusao p1 src/back/build/x.cpp 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/build/x\.cpp:1: '

# --- Poda só na raiz: worktrees e build-* na raiz do repositório não são varridos (L-55) ---
copiar_arvore p2
mkdir -p "$TMP/p2/worktrees/ramo/src/back" && printf '#include <nl/mid/camada.h>\n' > "$TMP/p2/worktrees/ramo/src/back/x.cpp"
controle p2 0 "$TMP/p2" 'violacoes=0$'
copiar_arvore p3
mkdir -p "$TMP/p3/build-x/src/back" && printf '#include <nl/mid/camada.h>\n' > "$TMP/p3/build-x/src/back/x.cpp"
controle p3 0 "$TMP/p3" 'violacoes=0$'

# --- Diretório ilegível: a varredura não passa em silêncio (INCOMPLETA, exit 1) ---
# Requer usuário sem poder de leitura irrestrito; como root o chmod não bloqueia e o controle reprova.
copiar_arvore o1
chmod 000 "$TMP/o1/src/back/include" || { echo "ERRO: chmod o1"; exit 1; }
controle o1 1 "$TMP/o1" '^ERRO VARREDURA INCOMPLETA: '
chmod 755 "$TMP/o1/src/back/include"

# --- Burlas do pré-processador (revisão 2): comentário, macro, digrafo, _next, emenda, string ---
controle_inclusao n1 src/back/camada.cpp 1 '/**/#include <QtSql/QSqlDatabase>' "$viol_back"
copiar_arvore n2
printf '#define NL_MID <nl/mid/camada.h>\n#include NL_MID\n' >> "$TMP/n2/src/back/camada.cpp"
controle n2 1 "$TMP/n2" '^VIOLACAO src/back/camada\.cpp:[0-9]+: .*nao literal'
controle_inclusao n3 src/back/camada.cpp 1 '%:include <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
controle_inclusao n3b src/back/camada.cpp 1 '%:   include <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
controle_inclusao n4 src/back/camada.cpp 1 '#include_next <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
controle_inclusao n5 src/back/camada.cpp 0 '#include <QtCore/QString> /* <QtSql/QSqlDatabase> */' 'violacoes=0$'
controle_inclusao n6 src/back/camada.cpp 1 'const char *s = "/*";
#include <QtSql/QSqlDatabase>' "$viol_back"
copiar_arvore n7
printf '#inc\\\nlude <nl/mid/camada.h>\n' >> "$TMP/n7/src/back/camada.cpp"
controle n7 1 "$TMP/n7" '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'

# --- Denylist Back (Proibido: rede, SQL, cripto, XML, arquivo, Qt GUI) ---
controle_inclusao B01 src/back/camada.cpp 1 '#include <QtQuick/QQuickItem>' "$viol_back"
controle_inclusao B02 src/back/camada.cpp 1 '#include <QtQml/QQmlEngine>' "$viol_back"
controle_inclusao B03 src/back/camada.cpp 1 '#include <QtGui/QImage>' "$viol_back"
controle_inclusao B04 src/back/camada.cpp 1 '#include <QtWidgets/QWidget>' "$viol_back"
controle_inclusao B05 src/back/camada.cpp 1 '#include <QtSql/QSqlDatabase>' "$viol_back"
controle_inclusao B06 src/back/camada.cpp 1 '#include <QtNetwork/QTcpSocket>' "$viol_back"
controle_inclusao B07 src/back/camada.cpp 1 '#include <QtXml/QDomDocument>' "$viol_back"
controle_inclusao B08 src/back/camada.cpp 1 '#include <openssl/evp.h>' "$viol_back"
controle_inclusao B09 src/back/camada.cpp 1 '#include <sodium.h>' "$viol_back"
controle_inclusao B10 src/back/camada.cpp 1 '#include <libxml/parser.h>' "$viol_back"
controle_inclusao B11 src/back/camada.cpp 1 '#include <xmlsec/xmlsec.h>' "$viol_back"
controle_inclusao B12 src/back/camada.cpp 1 '#include <fstream>' "$viol_back"
controle_inclusao B13 src/back/camada.cpp 1 '#include <filesystem>' "$viol_back"
controle_inclusao B14 src/back/camada.cpp 1 '#include <QFile>' "$viol_back"
controle_inclusao B15 src/back/camada.cpp 1 '#include <QDir>' "$viol_back"
controle_inclusao B16 src/back/camada.cpp 1 '#include <QSqlDatabase>' "$viol_back"
controle_inclusao B17 src/back/camada.cpp 1 '#include <QNetworkAccessManager>' "$viol_back"
controle_inclusao B18 src/back/camada.cpp 1 '#include <QQuickItem>' "$viol_back"
controle_inclusao B19 src/back/camada.cpp 1 '#include <QDomDocument>' "$viol_back"

# --- Denylist Mid (Proibido: QML, widget) ---
controle_inclusao M01 src/mid/camada.cpp 1 '#include <QtQuick/QQuickItem>' "$viol_mid"
controle_inclusao M02 src/mid/camada.cpp 1 '#include <QtQml/QQmlEngine>' "$viol_mid"
controle_inclusao M03 src/mid/camada.cpp 1 '#include <QtWidgets/QWidget>' "$viol_mid"
controle_inclusao M04 src/mid/camada.cpp 1 '#include <QWidget>' "$viol_mid"

# --- Denylist Front (Proibido: SQL, cripto, HTTP, include de back/foundation exceto base) ---
controle_inclusao F01 apps/cli/main.cpp 1 '#include <QtSql/QSqlDatabase>' "$viol_cli"
controle_inclusao F02 apps/cli/main.cpp 1 '#include <QtNetwork/QTcpSocket>' "$viol_cli"
controle_inclusao F03 apps/cli/main.cpp 1 '#include <openssl/evp.h>' "$viol_cli"
controle_inclusao F04 apps/cli/main.cpp 1 '#include <sodium.h>' "$viol_cli"
controle_inclusao F05 apps/cli/main.cpp 1 '#include <libxml/parser.h>' "$viol_cli"
controle_inclusao F06 apps/cli/main.cpp 1 '#include <xmlsec/xmlsec.h>' "$viol_cli"
controle_inclusao F07 apps/gui/main.cpp 1 '#include <QNetworkAccessManager>' "$viol_gui"

# --- Foundation de motor (nl/foundation/log) não é permitida ao Front; só nl/base/ (DA-2.55, 85b24e4) ---
viol_foundation_cli='^VIOLACAO apps/cli/main\.cpp:[0-9]+: front:cli inclui nl/foundation/log/log\.h \(foundation\) proibido pela matriz 1\.1'
viol_foundation_gui='^VIOLACAO apps/gui/main\.cpp:[0-9]+: front:gui inclui nl/foundation/log/log\.h \(foundation\) proibido pela matriz 1\.1'
controle_inclusao G01 apps/cli/main.cpp 1 '#include <nl/foundation/log/log.h>' "$viol_foundation_cli"
controle_inclusao G02 apps/gui/main.cpp 1 '#include <nl/foundation/log/log.h>' "$viol_foundation_gui"

# --- Negativos: o que a matriz permite não reprova ---
controle_inclusao N01 apps/gui/main.cpp 0 '#include <QtQuick/QQuickView>' 'violacoes=0$'
controle_inclusao N02 src/back/camada.cpp 0 '#include <QtCore/QString>' 'violacoes=0$'
controle_inclusao N03 src/foundation/base/camada.cpp 0 '#include <openssl/evp.h>' 'violacoes=0$'
controle_inclusao N04 apps/cli/main.cpp 0 '#include <nl/mid/composicao/fachada.h>' 'violacoes=0$'
controle_inclusao N05 apps/cli/main.cpp 0 '#include <nl/base/camada.h>' 'violacoes=0$'

# --- Processo na GUI (NL-2.38, item 2; DA-2.83): a GUI usa o núcleo, nunca a CLI ---
# Em apps/gui e em src: #include de QProcess, identificador QProcess, popen, system(, a família exec,
# posix_spawn, fork, vfork e o texto notalivre-cli. Em apps/cli: só as chamadas de processo (o literal é o próprio nome).
# tests/ fica fora da varredura (os e2e executam a CLI). Piso: varredura não vazia em apps/gui e em src (L-36).
viol_proc_gui='^VIOLACAO apps/gui/main\.cpp:[0-9]+: chamada de processo '
viol_proc_src='^VIOLACAO src/mid/camada\.cpp:[0-9]+: chamada de processo '
controle_inclusao PR01 apps/gui/main.cpp 1 '#include <QProcess>' "${viol_proc_gui}QProcess"
controle_inclusao PR02 apps/gui/main.cpp 1 '#include <QtCore/QProcess>' "${viol_proc_gui}QProcess"
controle_inclusao PR03 apps/gui/main.cpp 1 'QProcess proc;' "${viol_proc_gui}QProcess"
controle_inclusao PR04 apps/gui/main.cpp 1 'FILE *f = popen("ls", "r");' "${viol_proc_gui}popen"
controle_inclusao PR05 src/mid/camada.cpp 1 'int r = system("ls");' "${viol_proc_src}system"
controle_inclusao PR06 apps/gui/main.cpp 1 'execvp("ls", nullptr);' "${viol_proc_gui}execvp"
controle_inclusao PR07 apps/gui/main.cpp 1 'posix_spawn(&pid, "x", nullptr, nullptr, nullptr, nullptr);' "${viol_proc_gui}posix_spawn"
controle_inclusao PR08 apps/gui/main.cpp 1 'pid_t p = fork();' "${viol_proc_gui}fork"
controle_inclusao PR09 apps/gui/main.cpp 1 'pid_t p = vfork();' "${viol_proc_gui}vfork"
controle_inclusao PR10 apps/gui/main.cpp 1 'const char *n = "notalivre-cli";' \
    '^VIOLACAO apps/gui/main\.cpp:[0-9]+: texto notalivre-cli proibido fora de apps/cli'
controle_inclusao PR11 src/mid/camada.cpp 0 'const char *n = "notalivre-cli";' 'violacoes=0$'
controle_inclusao PR12 apps/cli/main.cpp 0 'const char *n = "notalivre-cli";' 'violacoes=0$'
controle_inclusao PR13 apps/cli/main.cpp 1 'FILE *f = popen("ls", "r");' \
    '^VIOLACAO apps/cli/main\.cpp:[0-9]+: chamada de processo popen'
# DA-2.84 (decisão do CTO): o literal é proibido só em apps/gui (PR11 passa em src, PR10 reprova em apps/gui).
# A família exec ganha execve e execveat (ratificados na DA-2.84): cada um com caso visto vermelho.
controle_inclusao PR16 apps/gui/main.cpp 1 'execve("ls", nullptr, nullptr);' "${viol_proc_gui}execve"
controle_inclusao PR17 src/mid/camada.cpp 1 'execveat(0, "ls", nullptr, nullptr, 0);' "${viol_proc_src}execveat"
controle_inclusao PR18 apps/gui/main.cpp 0 'int myexecve(int x);' 'violacoes=0$'
# Controle limpo e falsos positivos: comentário, texto entre aspas, identificador que contém a palavra, namespace.
controle_inclusao PR14 apps/gui/main.cpp 0 '// system( fork( QProcess popen( notalivre-cli
const char *s = "system(";
int myfork(int x);
auto r = filesystem(x);
auto q = std::filesystem::path{"a"};' 'violacoes=0$'
# Piso: uma árvore sem apps/gui tem de reprovar (varredura vazia em apps/gui, L-36).
mkdir -p "$TMP/pz/apps/cli" && cp -a "$RAIZ_REAL/src" "$TMP/pz/" && cp -a "$RAIZ_REAL/apps/cli/." "$TMP/pz/apps/cli/" \
    || { echo "ERRO: árvore do piso"; exit 1; }
controle pz 1 "$TMP/pz" 'processo: varridos apps/gui=0 '
controle PR15 0 "$RAIZ_REAL" 'processo: varridos apps/gui=[1-9][0-9]* \(cpp=[0-9]+ qml=[0-9]+\) src=[1-9][0-9]*'

# --- QML (QA da NL-2.38, A1 e A2): o portão varre .qml de apps/gui; o literal e openUrlExternally reprovam ---
# Fonte da regra de QML: documentação do Qt 6.12 (objeto Qt, qml-qtqml-qt.html): Qt.openUrlExternally "attempts to open
# the specified target url in an external application" (repassa a QDesktopServices::openUrl). É o único método do Qt
# que inicia programa externo; createComponent, createQmlObject, quit e exit não iniciam nada.
viol_qml='^VIOLACAO apps/gui/qml/Sabot\.qml:[0-9]+: '
controle_inclusao Q01 apps/gui/qml/Sabot.qml 1 'property string nome: "notalivre-cli"' \
    "${viol_qml}texto notalivre-cli proibido fora de apps/cli"
controle_inclusao Q02 apps/gui/qml/Sabot.qml 1 'Qt.openUrlExternally("https://example.invalid")' \
    "${viol_qml}chamada de processo openUrlExternally proibida"
# Comentário QML com o literal: não é texto, não reprova (negativo).
controle_inclusao Q03 apps/gui/qml/Sabot.qml 0 '// notalivre-cli' 'violacoes=0$'
# Piso: uma GUI só em QML (sem .cpp) tem de contar os .qml e passar o piso (A2).
mkdir -p "$TMP/pq/apps/gui" && cp -a "$RAIZ_REAL/src" "$TMP/pq/" && cp -a "$RAIZ_REAL/apps/cli" "$TMP/pq/apps/" \
    && cp -a "$RAIZ_REAL/apps/gui/qml" "$TMP/pq/apps/gui/" || { echo "ERRO: árvore do piso QML"; exit 1; }
controle pq 0 "$TMP/pq" 'processo: varridos apps/gui=[1-9]'

# --- A4 (QA): dígito separador 1'000 seguido de comentário com chamada não pode esconder nem reprovar o comentário ---
controle_inclusao A4 apps/gui/main.cpp 0 "int n = 1'000; // system( fork(" 'violacoes=0$'

# --- A6 (QA): um caso por nome da família exec e dos outros padrões (guardas: cada um tem de reprovar onde o padrão pede) ---
controle_inclusao E01 apps/gui/main.cpp 1 'execl("ls", "ls", nullptr);' "${viol_proc_gui}execl proibida"
controle_inclusao E02 apps/gui/main.cpp 1 'execlp("ls", "ls", nullptr);' "${viol_proc_gui}execlp proibida"
controle_inclusao E03 apps/gui/main.cpp 1 'execle("ls", "ls", nullptr, nullptr);' "${viol_proc_gui}execle proibida"
controle_inclusao E04 apps/gui/main.cpp 1 'execv("ls", nullptr);' "${viol_proc_gui}execv proibida"
controle_inclusao E05 apps/gui/main.cpp 1 'execvpe("ls", nullptr, nullptr);' "${viol_proc_gui}execvpe proibida"
controle_inclusao E06 apps/gui/main.cpp 1 'fexecve(0, nullptr, nullptr);' "${viol_proc_gui}fexecve proibida"
controle_inclusao E07 apps/gui/main.cpp 1 'posix_spawnp(&pid, "ls", nullptr, nullptr, nullptr, nullptr);' \
    "${viol_proc_gui}posix_spawnp proibida"
controle_inclusao E08 apps/gui/main.cpp 1 'int r = system("ls");' "${viol_proc_gui}system proibida"
controle_inclusao E09 apps/gui/main.cpp 1 'execveat(0, "ls", nullptr, nullptr, 0);' "${viol_proc_gui}execveat proibida"
controle_inclusao E10 src/mid/camada.cpp 1 'FILE *f = popen("ls", "r");' "${viol_proc_src}popen proibida"
controle_inclusao E11 src/mid/camada.cpp 1 'execve("ls", nullptr, nullptr);' "${viol_proc_src}execve proibida"

echo "autoteste_camadas: $APROVADOS/$TOTAL controles verdes"
[ "$APROVADOS" = "$TOTAL" ] || exit 1
exit 0
