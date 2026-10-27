# ADR 0003: envelope de campo e hierarquia de chaves (ADR-S2, porta P1)

**Status:** PROPOSTO, com recomendação do CISO (N4, 25/10/2026) na seção "Revisão do CISO (25/10/2026)" no fim. Porta P1, a escolha "DEK única ou por emissor", o mecanismo do envelope pelo A1 e o aceite do trade-off da cópia pelo A1 aguardam o líder (ponto L4, plano seção 6.1). Mudanças do CISO marcadas `[alterado pelo CISO, 25/10/2026]`.
**Data:** 25/10/2026.
**Origem:** plano, NL-1.9 e porta P1 (seção 1.4); spec 4.4 (decisões do líder: AES-256-GCM por campo, senha-mestra com Argon2id, chaveiro opcional, código de recuperação, cópia da chave pelo A1, senhas separadas).
**Relacionadas:** ADR-0002 (custódia do A1), ADR-0004 (índice cego), ADR-0006 (Argon2id).

## Contexto

- FATO, spec 4.4: a cifra é AES-256-GCM por campo de dado pessoal, com índice cego para busca. A chave mestra vem da senha-mestra com Argon2id, com chaveiro opcional e código de recuperação. Uma cópia da chave do banco fica cifrada com o A1 de cada emissor.
- FATO, lido em 25/10/2026, Secret portal: o segredo devolvido é "unique per application" e "does not change as long as the application is installed". Ele é uma chave-mestra por aplicação, não um lugar para guardar senha. A página avisa que o tamanho pode não bastar para o algoritmo e que a aplicação deve expandi-lo com um KDF (https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Secret.html).
- FATO, pesquisa Q6: falhas reportadas no Secret portal. KDE Discourse, 15/05/2024, com a causa afirmada de que o portal "generates a new secret every time it's called". Red Hat Bugzilla 2491140, 21/06/2026. Estado atual não verificado.
- FATO, lido em 25/10/2026, NIST SP 800-38D (PDF, texto extraído): IV de 96 bits é a recomendação (seção 5.2.1.1); tags de 128, 120, 112, 104 ou 96 bits, com 64 e 32 só em casos previstos no apêndice C (seção 5.2.1.2); o total de invocações da função de cifra com a mesma chave não pode passar de 2^32, com IVs gerados por RBG (seção 8.3).
- FATO, lido em 25/10/2026, RFC 8017 seção 7.1: RSAES-OAEP é parametrizado por hash e função de máscara, a escolha deve ficar fixa para a chave, e a nota da etapa 4 pede que o atacante não distinga as condições de erro.
- FATO, lido em 25/10/2026, RFC 5869: a seção 4 diz que o passo de extração "can concentrate existing entropy but cannot amplify entropy", e que senha exige um KDF de senha. A seção 3.3 aceita que o material de entrada "may already be present as a cryptographically strong key".
- FATO, lido em 25/10/2026, OWASP Key Management: uma chave deve ser usada para um só propósito; a chave deve ser cifrada por KEK antes de sair da forma em claro; a rotação considera o volume de uso, além do tempo.

## Decisão (proposta)

### Hierarquia de chaves

