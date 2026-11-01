#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# E2 da NL-2.10e (DA-2.70): conta as CAs que o runtime org.kde.Platform 6.11 traz dentro do sandbox do Flatpak e registra
# quais raízes ICP-Brasil (v5 a v13, v10 incluída) e a GlobalSign Root R46 estão presentes. Só lê o bundle de CAs (certificados
# públicos). Nenhum segredo, nenhum host. Roda no job smoke-host do CI, depois da instalação do bundle. Nada roda no host do líder.
#
# Uso: contar_cas_runtime.sh <app-id>
# Saída: uma linha por medida. Sai com 1 se o bundle não for achado ou se a contagem der zero (piso, L-36).
set -u

APP="${1:?uso: contar_cas_runtime.sh <app-id>}"
FLATPAK="${NL_FLATPAK_BIN:-flatpak}"
TMP="${RUNNER_TEMP:-${TMPDIR:-/var/tmp}}/cas-runtime"
mkdir -p "$TMP/subjects" || { echo "FALHA: não consegui criar $TMP"; exit 1; }
FALHAS=0

echo "MEDIDA: flatpak_versao_do_runner=$("$FLATPAK" --version 2>&1 | head -1)"

# Candidatos de bundle de CAs dentro do sandbox, na ordem em que o p11-kit e o ca-certificates costumam pôr o arquivo.
CANDIDATOS="/etc/ssl/certs/ca-certificates.crt /etc/pki/tls/certs/ca-bundle.crt /etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"
ACHADO=$("$FLATPAK" run --user --command=sh "$APP" -c "for f in $CANDIDATOS; do [ -f \"\$f\" ] && { echo \"\$f\"; break; }; done" 2>/dev/null)
echo "MEDIDA: bundle_de_cas_no_sandbox=${ACHADO:-nao_achado}"
echo "MEDIDA: entradas_em_etc_ssl_certs=$("$FLATPAK" run --user --command=sh "$APP" -c 'ls /etc/ssl/certs 2>/dev/null | wc -l' 2>/dev/null | tr -d ' ')"
echo "MEDIDA: entradas_em_etc_pki=$("$FLATPAK" run --user --command=sh "$APP" -c 'ls /etc/pki 2>/dev/null | tr "\n" " "' 2>/dev/null)"

if [ -z "$ACHADO" ]; then
    echo "FALHA: nenhum bundle de CAs achado dentro do sandbox (candidatos: $CANDIDATOS)"
    echo "RESULTADO: cas_runtime reprovado (1)"
    exit 1
fi

# Cópia do bundle de CAs para o runner: só leitura, por flatpak run --command=cat. A cópia fica no RUNNER_TEMP.
BUNDLE_CAS="$TMP/cas-runtime.pem"
"$FLATPAK" run --user --command=cat "$APP" "$ACHADO" > "$BUNDLE_CAS" 2>"$TMP/cat.err" || { echo "FALHA: flatpak run --command=cat não copiou o bundle"; FALHAS=$((FALHAS+1)); }

N_CAS=$(grep -c 'BEGIN CERTIFICATE' "$BUNDLE_CAS" 2>/dev/null || true)
[ -n "$N_CAS" ] || N_CAS=0
echo "MEDIDA: cas_no_runtime=$N_CAS"

# Assunto de cada certificado: lido com openssl do runner. Só o assunto sai no log.
awk -v d="$TMP/subjects" '/-----BEGIN CERTIFICATE-----/{n++; out=sprintf("%s/ca-%04d.pem", d, n)} /-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/{print > out}' "$BUNDLE_CAS"
for arq in "$TMP"/subjects/ca-*.pem; do
    [ -f "$arq" ] && openssl x509 -in "$arq" -noout -subject 2>/dev/null
done > "$TMP/assuntos.txt"
N_LIDOS=$(wc -l < "$TMP/assuntos.txt" | tr -d ' ')
echo "MEDIDA: cas_com_assunto_lido=$N_LIDOS de $N_CAS"
[ "$N_LIDOS" -eq "$N_CAS" ] || { echo "FALHA: nem todos os certificados do bundle foram lidos pelo openssl"; FALHAS=$((FALHAS+1)); }

# Raízes ICP-Brasil: v5 a v13 (v10 incluída). Presença por número da versão no assunto.
ICP_PRESENTES=""
ICP_AUSENTES=""
for v in 5 6 7 8 9 10 11 12 13; do
    if grep -E -q "Raiz Brasileira v${v}([^0-9]|\$)" "$TMP/assuntos.txt"; then
        ICP_PRESENTES="$ICP_PRESENTES v$v"
    else
        ICP_AUSENTES="$ICP_AUSENTES v$v"
    fi
done
echo "MEDIDA: raizes_icp_presentes=[${ICP_PRESENTES# }] ausentes=[${ICP_AUSENTES# }]"

# GlobalSign Root R46: a raiz que assina a cadeia da Sefin de Produção Restrita medida na E1 (AC SERPRO AR46 OV TLS CA 2025).
if grep -q 'GlobalSign Root R46' "$TMP/assuntos.txt"; then
    echo "MEDIDA: globalsign_root_r46=presente"
else
    echo "MEDIDA: globalsign_root_r46=ausente"
fi

if [ "$N_CAS" -eq 0 ]; then
    echo "FALHA: contagem de CAs zero (varredura quebrada, não bundle vazio; L-36)"
    FALHAS=$((FALHAS+1))
fi

if [ "$FALHAS" -eq 0 ]; then
    echo "RESULTADO: cas_runtime ok"
else
    echo "RESULTADO: cas_runtime reprovado ($FALHAS)"
fi
[ "$FALHAS" -eq 0 ]
