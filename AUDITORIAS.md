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

### Cadeia do Anexo A v1.00 de 10/12/2025 (tabelas de municípios, países e localidades)

- **Aceite:** Narciso (CISO), 28/11/2026, 13:06, sobre o commit 46f61db da NL-3.M. Pedido da revisão de segurança da NL-3.M (security-engineer, 28/11/2026, 13:01, APROVA condicionado, achados M-1 e M-2); a NL-3.M fica em `TODO.md`.
- **Objeto:** o XLSX do Anexo A v1.00, 218140 bytes, SHA-256 `238b715ab2dcc2c9e0857c44d69048e0af806b45ec8499568342de4a37f4419d` (âncora em `tools/tabelas/fontes.json`), e os três JSON derivados dele no configure (`municipios.json`, `paises.json`, `localidades.json`), fixados em `third_party/nfse-tabelas/anexo-a-20251210/SHA256SUMS`.
- **Risco:** o mesmo modelo de confiança do XSD. O hash do XLSX foi medido pelo próprio projeto, e o gov.br não publica hash para conferir. O espelho é a branch `espelho-xsd`, commit `bde0e13`, com o link raw fixado por esse SHA, no mesmo repositório que guarda o hash. Há dois riscos que o XSD não tem:
  - os JSON são derivados, então o conversor (`tools/tabelas/converter.py`) entra na cadeia de confiança. O `SHA256SUMS` prova que o build gerou os mesmos bytes da conversão de 28/11/2026, não que a conversão esteja certa;
  - o conversor parseia o XML de dentro do XLSX no configure, uma superfície nova no build.
- **Por que o aceite:** como no XSD, quem troca o hash no repositório também troca o código que o confere, então o espelho não abre um domínio de confiança novo. O parse do XLSX só acontece depois que os bytes batem com a âncora, então a superfície do parser só é alcançada por quem já pode alterar o repositório.
- **Fatos e quem os mediu:**
  - medido pelo CISO em 28/11/2026: o blob do XLSX no commit `bde0e13` tem 218140 bytes e o SHA-256 da âncora, e `refs/heads/espelho-xsd` no origin aponta para `bde0e13`;
  - medido pelo projeto e relatado pelo revisor: o SHA-256 gravado em `fontes.json` (download de 25/10/2026) e o medido pelo main em 28/11/2026, na URL nova do gov.br, são iguais. São duas medições do projeto, não uma fonte independente;
  - relatado no `TODO.md` (NL-3.M): o run 46158328529, sobre o commit d68b78e, ficou verde nas 4 distros e no asan, com a conversão conferida contra o `SHA256SUMS`. O CISO não abriu o log desse run.
- **Controles compensatórios:**
  - o SHA-256 do XLSX é conferido em todo caminho de download: `tools/ci/baixar_zip_xsd.cmake`, chamado por `third_party/nfse-tabelas/anexo-a-20251210/CMakeLists.txt:65`; `.github/workflows/ci.yml` (que confere também que o hash do `env` é o de `fontes.json`); e `flatpak/io.github.petrinhu.notalivre.yml`, com `sha256` na fonte;
  - o conversor confere tamanho e SHA-256 dos bytes antes de abrir o ZIP e lê só esses bytes (`tools/tabelas/fontes.py:20-30`), recusa DOCTYPE e ENTITY pelo próprio expat antes do parse (`tools/tabelas/xlsx_leitor.py:50-69`), tem teto de 16 MiB por parte e de 256 entradas (`xlsx_leitor.py:20-21` e `:37-47`) e não extrai arquivo para o disco;
  - o conversor confere as contagens fixadas (5570 municípios, 250 países, 1 localidade) e a consistência linha a linha, e qualquer divergência sai com status 1 sem gravar nada; o CMake trata status diferente de zero como erro fatal;
  - os três JSON são comparados com o `SHA256SUMS` versionado, e qualquer divergência aborta o configure (`CMakeLists.txt:88-107` do mesmo diretório);
  - o SHA-256 desse `SHA256SUMS` é embutido na Mid (`NL_TABELAS_MANIFESTO_SHA256`, `src/mid/composicao/dps_validar.cpp:52`, com `static_assert` de 64 caracteres), e a carga (`src/mid/tabelas/carregar_catalogo.cpp:211`) confere o pino, depois o hash de cada JSON e só então faz o parse, com os códigos validados pelo domínio;
  - o `dps validar` e o `emitir` (antes da senha e de novo na fachada) passam pela mesma carga com o pino embutido (`apps/cli/comando_dps_validar.cpp:75`, `apps/cli/comando_emitir.cpp:103`, `src/mid/composicao/emissao_cli.cpp:193`), e falha de carga para a operação: no `dps validar`, como erro de ambiente; no `emitir`, com a linha de DPS reprovada, que é enganosa para ambiente (NL-I.400), mas a emissão para;
  - a chave `[tabelas]` só pode ser definida em Homologação (`apps/cli/configuracao_cli.cpp:53-55`), e mesmo lá o pino vale.
- **Lacuna aceita (BAIXA):** algumas exceções do leitor (por exemplo índice ou número inválido numa célula, ou entrada cifrada no ZIP) não são capturadas pelo conversor (`converter.py:142`) e saem com traceback e status 1. A falha é fechada (o configure para), e só é alcançada depois de o SHA-256 bater. O conserto, capturar essas exceções e sair pelo mesmo caminho de parada, vai à INBOX.
- **Pior caso:** uma tabela adulterada afrouxa E0238, E0246 e E0302 na conferência local, e a Sefin aplica as mesmas regras de novo. Ou a tabela bloqueia DPS válida: o emitente não emite pelo app até sair uma versão nova, porque o pino embutido não tem contorno pelo usuário. Esse é um risco de disponibilidade, que o XSD também tem, mas aqui ele vem de dado. O mesmo efeito vem de tabela vencida: a v1.00 fica fixada até uma fatia trocar a âncora.
- **Não provado:**
  - que o SHA-256 do XLSX confere com o que o gov.br publicou. O gov.br não publica hash para este arquivo;
  - que a conversão está correta além das contagens e da consistência que o conversor confere;
  - que um mutante na ordem ou na presença das conferências da carga morre nos testes. A mutação dos passos 2 e 5 roda pela NF-12 e vai à decisão do líder, pela decisão dele de 21/11/2026 sobre mutação e sabotagem em segurança. **Provado depois, em 28/11/2026:** no run 46166336001 (commit 06de792), os mutantes do catálogo (pino sem conferência, hash por arquivo sem conferência, guarda de [tabelas] desligada e parse antes do hash) deram encontradas=4 aplicadas=4 mortas=4 sobreviventes=0, cada morte pela asserção exata do slot; conferido pelo Narciso às 14:00.
- **Como verificar:**
  - baixar o XLSX direto do gov.br, numa pasta vazia fora do CI, e rodar `sha256sum` sobre ele; comparar com a âncora;
  - comparar os códigos de `municipios.json` com a lista de municípios do IBGE e os de `paises.json` com a ISO 3166-1 alfa-2, que são fontes independentes do gov.br/nfse, e explicar cada diferença de contagem (a tabela tem 250 países; o número de códigos atribuídos da ISO 3166-1 não foi conferido para este registro);
  - se o gov.br passar a publicar hash ou manifesto assinado, conferir contra ele e reabrir este aceite.
- **Relação com outros itens:** o parecer K2 segue aberto em `TODO.md` (NL-I.077 e NL-I.393). Os JSON são derivados de dado sob CC BY-ND, o que torna o K2 mais relevante para esta cadeia do que para o XSD intacto. Este aceite não o substitui.
