# ADR 0006: parâmetros do Argon2id para a senha-mestra (ADR-S5)

**Status:** ACEITO por Narciso (CISO) em 25/10/2026 (N6), com as mudanças marcadas `[alterado pelo CISO, 25/10/2026]`. Ver a seção "Revisão do CISO (25/10/2026)" no fim. O paralelismo fora da RFC continua registrado como desvio de risco (seção "Desvio de risco").
**Data:** 25/10/2026.
**Origem:** plano, NL-1.9 (parâmetros gravados junto do dado, recomendação do CTO); spec 4.4 (senha-mestra com Argon2id, parâmetros em aberto).
**Relacionadas:** ADR-0003 (a KEK-senha sai desta derivação e envelopa a DEK).

## Contexto

- FATO, spec 4.4: a senha-mestra deriva a chave com Argon2id. Os parâmetros estavam em aberto.
- FATO, lido em 25/10/2026, RFC 9106 seção 4 (https://www.rfc-editor.org/rfc/rfc9106.html):
  - Primeira opção recomendada: Argon2id com t=1, p=4, m=2^21 KiB (2 GiB), salt de 128 bits e tag de 256 bits.
  - Segunda opção recomendada, para memória limitada: t=3, p=4, m=2^16 KiB (64 MiB), salt de 128 bits e tag de 256 bits.
  - Salt de 128 bits basta para todas as aplicações (o mínimo citado é 64 bits, "in the case of space constraints").
  - Tag de 128 bits basta para a maioria das aplicações, inclusive derivação de chave.
  - Argon2id é a escolha quando o leitor não tem certeza de qual variante usar, ou quando considera viável o ataque por canal lateral.
  - Tensão no texto: a seção 1 descreve Argon2i como "preferred for password hashing and password-based key derivation". Esta ADR segue a seção 4, que recomenda Argon2id como escolha geral.
- FATO, lido em 25/10/2026, OWASP Password Storage Cheat Sheet: configuração mínima de Argon2id com m=19456 KiB (19 MiB) e t=2, p=1; scrypt como alternativa "when the former is not available" (https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html).
- FATO, lido em 25/10/2026, libsodium, `crypto_pwhash` (https://doc.libsodium.org/password_hashing/default_phf): os parâmetros expostos são `opslimit` (máximo de operações), `memlimit` (RAM em bytes) e `alg`, com `crypto_pwhash_ALG_ARGON2ID13` (desde a versão 1.0.13). A página não expõe paralelismo e não declara o número de lanes.
- INFERÊNCIA: o número de lanes usado pelo libsodium é fixo e, pelo que se sabe, igual a 1. Não foi lido no código-fonte; confirmar em NL-3.2.
- `[alterado pelo CISO, 25/10/2026]` FATO, código-fonte do libsodium na branch `master`, lido em 25/10/2026 (`src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c`): `crypto_pwhash_argon2id` chama `argon2id_hash_raw((uint32_t) opslimit, (uint32_t) (memlimit / 1024U), (uint32_t) 1U, ...)`. Logo, lanes = 1, `opslimit` é o `t` (passes) e `memlimit` em bytes vira `m` em KiB. Conferir na versão empacotada em cada distro e no runtime Flatpak, em NL-3.2.
- `[alterado pelo CISO, 25/10/2026]` FATO, cabeçalho `crypto_pwhash_argon2id.h` na mesma branch: `OPSLIMIT_MIN` 1, `MEMLIMIT_MIN` 8192, `INTERACTIVE` 2 e 67108864 (64 MiB), `MODERATE` 3 e 268435456 (256 MiB), `SENSITIVE` 4 e 1073741824 (1 GiB), `SALTBYTES` 16.
- `[alterado pelo CISO, 25/10/2026]` FATO, OWASP Password Storage Cheat Sheet, lido em 25/10/2026: as configurações mínimas de Argon2id são todas com p=1 (por exemplo, m=19456, t=2, p=1), e "These configuration settings provide an equal level of defense, ... the only difference is a trade off between CPU and RAM usage."
- FATO (`docs/pesquisa/stack.md`, linha 132, INFERÊNCIA do CTO): Argon2 no OpenSSL só existe a partir da 3.2, e a base do Mint 22.3 traz 3.0. Por isso o plano escolheu libsodium.

## Decisão (proposta)

1. **Algoritmo:** Argon2id, versão 1.3, por `crypto_pwhash` com `crypto_pwhash_ALG_ARGON2ID13`.
2. **Parâmetros de criação:**
   - memória: 64 MiB (`memlimit` = 67108864 bytes), que é `m = 2^16 KiB` da RFC 9106;
   - passes: `opslimit` com valor 3, que corresponde a `t = 3` (INFERÊNCIA sobre o mapeamento, a confirmar em NL-3.2); `[alterado pelo CISO, 25/10/2026]` o mapeamento é FATO pelo código-fonte (Contexto);
   - paralelismo: o valor interno da biblioteca (ver Desvio de risco);
   - salt de 16 bytes gerado por CSPRNG;
   - saída de 32 bytes, para a chave AES-256 da KEK-senha.
3. **Registro gravado junto do dado:** algoritmo, versão, `memlimit`, `opslimit` e salt. **Nunca a saída.** A saída é a KEK-senha. Gravá-la, mesmo numa string de hash, entregaria a chave que envelopa a DEK e anularia a hierarquia da ADR-0003.
4. **Prova de senha correta:** é o envelope AES-256-GCM da DEK que decifra com a KEK derivada. Falha de tag significa senha errada ou registro íntegro falho, com a mesma mensagem única da ADR-0003. Não existe comparação com hash armazenado.
5. **Piso de aceitação ao ler:** `memlimit` ≥ 19 MiB e `opslimit` ≥ 2 (mínimo da OWASP). Registro abaixo do piso é recusado, sem derivar com parâmetros mais fracos (anti-downgrade, fail closed).
6. **Subida de custo:** se o registro estiver abaixo dos parâmetros vigentes e acima do piso, o desbloqueio bem-sucedido deriva a KEK com os parâmetros novos, re-envelopa a DEK e regrava o registro. Nenhuma saída de hash é gravada.
7. **Medição de referência:** a derivação deve levar entre 0,5 s e 1,5 s na máquina de referência do projeto. Abaixo de 0,5 s, ~~sobe-se `opslimit`~~ `[alterado pelo CISO, 25/10/2026]` sobe-se o custo na ordem do subitem abaixo. Valor proposto, a validar em NL-3.2. A medição é custo para o atacante, e não bloqueio.
   - `[alterado pelo CISO, 25/10/2026]` **Ordem de ajuste, decidida por Narciso:** 64 MiB e `opslimit` 3 são o piso de criação, e nunca se cria abaixo dele. Se a derivação ficar abaixo de 0,5 s, sobe primeiro a memória, em potências de 2, até o teto de 256 MiB (o valor `MODERATE` do libsodium); só depois sobe `opslimit`. Memória pesa mais no custo do atacante do que passes. Os valores finais saem da medição de NL-3.2 e entram neste ADR com a máquina de referência.
8. **Escopo:** Argon2id só deriva a KEK-senha. O código de recuperação não passa por ele (HKDF, ADR-0003). A senha do certificado não é derivada pelo app, porque é o segredo do próprio PKCS#12.
9. `[alterado pelo CISO, 25/10/2026]` **Parâmetros presos ao envelope:** algoritmo, versão, `memlimit`, `opslimit` e salt entram na AAD do envelope da DEK pela senha (ADR-0003, regra 9), independentemente do que o líder decidir sobre o restante da regra 9 do ADR-0003. Adulterar o registro faz o desbloqueio falhar, em vez de derivar com outro custo.

## Alternativas consideradas

- **Primeira opção da RFC 9106 (2 GiB, t=1):** rejeitada. Exige 2 GiB por desbloqueio num desktop que roda outros programas. INFERÊNCIA: risco de falta de memória, a medir em NL-3.2.
- **Piso da OWASP como padrão (19 MiB, t=2):** rejeitada como padrão. Serve de piso de aceitação, mas deixa o custo baixo para força bruta offline.
- **Argon2i ou Argon2d:** rejeitadas. A seção 4 da RFC 9106 recomenda Argon2id quando canal lateral é ameaça viável. A seção 1, lida só na parte citada acima, não foi reconferida quanto ao Argon2d.
  - `[alterado pelo CISO, 25/10/2026]` A variante não é escolha deste ADR: Argon2id é decisão do líder (spec 4.4). A seção 1 da RFC 9106, reconferida, diz também que "Argon2id MUST be supported by any implementation of this document, whereas Argon2d and Argon2i MAY be supported".
- **scrypt:** não adotado. A OWASP o indica só para quando Argon2id não existe, e o libsodium oferece Argon2id.
- **Biblioteca de referência do Argon2, ou OpenSSL 3.2 ou mais novo, com paralelismo 4:** não adotada nesta ADR. Muda a stack já decidida no plano e não serve ao Mint 22.3 (OpenSSL 3.0). É decisão de stack, para o CTO e o líder (L-01).

## Desvio de risco (L-77)

**Desvio:** o paralelismo da opção 2 da RFC 9106 (p=4) não é alcançável com libsodium, que não o expõe. A escolha de libsodium veio da stack já decidida. O desvio é registrado como **desvio de risco com controles compensatórios**, e não como conformidade.

`[alterado pelo CISO, 25/10/2026]` **O que o desvio custa, de fato:** o custo do atacante por tentativa vem da memória preenchida e do número de passes; as lanes só dividem esse trabalho entre núcleos do lado de quem defende. Com p=1, o mesmo m e o mesmo t custam ao atacante o mesmo que com p=4, e o usuário espera mais tempo de relógio. A OWASP lista só configurações com p=1 e afirma que elas dão "an equal level of defense" (lido em 25/10/2026). O que se perde é a chance de usar mais memória no mesmo tempo de espera. Continua sendo desvio, porque a RFC 9106 diz "Select p=4 lanes" (seção 4).

| # | Controle compensatório | Como se verifica | Estado |
|---|---|---|---|
| C1 | Memória (64 MiB) e passes (3) iguais à opção 2 da RFC | Teste que lê o registro e confere a chamada à biblioteca: `memlimit` = 67108864 e `opslimit` = 3. `[alterado pelo CISO, 25/10/2026]` Como as regras 6 e 7 sobem o custo, o teste confere `memlimit` ≥ 67108864 e `opslimit` ≥ 3 em todo registro criado, e o valor exato da medição de NL-3.2 no registro de referência | Proposto (NL-3.2) |
| C2 | Tempo de derivação entre 0,5 s e 1,5 s, com `opslimit` ajustável | Medição registrada no relatório da fatia, na máquina de referência | Proposto (NL-3.2) |
| C3 | Biblioteca reavaliada a cada atualização do libsodium nas quatro distros | Revisão de Narciso em cada atualização. Sem teste automático, porque o paralelismo não é exposto | Por revisão (modelo de ameaças, C7) |
| C4 | Troca para biblioteca com paralelismo 4 | Decisão de stack, pendente do CTO e do líder | Aberto |

## Consequências

- Cada desbloqueio leva o tempo da derivação e usa 64 MiB de RAM.
- Trocar parâmetros não exige migração de formato, porque o registro grava `memlimit` e `opslimit`.
- A dependência é libsodium para Argon2id. Ela precisa existir nas quatro distros e no runtime Flatpak (verificar em NL-3.2).

## Verificação

- NL-3.2: vetores de Argon2id (RFC 9106, apêndice de exemplos, ou suíte do próprio libsodium), a conferir na implementação.
- `[alterado pelo CISO, 25/10/2026]` O vetor de Argon2id da RFC 9106 (seção 5.3) usa 4 lanes, segredo de 8 bytes e dado associado de 12 bytes, e `crypto_pwhash` não expõe nenhum dos três: esse vetor não pode passar e sai da verificação. Valem: (a) os vetores de Argon2id da suíte de testes do próprio libsodium; (b) um oráculo independente com p=1 e sem segredo nem dado associado. Candidato sem download novo: `openssl kdf` com o algoritmo `ARGON2ID` e `lanes:1`, se o OpenSSL da imagem `fedora:44`, já autorizada, for 3.2 ou mais novo (INFERÊNCIA, conferir na imagem em NL-3.2). Qualquer outra ferramenta de oráculo pede autorização do líder antes (L-51).
- `[alterado pelo CISO, 25/10/2026]` NL-3.2: o envelope da DEK pela senha falha quando um parâmetro gravado é adulterado (regra 9). Sabotagem: tirar os parâmetros da AAD e ver o teste vermelho.
- NL-3.2: o registro gravado não contém a KEK derivada da fixture (mesmo padrão de `campoGravadoNaoTemTextoClaro`). Sabotagem: gravar a saída e ver o teste ficar vermelho.
- NL-3.2: anti-downgrade. Registro com `memlimit` abaixo do piso é recusado. Sabotagem: remover a checagem e ver o teste ficar vermelho.
- NL-3.2: leitura de parâmetros. O desbloqueio usa os valores gravados, e não constantes. Sabotagem: fixar `opslimit` = 1 no código e ver o teste ficar vermelho.
- NL-3.2: medição de tempo registrada no relatório da fatia (não é portão de CI, porque o runner do CI não representa o usuário).

## Riscos residuais

- Senha-mestra fraca escolhida pelo usuário continua sendo o risco principal. A política mínima de senha está em aberto e deve ser decidida junto com o assistente de primeiro uso (NL-8.9).
- Paralelismo fixo: o tempo legítimo de derivação é maior. INFERÊNCIA: o custo do atacante por tentativa é dominado pela memória, e não pelo número de lanes. Isso precisa de medição, e por isso fica como controle C2.
- A tensão entre as seções 1 e 4 da RFC 9106 está registrada. ~~Se Narciso preferir Argon2i, a troca é possível sem migração de dado, desde que o registro grave a variante.~~ `[alterado pelo CISO, 25/10/2026]` Retirado: a variante é decisão do líder (spec 4.4), e não de Narciso. O registro grava a variante, o que deixa a troca possível sem migração se o líder um dia decidir outra.

## Fontes

- RFC 9106 (Argon2), seções 1 e 4, lidas em 25/10/2026: https://www.rfc-editor.org/rfc/rfc9106.html
- OWASP Password Storage Cheat Sheet, lido em 25/10/2026: https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html
- libsodium, `crypto_pwhash`, lido em 25/10/2026: https://doc.libsodium.org/password_hashing/default_phf
- `[alterado pelo CISO, 25/10/2026]` libsodium, código-fonte na branch `master`, lido em 25/10/2026: https://github.com/jedisct1/libsodium/blob/master/src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c e https://github.com/jedisct1/libsodium/blob/master/src/libsodium/include/sodium/crypto_pwhash_argon2id.h
- `[alterado pelo CISO, 25/10/2026]` RFC 9106, seções 1, 4 e 5.3, reconferidas em 25/10/2026.
- Pesquisa: `docs/pesquisa/stack.md`, linha 132 (INFERÊNCIA do CTO sobre a versão do OpenSSL).
- Plano, NL-1.9 e N6.

## Revisão do CISO (25/10/2026)

**Revisor:** Narciso (CISO), distinto do security-engineer que escreveu este ADR (L-12). Modo autônomo (L-15).

**Veredito: ACEITO** (N6, antes de NL-3.2). As mudanças estão no corpo, marcadas `[alterado pelo CISO, 25/10/2026]`. Os valores finais de memória e passes saem da medição de NL-3.2, dentro das regras escritas aqui.

**Decisões:**

1. **Desvio de paralelismo aceito como desvio de risco (L-77)** (ponto 6 do autor). Li o código-fonte: o libsodium fixa lanes = 1 e passa `opslimit` direto como passes. O custo real do desvio é tempo de espera do usuário, e não força contra o atacante, que paga pela memória e pelos passes (OWASP, "equal level of defense" para p=1). Fica como desvio porque a RFC 9106 diz p=4. Trocar de biblioteca é decisão de stack (controle C4) e não está no meu escopo.
2. **Piso de criação 64 MiB e 3 passes; ajuste sobe a memória primeiro, até 256 MiB, e depois os passes.** Controle C1 passa a conferir "pelo menos", e não valor exato.
3. **Parâmetros na AAD do envelope da senha** (item 9), com teste e sabotagem.
4. **Vetor da RFC 9106 fora da verificação**, porque usa 4 lanes, segredo e dado associado. Entram os vetores do libsodium e um oráculo independente com p=1; o candidato sem download novo é o `openssl kdf` da imagem `fedora:44`, a conferir. Ferramenta nova de oráculo pede autorização do líder (L-51).
5. **Retirada a frase que deixava a variante a cargo de Narciso.** Argon2id é decisão do líder.

**Por quê:** a força da senha-mestra contra força bruta offline depende de memória e de passes, e o que o ADR prometia como prova (o vetor da RFC) não podia passar com a biblioteca escolhida. Um portão que não pode passar acaba desligado ou contornado; troquei por vetores que a biblioteca consegue reproduzir e por um oráculo de outra implementação.

**Sobe ao líder:** nada novo. O controle C4 (biblioteca com paralelismo 4) continua aberto para o CTO e o líder, como o ADR já dizia.
