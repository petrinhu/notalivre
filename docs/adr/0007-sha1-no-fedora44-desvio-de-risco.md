# ADR 0007: SHA-1 no arnês de teste do fedora44 (desvio de risco com controles compensatórios)

**Status:** PROPOSTO. Executor: NL-2.6. Revisão pendente do security-engineer (NL-2.6, DA-2.10). Aceite pelo líder na ratificação das decisões autônomas da Onda 2.
**Data:** 27/10/2026.
**Origem:** plano, NL-2.6 (Desvio de risco, DA-2.10) e NL-2.3 (assinador). Medição: `/var/tmp/notalivre-smoke-NL-2.6/` (matriz de quatro imagens, rc medido).
**Relacionadas:** ADR-0003 (envelope e hierarquia de chaves); DA-2.1 e DA-2.2 (assinatura com OpenSSL, rsa-sha1 como contrato de interoperabilidade).

## Natureza deste registro

Isto é um **desvio de risco com controles compensatórios** (L-77). Não é conformidade: o ambiente de teste do fedora44 passa a
assinar com um algoritmo que a política de cripto da própria distro recusa. Enquanto os controles abaixo estiverem ativos e
verificados, o desvio fica limitado ao arnês de teste. Sem verificação periódica, o desvio se torna "normal" sem ninguém notar.

## Contexto

- FATO medido (NL-2.6, rc por sign e verify): na imagem fedora44 (xmlsec1 1.2.41, OpenSSL 3.5.9, crypto-policy DEFAULT), `xmlsec1 --sign`
  com rsa-sha1 sai com `invalid digest`, e `openssl dgst -sha1 -sign` também. O verify de um arquivo SHA-1 assinado em outra distro dá rc 1.
- FATO medido: com `OPENSSL_ENABLE_SHA1_SIGNATURES=1` no contêiner fedora44, `openssl dgst` sign e verify, e `xmlsec1` sign e verify
  com rsa-sha1, dão rc 0. Sem a variável, o verify dá rc 1.
- FATO medido: Ubuntu 26.04, Manjaro e Mint 22.3 assinam e verificam rsa-sha1 sem variável.
- INFERÊNCIA: a variável é um mecanismo de patch de crypto-policy do Fedora. A origem exata não foi lida no código-fonte.
- FATO: o algoritmo rsa-sha1 é o da DA-2.2, escolha do CTO a ratificar (contrato de interoperabilidade, não escolha de segurança do app).
- Risco de produto: a NL-2.3 assina com OpenSSL. Se o runtime Flatpak KDE 6.11 recusar SHA-1, o app não emite. A medição do runtime
  é do passo 0 da NL-2.3 e não está feita neste registro.

## Decisão (DA-2.10, opção (a) do CTO)

1. O algoritmo continua rsa-sha1 (DA-2.2).
2. A variável `OPENSSL_ENABLE_SHA1_SIGNATURES=1` entra **somente** no ambiente do oráculo de teste (`tools/servidor_falso/oraculo.py`),
   e só quando o sistema é fedora 44. Nenhum outro lugar do repositório cita a variável.
3. O app nunca lê nem define a variável. Ele usa `OSSL_LIB_CTX` próprio (DA-2.10, NL-2.3).

## Controles compensatórios e como cada um se verifica

| # | Controle | Onde vive | Como se verifica | Estado nesta fatia |
|---|---|---|---|---|
| 1 | Assinatura SHA-1 no SDK KDE 6.11 **sem** a variável: o teste `assinaSha1NoSdkKdeSemVariavel` roda dentro do build do flatpak-builder | NL-2.3 (job Flatpak), citado aqui | O log do job Flatpak traz o teste verde e imprime que a variável está ausente. Roda a cada push. | Pendente: é entrega da NL-2.3. Esta fatia só cita o controle. |
| 2 | Portão de varredura: reprova a variável em qualquer arquivo fora do arnês do fedora44 | `tools/portoes/portao_sha1_env.py`, com testes em `tests/tools/portoes/` | (a) ctest: `portoes.portaoSha1ArvoreRealLimpa` varre a árvore do build a cada build por SHA, com contagem impressa; (b) `portoes.portaoSha1SabotagemReprova` prova que o portão reprova um vazamento, e `portoes.portaoSha1PisoVazioReprova` prova que varredura vazia reprova; (c) a sabotagem de estreia foi vista vermelha antes da implementação (commit de vermelho da NL-2.6). | Ativo no build por SHA (ctest) e rodado no host sobre o repositório inteiro (inclui `flatpak/` e `.github/`, que o git archive do build não leva). |

**Lacuna declarada (L-77):** o portão dentro do build cobre `src/`, `apps/`, `tests/` e `tools/`, porque o git archive não leva `flatpak/`
nem `.github/`. A cobertura desses dois só existe quando o host roda o portão, e no CI só quando o job de higiene do `ci.yml` o chamar.
Esse job é do devops-sre e fica como pendência. Até lá, o controle 2 cobre a árvore de build, não o repositório inteiro.

## Consequências

- Positivo: o desvio fica confinado ao arnês de teste; o app e a emissão real não herdam a variável.
- Negativo: o fedora44 do CI prova a assinatura pelo arnês, não pela política do sistema. Um usuário Fedora com a mesma política
  não tem o mesmo comportamento no oráculo. A NL-2.3 pode enfrentar o mesmo muro no runtime, por isso o passo 0 dela existe.
- Revisão: este ADR é reavaliado quando (a) a NL-2.10 confirmar ou refutar a rsa-sha1 na Sefin, (b) o passo 0 da NL-2.3 medir o runtime
  KDE 6.11, ou (c) a distro mudar a política. Se a NL-2.3 falhar no runtime, o desvio sobe ao CTO antes de qualquer release.
