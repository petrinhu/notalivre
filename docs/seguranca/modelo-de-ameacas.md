# Modelo de ameaças (STRIDE): NotaLivre

**Status:** ACEITO por Narciso (CISO) em 25/10/2026 (N1), com as mudanças marcadas `[alterado pelo CISO, 25/10/2026]`. Ver a seção "Revisão do CISO (25/10/2026)" no fim.
**Data:** 25/10/2026.
**Escopo:** app desktop Linux, entregue só por Flatpak (spec 4.3), que emite NFS-e pela API do Sistema Nacional (SEFIN e ADN). Ativos: A1, senha-mestra, cofre, mTLS, XMLDSIG, credenciais SMTP, tokens OAuth, logs e sandbox Flatpak.
**Fora de escopo:** CI e build (ADR-0001); infraestrutura interna do ADN e da SEFIN (fora do nosso controle); A3 (fora da v1, spec 4.5); orientação de backup ao usuário (só recomendação de documentação).
**Marcação:** FATO traz a fonte; INFERÊNCIA é separada e deve ser medida ou confirmada; EM ABERTO vai para a decisão indicada.

## 1. Ativos

| ID | Ativo | Sensibilidade | Onde mora |
|---|---|---|---|
| A1-ARQ | Arquivo `.pfx`/`.p12` original (chave privada protegida pela senha do certificado) | Crítica | Fora do app, no computador do usuário |
| A1-COF | Cópia do contêiner A1 no cofre (ADR-S1) | Crítica | Banco MySQL/MariaDB, cifrada pela DEK |
| SM | Senha-mestra | Crítica | Só na memória, durante o desbloqueio |
| CR | Código de recuperação (128 bits) | Crítica | Papel do usuário; não gravado pelo app |
| SC | Senha do certificado | Crítica | Memória, por tempo limitado (spec 4.5) |
| DEK | Chave de dados (256 bits) | Crítica | Só envelopada no banco |
| KEK | Chaves de envelope (senha, código, A1, chaveiro) | Crítica | Derivadas; nunca gravadas em claro |
| CHV | Segredo mestre do portal Secret (chaveiro opcional) | Alta | Portal do sistema (keyring) |
| CAMPOS | CPF/CNPJ, nome, endereço, telefone, e-mail, inscrição municipal de tomador | Alta (dado pessoal, LGPD) | Banco, cifrado por campo |
| IDX | Índice cego de CPF/CNPJ (ADR-S3) | Média | Banco |
| DB | Credencial do usuário MySQL da aplicação | Alta | Config do app (a definir) |
| SMTP | Senha de app SMTP (Yahoo, iCloud, personalizado) | Alta | Banco, cifrada |
| TOK | Tokens OAuth (access e refresh) de Gmail e Outlook | Alta | Banco, cifrada |
| LOG | Logs técnicos | Média (deve estar sem PII, ADR-S4) | Disco local, `$XDG_STATE_HOME` do app |
| AUD | Registro de auditoria de ações sensíveis | Média | Disco local, append-only |
| XML | DPS assinada e retorno da nota (com os dados da época) | Média (dado fiscal e pessoal) | Banco |
| NUM | Numeração de notas (série e número) | Alta (fato fiscal, spec R4) | Banco |

## 2. Atores

- Emissor legítimo (usuário do app).
- Pessoa com acesso físico ou de sessão ao computador (família, colega, roubo).
- Atacante com acesso a cópia do disco ou do backup.
- Atacante na rede local ou no caminho até o ADN, SEFIN e provedores (MITM, DNS).
- Software malicioso rodando como o mesmo usuário fora da sandbox.
- Provedor terceiro curioso ou comprometido (CNPJ, OAuth, SMTP, consulta de CNPJ).
- Adversário na cadeia de distribuição ou de build (ADR-0001).

## 3. Fronteiras de confiança (DFD)

- **TB1:** interface QML e núcleo (Front, Mid, Back, Foundation). Mesmo processo; a camada de vista não guarda segredo.
- **TB2:** sandbox Flatpak e host. Pontes: FileChooser, Documents, Secret, Print e as permissões estáticas do manifesto.
- **TB3:** app e MySQL/MariaDB local. TCP em `127.0.0.1` (ver C8 sobre socket).
- **TB4:** app e ADN/SEFIN, por mTLS com certificado ICP-Brasil (spec R1, linha 81).
- **TB5:** app e provedores de e-mail e OAuth (Gmail API, Microsoft, SMTP de provedores).
- **TB6:** app e consulta de CNPJ (BrasilAPI principal, CNPJ.ws reserva; spec 4.8 e 4.14).
- **TB7:** usuário e app (teclado, senhas, decisões de confirmação).