| Chave | Como nasce | O que protege | Como fica guardada |
|---|---|---|---|
| **DEK** (256 bits) | CSPRNG do OpenSSL, na criação da instalação | `[alterado pelo CISO, 25/10/2026]` Chave-raiz: não cifra nada diretamente; dela saem K_campo, K_a1 e K_idx (abaixo) | Só envelopada, em três cópias, mais a quarta do chaveiro se ativo (abaixo) |
| `[alterado pelo CISO, 25/10/2026]` **K_campo** (256 bits) | HKDF-SHA256(DEK, info `notalivre/campo/v1`) | Campos cifrados | Derivada no desbloqueio, não gravada |
| `[alterado pelo CISO, 25/10/2026]` **K_a1** (256 bits) | HKDF-SHA256(DEK, info `notalivre/conteiner-a1/v1`) | Contêiner A1 (ADR-0002) | Derivada no desbloqueio, não gravada |
| **KEK-senha** (256 bits) | Argon2id(senha-mestra, salt de 16 bytes), parâmetros em ADR-0006 | Cópia 1 da DEK | Não gravada; derivada no desbloqueio |
| **KEK-código** (256 bits) | HKDF-SHA256(código de recuperação, info `notalivre/kek-recuperacao/v1`) | Cópia 2 da DEK | Não gravada |
| **Envelope A1** | RSA-OAEP com SHA-256 (MGF1 com SHA-256, rótulo vazio), usando a chave pública do certificado. `[alterado pelo CISO, 25/10/2026]` Mecanismo pendente do líder para A1 sem `keyEncipherment` ou não RSA (Revisão do CISO, ponto 7) | Cópia 3 da DEK | Grava-se só o resultado cifrado, com a impressão SHA-256 do certificado ao lado (ADR-0002, decisão 8) |
| **KEK-chaveiro** (256 bits, opcional) | HKDF(segredo mestre do portal Secret, info `notalivre/kek-chaveiro/v1`) | Cópia 4 da DEK, só se o usuário ativar | Nunca grava o segredo; ele fica no portal |
| **Chave do índice cego** | HKDF(DEK, info `notalivre/indice-cego/v1`) | ADR-0004 | Derivada, não gravada |

- Por que HKDF no código de recuperação: a RFC 5869, seção 4, diz que o passo de extração não amplifica entropia, e por isso uma senha precisa de KDF de senha. O código tem 128 bits gerados por CSPRNG, então não é senha, e a seção 3.3 aceita material de entrada que já é chave forte. Por isso, o código não passa pelo Argon2id.
- O código de recuperação é formatado em Base32 com verificação, mostrado uma vez e confirmado pelo usuário. O app não o grava em claro. Onde ele fica (papel, cofre pessoal, outro) é decisão de Narciso (spec 4.4, em aberto).
- `[alterado pelo CISO, 25/10/2026]` O spec 4.4 marca "onde o código de recuperação fica e quem o custodia" como EM ABERTO, e EM ABERTO no spec é ponto do líder. Recomendação do CISO, sem decidir: o app nunca grava o código, mostra uma vez, oferece imprimir pelo portal Print e recomenda guardá-lo em papel, fora do computador e longe do `.pfx`.

### Regras

1. **Campo:** AES-256-GCM. Nonce aleatório de 96 bits por gravação (NIST 800-38D, 5.2.1.1). Tag de 128 bits (5.2.1.2). AAD = versão || tabela || coluna || id da linha || key_id, conforme a porta P1.
2. **Formato gravado:** versão (1 byte), key_id (1 byte), nonce (12 bytes), texto cifrado e tag (16 bytes). Versão ou key_id desconhecidos fazem o app recusar a leitura (fail closed).
3. **Limite de invocações:** o NIST limita a 2^32 o total por chave com IVs aleatórios. O app mantém um contador de cifragens por DEK, gravado no cofre. Ao chegar a 2^24, que é margem de 256 vezes abaixo do limite, a DEK é rotacionada e tudo é recifrado. O valor 2^24 é proposta, a confirmar por Narciso.
   - `[alterado pelo CISO, 25/10/2026]` **Decidido por Narciso: 2^24.** Regras do contador: (a) há um contador por chave que cifra com GCM (K_campo e K_a1; com a DEK como chave-raiz, ela não cifra; se o líder mantiver a DEK como chave de campo em P1, o contador é o da DEK); (b) toda cifragem conta, inclusive a do contêiner A1 e a recifragem de rotação; (c) o contador avança antes de cifrar, por reserva de bloco gravada e confirmada no banco antes do uso do bloco, e nunca diminui; reinício do app começa bloco novo, o que conta a mais e nunca a menos; (d) restaurar um backup antigo faz o contador voltar, e a margem de 256 vezes cobre esse caso; (e) chegar ao limite dispara a rotação da DEK (regra 8), que troca todas as chaves derivadas. O limite não se ajusta pelo volume de uso esperado de um emissor: vale igual para qualquer instalação.
