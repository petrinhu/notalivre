# ADR 0007: SHA-1 no arnês de teste do fedora44 (desvio de risco com controles compensatórios)

**Status:** ACEITO por Narciso (CISO) em 30/10/2026, decisão autônoma (L-15) a ratificar pelo líder, depois da re-revisão que aprovou a NL-2.3 (`/var/tmp/notalivre-logs/NL-2.3-rev2-revisao.log`, merge 20eb6bf). Condição de 27/10 cumprida: o controle 1 foi visto vermelho e verde no job Flatpak, e o controle 2 foi visto verde no job higiene do CI (evidência na tabela). Antes: PROPOSTO (DA-2.27, decisão do CTO), reprovado nas rodadas 1 e 2 do revisor.
**Data:** 27/10/2026 (revisto em 28/10/2026, rodadas 2 e 3 do revisor).
**Origem:** plano, NL-2.6 (Desvio de risco, DA-2.10) e NL-2.3 (assinador). Medição: `/var/tmp/notalivre-smoke-NL-2.6/` (matriz de quatro imagens, rc medido).
**Relacionadas:** ADR-0003 (envelope e hierarquia de chaves); DA-2.1 e DA-2.2 (assinatura com OpenSSL, rsa-sha1 como contrato de interoperabilidade).

## Natureza deste registro

Isto é um **desvio de risco com controles compensatórios** (L-77). Não é conformidade: o ambiente de teste do fedora44 passa a
assinar com um algoritmo que a política de cripto da própria distro recusa. O desvio fica confinado ao arnês de teste somente enquanto
os controles abaixo estiverem ativos e verificados. Sem verificação periódica, o desvio vira "normal" sem ninguém notar.

## Contexto

- FATO medido (NL-2.6, rc por sign e verify): na imagem fedora44 (xmlsec1 1.2.41, OpenSSL 3.5.9, crypto-policy DEFAULT), `xmlsec1 --sign`
  com rsa-sha1 sai com `invalid digest`, e `openssl dgst -sha1 -sign` também. O verify de um arquivo SHA-1 assinado em outra distro dá rc 1.
- FATO medido: com `OPENSSL_ENABLE_SHA1_SIGNATURES=1` no contêiner fedora44, `openssl dgst` sign e verify, e `xmlsec1` sign e verify
  com rsa-sha1, dão rc 0. Sem a variável, o verify dá rc 1.