## 4. Componentes e ameaças

Convenção das colunas: **STRIDE** (S, T, R, I, D, E); **ADR** que decide; **Risco residual**.

### C1. Custódia do A1 (A1-ARQ, A1-COF, memória). Fronteiras TB1 e TB2

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C1.1 | I | Outro processo do host lê o `.pfx` original | Sandbox; o FileChooser concede acesso só ao arquivo escolhido (FATO: manual Flatpak, `docs/pesquisa/flatpak.md` Q7) | ADR-S1 | Malware do mesmo usuário fora da sandbox |
| C1.2 | I | Cópia do contêiner A1 no banco vaza por backup ou disco | Contêiner cifrado pela DEK; sem a DEK o contêiner não tem uso | ADR-S1, ADR-S2 | Cópia do banco mais `.pfx` original mais senha do certificado abrem a DEK (trade-off da spec 4.4, aceito por N4) |
| C1.3 | I | Chave privada em memória é lida depois do uso | Zeragem dos buffers da senha e liberação de EVP_PKEY e PKCS12 ao fim de cada operação; bloqueio em 15 min parado, bloqueio de tela e fechamento | ADR-S1 | Zeragem é melhor esforço: cópias internas de OpenSSL e Qt (INFERÊNCIA, medir em NL-2.2) |
| C1.4 | I | Senha do certificado em log, erro ou tela | Proibido em log (ADR-S4); mensagem de erro sem o segredo | ADR-S4 | Baixo |
| C1.5 | T | Certificado de outro CNPJ ou cadeia inválida é aceito como A1 do emissor | Validação de CNPJ raiz e cadeia ICP-Brasil; erros distintos (SemInscricao, CadeiaInvalida) em NL-2.2 | ADR-S1 | Depende da regra de validação, a implementar |
| C1.6 | R | Emissor nega ter usado o A1 | Registro de auditoria com quem, o quê, quando e de onde (AUD) | ADR-S4 | Sem atestação de tempo externa; o relógio do sistema é premissa |
| C1.7 | D | Certificado vencido bloqueia a emissão | Erro próprio `Vencido`; fluxo de substituição | ADR-S1 | Operacional |
| C1.8 | E | Uso do A1 sem nova senha depois de bloqueio | Política de senha da spec 4.5 e 4.12 (Produção sempre pede) | ADR-S1 | Escolha de UX dos valores em aberto (spec 4.5) |
| `[alterado pelo CISO, 25/10/2026]` C1.9 | I | Core dump ou swap grava em disco a chave privada, a DEK ou uma senha | `RLIMIT_CORE` = 0 no início do processo; segredos em memória do libsodium (`sodium_malloc` ou `sodium_mlock`), que não vai para swap (ADR-S1, decisão 4) | ADR-S1 | Cópias internas de OpenSSL e Qt fora dessa memória (igual a C1.3) |
| `[alterado pelo CISO, 25/10/2026]` C1.10 | I | Senha do certificado guardada sob a DEK faria a senha-mestra abrir a chave privada | Em repouso, só sob a KEK-chaveiro, nunca sob a DEK (ADR-S1, decisão 6) | ADR-S1 | Desenho de NL-4.3 a revisar contra esta regra. Com o chaveiro ativo para a senha-mestra e para a senha do certificado, o segredo do portal chega à chave privada: escolha do usuário, avisada na tela de ativação |