4. **Desbloqueio:** qualquer um dos três fatores (senha-mestra, código de recuperação ou A1) abre a DEK. `[alterado pelo CISO, 25/10/2026]` Desbloqueio pelo código ou pelo A1 é recuperação e segue a ADR-0002, decisão 7: senha-mestra nova, código novo, re-envelope e registro na auditoria. A ausência dos três trava o cofre (fail secure). Com o chaveiro ativo, o desbloqueio passa pelo portal e dispensa a senha-mestra: é o trade-off que o usuário escolhe, e o controle C1 o deixa visível. O teste de "ausência dos três trava" roda com o chaveiro desligado. A falha de desbloqueio devolve uma mensagem única, "não foi possível desbloquear o cofre", sem dizer qual fator falhou. Isso segue a regra da RFC 8017 por analogia.
5. **Chaveiro (opcional, desligado por padrão):** o segredo do portal entra só como entrada de HKDF, nunca como senha. `[alterado pelo CISO, 25/10/2026]` Se a senha do certificado também for guardada pelo chaveiro (spec 4.5), a chave que a protege sai do mesmo segredo do portal com `info` própria, e nunca da DEK (ADR-0002, decisão 6). Na ativação, o app grava um valor de verificação, e a cada desbloqueio por chaveiro confere o segredo contra ele. Se o portal devolver outro segredo, o app recusa e pede a senha-mestra. Não há fallback para arquivo em claro.
6. **Troca de senha-mestra:** re-envelopa só a DEK, sem recifrar os campos. Exige desbloqueio atual.
7. **Troca ou vencimento de certificado:** re-envelopa a cópia pelo A1 com a chave pública do novo certificado, e apaga a cópia antiga no mesmo ato (ADR-0002, decisão 3).
8. **Rotação da DEK:** recifra todos os campos sob DEK nova, em uma transação, e atualiza o key_id.
9. `[alterado pelo CISO, 25/10/2026]` **AAD dos envelopes da DEK.** Cada cópia envelopada com AES-256-GCM (senha, código, chaveiro) leva AAD = rótulo `notalivre/envelope-dek/v1` || tipo do fator || key_id da DEK || versão do formato, e, na cópia da senha, os parâmetros do Argon2id gravados (ADR-0006). Trocar um envelope de lugar ou adulterar os parâmetros faz a tag falhar.
10. `[alterado pelo CISO, 25/10/2026]` **Novo emissor só com o cofre aberto por senha-mestra ou chaveiro.** Criar a cópia da DEK para o A1 de um emissor novo exige o cofre desbloqueado. Numa recuperação, isso só acontece depois da senha-mestra nova da regra 4. A tela de cadastro de emissor avisa que o A1 dele, com a senha, abre o cofre inteiro da instalação.

### Consequências e trade-off aceito pela porta P1

- **Trade-off (spec 4.4, herdado):** quem tiver o `.pfx` original, a senha do certificado e o banco com a cópia da DEK abre os dados pessoais, sem a senha-mestra. Isso é consequência direta da cópia pelo A1 decidida pelo líder, e não é defeito novo deste ADR. O que resta de proteção: a senha-mestra e o código continuam necessários para os outros dois caminhos, e o arquivo A1 não fica no banco quando o usuário escolhe a opção A (ADR-0002).
- **Mais de um emissor:** cada A1 abre a mesma DEK. Por isso, o A1 de qualquer emissor dá acesso ao cofre inteiro. **Pergunta para Narciso e o líder:** DEK única por instalação (recomendada, porque o índice cego e a busca entre emissores pedem isso) ou DEK por emissor (isola os dados, mas exige índice e migração por emissor).
- **Perda dos três fatores:** dados perdidos. As notas voltam pelo ADN. O cadastro local de tomadores não volta (spec 4.4, em aberto e na INBOX).

## Alternativas consideradas

- **Chave derivada da senha-mestra direto, sem DEK:** rejeitada. Trocar a senha obriga a recifrar tudo, e não há como acrescentar fator.
- **Chaveiro do sistema como única fonte de chave:** rejeitada. O portal tem falhas reportadas (FATO, pesquisa Q6), e uma fonte única de chave é um ponto único de falha.
- **XChaCha20-Poly1305:** não adotada agora. O líder decidiu AES-256-GCM (spec 4.4). Em CPUs com AES nativo, o GCM é mais rápido.
- **Biblioteca:** OpenSSL 3 para AES-GCM, RSA-OAEP, HMAC, HKDF e PKCS#12, que já é dependência do xmlsec1. Argon2id via libsodium, como decidido na pesquisa de stack (`docs/pesquisa/stack.md`). Não se implementa primitiva própria.

