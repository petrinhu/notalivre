#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# E1 da NL-2.10e (DA-2.71): sondagem TLS SÓ DE LEITURA do host da Sefin de Produção Restrita. Usa só openssl s_client, sem
# certificado de cliente, sem POST e sem corpo. A única requisição HTTP é um GET sem corpo, no caminho de H8. Nenhuma chave
# é lida nem guardada. Roda dentro de contêiner com rede (imagem fedora44). Não decide nada: só registra o que o servidor faz.
#
# Uso: NL_SONDA_SEFIN_CONFIRMO=1 sondar_sefin_tls.sh [HOST]
# Saída: texto no stdout, com a ordem das medidas numeradas. Sai com 1 só se o servidor não responder ao handshake.
# Trava (revisão de segurança da NL-2.10e): conecta ao host real só com NL_SONDA_SEFIN_CONFIRMO=1 explícito, e recusa
# (rc 2) se a variável CI estiver definida. Sem a variável, nada é conectado.
set -u

if [ -n "${CI:-}" ]; then
    echo "RECUSADA: a sonda não roda em CI (variável CI definida)"
    exit 2
fi
if [ "${NL_SONDA_SEFIN_CONFIRMO:-}" != 1 ]; then
    echo "RECUSADA: falta NL_SONDA_SEFIN_CONFIRMO=1 (confirmação explícita para conectar à Sefin real)"
    exit 2
fi

HOST="${1:-sefin.producaorestrita.nfse.gov.br}"
PORTA=443
# Base de temporários: NL_TMP_BASE, padrão /var/tmp (L-11). TMPDIR segue a mesma base.
NL_TMP_BASE="${NL_TMP_BASE:-/var/tmp}"
export TMPDIR="$NL_TMP_BASE"
# Repositório de CAs do sistema da imagem (Fedora, p11-kit). É o que o contêiner confia; nenhum certificado é baixado.
CA_SISTEMA=/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem
TMPD="$(mktemp -d "$NL_TMP_BASE/sonda-sefin.XXXXXX")" || { echo "FALHA: não criei a pasta temporária"; exit 1; }

conectar() {
    # Sem entrada de dados: o handshake completa e a conexão fecha. $1 é o arquivo de saída; o resto são opções do s_client.
    local saida="$1"; shift
    openssl s_client -connect "$HOST:$PORTA" -servername "$HOST" "$@" </dev/null > "$saida" 2>&1
}

echo "== 0. Ambiente"
echo "data_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "host=$HOST porta=$PORTA"
echo "openssl=$(openssl version)"
echo "ca_sistema=$CA_SISTEMA cas_no_arquivo=$(grep -a -c 'BEGIN CERTIFICATE' "$CA_SISTEMA" 2>/dev/null || echo ausente)"

echo "== 1. Cadeia enviada pelo servidor (-showcerts, ALPN h2,http/1.1, sem certificado de cliente)"
conectar "$TMPD/cadeia.txt" -showcerts -alpn h2,http/1.1; RC_CADEIA=$?
echo "rc_s_client=$RC_CADEIA"
grep -a -E '^(Protocol|Cipher|ALPN protocol|Verify return code)' "$TMPD/cadeia.txt" | sed 's/^/  /'
if ! grep -a -q 'BEGIN CERTIFICATE' "$TMPD/cadeia.txt"; then
    echo "FALHA: o servidor não respondeu ao handshake (nenhum certificado recebido)"
    grep -a -E 'error|Error' "$TMPD/cadeia.txt" | head -5 | sed 's/^/  /'
    exit 1
fi
awk -v d="$TMPD" '/-----BEGIN CERTIFICATE-----/{n++; out=sprintf("%s/cert-%02d.pem", d, n)} /-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/{print > out}' "$TMPD/cadeia.txt"
N_CERT=$(ls "$TMPD"/cert-*.pem 2>/dev/null | wc -l | tr -d ' ')
echo "certificados_enviados=$N_CERT (folha primeiro)"
for arq in "$TMPD"/cert-*.pem; do
    echo "  $(basename "$arq"):"
    openssl x509 -in "$arq" -noout -subject -issuer -enddate -fingerprint -sha256 2>&1 | sed 's/^/    /'
done
if grep -a -l -i 'SSLv1' "$TMPD"/cert-*.pem >/dev/null 2>&1; then
    echo "intermediario_serpro_sslv1_enviado=sim"
else
    echo "intermediario_serpro_sslv1_enviado=nao"
fi
for arq in "$TMPD"/cert-*.pem; do
    openssl x509 -in "$arq" -noout -subject 2>/dev/null | grep -a -q 'SSLv1' && echo "  (presente: $(openssl x509 -in "$arq" -noout -subject 2>/dev/null))"
done