### C2. Senha-mestra, KEK e código de recuperação (SM, CR, KEK). Fronteiras TB7 e TB1

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C2.1 | S | Binário ou build adulterado pede a senha-mestra ao usuário | Entrega só por Flatpak (spec 4.3); distribuição por canal assinado; CI com job privilegiado restrito | ADR-0001 | Cadeia de build (ADR-0001, C6 aberto) |
| C2.2 | T/I | Força bruta offline sobre o envelope da senha-mestra | Argon2id com memória de 64 MiB, 3 passes, salt de 16 bytes, via libsodium (ADR-S5). O paralelismo fica no valor interno da biblioteca, desvio registrado | ADR-S5 | Senha fraca escolhida pelo usuário (política mínima em aberto) |
| C2.3 | I | Senha-mestra vai para log, para QString ou para chaveiro sem necessidade | Proibição em log (ADR-S4); buffer dedicado (ADR-S1); chaveiro opcional e desligado por padrão (ADR-S2) | ADR-S2, ADR-S4 | Campo de texto QML guarda cópia em string JS imutável (INFERÊNCIA, medir em NL-4.3) |
| C2.4 | D | Usuário perde senha-mestra e código | Recuperação por A1 ("1+2" da spec 4.4), com a cópia da DEK envelopada pelo certificado (ADR-S2) | ADR-S2 | Sem A1 original e sem código, os dados cifrados se perdem (decisão do líder, spec 4.4); notas voltam pelo ADN, o cadastro local não |
| C2.5 | I | Código de recuperação anotado em lugar inseguro | Exibição única com confirmação; app não grava o código em claro | ADR-S2 | Custódia é do usuário; onde mora o código está em aberto (spec 4.4) |
| C2.6 | E | Troca de senha-mestra sem prova da senha atual | Troca exige desbloqueio atual e re-envelopa só a DEK (ADR-S2, regra 6) | ADR-S2 | Baixo |
| `[alterado pelo CISO, 25/10/2026]` C2.7 | E | Quem copiou o `.pfx` ou viu o código usa o mesmo caminho de recuperação de novo, sem rastro | Toda recuperação força senha-mestra nova e código novo, re-envelopa a DEK e grava na auditoria (ADR-S1, decisão 7) | ADR-S1, ADR-S2 | Uma recuperação indevida, uma vez, continua possível com `.pfx` e senha |
| `[alterado pelo CISO, 25/10/2026]` C2.8 | E | A1 sem `keyEncipherment` usado para RSA-OAEP, contra o `keyUsage` crítico do certificado | O importador lê `keyUsage` e tipo de chave e não usa a chave fora do que o certificado autoriza; mecanismo para esse A1 é do líder (ponto L4) | ADR-S1, ADR-S2 | `keyEncipherment` é opcional na ICP-Brasil (FATO, PC AC FECOMERCIO MG RFB A1 v1.1, item 7.1.2.7 f) |

### C3. Cofre de campos, DEK, índice cego e banco (CAMPOS, IDX, DEK, XML, NUM). Fronteira TB3

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C3.1 | T | Alguém com acesso ao banco troca valor de uma linha por outra | AES-256-GCM com AAD (tabela, coluna, id da linha, versão, key_id); teste `linhaTrocadaDetectadaPelaAad` (NL-3.4) | ADR-S2 | Apagar linhas continua possível (D) |
| C3.2 | I | Administrador do banco lê dados pessoais | Campos cifrados por DEK; sem DEK, só o texto cifrado | ADR-S2 | Metadados: tamanho, quantidade e igualdade entre linhas |
| C3.3 | I | Dicionário offline sobre CPF (10^11 valores) via índice cego | HMAC-SHA-256 com chave derivada da DEK, separada da chave de campo (ADR-S3) | ADR-S3 | Vazamento de igualdade (mesmo tomador repetido) |
| C3.4 | I | Roubo do disco expõe o MySQL | Cifra InnoDB ou MariaDB file_key_management; chave em arquivo no mesmo host (FATO, flatpak.md Q5) | ADR-S2 (nota) | Camada complementar, não principal: a chave fica no mesmo host |
| C3.5 | R | Nota emitida é alterada depois | XML assinado e retorno guardados; auditoria de alterações | ADR-S4 | Sem assinatura do próprio registro (proposta a avaliar) |
| C3.6 | D | Perda do banco | Recomendar ao usuário backup cifrado e fora do computador (documentação); recuperação da numeração em aberto (INBOX) | ADR-S2 | Numeração pode voltar a colidir se o banco se perder (E0014 e P3 do plano) |
| C3.7 | E | Injeção de SQL pelo código de aplicação | Executor parametrizado; consulta com concatenação não compila (NL-3.1) | ADR-S2 (executor) | Erro de implementação, coberto por teste |

