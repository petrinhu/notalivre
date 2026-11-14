# Testes do Projeto

> Tipos de teste aplicáveis a este projeto (stack: C++23, Qt 6.12 com QML, MySQL e MariaDB, API HTTPS com mTLS, XMLDSIG, SMTP e OAuth, Flatpak, CI no GitHub Actions em contêineres; servidor falso e ferramentas de teste em Python na pasta tools/). T1 unitário fica sob o hook de TDD, não listado aqui; o critério de verde do T1 é zero falha, sem meta numérica de cobertura (L-35; desvio do "70%" registrado em NL-P.3 do TODO.md). Cada tipo que não é coberto pelo plano vira um item TST-* na tabela de pendências; os que o plano já cobre apontam para o item dele, sem duplicar.
>
> Política - ferramenta ausente (agnóstica de SO): ao rodar um TST-* cuja ferramenta falta, NÃO falhar em silêncio nem instalar sozinho: detectar conforme o SO (`command -v` no Unix/WSL; `Get-Command` ou `where` no Windows), OFERECER instalar via AskUserQuestion com o comando adequado ao SO e ao gerenciador disponível (apt, dnf, brew, winget, choco, scoop), preferindo gerenciadores cross-platform (pip ou uv, cargo, npm) quando a ferramenta os suporta e, sem confirmação, deixar o item pendente com nota.
>
> Regra de rede e de execução (decisões do líder de 25/10/2026, ponto L7): testes rodam primeiro em contêiner local, nunca na sessão do líder (L-50), com a lista fechada de imagens do plano; um trabalho pesado por vez, com flock. Fonte completa: docs/superpowers/plans/2026-10-25-notalivre-plano.md, seção 5.

## Cobertos pelo plano (sem item próprio)

- **T2 Análise estática** (cppcheck e clang-tidy): coberto por NL-10.1 (zero erro de segurança); presets de NL-0.2.
- **T3 Fuzzing de inputs**: leitor CSV em NL-6.5 e codec XML em NL-1.6 (propriedade e fuzz do plano). O parser da resposta da Sefin e o carregador PKCS#12 não estão no plano; cobertos pelo item TST-T3 injetado (ver abaixo).
- **T4 Análise dinâmica de memória** (ASan e UBSan): preset asan-ubsan de NL-0.2 e NL-10.1 (zero). NL-3.0h: roda em push de `onda-*` e `main` e em `pull_request`, no job `asan` (fedora45 e mint22.3), e o espelho local tem as duas linhas. Gate de sanitizer: o CTest ignora o código de saída quando `PASS_REGULAR_EXPRESSION` casa, e um processo que sai com erro do LeakSanitizer ficaria Passed. Por isso nenhum teste usa regex de passe (NL-3.A; portão estático `tools/portoes/codigo_de_saida.py`, que também barra `WILL_FAIL` e `SKIP_*` em todo CMakeLists.txt e .cmake rastreado): o código de saída decide, e o do sanitizer é 86. Como segunda defesa, `nl_gate_sanitizer_fecha()` (CMakeLists da raiz) acrescenta `(Leak|Address)Sanitizer` e `runtime error:` ao `FAIL_REGULAR_EXPRESSION` de todo teste do CTest, percorrendo a árvore de diretórios, sem registro manual nem isentos (NL-3.A, condição C4.1 do CISO). O pulo e a contagem exata de funções ficam com `tools/portoes/pulos_testes.sh`, pelo junitxml, também no job asan. Verificação, cada uma vista reprovando: `tools/portoes/sanitizer_ativo.py`, depois do build do preset (nenhum teste com PASS, `WILL_FAIL` ou `SKIP_*`; todo teste do CTest tem os dois regex, sem isentos; executáveis de teste, CLI e GUI trazem o ASan; o canário `tests/sanidade/canario_sanitizer.cpp` dá relatório de vazamento, de heap e de UB; o CTest real reprova o canário que vaza, com os regex e só pelo código) e `tools/portoes/sanitizer_ligado.py`, nos portões estáticos (ambiente do preset fixo, portão ligado no job e no contêiner, nenhum ajuste de sanitizer fora do preset, nenhum `codigo != 0` em `tests/e2e`). Cada um confere a ligação do outro, porque portão retirado da lista e do job não roda para avisar; retirar os dois no mesmo commit, só a revisão do diff pega (risco aceito, ratificado pelo líder em 06/11/2026; `CLAUDE.md`, Decisões do líder). Retirada de um portão só, medida na NL-3.A (14/11/2026, sobre `d80cfb9`): sem a linha de `sanitizer_ligado` na lista e com o piso ajustado, os portões estáticos do build debug local passam; reprovam o `sanitizer_ativo` do build asan, pela guarda cruzada, e o próprio `sanitizer_ligado` no job higiene do espelho, que segue chamando o portão. O CI roda o mesmo comando na mesma imagem. O sanitizer sai com o código 86: sob ASan o padrão é 1, igual à saída de recusa da CLI, e um vazamento em caminho de erro passaria. Limite conhecido, INFERÊNCIA não medida: o LeakSanitizer pode não reportar ponteiro que a varredura ainda alcança; medida com relatório, só a alocação em thread que termina (objeto global e ponteiro local não medidos; `tests/sanidade/canario_sanitizer.cpp`). Limite conhecido do portão (registro do CTO na NL-3.0h, não remedido depois): o relatório de sanitizer em processo filho só aparece se o teste conferir o código de saída exato. Na data do registro, só `tst_cli_emitir` e `tst_dps_validar` lançavam binário instrumentado, e `sanitizer_ligado.py` barra `codigo != 0` só em `tests/e2e`; um lançador novo fora de `tests/e2e` fica fora dessa regra. Se isso mudar, a alternativa é `log_path` com conferência da pasta depois do ctest (técnica da suíte do Git), ao custo de o relatório sair do stderr e os regex do gate deixarem de casar; a decisão, se vier, é do CTO. Limite de compilador (mesmo registro): `tests/unit/foundation/pkcs12/tst_vazamento_ossl.cpp` (linhas 16 e 45) depende de `__SANITIZE_ADDRESS__`, e a matriz tinha só GCC; se o Clang entrar, trocar por `__has_feature(address_sanitizer)`. Canário e portões não dependem de compilador.
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