## Verificação

- NL-3.2: vetores de teste de AES-GCM (NIST), de Argon2id (RFC 9106, apêndice de exemplos) e de HKDF (RFC 5869, apêndice A). Vetores a conferir na implementação.
- `[alterado pelo CISO, 25/10/2026]` O vetor de Argon2id da RFC 9106 (seção 5.3) usa 4 lanes, segredo e dado associado, que `crypto_pwhash` não expõe, e não serve para este app. Os vetores e o oráculo independente de Argon2id estão na ADR-0006, seção Verificação.
- NL-3.3: os três fatores desbloqueiam; a ausência dos três trava; falha de um fator não revela qual. Sabotagem: aceitar desbloqueio sem fator, e o teste precisa ficar vermelho.
- NL-3.4: `linhaTrocadaDetectadaPelaAad`. Sabotagem: remover a AAD e o teste de troca de linha precisa ficar vermelho.
- NL-3.4: `campoGravadoNaoTemTextoClaro`, lendo os bytes gravados no banco.
- Teste de limite: contador próximo do limite (valor reduzido em teste) dispara a rotação. Sabotagem: desligar a rotação e ver o teste vermelho.
- Teste do chaveiro: segredo diferente no portal simulado recusa o desbloqueio e pede a senha-mestra.

## Controles compensatórios (desvio de risco, L-77)

Este ADR registra como **desvio de risco com controles compensatórios**, e não como conformidade, a cópia da DEK pelo A1 e o chaveiro opcional. Para cada controle, a verificação:

| # | Controle | Como se verifica | Estado |
|---|---|---|---|
| C1 | Chaveiro só como fator opcional, desligado por padrão | Teste de ativação e de desbloqueio; sabotagem com segredo trocado precisa dar vermelho | Proposto (NL-3.3) |
| C2 | Falha do portal fecha e pede a senha-mestra | Teste com portal simulado devolvendo segredo diferente | Proposto (NL-3.3) |
| C3 | Limite de invocações com rotação | Teste de contador e de rotação (ver acima) | Proposto (NL-3.2 e NL-3.3) |
| C4 | Cópia da DEK pelo A1 (trade-off da spec 4.4) | Aceite registrado de Narciso (N4) e do líder (L4). Revisão a cada mudança de certificado ou de desenho do cofre, e como item fixo da revisão de L-70 quando o schema ou a chave mudar | Aceite pendente |
| `[alterado pelo CISO, 25/10/2026]` C5 | Recuperação força senha-mestra nova e código novo | Teste em NL-3.3: depois de `desbloqueioComCodigoDeRecuperacao` e de `desbloqueioComA1ESenhaDoA1`, o cofre não libera uso sem a troca, e o código antigo deixa de abrir. Sabotagem: pular a troca e ver o teste vermelho | Proposto (NL-3.3) |
| `[alterado pelo CISO, 25/10/2026]` C6 | O envelope pelo A1 nunca usa a chave fora do `keyUsage` | Teste em NL-2.2 e NL-3.3 com fixture RSA só com digitalSignature: o importador não cria envelope RSA-OAEP para ela. Sabotagem: remover a checagem e ver o teste vermelho | Proposto (NL-2.2, NL-3.3) |
| `[alterado pelo CISO, 25/10/2026]` C7 | Orientação para guardar o `.pfx` fora do computador | Texto do assistente de primeiro uso conferido na revisão de NL-8.9 e a cada mudança dele | Proposto (NL-8.9) |

## Riscos residuais

