# Auditorias do Projeto

> Auditorias aplicáveis a este projeto (stack: C++23, Qt 6.12 com QML, MySQL e MariaDB, API HTTPS com mTLS, XMLDSIG, SMTP e OAuth, Flatpak, CI no GitHub Actions em contêineres). Cada auditoria aplicável é coberta por um item do plano ou por um item AUD-* injetado na tabela de pendências (TODO.md), nas ondas finais, depois dos testes (TST-*) e do código.
>
> Política - ferramenta ausente (agnóstica de SO): ao rodar um AUD-* cuja ferramenta falta, NÃO falhar em silêncio nem instalar sozinho: detectar conforme o SO (`command -v` no Unix/WSL; `Get-Command` ou `where` no Windows), OFERECER instalar via AskUserQuestion com o comando adequado ao SO e ao gerenciador disponível (apt, dnf, brew, winget, choco, scoop), preferindo gerenciadores cross-platform (pip ou uv, cargo, npm) quando a ferramenta os suporta e, sem confirmação, deixar o item pendente com nota.

## Cobertas pelo plano (sem item AUD-* próprio)

- **AUD-DISC Descoberta e modelagem**: STRIDE sobre custódia do A1, senha-mestra, cofre, mTLS, XMLDSIG, SMTP, OAuth, logs e Flatpak em NL-1.9 (decisões de Narciso, N1 a N6).
- **AUD-ARCH Arquitetura e camadas**: portão A2 de camadas em NL-0.3, vermelho por sabotagem de estreia; revisão de camadas pelo internal-auditor em cada fechamento de onda.
- **AUD-SEC Segurança**: NL-10.1 (AppSec: OWASP, CWE, cripto, custódia, rede, SQL, com SAST, segredos e CVE); NL-2.7 (PII em log); NL-9.3 (SBOM).
- **AUD-UI UI/UX e acessibilidade**: NL-8.11 (A3, WCAG 2.2 AA, pela árvore de acessibilidade em contêiner); captura de tela conferida pelo qa-engineer (L-13) em NL-1.8, NL-4.5 e Onda 8.
- **AUD-API Contratos de API**: o app é cliente da API oficial, não expõe API. Contrato coberto pelo servidor falso (NL-2.6) e pela Sefin real na Onda 11 (NL-11.2).
- **AUD-REPORT Relatório final**: NL-10.3 (dossiê de auditoria do internal-auditor, com achados por severidade).

## Injetados na tabela (o plano não cobre)

## AUD-DB Banco de dados
Schema, EXPLAIN das consultas críticas, índices, migrations versionadas (NL-3.1), sqlfluff como lint de SQL, e LGPD no que é gravado (dados cifrados, índice cego). Pré-requisito: TST-T3 e TST-T7, NL-10.1 e as fatias de persistência NL-3.1, NL-3.4 e NL-3.7.
**Ferramentas:** EXPLAIN e EXPLAIN ANALYZE no MySQL 8.4 e no MariaDB 11.8, sqlfluff, revisão manual de migrations e índices.

## AUD-DEPS Dependências e acoplamento
Grafo de dependências entre bibliotecas e entre camadas, ciclos e acoplamento. O SBOM de NL-9.3 cobre licença e CVE, mas não o grafo. Pré-requisito: TST-T3 e TST-T7, NL-10.1 e NL-9.3.
**Ferramentas:** `cmake --graphviz` e revisão do grafo de dependências.

## AUD-FRAMEWORK Padrões Qt
Connect com ponteiro de função tipado, QPointer em objetos de vida curta, sinais e slots, model e view, uso correto da fachada QML. Pré-requisito: TST-T3 e TST-T7, NL-10.1 e NL-8.11.
**Ferramentas:** clazy (checks de Qt), revisão de padrões do framework.

## AUD-LANG Idiomas modernos da linguagem (prioridade baixa, do catálogo)
Uso de std::expected para erro, RAII, ausência de ponteiro cru em posse, idioms do C++23 que o GCC 13 do Mint compila (decisão D0.4b). Pré-requisito: TST-T3 e TST-T7, NL-10.1 e NL-0.4.
**Ferramentas:** clang-tidy (modernize, cppcoreguidelines), revisão de idioms.