### C4. mTLS com o ADN e a SEFIN. Fronteira TB4

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C4.1 | S | MITM ou DNS falso se apresenta como ADN | Verificação de cadeia e de nome do servidor; nunca desligar a verificação (anti-padrão do CONTRACT); teste com servidor de cadeia inválida (NL-2.5) | ADR-S1 | Confiança no armazenamento de raízes do runtime Flatpak |
| C4.2 | T | Alteração do XML em trânsito | TLS 1.2 no mínimo, 1.3 preferido; XMLDSIG por cima (C5) | ADR-S1 | Depende do que o ADN aceita (medir em NL-2.5) |
| C4.3 | I | Chave privada copiada para estrutura de conexão | QSslKey e QSslConfiguration mantêm cópia; liberar após a conexão | ADR-S1 | Cópia interna do Qt (INFERÊNCIA, medir) |
| C4.4 | D | Reenvio cego de POST depois de queda | Sem retry automático de POST (plano, TransporteHttps); retomada por GET /dps/{id} (spec 4.14) | ADR-S1 | Ponto para o retry com backoff da spec 4.6 |
| C4.5 | E | Certificado de outro CNPJ raiz usado na conexão | Mesmo certificado da assinatura, ou do mesmo CNPJ raiz (spec R1) | ADR-S1 | Depende da validação de C1.5 |

### C5. XMLDSIG da DPS e leitura do retorno. Fronteiras TB1 e TB4

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C5.1 | T/E | Signature wrapping: o retorno tem uma assinatura válida sobre elemento que não é o consumido | Uma única Reference, apontando para o Id do `infDPS` ou `infNFSe` que o app lê; comparação do elemento assinado com o elemento usado; teste de XSW | Sem ADR nova; pendência DA-1.4 | Regra da Reference não fixada no manual extraído (`docs/pesquisa/onda1_leiaute.md` 1.2) |
| C5.2 | I/D | XXE e entidades externas no parser XML | Parser sem DTD, sem entidades externas e sem rede (controle proposto para NL-1.6 e NL-2.3) | Sem ADR nova | Depende da implementação |
| C5.3 | S | Assinatura feita com chave de outro emissor | Chave vem do cofre do emissor escolhido; validação do CNPJ raiz | ADR-S1 | Baixo |
| C5.4 | R | Negar que a DPS foi assinada | Registro de auditoria da assinatura (sem conteúdo da chave) | ADR-S4 | Igual a C1.6 |

### C6. Credenciais SMTP e tokens OAuth (SMTP, TOK). Fronteiras TB5 e TB2

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C6.1 | S | Cliente OAuth desktop não guarda segredo | Cliente público com PKCE S256 e redirecionamento para `127.0.0.1` em porta aleatória, no navegador do sistema (FATO: `docs/pesquisa/email.md` Q1 e Q3; Google "Desktop apps are public clients") | ADR-S2 (cofre) | Outro processo local pode escutar a porta; reduzido por `state` e PKCE |
| C6.2 | I | Refresh token em claro ou em log | Tokens no cofre, cifrados pela DEK (ADR-S2); proibido em log (ADR-S4) | ADR-S2, ADR-S4 | Baixo |
| C6.3 | I | Senha de app SMTP em arquivo de configuração | Guardada só no cofre, cifrada; a configuração mora no app, e não em arquivo de ambiente (decisão do líder de 25/10/2026, registrada no CLAUDE.md do projeto) | ADR-S2 | Baixo |
| C6.4 | E | Escopo amplo demais no OAuth | Gmail com `gmail.send` (sensível, não restrito: FATO email.md Q2), evitando `mail.google.com` (restrito); Outlook com `SMTP.Send` e `offline_access` | ADR-S2 | Limite de 100 usuários para app não verificado (FATO email.md Q2), risco de distribuição |
| C6.5 | R | Envio de e-mail sem rastro | Auditoria de envio (destinatário mascarado, sem corpo e sem anexo) | ADR-S4 | Baixo |

### C7. Logs e auditoria (LOG, AUD). Fronteira TB1 e disco local

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C7.1 | I | PII ou segredo entram no log | Lista de campos permitidos (allowlist), mascaramento da inscrição do emitente, testes com varredura não vazia (NL-2.7) | ADR-S4 | Texto de exceção de biblioteca de terceiro (INFERÊNCIA) |
| C7.2 | T | Alteração do log pelo mesmo usuário | Auditoria encadeada por hash, a propor no ADR-S4; log sem tamanho ilimitado | ADR-S4 | O dono da máquina pode apagar tudo: não há servidor de auditoria (sem telemetria, spec 3.2) |
| C7.3 | I | Caminho do arquivo A1 ou nome de usuário no log | Caminho nunca é registrado; só o identificador do emissor | ADR-S4 | Baixo |
| C7.4 | T | Injeção de linha falsa no log (CRLF) | Sanitização de quebras de linha em todo campo (OWASP Logging Cheat Sheet, lido) | ADR-S4 | Baixo |
| C7.5 | D | Log enche o disco | Rotação e teto de tamanho (proposta no ADR-S4) | ADR-S4 | Baixo |
| `[alterado pelo CISO, 25/10/2026]` C7.6 | I | Mensagem do Qt, do QML ou de biblioteca vai crua para a saída de erro e para o journal do usuário, fora da allowlist | `qInstallMessageHandler` manda tudo pelo `Log`; erro de OpenSSL e xmlsec só por código; texto livre da API não entra (ADR-S4, seção 3) | ADR-S4 | Biblioteca que escreve direto na saída de erro sem passar pelo Qt (INFERÊNCIA, conferir em NL-2.7) |