- O trade-off do item C4 permanece enquanto a porta P1 valer.
- A falha de portal reportada em 2024 e 2026 pode continuar sem correção (FATO, pesquisa Q6). O controle C2 cobre o caso, pela via de pedir a senha.
- `[alterado pelo CISO, 25/10/2026]` FATO, PC AC FECOMERCIO MG RFB A1 v1.1 (agosto de 2025), lida em 25/10/2026: essa AC emite RSA de 2048 bits com digitalSignature, nonRepudiation e keyEncipherment (itens 6.1.1.3 e 6.1.7); o mínimo da ICP-Brasil, no item 7.1.2.7 f, é "deve conter o bit digitalSignature ativado, podendo conter os bits keyEncipherment e nonRepudiation ativados", e a extensão é crítica (7.1.2.2 b). Logo, `keyEncipherment` é opcional na ICP-Brasil, e o risco abaixo é real. INFERÊNCIA: o que as outras ACs fazem não foi lido.
- **Chave do A1 e uso da chave (INFERÊNCIA, a medir):** o envelope RSA-OAEP exige chave RSA do A1 com uso de cifragem de chave (`keyEncipherment`). Certificado com uso só de assinatura (`digitalSignature` ou `nonRepudiation`) não deve servir para transporte de chave, porque isso viola a regra de um propósito por chave (OWASP, lido). Se o A1 real for só de assinatura, ou for EC, a cópia pelo A1 não se sustenta e o desenho volta ao líder. A ADR não decide isso: o importador lê `keyUsage` e o tipo de chave, e recusa com erro próprio o que não servir (fail closed). Medir em NL-2.2 com um `.pfx` de teste e, quando houver, com o A1 real.

## Fontes