- FATO medido: Ubuntu 26.04, Manjaro e Mint 22.3 assinam e verificam rsa-sha1 sem variável.
- FATO, lido em 29/10/2026 (DA-2.40): a variável vem do patch `0008-RH-Allow-disabling-of-SHA1-signatures.patch` do OpenSSL do Fedora (https://src.fedoraproject.org/rpms/openssl/raw/rawhide/f/0008-RH-Allow-disabling-of-SHA1-signatures.patch). Todo `OSSL_LIB_CTX` novo nasce com assinatura SHA-1 recusada, e a variável, se existir no ambiente, libera a assinatura SHA-1 em qualquer contexto do processo.
- FATO: o algoritmo rsa-sha1 é o da DA-2.2, escolha do CTO a ratificar (contrato de interoperabilidade, não escolha de segurança do app).
- Risco de produto: a NL-2.3 assina com OpenSSL. Se o runtime Flatpak KDE 6.11 recusar SHA-1, o app não emite. A medição do runtime
  é do passo 0 da NL-2.3 e não está feita neste registro.

## Decisão (DA-2.10, opção (a) do CTO)

1. O algoritmo continua rsa-sha1 (DA-2.2).
2. A variável `OPENSSL_ENABLE_SHA1_SIGNATURES=1` entra **somente** no ambiente do oráculo de teste (`tools/servidor_falso/oraculo.py`),
   e só quando o sistema é fedora 44. O `CMakeLists.txt` do arnês a põe no ambiente dos testes só nessa distro. Nenhum outro arquivo do
   repositório cita a variável (`tools/portoes/portao_sha1_env.py`).
3. O app não deve ler nem definir a variável. Ele usa `OSSL_LIB_CTX` próprio (DA-2.10, NL-2.3), que o isola da configuração do sistema (`openssl.cnf`), mas não da variável: no OpenSSL com o patch do Fedora, a variável vale também no contexto próprio (correção (c) da DA-2.40, provada pelos testes `variavelDoFedoraValeNoContextoProprio` e `contextoProprioIgnoraConfiguracaoGlobal`). O produto não é afetado, porque o runtime KDE 6.11 assina SHA-1 sem a variável (controle 1). No fedora44 esse contexto recusa rsa-sha1 sem a variável (medido na NL-2.3, passo 1, em docs/pesquisa/ci_medicao.md): por isso a suíte do assinador roda ali com rsa-sha256 (DA-2.38), e o desvio continua restrito ao arnês de teste. Esta afirmação só vale na medida em que
   o portão varrer `src/` e `apps/` e o job de higiene do CI rodar verde; hoje o portão roda no build por SHA (ctest) e no job higiene
   do `ci.yml` (passo `sha1_env`), visto verde no run 40984792478 (job higiene 123833170332).

## Controles compensatórios e como cada um se verifica (L-77)

Estado em 28/10/2026, medido no commit da rodada 3. "Verifica-se por" diz o comando ou o log que prova o controle; quem não tem prova ainda tem o estado pendente.

| # | Controle | Onde vive | Estado | Como se verifica (periodicamente) |
|---|---|---|---|---|
| 1 | Assinatura SHA-1 no SDK KDE 6.11 **sem** a variável: teste `assinaSha1NoSdkKdeSemVariavel` dentro do build do flatpak-builder (tests/unit/foundation/xmldsig, só com NL_FLATPAK_BUILD=ON) | NL-2.3 | **FECHADO em 30/10/2026.** O módulo do manifesto roda o slot por `test-commands` e reprova se o ctest não listar e aprovar exatamente 1 slot. Vermelho visto: run 40982658852 (SHA e194302, slot renomeado de propósito: "slot listado pelo ctest: 0", PARADA). Verde visto: run 40984792478 (SHA ecfea3d, job Flatpak 123833170666, "slot aprovado: 1"), conferido no log pelo CISO. Passo 0 em docs/pesquisa/ci_medicao.md. | A cada push, o job Flatpak do CI; o log tem de trazer "slot aprovado: 1". |
| 2 | Portão de varredura: a variável só pode aparecer no arnês do fedora44 | `tools/portoes/portao_sha1_env.py`; testes `portoes.*` no ctest; passo `sha1_env` no job `higiene` do `ci.yml` | **ATIVO no build e no host.** Visto vermelho antes da implementação (commit de vermelho `ca3523b` e rodadas 1 e 2 do revisor: `portaoSha1ViolacaoEmSrcOutReprova` e `portaoSha1ViolacaoEmSrcBuildXReprova`, mutante de poda antiga). **Visto verde no CI:** run 40984792478, job higiene 123833170332, passo "Variável de SHA-1 fora do arnês fedora44 (portao_sha1_env)". | `ctest` no build por SHA (`portoes.*`); `python3 -I tools/portoes/portao_sha1_env.py <raiz>` no host e na imagem `notalivre-higiene:ci`; log do job `higiene` do CI com o passo `sha1_env` verde, citado com o SHA. |

Cobertura do portão no build: `src/`, `apps/`, `flatpak/`, `tests/`, `tools/`, `.github/workflows/ci.yml` (acrescidos à `LISTA_FONTE` na rodada 2) e os CMakeLists da raiz. Os demais arquivos de `.github/` não entram no build; o job higiene do CI varre o workspace inteiro.

**Afirmação sobre o app.** O app não lê nem define a variável. Essa afirmação só é verdadeira na medida em que o controle 2 roda sobre `src/` e `apps/` no CI, o que está visto desde o run 40984792478.

## Consequências

- Positivo: o desvio fica confinado ao arnês de teste; o app e a emissão real não herdam a variável (sujeito à verificação do controle 2 no CI).
- Negativo: o fedora44 do CI prova a assinatura pelo arnês, não pela política do sistema. Um usuário Fedora com a mesma política
  não tem o mesmo comportamento no oráculo. A NL-2.3 pode enfrentar o mesmo muro no runtime, por isso o passo 0 dela existe.
- Revisão: este ADR é reavaliado quando (a) a NL-2.10 confirmar ou refutar a rsa-sha1 na Sefin, (b) o passo 0 da NL-2.3 medir o runtime
  KDE 6.11, ou (c) a distro mudar a política. Se a NL-2.3 falhar no runtime, o desvio sobe ao CTO antes de qualquer release.