## AUD-QUALITY Qualidade de código
Complexidade ciclomática, god classes, código morto, duplicação (regra de três, L-33). Pré-requisito: TST-T3 e TST-T7, NL-10.1 e NL-8.11.
**Ferramentas:** clang-tidy, lizard (complexidade), revisão de duplicação.

## Não aplicadas

- **AUD-COV Cobertura de testes**: não aplicada como meta. A L-35 proíbe meta numérica de cobertura; cobertura é consequência de TDD. O 70% do TESTES T1 foi registrado como desvio na NF-34 (b) do `GODS_LAWS.md`.

## Riscos aceitos

Aceites de risco de segurança e privacidade, com o controle compensatório e o jeito de verificá-lo. Cada aceite nomeia quem aceitou, quando e sobre qual commit. O arquivo não tinha seção de aceite; esta é a primeira.

### Cadeia do XSD de Produção Restrita de 24/11/2026

- **Aceite:** Narciso (CISO), 27/11/2026, 14:50, sobre o commit 04cbc91 da NL-3.L. Achado da revisão de segurança da Onda 3; a NL-3.L e o achado ficam em `TODO.md`.
- **Objeto:** o ZIP do XSD de Produção Restrita de 24/11/2026, SHA-256 `1ca83c1a7929f6d5684c9a17700fe3d3a375e4ea12b7ccb832bfc369768902c0`.
- **Risco:** o hash foi medido pelo próprio projeto. O gov.br não publicou um hash independente para conferir. O espelho do ZIP é a branch órfã `espelho-xsd`, commit `4e2ced7`, com o link raw fixado por esse SHA. O espelho mora no mesmo repositório que guarda o hash.
- **Por que o aceite:** quem troca o hash no repositório também troca o código que o confere, então o espelho não abre um domínio de confiança novo. Dois downloads do próprio gov.br, nos jobs `asan (fedora45)` e `nativo (mint22.3)` do run 46014900162, deram o mesmo hash. Isso mostra que o gov.br serve os mesmos bytes a dois runners; não prova que esse é o hash que o gov.br publica.
- **Controles compensatórios:**
  - o SHA-256 do ZIP é conferido em todo caminho de download: `tools/ci/baixar_zip_xsd.cmake`, `.github/workflows/ci.yml` e `flatpak/io.github.petrinhu.notalivre.yml`;
  - cada `.xsd` é conferido contra o `SHA256SUMS` versionado em `third_party/nfse-xsd/prodrest-20261124/`, pelo CMake desse diretório;
  - o SHA-256 desse `SHA256SUMS` é embutido na Mid (`NL_XSD_PRODREST_MANIFESTO_SHA256`, `third_party/nfse-xsd/prodrest-20261124/CMakeLists.txt:151`), e a validação recusa outro hash;
  - o parse é com NONET (`src/foundation/xml/parse_seguro.cpp:57`).
- **Pior caso:** um XSD adulterado afrouxa a validação local. A Sefin valida a emissão de novo.
- **O que a emissão de teste prova:** a emissão de teste aceita (DPS nº 10, HTTP 201, 27/11/2026, 14:31) corrobora o esquema, não os bytes do ZIP.
- **Não provado:** que o hash confere com o que o gov.br publicou. Não há fonte independente publicada pelo gov.br para o SHA-256 deste ZIP.
- **Como verificar:** baixar o ZIP direto do gov.br, numa pasta vazia fora do CI, e rodar `sha256sum` sobre ele; comparar cada `.xsd` com o `SHA256SUMS`. Se o gov.br passar a publicar um hash ou um manifesto assinado, conferir contra ele e reabrir este aceite.
- **Relação com outros itens:** o parecer K2 (licença CC BY-ND do gov.br sobre o XSD redistribuído) segue aberto em `TODO.md`, NL-I.393. Este aceite não o substitui.