- Secret portal, lido em 25/10/2026: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Secret.html
- NIST SP 800-38D, PDF lido em 25/10/2026 (texto extraído; seções 5.2.1.1, 5.2.1.2 e 8.3): https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf. A página de aviso do NIST diz que a publicação está em revisão; conferir a versão vigente antes da implementação.
- RFC 8017 (PKCS #1 v2.2), seção 7.1, lida em 25/10/2026: https://www.rfc-editor.org/rfc/rfc8017.html
- RFC 5869 (HKDF), seções 3.2, 3.3 e 4, lida em 25/10/2026: https://www.rfc-editor.org/rfc/rfc5869.html
- OWASP Key Management Cheat Sheet, lido em 25/10/2026: https://cheatsheetseries.owasp.org/cheatsheets/Key_Management_Cheat_Sheet.html
- Pesquisa: `docs/pesquisa/flatpak.md` (Q5 e Q6); `docs/pesquisa/stack.md`.
- Spec 4.4 e plano, seção 1.4 (porta P1).
- `[alterado pelo CISO, 25/10/2026]` PC AC FECOMERCIO MG RFB A1, versão 1.1, lida em 25/10/2026: https://repositorio.serpro.gov.br/docs/pcacfecomerciomgrfba1.pdf
- `[alterado pelo CISO, 25/10/2026]` NIST SP 800-38D, seção 8.3, reconferida em 25/10/2026 no PDF (texto extraído): "The total number of invocations of the authenticated encryption function shall not exceed 2^32, including all IV lengths and all instances of the authenticated encryption function with the given key" (expoente sobrescrito perdido na extração do texto, lido como 2^32), para a construção por RBG.

## Revisão do CISO (25/10/2026)

**Revisor:** Narciso (CISO), distinto do security-engineer que escreveu este ADR (L-12). Modo autônomo (L-15).

**Veredito: PROPOSTO, recomendação do CISO: ratificar P1 com as mudanças marcadas `[alterado pelo CISO, 25/10/2026]`, DEK única por instalação, envelope pelo A1 na forma (a) do ponto 7 e aceite do trade-off N4 com os controles C4 a C7.** O aceite final de N4 e a ratificação de P1 são do líder (ponto L4, em bloco).

**Decidido por mim (dentro do escopo técnico, registrado para ratificação):**

1. **Limite de rotação 2^24** (ponto 3 do autor), com as regras do contador escritas na regra 3. A margem de 256 vezes sob o limite do NIST (seção 8.3, reconferida) cobre contagem perdida por restauração de backup. O limite não depende do volume de uso de nenhum emissor.
2. **Recuperação força troca** (regra 4, que aplica a ADR-0002 decisão 7, já aceita) e controles C5 a C7, com a verificação de cada um (L-77).
3. **Vetor de Argon2id da RFC 9106 fora da verificação**, substituído pelos da ADR-0006.

**Sobe ao líder no pacote L4, com a minha recomendação primeiro:**

- **Ponto 2, DEK única ou por emissor.** (a) **Recomendada: DEK única por instalação.** É a leitura direta da decisão do líder ("uma cópia da chave do banco fica também cifrada com o certificado A1 de cada emissor. Quem tem o .pfx e a senha dele recupera tudo", spec 4.4, paráfrase), e mantém um índice cego só. Custo: o A1 de qualquer emissor, com a senha dele, abre os dados de todos os emissores da instalação; a regra 10 deixa isso visível no cadastro. (b) DEK por emissor: isola emissores, mas cada A1 passa a recuperar só o próprio emissor, o que muda o "recupera tudo", e exige índice, rotação e migração por emissor.
- **AAD dos envelopes da DEK** (regra 9) e **novo emissor só com cofre aberto** (regra 10): entram na proposta de P1, porque mexem no formato e na hierarquia.
- **Mudança de hierarquia que recomendo para P1:** a DEK vira chave-raiz e não cifra nada diretamente; K_campo, K_a1 e K_idx saem dela por HKDF com `info` própria. Sem isso, a mesma DEK seria chave de AES-GCM e entrada de HKDF da chave do índice, o que contraria "uma chave, um propósito" (OWASP, lido pelo autor) e a frase da ADR-0004 de que "a chave de campo é outra". O custo é uma derivação por desbloqueio.
- **Ponto 5, trade-off N4.** Recomendação: **aceitar**, com o residual dito por inteiro. Quem tem a cópia do banco, o `.pfx` e a senha do certificado abre os dados sem a senha-mestra. Na prática, o `.pfx` costuma estar no mesmo disco do banco, e o PKCS#12 pode cair por força bruta offline se a senha for curta (ADR-0002, riscos residuais). Por isso, quem copia o disco pode estar a uma senha de certificado fraca dos dados pessoais e da chave de assinatura. O desenho não reduz isso sem mudar a decisão do líder; os controles são orientação (C7), troca forçada depois da recuperação (C5) e o aviso de cadastro (regra 10). Opções: (a) aceitar com os controles (recomendada); (b) aceitar e dar ao usuário, por emissor, a opção de desligar a cópia pelo A1, o que reduz o risco para quem guarda o `.pfx` no mesmo disco e tira dele esse caminho de recuperação; (c) rever o "1+2", que não recomendo.
- **Ponto 7, A1 só de assinatura ou não RSA.** FATO lido: `keyEncipherment` é opcional na ICP-Brasil (item 7.1.2.7 f da PC lida) e a extensão é crítica. Usar RSA-OAEP numa chave sem esse bit seria violar, por conta própria, o uso que o certificado declara; o importador recusa (C6). Para a decisão "1+2" continuar valendo para todo A1, sem mudar a decisão do líder, o requisito do envelope é: abrir só com o `.pfx` e a senha dele mais o banco; ser determinístico entre versões do OpenSSL; ser refeito na renovação. Opções:
  - (a) **Recomendada:** RSA-OAEP com SHA-256 quando a chave é RSA com `keyEncipherment`, que é o mecanismo padrão de transporte de chave para um certificado; para qualquer outro A1, KEK-A1 = HKDF-SHA256(segredo canônico da chave privada, info `notalivre/kek-a1/v1`), e a DEK envelopada com AES-256-GCM sob ela. Segredo canônico: para RSA, os primos p e q em ordem crescente, em big-endian de tamanho fixo; para EC, o escalar privado no tamanho fixo da curva. A derivação não faz nenhuma operação que o `keyUsage` regule, porque não gera assinatura nem texto cifrado verificável pela chave pública. O custo é ser construção sem norma própria, com dois caminhos de código e dois formatos de envelope (o byte de versão os separa).
  - (b) KEK derivada da chave privada para todo A1: um caminho só, mas sem norma também para o caso comum.
  - (c) Só RSA-OAEP: um A1 que não sirva fica sem o caminho de recuperação, e o app avisa no cadastro. Isso muda a decisão "1+2" para esses usuários e por isso não recomendo.
  - Em qualquer opção, medir em NL-2.2 com fixture e, quando houver, com o A1 real.

**Por quê:** P1 é porta de mão única (plano 1.4): depois de haver dado de usuário, trocar a hierarquia exige migração com a chave aberta em cada máquina e passa pelas 7 fases da L-70. Separar as chaves, fixar o envelope do A1 para todo tipo de certificado e deixar o trade-off N4 escrito com o residual real são coisas que custam pouco agora e muito depois.
