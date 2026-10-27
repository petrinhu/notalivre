# ADR 0004: índice cego de CPF e CNPJ (ADR-S3, porta P2)

**Status:** PROPOSTO, com recomendação do CISO (N5, 25/10/2026) na seção "Revisão do CISO (25/10/2026)" no fim. Porta P2 aguarda a ratificação do líder (ponto L4). Mudanças do CISO marcadas `[alterado pelo CISO, 25/10/2026]`.
**Data:** 25/10/2026.
**Origem:** plano, NL-1.9 e porta P2 (seção 1.4); spec 4.4 (índice cego por campo, parâmetros em aberto).
**Relacionadas:** ADR-0003 (hierarquia de chaves, de onde sai a chave do índice); seção DA-1.1 do plano (normalização do CNPJ).

## Contexto

- FATO, spec 4.4: o índice cego serve para busca. Os parâmetros (chave, algoritmo e o que é indexado) estavam em aberto. O plano, NL-3.4, traz como decisão do CTO indexar CPF e CNPJ normalizados, a confirmar aqui.
- FATO, plano, seção DA-1.1 (linha 265): `Cnpj::de` remove ponto, barra, hífen e espaço, converte a-z em A-Z e exige 14 caracteres `[0-9A-Z]`, com posições 13 e 14 numéricas e os dois dígitos verificadores. A normalização é a entrada do índice.
- FATO, spec 5.7 (URL da Agência Gov, notícia de 03/07/2026): a implantação do CNPJ alfanumérico começa em 31/07/2026. Por isso a normalização precisa estar fixada antes de gravar dado de produção.
- INFERÊNCIA: o CNPJ de pessoa jurídica aparece em base pública (a spec 5.7 cita os dados abertos da Receita), o que torna um índice sem chave vulnerável a comparação com essa base. Para o CPF, não há base pública gratuita (spec 5.7), mas o espaço é de 10^11 valores, pequeno para dicionário.
- FATO, lido em 25/10/2026, OWASP Key Management: uma chave serve a um só propósito. Por isso, a chave do índice não é a chave de campo.
- FATO, lido em 25/10/2026, RFC 5869 seção 3.2: o parâmetro `info` liga a chave derivada a um contexto específico da aplicação. Esse é o mecanismo usado para separar a chave do índice.

## Decisão (proposta)

1. **Tag do índice:** HMAC-SHA-256, com saída completa de 256 bits, sobre a entrada `tipo || valor normalizado`, em que `tipo` é 1 byte (`0x01` para CPF, `0x02` para CNPJ) e o valor normalizado é:
   - CNPJ: saída de `Cnpj::de` (DA-1.1), 14 caracteres maiúsculos, sem máscara.
   - CPF: 11 dígitos, sem máscara, validado antes da indexação.
2. **Chave do índice:** `K_idx = HKDF-SHA256(DEK, info = "notalivre/indice-cego/v1")` (ADR-0003). A chave de campo é outra, derivada ou envelopada por outro caminho. `[alterado pelo CISO, 25/10/2026]` Na forma recomendada pelo CISO para P1 (ADR-0003), a chave de campo é `K_campo = HKDF-SHA256(DEK, info = "notalivre/campo/v1")`, e a DEK não cifra nada diretamente; as duas chaves saem da mesma raiz com `info` distinta.
3. **Não se grava o CPF ou o CNPJ em claro**, nem truncado, nem com hash sem chave.
4. **Escopo do índice:** só CPF e CNPJ de tomador e de emissor, que são as chaves de busca do cadastro. A busca por nome não usa índice: decifra em memória, como decidido no plano (NL-3.4).
5. **Mudança de normalização ou de chave obriga reindexação.** A reindexação abre cada registro com a DEK. Por isso, a normalização de DA-1.1 é fixada antes de gravar dado real, e a sua mudança passa pela cadência de migração (L-70).
6. `[alterado pelo CISO, 25/10/2026]` **A rotação da DEK (ADR-0003, regras 3 e 8) troca também `K_idx`,** e por isso recalcula todas as tags na mesma transação da recifragem. Tag antiga com chave nova não acha nada, e o teste de rotação de NL-3.3 confere a busca depois da rotação.

## Por que chave, e não SHA-256 puro

- Sem chave, o índice de CPF cai para dicionário trivial: um atacante com o banco testa os 10^11 valores.
- Sem chave, o índice de CNPJ compara com a base pública de CNPJs. Com a chave, quem tem só o banco não consegue montar a comparação.

## Alternativas consideradas

- **Sem índice, decifrar tudo a cada busca:** é viável para um emissor com alguns milhares de tomadores, e evita vazamento de igualdade. Foi rejeitada porque o líder decidiu índice cego (spec 4.4), e porque, com vários emissores, a busca cresce. Fica como plano B se o vazamento de igualdade for considerado inaceitável por Narciso.
  - `[alterado pelo CISO, 25/10/2026]` O que rejeita esta alternativa é a decisão do líder pelo índice cego, e não o tamanho do cadastro de algum emissor: volume de uso de um consumidor não corta nem acrescenta desenho (L-21). Narciso considera o vazamento de igualdade aceitável (Revisão do CISO), e o plano B não é acionado.