### C8. Flatpak e sandbox (permissões, portais, rede). Fronteira TB2

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C8.1 | E | Permissões estáticas excessivas no manifesto | Sem `--filesystem`; arquivos por portal (FileChooser, Documents); Secret por portal, não `--talk-name` (FATO: Flathub, "Static permissions must be kept to an absolute minimum" e "Applications should rely on XDG Portals", lido em 25/10/2026) | ADR-0001 e este modelo | Revisão do manifesto em cada mudança (a fazer) |
| C8.2 | I | Acesso ao socket do MySQL do host exigiria `--filesystem` | Conexão só por TCP em `127.0.0.1` (decisão de desenho, a confirmar no assistente da NL-3.6); `--share=network` já é necessário para a API | ADR-S2 (nota) | Qualquer processo local pode abrir TCP em 127.0.0.1; mitigação: usuário de banco com privilégio só na base da app |
| C8.3 | T | Instalação de `.flatpak` adulterado | Distribuição por Flathub ou canal assinado; instalar sobre versão antiga com cache de permissões é risco (FATO: Podman Desktop troubleshooting, pesquisa) | ADR-0001 | Cadeia de distribuição |
| C8.4 | E | Escape da sandbox pelo runtime | Atualizar o runtime KDE pela cadência de patch (crítico em 7 dias) | Este modelo | Residual do runtime |
| C8.5 | I | Secret portal entrega segredo diferente a cada chamada (relato de 2024) | Teste de verificação na ativação e a cada desbloqueio; falha fecha e pede a senha-mestra | ADR-S2 (regra 5) | Falha de portal reportada, sem correção confirmada (FATO: flatpak.md Q6) |

### C9. Consulta de CNPJ (terceiros). Fronteira TB6

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C9.1 | I | CNPJ de tomador (dado pessoal se for MEI) vai para terceiro sem base | Consentimento antes da consulta (spec 4.8); texto de consentimento revisado por Cláudio (spec 4.14) | Este modelo | Termos da BrasilAPI e do CNPJ.ws não lidos (spec 4.8) |
| C9.2 | T | Resposta do terceiro altera endereço do tomador sem confirmação | Mostrar a diferença e pedir confirmação antes de gravar (spec 4.8, rodada 6) | Este modelo | Baixo |
| C9.3 | D | Limite de taxa do terceiro (CNPJ.ws: 3 por minuto por IP, FATO) | Fila e mensagem de limite; sem laço de retry | Este modelo | Operacional |

### C10. Entrada de senha e interface (TB7)

| ID | STRIDE | Ameaça | Mitigação | ADR | Risco residual |
|---|---|---|---|---|---|
| C10.1 | I | Senha fica em memória da interface e em cache de tela | Campo sem histórico e sem cópia para área de transferência; nada de log de eventos da UI | ADR-S1 | Residual de QML (INFERÊNCIA, medir em NL-4.3) |
| C10.2 | S | Janela falsa pede senha | Entrega por Flatpak; sem conteúdo remoto na tela de senha | ADR-0001 | Baixo |

## 5. Riscos residuais de maior impacto

1. Cópia do banco mais `.pfx` original mais senha do certificado abrem os dados pessoais (C1.2). Trade-off da spec 4.4, herdado da porta P1, aceito ou não por Narciso (N4). `[alterado pelo CISO, 25/10/2026]` Narciso recomenda aceitar, e o aceite final é do líder (ponto L4). Residual real: o `.pfx` costuma ficar no mesmo disco, e o PKCS#12 pode cair por força bruta offline se a senha for curta.
2. Perda simultânea de senha-mestra, código e arquivo A1 (C2.4). Perda de dados, decisão do líder.
3. Cadeia de build e distribuição (C2.1, C8.3). Depende de ADR-0001 e da revisão do job privilegiado (C6 da ADR-0001, aberto para o líder).
4. Cópias de memória fora do controle (C1.3, C4.3, C10.1). Mitigação parcial, medir.
5. Signature wrapping no retorno (C5.1), com a regra de Reference ainda não fixada no manual extraído (DA-1.4).