echo "== 2. Verificação contra o repositório de CAs do contêiner (sem -k, sem VerifyNone)"
conectar "$TMPD/verif-sistema.txt" -CAfile "$CA_SISTEMA" -verify_return_error; RC_V1=$?
echo "rc_s_client_com_CAfile_sistema=$RC_V1"
grep -a -E '(Verify return code|verify error)' "$TMPD/verif-sistema.txt" | sed 's/^/  /'
conectar "$TMPD/verif-padrao.txt" -verify_return_error; RC_V2=$?
echo "rc_s_client_sem_CAfile=$RC_V2 (caminho padrão do openssl: $(openssl version -d 2>/dev/null))"
grep -a -E '(Verify return code|verify error)' "$TMPD/verif-padrao.txt" | sed 's/^/  /'
echo "verificacao_com_raiz_v10=nao_executada (L1 pendente: nada baixado nesta sonda)"

echo "== 3. ALPN escolhido pelo servidor"
conectar "$TMPD/alpn-h2.txt" -alpn h2,http/1.1
echo "  oferta h2,http/1.1 -> $(grep -a -E '^ALPN protocol' "$TMPD/alpn-h2.txt" || echo 'sem linha ALPN')"
conectar "$TMPD/alpn-h1.txt" -alpn http/1.1
echo "  oferta http/1.1    -> $(grep -a -E '^ALPN protocol' "$TMPD/alpn-h1.txt" || echo 'sem linha ALPN')"

echo "== 4. Versões de TLS aceitas (sem certificado de cliente)"
for versao in -tls1_2 -tls1_3; do
    conectar "$TMPD/versao$versao.txt" "$versao" -alpn http/1.1; RC_VER=$?
    echo "  $versao rc=$RC_VER: $(grep -a -E '^Protocol' "$TMPD/versao$versao.txt" | tr -s ' ' || true) $(grep -a -E '^Cipher' "$TMPD/versao$versao.txt" | tr -s ' ' || true)"
    [ "$RC_VER" -eq 0 ] || grep -a -i -E 'error|alert|reset|closed' "$TMPD/versao$versao.txt" | head -3 | sed 's/^/    erro: /'
done
conectar "$TMPD/versao-tls11.txt" -tls1_1 -cipher 'DEFAULT:@SECLEVEL=0'; RC_11=$?
echo "  -tls1_1 (SECLEVEL=0) rc=$RC_11: $(grep -a -E '^(Protocol|Cipher|[0-9]+:error)' "$TMPD/versao-tls11.txt" | head -2 | tr -s ' ' || true)"

echo "== 5. Pedido de certificado de cliente (CertificateRequest) no handshake inicial"
for arq in "$TMPD/cadeia.txt" "$TMPD/versao-tls1_2.txt" "$TMPD/versao-tls1_3.txt"; do
    echo "  arquivo $(basename "$arq"):"
    if grep -a -q 'Acceptable client certificate CA names' "$arq"; then
        echo "    CertificateRequest_no_handshake=sim"
        sed -n '/Acceptable client certificate CA names/,/^---/p' "$arq" | sed 's/^/    /' | head -12
    elif grep -a -q 'No client certificate CA names sent' "$arq"; then
        echo "    CertificateRequest_no_handshake=nao (servidor declara: nenhum nome de CA pedido)"
    else
        echo "    CertificateRequest_no_handshake=indeterminado (nenhuma das duas linhas)"
    fi
done

echo "== 6. H8: GET sem corpo para ver pedido tardio de certificado ou renegociação"
echo "  6a. TLS 1.2, ALPN http/1.1:"
printf 'GET /SefinNacional/nfse HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n' "$HOST" \
    | timeout 40 openssl s_client -connect "$HOST:$PORTA" -servername "$HOST" -tls1_2 -alpn http/1.1 -ign_eof > "$TMPD/h8-tls12.txt" 2>&1
echo "  status=$(grep -a -m1 -E '^HTTP/' "$TMPD/h8-tls12.txt" || echo 'sem linha HTTP')"
echo "  marcas: RENEGOTIATING=$(grep -a -c 'RENEGOTIATING' "$TMPD/h8-tls12.txt") HelloRequest=$(grep -a -c -i 'hello request' "$TMPD/h8-tls12.txt") CertificateRequest=$(grep -a -c -i 'certificate request' "$TMPD/h8-tls12.txt")"
echo "  6b. TLS 1.3, ALPN http/1.1, cliente oferece autenticação pós-handshake (-enable_pha):"
printf 'GET /SefinNacional/nfse HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n' "$HOST" \
    | timeout 40 openssl s_client -connect "$HOST:$PORTA" -servername "$HOST" -tls1_3 -enable_pha -alpn http/1.1 -ign_eof > "$TMPD/h8-tls13.txt" 2>&1
echo "  status=$(grep -a -m1 -E '^HTTP/' "$TMPD/h8-tls13.txt" || echo 'sem linha HTTP')"
echo "  marcas: post_handshake=$(grep -a -c -i 'post-handshake\|post handshake' "$TMPD/h8-tls13.txt") CertificateRequest=$(grep -a -c -i 'certificate request' "$TMPD/h8-tls13.txt")"

echo "== 7. H6: curl não rodado nesta sonda (depende de L1, a raiz v10); ALPN da seção 3 é a única medida de H6"
echo "== fim: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
