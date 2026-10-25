# Testes do Projeto

> Tipos de teste aplicáveis a este projeto (stack: C++23, Qt 6.12 com QML, MySQL e MariaDB, API HTTPS com mTLS, XMLDSIG, SMTP e OAuth, Flatpak, CI no GitHub Actions em contêineres; servidor falso e ferramentas de teste em Python na pasta tools/). T1 unitário fica sob o hook de TDD, não listado aqui; o critério de verde do T1 é zero falha, sem meta numérica de cobertura (L-35; desvio do "70%" registrado em NL-P.3 do TODO.md). Cada tipo que não é coberto pelo plano vira um item TST-* na tabela de pendências; os que o plano já cobre apontam para o item dele, sem duplicar.
>
> Política - ferramenta ausente (agnóstica de SO): ao rodar um TST-* cuja ferramenta falta, NÃO falhar em silêncio nem instalar sozinho: detectar conforme o SO (`command -v` no Unix/WSL; `Get-Command` ou `where` no Windows), OFERECER instalar via AskUserQuestion com o comando adequado ao SO e ao gerenciador disponível (apt, dnf, brew, winget, choco, scoop), preferindo gerenciadores cross-platform (pip ou uv, cargo, npm) quando a ferramenta os suporta e, sem confirmação, deixar o item pendente com nota.
>
> Regra de rede e de execução (decisões do líder de 25/10/2026, ponto L7): testes rodam primeiro em contêiner local, nunca na sessão do líder (L-50), com a lista fechada de imagens do plano; um trabalho pesado por vez, com flock. Fonte completa: docs/superpowers/plans/2026-10-25-notalivre-plano.md, seção 5.

## Cobertos pelo plano (sem item próprio)

- **T2 Análise estática** (cppcheck e clang-tidy): coberto por NL-10.1 (zero erro de segurança); presets de NL-0.2.
- **T3 Fuzzing de inputs**: leitor CSV em NL-6.5 e codec XML em NL-1.6 (propriedade e fuzz do plano). O parser da resposta da Sefin e o carregador PKCS#12 não estão no plano; cobertos pelo item TST-T3 injetado (ver abaixo).
- **T4 Análise dinâmica de memória** (ASan e UBSan): preset asan-ubsan de NL-0.2 e NL-10.1 (zero).
- **T5 Scanning de dependências**: coberto por NL-10.1 (T12 sobre o SBOM de NL-9.3); sem item próprio, por sobreposição com T12.
- **T6 Teste de APIs**: contrato com a Sefin pelo servidor falso (NL-2.6 e NL-2.8) e pela Sefin real na Onda 11 (NL-11.2). Sem item próprio.
- **T8 Verificação de secrets**: gitleaks em NL-0.6 e NL-10.1 (zero).
- **T9 Teste de rede**: handshake, timeout e erro transitório em NL-2.5 (integração contra o servidor falso). Sem item próprio.
- **T10 SQL injection**: NL-3.1 (consulta sem concatenação não compila) e NL-10.1 adaptado a MySQL e MariaDB.
- **T12 Busca de CVEs**: NL-10.1 sobre o SBOM de NL-9.3.
- **T14 Integração fim a fim**: plano, seção 5 (MySQL 8.4 e MariaDB 11.8 em contêiner e como services do CI; servidor falso; CLI ponta a ponta a partir da Onda 1).
- **T15 Pré-CI (espelhar o CI local)**: plano, seção 4 (espelho local antes de todo push, com a mesma lista de jobs do CI).
- **Injeção de falha, propriedade, mutação e interface offscreen**: plano, seção 5 (NL-3.7, NL-6.6, NL-1.1, NL-1.6, NL-6.5, protocolo da L-27, NL-1.8).

## Injetados na tabela (o plano não cobre)

## TST-T3 Fuzzing de inputs não confiáveis
Fuzzing do parser da resposta da Sefin (NL-2.8; o formato ainda é INFERÊNCIA até a Onda 11) e do carregador PKCS#12 (NL-2.2), em contêiner local antes do push. Item TST-T3 em TODO.md, Onda W10; injetado por decisão do orquestrador (gap de segurança), porque o plano não cobre esses dois parsers.
**Ferramentas:** libFuzzer ou AFL (C++), com sanitizers do preset asan-ubsan de NL-0.2.

## TST-T7 Scanning de binário (hardening)
Verificar flags de hardening (PIE, RELRO, stack canary, NX, FORTIFY) no binário compilado e no bundle Flatpak da Onda 9. Item TST-T7 em TODO.md, Onda W10.
**Ferramentas:** checksec ou hardening-check (apt, dnf ou pip conforme o SO).

## Não aplicados

- **T11 Fuzzing de protocolos**: o app fala HTTPS com a API oficial, sem protocolo de rede próprio. Não se aplica.
- **T1 Unitário**: fora desta lista (hook de TDD).