- **HMAC com a DEK diretamente:** rejeitada, porque mistura o uso da chave de campo com o do índice (OWASP, um propósito por chave).
- **Hash com salt por registro:** rejeitada, porque impede a busca por igualdade.
- **Truncar a tag para 128 bits:** não adotada. A tag de 256 bits custa 32 bytes por campo indexado e evita colisão sem análise adicional.

## Consequências

- Linhas com o mesmo CPF ou CNPJ têm a mesma tag. Quem lê o banco sem a chave vê a igualdade entre linhas, e não o valor. Isso é risco residual aceito, documentado em `docs/seguranca/modelo-de-ameacas.md` (C3.2 e C3.3).
- A busca de tomador por CPF ou CNPJ é uma consulta por igualdade no índice, sem decifrar o cadastro inteiro.
- Trocar a normalização (por exemplo, para o CNPJ alfanumérico) obriga reindexar o cadastro inteiro.

## Verificação

- NL-3.4: `buscaPorCnpjPeloIndiceCego`, com os quatro vetores oficiais de CNPJ e a normalização de DA-1.1. Entradas com máscara, minúsculas e a forma alfanumérica dão a mesma tag.
- NL-3.4: `campoGravadoNaoTemTextoClaro`, lendo a coluna do índice no banco. Teste de dicionário: o valor gravado não pode ser igual a `SHA-256(CPF)` nem a `SHA-256(CNPJ)` das fixtures. Sabotagem: trocar o HMAC por SHA-256 puro e ver o teste ficar vermelho.
- NL-3.4: teste de separação de chaves. Uma tag gerada com `K_idx` não bate com a tag gerada com a chave de campo, para o mesmo valor.
- NL-3.4: CPF inválido (dígito verificador errado) não é indexado; a entrada é recusada, e não gravada com tag de um valor inválido.
- `[alterado pelo CISO, 25/10/2026]` NL-3.3: depois da rotação da DEK, `buscaPorCnpjPeloIndiceCego` continua achando o registro, e nenhuma tag antiga sobra na coluna. Sabotagem: rotacionar sem recalcular as tags e ver o teste vermelho.

## Riscos residuais

- Vazamento de igualdade: o número de tomadores repetidos e a frequência de cada um ficam visíveis a quem lê o banco. Aceito pela decisão do índice (spec 4.4).
- Com a DEK e o banco, a tag é reversível por dicionário. O mesmo vale para qualquer cópia de DEK (ADR-0003, C4).
- `[alterado pelo CISO, 25/10/2026]` Com DEK única (recomendação do CISO no ADR-0003), o mesmo tomador cadastrado por dois emissores da instalação tem a mesma tag. Quem lê o banco vê que os dois emissores atendem a mesma pessoa, sem ver quem é.

## Fontes

- Plano, seção 1.4 (porta P2), seção DA-1.1 e NL-3.4.
- Spec, seções 4.4 e 5.7.
- OWASP Key Management Cheat Sheet, lido em 25/10/2026: https://cheatsheetseries.owasp.org/cheatsheets/Key_Management_Cheat_Sheet.html
- RFC 5869 (HKDF), seção 3.2, lida em 25/10/2026: https://www.rfc-editor.org/rfc/rfc5869.html
- Pesquisa de CNPJ alfanumérico, não relida nesta sessão: `docs/pesquisa/onda1_cnpj_alfanumerico.md`.

## Revisão do CISO (25/10/2026)

**Revisor:** Narciso (CISO), distinto do security-engineer que escreveu este ADR (L-12). Modo autônomo (L-15).

**Veredito: PROPOSTO, recomendação do CISO: ratificar P2 como está escrito, com HMAC-SHA-256 de 256 bits sobre `tipo || valor normalizado`, `K_idx` derivada da DEK por HKDF com `info` própria, escopo só em CPF e CNPJ de tomador e emissor, e as mudanças marcadas `[alterado pelo CISO, 25/10/2026]`.** A ratificação de P2 é do líder (ponto L4).

**Decidido por mim (N5, registrado para ratificação):**

1. **O vazamento de igualdade é aceitável.** Quem lê o banco sem a chave vê repetição de tomador, e não o valor. O plano B (sem índice) não é acionado. O motivo é o modelo de ameaças (C3.2 e C3.3), e não o volume de cadastro de algum emissor (L-21).
2. **A rotação da DEK recalcula as tags** (decisão 6), com teste e sabotagem em NL-3.3.

**Por quê:** sem chave, o índice de CPF cai por dicionário de 10^11 valores e o de CNPJ por comparação com a base pública; com chave, quem tem só o banco não monta nenhum dos dois. Os riscos que sobram (igualdade entre linhas e entre emissores) não revelam o valor. A normalização de DA-1.1 precisa ficar fixa antes de gravar dado real, porque trocá-la depois obriga reindexar (porta P2).

**Sobe ao líder:** só a ratificação de P2, no pacote L4. A forma da chave de campo depende da recomendação de hierarquia do ADR-0003, que vai no mesmo pacote.
