# ADR 0002: custódia do certificado A1 (ADR-S1)

**Status:** ACEITO por Narciso (CISO) em 25/10/2026 (N2), com a opção A e com as mudanças marcadas `[alterado pelo CISO, 25/10/2026]`. Ver a seção "Revisão do CISO (25/10/2026)" no fim.
**Data:** 25/10/2026.
**Origem:** recomendação do CTO no plano, seção NL-1.9; custódia do certificado é de Narciso (spec 4.5); modelo de ameaças em `docs/seguranca/modelo-de-ameacas.md` (C1.x e C4.x).
**Relacionadas:** ADR-0003 (envelope e hierarquia de chaves, porta P1), ADR-0005 (logs sem PII).

## Contexto

- FATO: o A1 é um arquivo `.pfx` ou `.p12` com senha (spec 4.5). Ele serve à conexão mTLS e à assinatura XMLDSIG, com o mesmo certificado ou de um mesmo CNPJ raiz (spec, linha 81, R1).
- FATO: o app é Flatpak e não tem acesso livre ao sistema de arquivos. O arquivo entra pelo FileChooser (`docs/pesquisa/flatpak.md`, Q7).
- FATO, lido em 25/10/2026: o portal Documents devolve um ID de documento em `Add()`, com caminho `/run/user/$UID/doc/$DOC_ID/`, descrito como "typically" em `GetMountPoint()`. A flag `persistent` decide se o acesso é só da sessão ou permanente. A página não diz se a permissão sobrevive a reinício (https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Documents.html).
- INFERÊNCIA, pesquisa Q7 (fórum GNOME, não confirmado): o caminho salvo pode ficar obsoleto depois de reinício.
- FATO, plano linha 58: `.pfx` com cifra antiga (RC2 ou 3DES) só abre com o provider legado do OpenSSL 3. INFERÊNCIA do CTO, a medir em NL-2.2.
- FATO, lido em 25/10/2026: OWASP Key Management Cheat Sheet manda não guardar chave em claro ("Keys should never be stored in plaintext format"), cifrar a chave com outra de força igual ou maior, e limitar o tempo em que a chave privada fica em texto claro (https://cheatsheetseries.owasp.org/cheatsheets/Key_Management_Cheat_Sheet.html).

## Decisão (proposta)

1. **O A1 entra no cofre como o próprio contêiner PKCS#12, sem transformação**, cifrado pela DEK (ADR-0003) como camada externa. A chave privada nunca é extraída para armazenamento, nem em claro nem sob a DEK em forma separada. Ela continua protegida pela senha do certificado dentro do contêiner.
   - Aceito do CTO: "cifrado pela DEK" e a troca do caminho por cópia.
   - Leitura contestada: se "importar para o cofre" significar extrair a chave privada. O plano (linha 401) diz "importado para dentro do cofre, cifrado pela DEK", sem dizer extração. Esta ADR fixa a forma verbatim do contêiner como correção dessa leitura. Extrair anularia a senha do certificado como segredo separado (spec 4.4 e 4.5).
2. **O caminho do arquivo original é dica de interface, não fonte de verdade.** Se o caminho falhar, o app pede o arquivo de novo pelo FileChooser. O caminho nunca entra em log (ADR-0005), porque traz o nome do usuário do sistema.
3. **Trocar de certificado apaga o contêiner antigo no mesmo ato**, depois de gravar o novo e confirmar a leitura (recomendação do CTO, INBOX do `TODO.md`).
4. **Uso em memória:**
   - A senha do certificado vai para um buffer dedicado, nunca para `QString` nem `std::string`, e é zerada logo após o uso.
   - EVP_PKEY, PKCS12 e QSslKey são liberados ao fim de cada operação (assinatura ou conexão).
   - A chave fica em memória só entre o desbloqueio e um dos gatilhos de bloqueio: 15 minutos parados, bloqueio de tela ou fechamento do app. Emissão em Produção sempre pede a senha (spec 4.5 e 4.12).
   - O "a cada 20 notas" fica desligado por padrão, como decidido pelo líder.
   - `[alterado pelo CISO, 25/10/2026]` O "buffer dedicado" é memória de segredo do libsodium, que já é dependência do projeto: `sodium_malloc` (ou `sodium_mlock` sobre o buffer), zeragem por `sodium_memzero` e liberação por `sodium_free`. Vale para a senha do certificado, a senha-mestra, a DEK e as KEKs. No início do processo, `RLIMIT_CORE` vai a 0, para que um core dump não grave segredo em disco. Verificação em NL-2.2 (teste do wrapper) e em NL-3.3 (DEK e KEK); não é pré-condição deste aceite.
5. **Recuperação "1+2" depende do arquivo original.** Ver a escolha A ou B abaixo. `[alterado pelo CISO, 25/10/2026]` Decidido: opção A.
6. `[alterado pelo CISO, 25/10/2026]` **Senha do certificado em repouso.** Se o usuário guardar a senha do certificado pelo chaveiro (decisão do líder, spec 4.5), ela só pode ficar em repouso cifrada sob a KEK-chaveiro (ADR-0003), nunca sob a DEK. Motivo: se a DEK abrisse a senha do certificado, a senha-mestra passaria a abrir a chave privada (senha-mestra, DEK, senha do certificado, chave privada), e a separação de senhas decidida pelo líder (spec 4.4 e 4.5) cairia. O desenho do armazenamento fica com NL-4.3; esta regra é o limite dele. `[alterado pelo CISO, 25/10/2026]` Ressalva: com o chaveiro ativo ao mesmo tempo para a senha-mestra (ADR-0003, regra 4) e para a senha do certificado, o segredo do portal sozinho chega à DEK e à senha do certificado, e portanto à chave privada. É escolha do usuário, mas a tela de ativação tem que dizer isso com essas palavras, e o teste de NL-4.3 confere que o aviso aparece.
7. `[alterado pelo CISO, 25/10/2026]` **Depois de qualquer recuperação** (pelo código ou pelo A1), o app exige senha-mestra nova e gera código de recuperação novo antes de liberar o uso, re-envelopa a DEK com os dois e grava o evento na auditoria (ADR-0005, seção 4). Sem isso, quem copiou o `.pfx` uma vez teria acesso silencioso e permanente pelo mesmo caminho, e um código já exposto continuaria válido.
8. `[alterado pelo CISO, 25/10/2026]` **Recuperação com certificado vencido.** O vencimento impede assinar e conectar, mas não impede abrir a cópia da DEK: na recuperação, o app aceita o A1 cuja cópia da DEK está gravada, mesmo vencido. Para saber qual cópia abrir sem guardar o certificado em claro, o envelope grava ao lado só a impressão SHA-256 do certificado. O certificado em si traz nome e CPF ou CNPJ do titular e fica só dentro do contêiner cifrado.
9. `[alterado pelo CISO, 25/10/2026]` **Tipo de chave e uso da chave.** O importador lê `keyUsage` e o tipo de chave do A1 e registra os dois. O que acontece com um A1 que não sirva ao envelope escolhido para a cópia da DEK depende da escolha do líder no ponto L4 (ADR-0003, "Revisão do CISO", ponto 7). Em nenhum caso o app usa a chave para uma operação que o `keyUsage` do certificado não autorize: na ICP-Brasil a extensão é crítica (FATO, PC AC FECOMERCIO MG RFB A1, v1.1, item 7.1.2.2 b).

## Escolha para Narciso: contêiner cifrado pela DEK ou sem essa camada

| Opção | O que é | Recuperação sem o arquivo original | Separação de segredos |
|---|---|---|---|
| **A (recomendada)** | Contêiner verbatim cifrado pela DEK. O original fica com o usuário. | Não. Exige o arquivo que o usuário fornece no momento da recuperação. | Melhor. A DEK sozinha não abre a chave privada; é preciso DEK e senha do certificado, e no caminho de recuperação, o arquivo. |
| B | Contêiner verbatim sem a camada da DEK, protegido só pela senha do certificado. | Sim, pelo cofre. | Pior. Cópia do banco mais senha do certificado dão acesso ao contêiner. |

- Recomendação: **A**, com aviso obrigatório no assistente de primeiro uso (NL-8.9) para guardar o `.pfx` original fora do app, e um teste de recuperação.
- INFERÊNCIA: a spec 4.4 escreve "1+2" para "recuperação pelo A1", e a leitura mais natural é que o "1" é o arquivo do certificado que o usuário guarda. Isso combina com A.
- Narciso decide. A escolha altera a recuperação, por isso é decisão de custódia, não só de implementação.
- `[alterado pelo CISO, 25/10/2026]` **Decisão de Narciso: opção A.** A B põe a chave privada ao alcance de quem tem só a cópia do banco e a senha do certificado. A chave privada do A1 é a credencial de assinatura do emissor na ICP-Brasil (assina a DPS e autentica no ADN e na SEFIN), e vale mais do que o cadastro. Com a A, a DEK sozinha não abre essa chave. A A também é a leitura literal da decisão do líder: "Quem tem o .pfx e a senha dele recupera tudo" (spec 4.4, paráfrase da rodada builder check).

## Alternativas consideradas

- **Guardar só o caminho:** rejeitada. O caminho não é estável (FATO, Documents) e revela o usuário do sistema. Não garante acesso depois de reinício (INFERÊNCIA, pesquisa Q7).
- **Extrair a chave privada e cifrá-la com a DEK:** rejeitada. Remove a senha do certificado da cadeia de proteção, contra a decisão do líder de senhas separadas (spec 4.4 e 4.5).
- **Re-embrulhar o contêiner antigo (RC2 ou 3DES) em PBES2 com AES-256 na importação:** não entra na v1. Exigiria a senha no momento da importação e não muda a proteção dada pela DEK. A abertura com provider legado fica restrita à importação e ao desbloqueio, e isso é registrado na NL-2.2.

## Consequências

- Qualquer uso do A1 exige o desbloqueio do cofre pela senha-mestra (ADR-0003).
- O banco contém o contêiner cifrado. Sem a DEK ele não serve para nada. Com a DEK e a senha do certificado, serve.
- Vencimento e substituição têm fluxo próprio (erro `Vencido` na NL-2.2).
- Logs e auditoria não recebem caminho, nome de arquivo nem conteúdo do certificado (ADR-0005).

## Verificação

- NL-2.2: erros distintos `SenhaErrada`, `CifraLegadaIndisponivel`, `Vencido`, `CadeiaInvalida` e `SemInscricao`, cada um testado com um `.pfx` de fixture.
- NL-2.2: teste de zeragem. O wrapper da senha zera o buffer ao sair de escopo. Sabotagem: remover o zeramento e ver o teste ficar vermelho, antes de declarar o teste forte (L-36).
- NL-3.4: `campoGravadoNaoTemTextoClaro` aplicado ao contêiner. Os bytes gravados no banco e em qualquer arquivo temporário não contêm a chave privada da fixture nem a senha da fixture.
- NL-3.3: sem senha-mestra, sem código e sem arquivo, o contêiner não abre (ausência dos três fatores trava).
- Revisão de código: nenhuma função de assinatura ou conexão recebe a senha do certificado como `QString` (a checar no portão da NL-2.2).

## Riscos residuais

- A zeragem é de melhor esforço. OpenSSL e Qt podem manter cópias internas. INFERÊNCIA, medir em NL-2.2.
- O campo de senha em QML guarda o texto como string JS imutável. INFERÊNCIA, sem fonte lida; medir em NL-4.3.
- QSslKey copia a chave para a conexão TLS. INFERÊNCIA, medir em NL-2.5.
- `[alterado pelo CISO, 25/10/2026]` **O `.pfx` original costuma ficar no mesmo disco do banco** (pasta de downloads ou de documentos do usuário). INFERÊNCIA sobre o hábito do usuário, sem medição. Quem copia o disco leva o banco e o `.pfx`, e falta só a senha do certificado. A proteção do PKCS#12 contra força bruta offline depende da cifra e da contagem de iterações com que a AC o gerou, e pode ser fraca: há contêineres legados com RC2 ou 3DES (plano linha 58, INFERÊNCIA do CTO a medir em NL-2.2), e a contagem de iterações do PKCS#12 é escolha de quem gerou o arquivo; senha curta cai rápido. O app não muda o `.pfx` do usuário. A mitigação é orientação: o assistente de primeiro uso (NL-8.9) recomenda guardar o `.pfx` fora do computador, depois de importado.

## Fontes

- Documents portal, lido em 25/10/2026: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Documents.html
- OWASP Key Management Cheat Sheet, lido em 25/10/2026: https://cheatsheetseries.owasp.org/cheatsheets/Key_Management_Cheat_Sheet.html
- Pesquisa, não relida nesta sessão: `docs/pesquisa/flatpak.md` (Q7, FileChooser e persistência).
- Plano: `docs/superpowers/plans/2026-10-25-notalivre-plano.md`, linhas 58 e 401 (recomendação do CTO).
- Spec: `docs/superpowers/specs/2026-10-25-notalivre-design.md`, seções 4.4 e 4.5.
- `[alterado pelo CISO, 25/10/2026]` PC AC FECOMERCIO MG RFB A1, versão 1.1 de agosto de 2025, lida em 25/10/2026 (texto extraído do PDF): itens 6.1.1.3 e 6.1.5 (RSA de 2048 bits), 6.1.7 (`keyUsage` com digitalSignature, nonRepudiation e keyEncipherment nessa AC), 7.1.2.2 b (Key Usage crítica) e 7.1.2.7 f (mínimo da ICP-Brasil: "deve conter o bit digitalSignature ativado, podendo conter os bits keyEncipherment e nonRepudiation ativados"): https://repositorio.serpro.gov.br/docs/pcacfecomerciomgrfba1.pdf
- `[alterado pelo CISO, 25/10/2026]` libsodium, memória segura (`sodium_malloc`, `sodium_mlock`, `sodium_memzero`): https://doc.libsodium.org/memory_management (não relida nesta revisão; conferir a API na implementação).

## Revisão do CISO (25/10/2026)

**Revisor:** Narciso (CISO), distinto do security-engineer que escreveu este ADR (L-12). Modo autônomo (L-15): as decisões abaixo estão no registro de decisões autônomas, para a ratificação do líder.

**Veredito: ACEITO** (N2). Todas as mudanças que exigi já estão escritas no corpo, marcadas `[alterado pelo CISO, 25/10/2026]`. O que resta é verificação nas fatias que este ADR já cita (NL-2.2, NL-2.5, NL-3.3, NL-4.3, NL-8.9), e não condição deste aceite.

**Decisões:**

1. **Opção A** (ponto 1 do autor). Contêiner PKCS#12 verbatim, cifrado pela DEK, com o `.pfx` original fora do app. A B entrega a chave privada a quem tem a cópia do banco e a senha do certificado, sem precisar do arquivo. A chave privada assina documento fiscal em nome do emissor; protegê-la vale mais do que proteger o cadastro.
2. **Senha do certificado em repouso só sob a KEK-chaveiro, nunca sob a DEK** (decisão 6). Preserva a separação de senhas decidida pelo líder.
3. **Recuperação força senha-mestra nova e código novo** (decisão 7), com registro na auditoria.
4. **Recuperação aceita o A1 vencido** cuja cópia está gravada, localizado pela impressão SHA-256 do certificado (decisão 8).
5. **Memória de segredo pelo libsodium e `RLIMIT_CORE` = 0** (decisão 4). Torna concreto o "buffer dedicado".
6. **O importador registra `keyUsage` e tipo de chave e nunca usa a chave fora do que o certificado autoriza** (decisão 9). O destino de um A1 que não sirva ao envelope é do líder (ADR-0003, ponto 7).

**Por quê:** o modelo de ameaças mostra que o ativo de maior impacto é a chave privada (C1.2, C1.3). Cada mudança fecha um caminho em que um segredo abriria outro que o líder quis separado, ou em que um acesso de uma vez viraria acesso permanente.

**Risco residual que registro, e não mitigo por desenho:** o `.pfx` original costuma ficar no mesmo disco do banco, e o PKCS#12 pode ser fraco contra força bruta offline (seção "Riscos residuais"). A mitigação é orientação no assistente de primeiro uso (NL-8.9). Isso alimenta o aceite do trade-off da cópia pelo A1, que é do líder (ponto L4, ADR-0003).

**Nada deste ADR sobe ao líder,** exceto o que já é dele no ADR-0003 (ponto 7 e trade-off N4).