## 6. Abuse cases

1. Atacante copia o disco do emissor: encontra o banco cifrado e o contêiner A1 cifrado. Sem senha-mestra nem código, não abre (C1.2, C3.2). O caso que abre é a cópia do `.pfx` original mais a senha do certificado (C1.2). `[alterado pelo CISO, 25/10/2026]` Se o `.pfx` estiver no mesmo disco, o atacante já o tem, e o que falta é só a senha do certificado, que ele pode atacar offline contra o próprio `.pfx`. Com ela, leva os dados pessoais e a chave de assinatura do emissor. A mitigação é orientar o usuário a guardar o `.pfx` fora do computador (NL-8.9).
2. Atacante com acesso à sessão desbloqueada: a política de 15 minutos e o bloqueio de tela limitam a janela (C1.8). Emissão em Produção sempre pede a senha (spec 4.12).
3. Atacante na rede com certificado falso para o ADN: a verificação de cadeia reprova (C4.1). Se a verificação estiver desligada, o caso é incidente de release.
4. Atacante que lê o log: não encontra CPF, CNPJ, senha, token ou chave (C7.1), desde que a varredura de NL-2.7 esteja verde e tenha sido provada vermelha.

## 7. Premissas

- O sistema operacional do usuário não está comprometido por malware com o mesmo privilégio do app (fora deste modelo).
- O runtime Flatpak e a sandbox funcionam como documentados.
- OpenSSL 3, libsodium, libxml2 e xmlsec1 recebem correção de CVE pela cadência de patch declarada (crítico em menos de 7 dias, alto em menos de 30 dias, premissa da política de vulnerabilidades do projeto, a formalizar).
- O usuário guarda o `.pfx` original, a senha-mestra e o código de recuperação fora do app, em lugares que ele controla.
- O relógio do sistema está correto (validade do certificado e tempo de auditoria).

## 8. Pendências que alteram este modelo

- ~~Narciso: aceitar ou contestar ADR-S1 a ADR-S5 (N2 a N6).~~ `[alterado pelo CISO, 25/10/2026]` Feito em 25/10/2026: ADR-S1, ADR-S4 e ADR-S5 aceitos; ADR-S2 e ADR-S3 propostos com recomendação.
- Líder: ratificar P1 e P2 (ponto L4), e decidir a pergunta de desenho "DEK única ou por emissor" (ADR-S2). `[alterado pelo CISO, 25/10/2026]` No mesmo pacote: o mecanismo do envelope pelo A1 para certificado sem `keyEncipherment` ou não RSA (C2.8), o aceite do trade-off N4 (seção 5, item 1) e o lugar do código de recuperação (spec 4.4, EM ABERTO).
- Regra de Reference da assinatura (C5.1), pendência DA-1.4.
- Relação entre a chave de acesso (50 posições) e o Id da NFS-e (53 posições), INFERÊNCIA a conferir no manual (ADR-S4).
- Termos de uso da BrasilAPI e do CNPJ.ws (C9.1).

## Revisão do CISO (25/10/2026)

**Revisor:** Narciso (CISO), distinto do security-engineer que escreveu este modelo (L-12). Modo autônomo (L-15).

**Veredito: ACEITO** (N1). O modelo cobre os ativos e as fronteiras que a spec define, separa FATO de INFERÊNCIA e liga cada ameaça a um ADR. Acrescentei as linhas C1.9, C1.10, C2.7, C2.8 e C7.6 e corrigi o abuse case 1 e o risco 1 da seção 5, marcados `[alterado pelo CISO, 25/10/2026]`.

**Por quê:** o abuse case 1 dizia que o caso que abre é ter o `.pfx` e a senha, sem dizer que o `.pfx` costuma estar no mesmo disco copiado e que a senha dele pode ser atacada offline. Esse é o risco residual de maior impacto do produto, e o líder precisa vê-lo como é para decidir o trade-off N4. As outras linhas cobrem caminhos que o modelo não tinha: segredo indo para disco por core dump ou swap, a senha do certificado sob a DEK, reuso silencioso de um caminho de recuperação, uso da chave fora do `keyUsage` e saída crua do Qt para o journal.

**Pendências deste modelo que seguem abertas:** as da seção 8, mais a revisão do manifesto Flatpak em cada mudança (C8.1), que fica com a Onda 9.
