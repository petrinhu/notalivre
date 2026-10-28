# NotaLivre: documento de design (spec de produto)

**Status:** v1 APROVADA pelo líder em 25/10/2026 ("Aprovo", AskUserQuestion), após o builder check. As decisões da seção 4.14 continuam "a confirmar" até ratificação explícita. Ratificação parcial: o líder aprovou o playback das partes 1 e 2 da rodada 8 com "Está certo, siga" (memória do projeto).
**Data:** 25/10/2026.
**Natureza:** organização das decisões que o líder já tomou no brainstorm e no builder check. As propostas do orquestrador estão separadas, na seção 4.14, e aguardam confirmação do líder. Onde faltou decisão, está escrito EM ABERTO.

## Legenda de rótulos

- **Decisão do líder:** tomada pelo líder na rodada indicada. Entre aspas, só quando é verbatim (arquivo de memória do projeto). Sem aspas, é paráfrase da rodada indicada.
- **Decisão do orquestrador, a confirmar:** proposta do orquestrador, não aprovada pelo líder.
- **FATO:** dado com fonte. As URLs estão na seção 5.
- **Inferência de pesquisa:** conclusão de agente pesquisador, não confirmada em texto oficial. Não é decisão.
- **Preenchimento de C-level:** opinião de agente C-level (porte, LGPD, fiscal). Não é decisão.
- **EM ABERTO:** o líder ainda não decidiu. Não preencher por presunção.
- **JÁ RESPONDIDO (rodada N):** item que estava EM ABERTO e que o líder decidiu depois. Traz a rodada de origem. Quando a rodada não consta no arquivo de memória, a linha diz isso.

**Fontes de decisão:** memória local do projeto, não versionada (rodadas 1 a 8 e rodada do builder check). **Brief original:** `docs/origem/prompt-emissor-nfse-nacional.md`. **Pesquisa fiscal aprofundada (25/10/2026):** `docs/pesquisa/fiscal_aprofundada.md`, usada na seção 6. Quando brief e memória conflitam, a memória prevalece (ver 4.0).

## 1. Intenção e porquê

**O que é.** Aplicativo desktop para Linux que emite NFS-e pela API oficial do Sistema Nacional NFS-e (SEFIN Nacional e ADN). Inclui impressão do DANFSe, envio por e-mail, emissão em lote e cadastro de tomadores. Nome: NotaLivre (decisão do líder, rodada 6; ver 4.1).

**Para quem (consumidor declarado).** MEI, autônomos e pequenas empresas do Simples Nacional. Escritórios contábeis ficam fora da v1 (paráfrase da resposta do líder na rodada 2). O projeto é para distribuição a terceiros (decisão do líder, rodada 1: emissores = "Distribuir a terceiros").

**Regra de canon (L-21).** Uso ou contagem de uso por um consumidor identificado nunca corta escopo, desenho ou qualidade deste produto. A dor de um consumidor é evidência de lacuna, não prova de que algo pode sair.

**Problema, como o líder descreveu.** Emitir NFS-e nacional sem depender de sistema pago, com impressão, envio por e-mail, lote e cadastro de tomadores. Verbatim do líder (rodada 1): "Quero fazer um app linux, qt[latest]."

**Por que agora.** EM ABERTO. O líder não respondeu à pergunta "Por que agora?". Fatos que mudam o cenário em 2026 estão na seção 5: a geração do DANFSe pela API foi suspensa em 03/08/2026, o CNPJ passou a aceitar formato alfanumérico e o leiaute inclui IBS/CBS.

**O que acontece se não fizermos.** EM ABERTO. O líder não respondeu.

**Métrica de sucesso.** EM ABERTO. O líder não definiu métrica nem threshold. Sem isso, o documento não tem critério de sucesso fechado.

**Sequência de entrega.** Verbatim do líder (rodada 1): "Ainda estou sem minha chave AC1, mas quero fazer o que já for possível sem a chave." AC1 é lido como certificado A1, conforme a memória do projeto, que cita o .md do brief. O teste sem A1 está em 4.13.

## 2. Objetivos, não-objetivos e anti-objetivos

### 2.1 Objetivos da v1

| # | Objetivo | Origem |
|---|---|---|
| O1 | Emitir NFS-e nacional pela API oficial, começando em Produção Restrita e trocando para Produção por configuração | Brief, decisão do líder na rodada 5 (ambiente por emissor) |
| O2 | Imprimir a nota (DANFSe) | Verbatim do líder (rodada 1): "Quroo poder imprimir a nota" |
| O3 | Enviar a nota por e-mail, com configurações SMTP nas configurações do app | Verbatim do líder (rodada 1): "quero poder enviar por email colocando configs smtp nas configs do app." |
| O4 | Emitir como CPF ou CNPJ, com CNPJ como padrão | Verbatim do líder (rodada 1): "Quero opcao de emitir como cpf ou cnpj, cnpj por default." |
| O5 | Abrir com o último emissor utilizado | Verbatim do líder (rodada 1): "App deve abrir com o último usuário emissor utilizado." |
| O6 | Emissão em lote | Verbatim do líder (rodada 1): "Quero emissão de notas em lote." Regras nas rodadas 3 e 5 |
| O7 | Senha do certificado com opções de uso | Verbatim do líder (rodada 1). Regras na rodada 3 |
| O8 | Gravar tomadores já emitidos (CPF ou CNPJ, nome) e manter o cadastro | Verbatim do líder (rodada 1): "quero que o app grave cpf/cnpj/nome de tomadores que já tiveram nf já emitidas". Rodadas 3 e 6 |
| O9 | Criptografia de alto nível no banco | Verbatim do líder (rodada 1): "quero cripto de alto nível no banco". Rodada 4 |
| O10 | Cancelar, substituir, usar modelos de nota e baixar notas do governo (ADN) | Rodada 5 (paráfrase) |
| O11 | Regimes MEI e Simples Nacional, com grupo IBS/CBS | Rodada 8 (paráfrase) |
| O12 | Atualizar tabelas fiscais ao abrir e manualmente | Rodada 8, verbatim (ver 4.10) |
| O13 | LGPD: editar, excluir e exportar dados de um tomador; apagar inativos | Rodada 3 (paráfrase) |
| O14 | Modelos de e-mail com menu dropdown | Rodada 7, verbatim (ver 4.9) |
| O15 | Interface moderna em QML e Qt Quick | Verbatim do líder (rodada 2): "acione a skill de frontend para fazer um app moderno e bonito". Rodada 4 |
| O16 | CI em Fedora, Ubuntu, Manjaro e Mint | Verbatim do líder (rodada 1). Rodada 4 (ver 4.3) |
| O17 | Arquitetura em camadas bem definidas | Verbatim do líder (rodada 1): "quero um app bem atomico, com camadas bem definidas" |

### 2.2 Não-objetivos da v1 (decisão do orquestrador, a confirmar)

- Certificado A3 (token ou cartão).
- Windows e macOS.
- Idiomas além de pt-br.
- Escritórios contábeis multi-cliente.
- Telemetria ou nuvem.
- Lucro Presumido: JÁ RESPONDIDO (rodada 8). Regimes da v1 são MEI e Simples Nacional (ver 4.11). Não está nesta lista de propostas do orquestrador.

### 2.3 Anti-objetivos (fracasso mesmo com o app funcionando; decisão do orquestrador, a confirmar)

- Emitir nota real por engano.
- Vazar certificado, senha ou dado pessoal.
- Aceitar nota com dado fiscal errado sem aviso.
- Perder a numeração da DPS em lote.

## 3. Restrições

### 3.1 Legais e fiscais

- **R1 API.** Só a API oficial SEFIN Nacional e ADN (brief). A autenticação é mTLS com certificado ICP-Brasil. O XML da DPS é assinado em XMLDSIG. O certificado da conexão e o da assinatura são o mesmo, ou do mesmo CNPJ raiz (brief). FATO: a transmissão exige cadeia ICP-Brasil válida, certificado não revogado e validade (regras E1203 a E1208, Anexo I).
- **R2 Ambientes.** Produção Restrita (notas sem validade fiscal) e Produção (notas com validade fiscal). As URLs estão no brief. FATO: o caminho base da API não foi confirmado, porque o Swagger de Produção Restrita respondeu 403 na pesquisa.
- **R3 DANFSe.** FATO: a API de geração do DANFSe seria suspensa em 03/08/2026 (NT 008 v1.02, item 1). Decisão do orquestrador, a confirmar: o DANFSe é gerado localmente, no leiaute da NT 008. Em emissão com tpAmb=2, o DANFSe deve conter "NFS-e SEM VALIDADE JURÍDICA" (FATO, NT 008). O "modelo v2.0" do DANFSe não está confirmado; não usar como fato.
- **R4 Numeração.** FATO: o Id da DPS tem 45 posições. A série 00001 a 49999 é do aplicativo próprio. A combinação série, número, município e inscrição é única (E0014). O número nNFSe é atribuído pela Sefin. Número reservado e não persistido gera "pulos", e não pode ser reutilizado (NT 007, p. 9).
- **R5 Lote.** FATO: o leiaute não tem lote. Inferência de pesquisa, com força alta: a API é síncrona, com uma DPS por chamada. O lote é função do app.
- **R6 CNPJ alfanumérico.** FATO: a NT 009 §2.1 passa todos os campos CNPJ de numérico (N) para alfanumérico (C), a partir de julho de 2026. Decisão do orquestrador, a confirmar: tratar o CNPJ como texto. DV do formato alfanumérico: algoritmo documentado; pendências em a3 e a4 de `docs/pesquisa/onda1_cnpj_alfanumerico.md`.
- **R7 Pessoa física.** FATO: o CPF é aceito como identificador do prestador (tpInsc=1). Pelo E0099, o CPF precisa ter domicílio no município emissor. Pelo E0675, o emitente CPF não pode preencher PIS/COFINS (tribFed) [só no Anexo de 09/02, linha Excel 517; no de 20/10 o E0675 não existe, e o caminho valores/trib/tribFed tem E0676, linha Excel 539, com critério MEI, igual ao do de 09/02 (linha Excel 518)]. EM ABERTO: a regra oficial de quem pode emitir como pessoa física não foi encontrada.
- **R8 MEI e Simples.** FATO: opSimpNac=2 identifica o MEI. A regra E1302 (MEI só informa vLiq no grupo valores) trata do grupo valores da NFS-e que a Sefin gera, não da DPS (FATO, Anexo I de Produção Restrita de 20/10/2026, aba RN DPS_NFS-e 1.01, linha Excel 49: B49 = `NFSe/infNFSe/`, C49 = `valores`); por isso não há regra local E1302 sobre a DPS (DA-1.22 do plano, decisão autônoma do CTO, a ratificar pelo líder). FATO: o grupo IBS/CBS é obrigatório para optantes do Simples a partir de 2027 (nota I336 do Anexo I v1.01). Aplicar essa regra ao MEI é inferência de pesquisa, não texto normativo.
- **R9 IBS/CBS.** FATO: o grupo IBSCBS é opcional (0-1) no leiaute v1.01. A DPS não traz alíquota, base de cálculo nem valor de IBS ou CBS. Esses valores são calculados pela Calculadora (inferência). As tabelas de classificação mudam várias vezes por ano (secundário, ver seção 6).
- **R10 Endereço do tomador.** FATO: a NT 009 §2.10 diz que o endereço do tomador é validado contra o cadastro CNPJ ou CPF, e que a DPS é rejeitada se o código IBGE divergir. Isto conflita com a leitura anterior da pesquisa de tomador (ver 4.0, C7).
- **R11 Cadastro de terceiro.** FATO: os manuais v1.2 da API e ADN não têm endpoint de consulta nem de escrita de cadastro de tomador ou de outra pessoa. Os únicos POST são /nfse e /nfse/{chave}/eventos.
- **R12 Fiscal.** Pesquisa fiscal aprofundada concluída em 25/10/2026, com fontes primárias e secundárias marcadas. Ver seção 6, que contém prazos, pontos para o contador e a condição de go/no-go para emissão em Produção para terceiros.

### 3.2 LGPD

- O cadastro de tomador é dado pessoal: CPF, nome, endereço, telefone, e-mail e inscrição municipal. Decisão do líder, rodadas 1 e 3: LGPD entra no escopo, com retenção e exclusão.
- Papel do autor e do usuário: Preenchimento de C-level (Cósimo). O prestador que instala o app seria o controlador, e o autor seria fornecedor, salvo se houver telemetria, sincronização ou suporte com acesso aos dados. A validar com Cláudio.
- Direitos do titular (exportação, exclusão e retenção, art. 18 da LGPD): Preenchimento de C-level (Cósimo), a validar.
- Apagar inativos entra na v1: JÁ RESPONDIDO (rodada 3, LGPD com "apagar inativos após N anos"). O valor de N segue como decisão do orquestrador, a confirmar: 5 anos, configurável, até o contador confirmar a guarda fiscal (ver 4.14).
- EM ABERTO para o contador: o prazo de guarda fiscal das NFS-e não foi pesquisado. Pode haver conflito entre a exclusão de tomador inativo e a guarda fiscal da nota emitida.

### 3.3 Plataforma

- **Entrega somente em Flatpak.** Decisão do líder (rodada 2): "só flatpak".
- **Qt 6.12.** Decisão do líder (rodada 2): Qt 6.12 via runtime. FATO: o Qt 6.12 LTS foi publicado em 30/09/2026 (qt.io). FATO: existe a branch qt6.12 no repositório de empacotamento do KDE. NÃO ENCONTRADO: a publicação do runtime KDE 6.12 no Flathub. Decisão do orquestrador, a confirmar: se o runtime não tiver Qt 6.12, o Qt 6.12 é embutido no manifesto, cumprindo LGPL (ver 4.14).
- **Permissões do Flatpak.** FATO: a política do Flathub exige permissões estáticas no mínimo possível, com preferência por portals. Decisão do orquestrador, a confirmar: MySQL por TCP em 127.0.0.1, o que exige acesso de rede ao sandbox (INFERÊNCIA: o mínimo para esse caso é --share=network). FATO: certificado e arquivos são escolhidos pelo FileChooser portal.
- **Segredos.** FATO: o Secret portal tem falhas reportadas em 2024 (KDE Discourse) e em 2026 (Red Hat Bugzilla 2491140). O estado atual não foi verificado.
- **Licença.** Decisão do líder (rodada 3): GPL-3.0-or-later. Preenchimento de C-level (Cósimo): o Qt LGPLv3 embutido gera obrigações LGPL. A validar com Cláudio.
- **Modelo do produto.** Decisão do líder (rodada 3): gratuito, sem serviço online.

### 3.4 Porte e governança (roteamento do Cósimo, 25/10/2026)

- Porte: early, com variante Pipeline-Lean e criticidade elevada (domínio fiscal, certificado de terceiros, PII).
- C-levels ativos: Celso (go/no-go), Caetano (arquitetura), Capitolino (distribuição a terceiros, acionado), Narciso (custódia de certificado e senha, mTLS, XMLDSIG, credenciais SMTP) e Cláudio (LGPD, licença, responsabilidade fiscal).
- Gatilhos de re-roteamento: primeira emissão em Produção Restrita com A1; virada para Produção; distribuição a terceiros (já disparado). O porte sobe para scale com escritório contábil, monetização ou suporte, telemetria ou nuvem, ou base real de usuários.
- Virada para Produção: go/no-go do Celso, com assinatura de Narciso e Cláudio (Preenchimento de C-level).
- EM ABERTO: suporte e atendimento ao usuário. Os agentes de suporte ficaram dormentes até o líder responder sobre suporte. Não respondido.

## 4. Decisões do líder (propostas do orquestrador ficam em 4.14)

### 4.0 Conflitos entre fontes e supersessões

- **C1 Stack.** O brief sugere Python, Node.js, C# ou Java. Decisão do líder (rodada 1): C++23 com Qt 6. Prevalece a decisão do líder.
- **C2 Interface.** O brief coloca a interface na Fase 2 e pede mTLS e XMLDSIG antes. O líder pediu interface moderna desde o início (O15). Prevalece o líder. A ordem de entrega dentro da v1 é EM ABERTO, e cabe ao C-level no plano da onda (L-16).
- **C3 DANFSe.** O brief pede "Geração de DANFSe (PDF) quando disponível". A API foi suspensa (NT 008). Prevalece o DANFSe local (decisão do orquestrador, a confirmar).
- **C4 Envio GZip e Base64.** O brief diz GZip mais Base64. Correção de 27/10/2026 (DA-2.6 do plano, decisão autônoma do CTO, a ratificar pelo líder): a afirmação anterior "FATO: o Base64 está confirmado no texto oficial" não tem fonte; ela vinha de `docs/pesquisa/dominio.md`, linha 43, que não cita fonte, e nenhum texto oficial lido (manual de contribuintes, que baixa como v1.0 de 17/03/2025; manual de decisão judicial; manual do ADN; Anexo I; XSD) traz Base64 nem GZip no envio (`docs/pesquisa/onda2_api_sefin.md`, 1.1). INFERÊNCIA de três fontes secundárias (mesmo arquivo, 1.2): corpo JSON `{"dpsXmlGZipB64": ...}`, GZip antes do Base64, `application/json`. Confirmação na emissão de teste do líder em Produção Restrita (NL-2.10 do plano).
- **C5 Configuração.** O brief prevê README e .env.example. O líder pediu configurações no app ("configs do app"). EM ABERTO: se README e .env.example continuam como entregáveis.
- **C6 Atualização do cadastro de tomador.** A rodada 3 diz que o que for preenchido "deve atualizar o banco do servidor do governo". A rodada 6 corrige para atualizar o cadastro do tomador. A memória registra a rodada 6 como cadastro LOCAL. A pesquisa não encontrou canal documentado de escrita de tomador no governo (FATO). Decisão registrada: cadastro local. JÁ RESPONDIDO (rodada 6): cadastro de tomador é LOCAL, e o app pergunta ao emitir se atualiza. Nada sobe ao governo além da própria nota. Nota de rastreio: o arquivo de memória registra o cadastro local na rodada 6, mas não traz a frase "nada sobe ao governo". Ela veio da ordem do orquestrador de 25/10/2026, e precisa de confirmação no arquivo de memória. O texto da rodada 6 diz "ao cadastro da pessoa/empresa", que aceita as duas leituras.
- **C7 Validação do endereço do tomador.** A pesquisa de tomador diz que a Sefin valida só existência de CNPJ ou CPF (E0190, E0207). A NT 009 (11/09/2026, mais recente) diz que o endereço é validado contra o cadastro (§2.10). Os dois fatos são registrados. EM ABERTO: se a Sefin devolve o cadastro ao emissor.
- **C8 Versão do Anexo VII (cIndOp).** A NT 009 cita v1.02.01. A página da biblioteca lista v1.03.00. A NT 007 cita v1.01.00. EM ABERTO: qual versão usar.
- **C9 Provedores SMTP.** Fontes divergentes para Outlook e Locaweb. Prevalece a fonte oficial quando existe (ver 4.9).
- **C10 Modelo DANFSe v2.0.** Uma pesquisa afirma "v2.0", e outra diz NÃO CONFIRMADO. Não usar como fato.
- **C11 Datas de obrigatoriedade IBS/CBS.** Fontes secundárias conflitantes. Ver seção 6.
- **C12 Caminho de criação do DANFSe.** O brief não prevê geração local. A decisão do orquestrador (C3) não tem aprovação do líder.

### 4.1 Identidade

- **Nome: NotaLivre.** Decisão do líder (rodada 6). Camilo propôs três nomes: Notinha, NotaLivre e Carimbo. O líder pediu, na rodada 3, "um nome de melhor apelo de marketing, mais viralizavel".
- **Repositório:** petrinhu/notalivre, público no GitHub. Verbatim do líder (rodada 1): "Pode criar repo publico no github."
- **app-id:** io.github.petrinhu.notalivre (decisão do líder, rodada 6).
- **INPI:** NÃO CONSULTADO. EM ABERTO: checagem de marca antes de publicar.
- **Licença:** GPL-3.0-or-later (decisão do líder, rodada 3).

### 4.2 Stack e arquitetura

- **Linguagem:** C++23 com Qt 6 (decisão do líder, rodada 1). Verbatim da mesma rodada: "Quero fazer um app linux, qt[latest]."
- **Versão do Qt:** 6.12, via runtime (decisão do líder, rodada 2). FATO: Qt 6.12 LTS, suporte até 30/09/2031 (doc.qt.io/qt-6/qt-releases.html). Se o runtime KDE não trouxer o 6.12, a proposta do orquestrador é embutir o Qt 6.12 no manifesto (ver 3.3 e 4.14). Isso se desvia de "via runtime", da rodada 2, e precisa de confirmação do líder.
- **Interface:** QML e Qt Quick (decisão do líder, rodada 4).
- **Arquitetura:** biblioteca núcleo, CLI e interface gráfica (decisão do líder, rodada 2). A CLI faz tudo que a GUI faz e tem modo headless (decisão do líder, 27/10/2026; ver 4.15).
- **Camadas.** Verbatim do líder (rodada 1): "quero um app bem atomico, com camadas bem definidas". Lei aplicável: L-33 (quatro camadas Front, Mid, Back e Foundation, com dependência em uma só direção; SOLID; DRY só a partir da terceira ocorrência) e L-34 (nenhum monolito). EM ABERTO: o mapeamento dos módulos nas quatro camadas. É trabalho do C-level (L-16).
- **Padrão de stack.** Lei aplicável: L-66 (default global de C++ e Qt, com práticas Qt 6: connect tipado, QPointer, RAII, nenhum I/O pesado na thread da UI). Este projeto tem decisão própria do líder, que prevalece sobre o default.

### 4.3 Entrega e CI

- **Entrega:** só Flatpak (decisão do líder, rodada 2).
- **Matriz de CI:** compilação nativa do núcleo e da CLI com Qt 6.12 em contêineres Fedora, Ubuntu, Manjaro e Mint. O Flatpak é gerado uma vez e testado nas quatro, em contêiner privilegiado (decisão do líder, rodada 4, paráfrase).
- **Pedido literal do líder (rodada 1):** "Quero runners/ci fedora, ubuntu, manjaro (específico, não arch), mint(específico)."
- **Versões:** EM ABERTO. O líder pediu "específico" para Manjaro e Mint, mas não fixou versões. Ubuntu e Fedora também não têm versão definida.
- **Qt disponível por distro (FATO na data da pesquisa):** Fedora 44 com qt6-base 6.11.2. Ubuntu 26.04 LTS com 6.10.2. Manjaro stable com 6.11.2. Mint 22.3 com 6.4.2 (inferência de pesquisa, não verificado). Nenhuma das quatro traz Qt 6.12 como pacote. Origem do Qt 6.12 dentro de cada contêiner: JÁ RESPONDIDO (rodada 4, baixado nos contêineres). Método de download ainda não escolhido. aqtinstall 3.3.0 com suporte a Qt 6.11 ou superior em Linux NÃO CONFIRMADO.
- **GitHub Actions (FATO):** os runners são só Ubuntu, e a GitHub declara que não pretende oferecer outras distros (github.com/actions/runner-images). As demais distros entram por contêiner.
- **Imagens de contêiner (FATO):** fedora:44 e ubuntu:26.04 são imagens oficiais Docker. Para Mint e Manjaro não há imagem oficial (linuxmintd/mint22.3-amd64 e manjarolinux/base são de terceiros).
- **Flatpak no CI (FATO):** flatpak-github-actions v6, com inputs manifest-path, bundle e run-tests. Rodar o bundle dentro de contêiner exige relaxar capacidades do bwrap, ou usar --privileged. Há relatos de falha até com contêiner privilegiado. EM ABERTO: viabilidade medida em runner real (INFERÊNCIA: o teste de Flatpak em contêiner é o ponto de maior risco da matriz).
- **Root no contêiner:** lei aplicável L-49. Contêiner do GitHub Actions roda como root e ignora bits de permissão. Verificar isso antes de atribuir falha a bug de distro.

### 4.4 Banco e criptografia

- **Banco: MySQL,** servidor local na máquina. Verbatim do líder (rodada 1): "Se precisar de banco de dados, use mysql."
- **Assistente de primeiro uso** (decisão do líder, rodada 2, paráfrase). O que ele faz foi definido no builder check, no bullet seguinte. O app em Flatpak não instala pacotes.
- **MySQL, comportamento do assistente** (decisão do líder, rodada builder check, item c; paráfrase do arquivo de memória): o assistente detecta o MySQL do sistema. Se existe, cria banco, usuário e liga a cifra. Se não existe, identifica a distro e mostra o comando exato (dnf, apt ou pacman) para o usuário copiar, e testa de novo.
- **Quem mantém o MySQL:** o usuário, guiado pelo assistente. JÁ RESPONDIDO (rodada builder check, item c).
- **EM ABERTO (novo, para o orquestrador e o C-level):** com que credencial administrativa o assistente cria banco e usuário e liga a cifra. INFERÊNCIA: no sandbox Flatpak, o socket local do MySQL do host provavelmente não fica acessível, e a conexão seria por TCP em 127.0.0.1 (ver 3.3). A forma de obter a credencial não está escrita em nenhum lugar lido.
- **Cifra no app:** AES-256-GCM por campo de dado pessoal, mais índice cego para busca (decisão do líder, rodada 4, paráfrase). Verbatim do líder (rodada 1): "quero cripto de alto nível no banco". EM ABERTO: parâmetros do índice cego (chave, algoritmo, o que é indexado).
- **Cifra de arquivos do MySQL:** ligada pelo assistente (decisão do líder, rodada 4). FATO: o InnoDB cifra com AES, com CBC para dados e ECB para a chave do tablespace (MySQL 8.4). FATO: o MySQL Community traz component_keyring_file, que guarda a chave em arquivo local ao host. O component_keyring_encrypted_file é Enterprise (MySQL 9.4). INFERÊNCIA: a chave fica no mesmo host dos dados, o que limita a proteção contra roubo do disco. Ponto para Narciso e Cláudio.
- **Chave mestra:** senha-mestra com derivação Argon2id, chaveiro opcional e código de recuperação (decisão do líder, rodada 4, paráfrase).
- **Perda de senha-mestra e código de recuperação** (decisão do líder, rodada builder check, item a; paráfrase do arquivo de memória): sem porta dos fundos, mas uma cópia da chave do banco fica também cifrada com o certificado A1 de cada emissor. Quem tem o .pfx e a senha dele recupera tudo. Sem a senha-mestra, sem o código de recuperação e sem o A1, os dados cifrados ficam perdidos, e as notas são re-baixadas do governo (ADN).
- **Consequência a registrar** (preenchimento do orquestrador, a validar com Narciso e Cláudio): a chave do banco passa a depender também do par .pfx e senha do certificado. Quem tiver o .pfx, a senha dele e uma cópia do banco consegue decifrar os dados pessoais, sem a senha-mestra. Isso enfraquece a separação de segredos que o anti-objetivo 2.3 protege. A decisão do líder não trata desse trade-off, e ele fica registrado aqui para ser aceito ou revisto.
- **Senha-mestra e senha do certificado são separadas** (decisão do líder, rodada builder check, item b). Ver 4.5.
- **Riscos de chaveiro (FATO):** o Secret portal tem falhas reportadas em Flatpak (KDE Discourse, 15/05/2024; Red Hat Bugzilla 2491140, 21/06/2026). Estado atual não verificado.
- **EM ABERTO:**
  - onde o código de recuperação fica e quem o custodia. A resposta 1+2 trata da cópia da chave cifrada com o A1, e não diz onde o código mora;
  - parâmetros do Argon2id;
  - como recuperar a numeração da série (R4) quando o banco se perde e as notas voltam do ADN. INFERÊNCIA: o cadastro local de tomadores não volta com o ADN (novo).

### 4.5 Emissores e certificado

- **Tipo de emissor:** CPF ou CNPJ, com CNPJ como padrão. Verbatim do líder (rodada 1): "Quero opcao de emitir como cpf ou cnpj, cnpj por default."
- **Abertura:** o app abre com o último emissor usado. Verbatim do líder (rodada 1): "App deve abrir com o último usuário emissor utilizado."
- **Vários emissores por instalação, cada um com seu A1, sua senha e sua numeração:** JÁ RESPONDIDO (playback aprovado, segundo a ordem do orquestrador de 25/10/2026). O arquivo de memória não detalha este item. Ele só registra a aprovação das partes 1 e 2 da rodada 8, e o verbatim de abertura da rodada 1 já pressupõe mais de um emissor. Limite de emissores: ver EM ABERTO abaixo.
- **Certificado:** A1 (.pfx ou .p12) com senha. O certificado com CPF é aceito pela API (E1209, FATO). O A3 está fora da v1 (decisão do orquestrador, a confirmar).
- **Política de senha do certificado** (decisão do líder, rodada 3, paráfrase). As opções são as que o líder listou na rodada 1, verbatim: "Quero opcao de colocar senha do certificado sempre/a cada x notas emitidas/após reabrir o app/após y minutos de inatividade/sugestoes suas." Também ficaram decididos:
  - chaveiro do sistema;
  - uma senha vale para o lote inteiro;
  - sempre pedir senha antes de emitir em Produção (no modo headless, como a 4.15 descreve);
  - travar ao bloquear a tela.
- **Senha do certificado separada da senha-mestra** (decisão do líder, rodada builder check, item b). Ver 4.4.
- **EM ABERTO:**
  - os valores de x (notas) e de y (minutos);
  - quais "sugestões suas" foram acolhidas;
  - qual opção vem marcada por padrão;
  - o que acontece com a senha no chaveiro quando o usuário troca de certificado;
  - limite de número de emissores por instalação (não fixado).
- **Custódia:** Narciso (preenchimento de C-level). Brief: nunca embutir certificado nem senha no código.
- **Botão "Trocar emissor" do cabeçalho (decisão do líder, 26/10/2026, opção escolhida: "Adiar para NL-4.4/8.5 (Recomendado)").** Até a tela de emissores existir (NL-4.4 e NL-8.5 do plano), o botão fica visível e desabilitado, com dica dizendo que a troca de emissor chega numa versão seguinte.

### 4.6 Emissão, eventos e consulta

- **Emissão.** POST /nfse, com a DPS assinada e enviada no formato definido pela API (FATO). O corpo é JSON com XML em GZip e Base64 (brief e fontes secundárias, INFERÊNCIA até a NL-2.10; ver C4).
- **Confirmação forte em Produção.** O usuário digita CPF ou CNPJ antes de emitir (decisão do líder, rodada 5, paráfrase). Em Produção Restrita, aparece a faixa "sem validade fiscal" (rodada 5).
- **Cancelamento (v1, rodada 5).** Evento 101101. Motivos: 1 erro de emissão, 2 serviço não prestado, 9 outros. Justificativa de 15 a 255 caracteres (FATO, Anexo II). O prazo é parametrizado por município (E0822, Anexo II, aba RN EVENTOSxEVENTOS; não consta do Anexo I de 09/02 nem do de 20/10). Exemplo municipal: 60 dias (FATO). EM ABERTO: os motivos exibidos ao usuário e o aviso de prazo.
- **Substituição (v1, rodada 5).** Evento 105102, mais POST /nfse com a chave a substituir (FATO). Londrina, município (não nacional), proíbe alterar tomador identificado por outro. EM ABERTO: se o app adota essa regra.
- **Modelos de nota (v1, rodada 5).** O brief fala em "templates/modelos pré-preenchidos". EM ABERTO: quais campos entram no modelo.
- **Baixa de notas do governo, ADN (v1, rodada 5).** GET /DFe/{NSU} e GET /NFSe/{chave}/Eventos (FATO, manual ADN 1.0, 12/02/2026). A consulta usa certificado do mesmo CNPJ raiz (FATO). EM ABERTO: a baixa para emissor CPF; a periodicidade; o que fazer com o NSU guardado.
- **Consulta.** GET /nfse/{chave}. GET /dps/{id} devolve só a chave de acesso, e só se o certificado for do prestador, do tomador ou do intermediário (FATO, manual v1.2, 1.4).
- **Retry.** Requisito do brief: retry com backoff para falha transitória de rede. EM ABERTO: número de tentativas e quais erros entram.
- **Logs.** Requisito do brief: logs detalhados, com ID da DPS e chave de acesso. EM ABERTO: quais campos pessoais ou fiscais entram nos logs.
- **Erros da API.** Requisito do brief: tratamento de erros retornados. O catálogo de erros é o conjunto de códigos E da API (FATO, Anexo I, aba RN).
- **Atividade de evento, grupo atvEvento (decisão do líder, 26/10/2026, opção escolhida: "Incluir na v1 (Recomendado)").** O grupo `serv/atvEvento` entra na v1 (FATO, Anexo I de Produção Restrita de 20/10/2026, aba LEIAUTE, linhas Excel 227 a 241). Sem ele, a regra E0390 (aba RN, linha Excel 380) recusa todo serviço do item 12 da LC 116/2003 (shows e eventos). Fatia NL-4.6 do plano.

### 4.7 Lote

- **Comportamento** (decisão do líder, rodada 3, paráfrase): o lote continua e relata no fim. Só as falhas são reenviadas.
- **Fontes do lote** (decisão do líder, rodada 5, paráfrase): planilha CSV ou XLSX, tomadores selecionados com um modelo, e notas recorrentes.
- **Mecânica:** POST síncrono, uma DPS por chamada (inferência de pesquisa, com força alta; ver R5). Cada DPS ocupa seu número na série. Número reservado e não usado gera pulo, que não pode ser reutilizado (NT 007, p. 9).
- **Anti-objetivo:** perder a numeração da DPS em lote (orquestrador, a confirmar).
- **Confirmação:** uma senha vale para o lote, e o app pede confirmação antes de emitir em Produção (rodada 3). EM ABERTO: o fluxo exato dessa confirmação no lote.
- **Notas recorrentes:** JÁ RESPONDIDO. O usuário confirma antes de enviar, e nenhuma nota recorrente roda sozinha. Fonte da resposta: ordem do orquestrador de 25/10/2026. O arquivo de memória não registra este item.
- **Retomada após queda:** decisão do orquestrador, a confirmar (ver 4.14). Antes de reenviar, o app consulta GET /dps/{id}.
- **Lote sem supervisão:** JÁ RESPONDIDO, por consequência do bullet de notas recorrentes. Não há lote sem supervisão do usuário.
- **EM ABERTO:**
  - colunas e formato da planilha;
  - frequência e forma de geração das notas recorrentes (o que se confirma já está decidido, acima);
  - tamanho máximo do lote.

### 4.8 Tomadores e LGPD

- **Gravação:** o app grava CPF ou CNPJ, nome e demais dados dos tomadores que já tiveram nota emitida. Verbatim do líder (rodada 1): "quero que o app grave cpf/cnpj/nome de tomadores que já tiveram nf já emitidas".
- **Campos** (decisão do líder, rodada 3, paráfrase): e-mail, endereço completo, telefone e inscrição municipal. Obrigatoriedade pelo leiaute (FATO, Anexo I v1.01):
  - xNome é obrigatório no Anexo de 09/02 (linha Excel 151, 1-1); no Anexo de 20/10 é 0-1 (linha Excel 153), decisão pendente na INBOX do TODO.md, item 5;
  - CNPJ ou CPF é obrigatório, em escolha;
  - endereço é obrigatório quando o ISSQN incide no local do tomador, ou para certos cIndOp (E0234, só no Anexo de 09/02, linha Excel 258);
  - e-mail e telefone são opcionais;
  - inscrição municipal é opcional, mas pode ser exigida (E0228 a E0232) quando o tomador emite.
- **Consulta de CNPJ** (decisão do líder, rodada 6, paráfrase): API pública, com consentimento. CPF digitado. A API padrão é decisão do orquestrador, a confirmar: BrasilAPI como principal e CNPJ.ws como reserva (ver 4.14). Fatos sobre as opções:
  - CNPJ.ws API pública: 3 requisições por minuto por IP; 360 erros 429 por hora geram bloqueio de 1 hora; dados sensíveis vêm nulos ou reduzidos (FATO, docs.cnpj.ws).
  - ReceitaWS: plano gratuito com 3 consultas por minuto (terceiro, não lido diretamente).
  - CNPJá: 5 consultas por minuto sem cadastro (secundário, não confirmado em página oficial).
  - BrasilAPI: sem limite confirmado (secundário).
  - Termos de uso de ReceitaWS, BrasilAPI e CNPJá: NÃO LIDOS.
- **CPF:** não há fonte pública gratuita de nome ou endereço (FATO, gov.br). O Serpro só contrata com órgãos públicos e pessoa jurídica com e-CNPJ. EM ABERTO: o CPF é digitado à mão, sem consulta.
- **Consentimento:** o texto será escrito pelo ux-writer e revisado por Cláudio (decisão do orquestrador, a confirmar; ver 4.14). EM ABERTO: onde e como o registro do consentimento fica guardado.
- **Edição de dados do tomador** (decisão do líder, rodada 6). Verbatim: "O que eu quero é que se o usuário modificar/acrescentar dados como endereço do tomador, isso seja modificado/acrescentado ao cadastro da pessoa/empresa." A alteração vai para o cadastro local do tomador (ver C6). Ao emitir, o app pergunta mostrando a diferença, com a opção "sempre atualizar" (rodada 6, paráfrase). Notas já emitidas guardam os dados da época (rodada 6, paráfrase).
- **LGPD** (decisão do líder, rodada 3, paráfrase): editar e excluir; exportar os dados de um tomador; apagar inativos após N anos (JÁ RESPONDIDO, rodada 3). N: decisão do orquestrador, a confirmar (ver 4.14). Guarda fiscal: EM ABERTO para o contador (ver 3.2 e seção 6).

### 4.9 Impressão e e-mail

- **Impressão.** Verbatim do líder (rodada 1): "Quroo poder imprimir a nota". Gera o DANFSe local (ver R3). FATO: existe o Print portal, versão 4. NÃO ENCONTRADO: se o Qt 6 usa esse portal dentro de Flatpak. EM ABERTO.
- **Configurações SMTP.** Verbatim do líder (rodada 1): "quero poder enviar por email colocando configs smtp nas configs do app."
- **Provedores pré-configurados** (decisão do líder, rodadas 5, 7 e 8). Rodada 8, verbatim: "incluia pre-configs do hostinger smtp". Rodada 5, verbatim: "opcao um, com servidores mais conhecidos já pre-configurados. O gmail tem de abrir janela extra onde o usuário coloca login e senha para autorizar. Veja como fazer isso, deve haver documentaçao na web explicando, da própria google."
- **Gmail e Outlook** (decisão do líder, rodada 7, paráfrase). Gmail usa a API Gmail com escopo gmail.send e verificação do Google (escopo sensível, sem CASA) no projeto Google Cloud do líder. Senha de app fica como alternativa. Outlook e Hotmail usam OAuth2 da Microsoft (Entra ID). JÁ RESPONDIDO: Gmail e Outlook por OAuth na v1, com registro em nome do líder. A rodada 7 registra o projeto Google Cloud do líder. O registro Entra do Outlook em nome do líder consta só da ordem do orquestrador de 25/10/2026, e não do arquivo de memória. Confirmar.
- **Verbatim da rodada 8, sobre os provedores:** "todos os 11 da lista, mas deve ser feita verificacao na documentacao de cada um". A lista tem 11 provedores e um Personalizado.
- **Verificação do Gmail (FATO):**
  - gmail.send é escopo sensível, e não consta na lista de escopos restritos (support.google.com/cloud/answer/13464325). Por esse escopo, não há avaliação CASA.
  - mail.google.com (SMTP ou IMAP por XOAUTH2) é escopo restrito. Ele exige avaliação de segurança a cada 12 meses se o dado passa por servidor de terceiro. A decisão do líder usa a API REST, e não SMTP por OAuth.
  - App não verificado tem teto de 100 usuários novos para escopos restritos ou sensíveis, por toda a vida do projeto, sem reset (support.google.com/cloud/answer/13463817).
  - Para cliente desktop, o Google pede o navegador do sistema, com redirect para loopback 127.0.0.1 e PKCE S256. OOB foi descontinuado. Webview embutido é proibido (FATO, developers.google.com).
  - Verificação técnica, não decisão aberta: confirmar na implementação que o envio usa a API REST gmail.send, e não SMTP XOAUTH2, como decidido na rodada 7. JÁ RESPONDIDO na decisão; a checagem é de implementação.
- **Verificação do Outlook (FATO):** o suporte da Microsoft indica smtp-mail.outlook.com, porta 587, STARTTLS, e diz que o Outlook.com exige OAuth2. O escopo é https://outlook.office.com/SMTP.Send, com offline_access. A autenticação básica foi desativada nos tenants (learn.microsoft.com). Status da aposentadoria do SMTP AUTH básico: conflitante (fonte terceira diz que está em espera). Como o Outlook já segue por OAuth (rodada 7), este ponto não decide o desenho da v1. Nota de fato: comunicado oficial não localizado. Registro Entra de cliente público: NÃO DETALHADO na página lida.

**Tabela dos 11 provedores, com a verificação feita na pesquisa:**

| Provedor | Autenticação | Servidor e porta (fonte) | Verificação |
|---|---|---|---|
| Gmail | API Gmail com gmail.send (principal). Senha de app como alternativa | Senha de app: smtp.gmail.com, 587 (terceiro). A página oficial não foi lida para host | Escopo verificado. Host SMTP não verificado em oficial |
| Outlook e Hotmail | OAuth2 Microsoft (Entra ID) | smtp-mail.outlook.com, 587, STARTTLS (suporte Microsoft). Terceiro cita smtp.live.com | Verificado no suporte Microsoft. Conflito com fonte terceira |
| Yahoo | Senha de app (Yahoo Help) | smtp.mail.yahoo.com, 465 SSL ou 587 | Verificado no Yahoo Help |
| iCloud | Senha específica de app (Apple, 03/02/2026) | smtp.mail.me.com, 587, SSL exigido | Verificado na Apple. 2FA não confirmado |
| Hostinger | Não verificado | smtp.hostinger.com, 465 SSL ou 587 TLS (resultado de busca, página não lida) | Parcial. O líder pediu pré-configuração |
| Zoho | Não verificado | smtp.zoho.com, 587 TLS ou 465 SSL (terceiros) | NÃO verificado: página oficial retornou 404 |
| UOL | Não verificado | smtps.uol.com.br, 587 (terceiro) | NÃO verificado. Há relatos de erro de login em fórum |
| BOL | Não verificado | smtps.bol.com.br, 587 (terceiro) | NÃO verificado |
| Terra | Não verificado | Fonte terceira traz host com placeholder ("xyz") e porta contraditória | NÃO verificado. Fonte confusa |
| Locaweb | Não verificado | Porta 587 STARTTLS (oficial, sem host). Host conflitante: smtplw.com.br (busca) ou smtp.email-ssl.com.br (terceiro) | Parcial, com conflito não resolvido |
| KingHost | Não verificado | smtpi.kinghost.net, 587 (terceiro) | NÃO verificado |
| Personalizado | Usuário informa | Host, porta, segurança, usuário e senha | Não se aplica |

- **Provedores sem documentação oficial encontrada** (decisão do orquestrador, a confirmar; ver 4.14): Zoho, UOL, BOL, Terra, KingHost, e também Locaweb (host em conflito) e Hostinger (página não lida), entram como sugestão marcada "não confirmada pelo provedor" e editável. **Desvio, que o líder precisa aceitar ou recusar:** a rodada 8 pediu verificação na documentação de cada um, e esta proposta não cumpre essa exigência para estes provedores.
- **Modelos de e-mail** (decisão do líder, rodada 7). Verbatim: "Nas opcoes do app deve ter já um modelo pre-formatado de email para encaminhar a nota com opcao de criar modelo proprio, ou mais de um modelo, selecionaveis em menu dropdown. A primeira opcao sempre o default/pre-formatado"
- **EM ABERTO:** variáveis do modelo (placeholders), assunto, e se o XML também vai anexo ao e-mail.

### 4.10 Tabelas fiscais e atualização

- **Pedido literal do líder** (decisão do líder, rodada 8). Verbatim: "Cada vez que o app abrir deve baixar e atualizar todas tabelas (inclusive municipios e codigos municipais) envolvidas na emeissao de nota, se houver versao nova apenas, para não ficar perdendo tempo. Deve haver opcao de atualizar tabelas manualmente nas configs e como algum botao visivel em alguma barra de botoes/menus da interface principal"
- **Tabelas embutidas no pacote** (decisão do líder, rodada 8, paráfrase).
- **Fontes conhecidas (FATO):**
  - CST e cClassTrib: consumo.tributos.gov.br (consulta online). O formato de download NÃO está confirmado.
  - Calculadora de Tributos: versão online e versão offline de código aberto, para embarcar em sistema via API (NT 009, p. 22-23).
  - cIndOp: Anexo VII. Ver conflito de versão em C8.
- **Fontes não encontradas:**
  - IBGE (municípios e códigos municipais): NÃO ENCONTRADO URL de download de arquivo. A API de localidades é a fonte proposta (ver acima).
  - cNBS (Anexo B): NÃO ENCONTRADO URL de download.
- **Frequência:** a NT 009 cita o Informe Técnico 2025.002 v1.60, de 23/06/2026. Fontes secundárias registram quatro versões de cClassTrib em 2026 (INFERÊNCIA com base em fontes secundárias).
- **Recomendação de pesquisa (INFERÊNCIA):** versionar cada tabela e gravar a versão usada em cada DPS.
- **Fonte de download por tabela** (decisão do orquestrador, a confirmar; ver 4.14). IBGE (municípios e códigos municipais): API de localidades do IBGE, em servicodados.ibge.gov.br/api/v1/localidades. A documentação oficial foi lida em 25/10/2026 (FATO; versão 1.0.0). cClassTrib e cIndOp: portais oficiais, sendo que o formato de download de cClassTrib ainda não está confirmado. cNBS: sem URL, embutida e atualizada por release.
- **EM ABERTO:**
  - como detectar que existe "versão nova";
  - se o app embarca a Calculadora offline ou chama a online;
  - o comportamento do app sem rede (as tabelas embutidas bastam?).

### 4.11 IBS/CBS e regimes

- **Regimes da v1:** não optante (opSimpNac 1), MEI (opSimpNac 2) e Simples Nacional ME/EPP (opSimpNac 3) (decisão do líder, 27/10/2026, opção escolhida: "Aceitar não optante (Recomendado)"; amplia a rodada 8, que dizia MEI e Simples Nacional, para incluir o autônomo pessoa física fora do MEI, que o consumidor da seção 1 nomeia). Domínio pelo XSD de 20/10/2026 (TSOpSimpNac, tiposSimples_v1.01.xsd:1003-1016; regApTribSN só com opSimpNac 3, tiposComplexos_v1.01.xsd:978), conforme a DA-1.16 do plano, agora ratificada.
- **Grupo IBS/CBS:** já implementado. Opcional em 2026 e obrigatório em 2027 (decisão do líder, rodada 8, paráfrase; FATO, nota I336 do Anexo I v1.01: "Para optantes dos Simples Nacional, os grupos IBSCBS só serão obrigatórios a partir de 2027").
- **Alerta do líder** (rodada 6). Verbatim: "Você está vendo os dados de ibs/cbs etc da nova nota fiscal? Cuidado com a legislacao fiscal!"
- **O que o app informa:** classificação (CST, cClassTrib, cIndOp, indicadores). Não informa alíquota, porque a DPS v1.01 não tem esse campo (FATO). O cálculo vem da Calculadora (inferência de pesquisa).
- **Lucro Presumido:** fora da v1. JÁ RESPONDIDO (rodada 8, por consequência de "regimes v1 = MEI + Simples"). A memória registrava o tema como "em aberto" até a rodada 7.
- **cIndOp:** o app sugere pelo serviço e o usuário confirma. Decisão do orquestrador, a confirmar (ver 4.14). A regra final depende do contador (seção 6).
- **Compra governamental, tpEnteGov (decisão do líder, 26/10/2026, opção escolhida: "Decidir com a INBOX 12 (Recomendado)").** O campo é decidido junto com o pacote de IBS/CBS especial (linha 12 da INBOX do plano, Onda 1). Até lá, o aviso D3 e o ramo tpEnteGov da regra E0903 ficam fora (DA-1.26 do plano).
- **Datas:** cronograma oficial NÃO PUBLICADO. A NT 009 (§1) e a NT 007 (p. 8) prometem publicação no portal. Datas de fontes secundárias estão na seção 6.
- **EM ABERTO:**
  - se o MEI entra no grupo IBS/CBS antes de 2027 (INFERÊNCIA: optante do Simples, código 2 de opSimpNac, segue a regra de 2027; texto normativo não encontrado).

### 4.12 Ambientes e proteção de Produção

- **Ambiente por emissor** (decisão do líder, rodada 5, paráfrase).
- **Faixa "sem validade fiscal"** em Produção Restrita (rodada 5).
- **Confirmação forte:** digitar CPF ou CNPJ antes de emitir em Produção (rodada 5).
- **Senha antes de emitir em Produção** (rodada 3). No modo headless, senha e confirmação seguem a 4.15.
- **Virada para Produção:** go/no-go do Celso, com assinatura de Narciso e Cláudio (Preenchimento de C-level, Cósimo). O brief pede "troca fácil para o ambiente de Produção quando validado".
- **Critério de "validado" (critério de pronto da v1):** decisão do orquestrador, a confirmar (ver 4.14). Não é decisão do líder ainda.
- **Quem aprova a virada:** Celso dá o go/no-go, com assinatura de Narciso e Cláudio (3.4). A decisão final de virar para Produção é do líder, por regra de governança (LEI ZERO: go/no-go é decisão dele). Esta linha não está decidida pelo líder e segue como proposta de encaminhamento.

### 4.13 Testes sem A1

- **Decisão do líder** (rodada 3, paráfrase): teste sem A1 com servidor falso da API e certificado de teste.
- **FATO:** não há teste oficial sem certificado (NÃO ENCONTRADO). A transmissão exige cadeia ICP-Brasil (E1203 a E1208).
- **INFERÊNCIA:** o servidor falso valida o que o app faz, mas não a aceitação pela Sefin. A validação real depende de A1.
- **Qual XSD o servidor falso usa:** decisão do orquestrador, a confirmar (ver 4.14). O leiaute-alvo é o vigente em Produção Restrita, de 20/10/2026 (FATO, URLs na seção 5), com gerador e XSD versionados para trocar de leiaute sem reescrever.
- **EM ABERTO:**
  - origem do certificado de teste (gerado pelo app ou de AC de teste).

### 4.14 Decisões do orquestrador pós-revisão

Propostas do orquestrador, registradas em 25/10/2026 após a revisão com as respostas finais do líder. **Nenhuma está aprovada pelo líder.** Cada uma diz quando se desvia de decisão anterior do líder.

- **Leiaute-alvo:** o vigente em Produção Restrita (20/10/2026), com gerador e XSD versionados para trocar de leiaute sem reescrever (ver 4.13).
- **Qt 6.12 no Flatpak:** se o runtime KDE do Flathub não tiver Qt 6.12, o Qt 6.12 é embutido no manifesto, cumprindo LGPL (ver 3.3 e 4.2). **Desvio:** a rodada 2 disse "via runtime". Precisa de confirmação do líder.
- **Provedores SMTP sem documentação oficial encontrada** (Zoho, UOL, BOL, Terra, KingHost, Locaweb com host em conflito, Hostinger com página não lida): entram como sugestão marcada "não confirmada pelo provedor" e editável (ver 4.9). **Desvio:** a rodada 8 pediu verificação na documentação de cada um. Precisa de confirmação do líder.
- **Fonte de cada tabela fiscal:** tabela sem URL oficial de download (cNBS) fica embutida e é atualizada por release. As que têm fonte oficial são conferidas e baixadas ao abrir, só quando há versão nova: IBGE pela API de localidades (documentação oficial lida em 25/10/2026) e cClassTrib e cIndOp pelos portais oficiais (ver 4.10). O formato de download de cClassTrib ainda não está confirmado. **Desvio:** a rodada 8 pediu atualização ao abrir de todas as tabelas. Precisa de confirmação do líder.
- **Consulta de CNPJ:** BrasilAPI como principal e CNPJ.ws como reserva. O texto de consentimento é escrito pelo ux-writer e revisado por Cláudio (ver 4.8). Termos de uso da BrasilAPI e do CNPJ.ws ainda não lidos.
- **Retomada após queda no lote:** antes de reenviar, o app consulta GET /dps/{id} para saber se a nota já saiu. Nunca reemite às cegas. FATO PRIMÁRIO (manual v1.2, 1.4): o endpoint devolve só a chave de acesso, e só com certificado do prestador, do tomador ou do intermediário.
- **Prazo de apagar inativos (N):** 5 anos, configurável, até o contador confirmar a guarda fiscal (ver 3.2 e seção 6).
- **cIndOp:** o app sugere pelo serviço e o usuário confirma. A regra final depende do contador (seção 6).
- **Critério de pronto da v1:** emitir, consultar, cancelar e substituir em Produção Restrita com o A1 real, mais o lote e o e-mail funcionando, com CI verde nas 4 distros. Duas ressalvas: (a) o A1 real é dependência. Na rodada 1 o líder disse que ainda não tinha o certificado (verbatim da memória); a situação atual não está registrada, e precisa ser confirmada; (b) CI verde é condição necessária, não suficiente para fechar uma onda (L-24): a definição de fechamento fica no plano da própria onda.

### 4.15 Modo CLI e modo headless (decisão do líder, 27/10/2026)

- **Pedido do líder, verbatim:** "quero opcao de modo cli e headless".
- **Modo CLI (decisão do líder, 27/10/2026, opção escolhida: "Tudo que a GUI faz (Recomendado)").** Emitir, lote, consultar, cancelar, DANFSe, e-mail, tomadores, emissores e configuração, tudo pelo terminal. Toda ação da GUI tem comando equivalente na CLI, sobre as mesmas fachadas (paridade verificada por portão; plano, NL-3.8).
- **Modo headless (decisão do líder, 27/10/2026, opção escolhida: "CLI sem tela nem pergunta (Recomendado)").** A CLI roda em script, cron ou servidor sem sessão gráfica, com saída JSON e códigos de saída estáveis, sem nada interativo. Quando falta um dado, sai com erro explícito, nunca pergunta. Não é serviço local em segundo plano, que o líder recusou.
- **Desenho técnico (decisão autônoma do CTO, a ratificar; plano, NL-3.8):** um objeto JSON por execução na saída padrão, com versão do contrato; mensagens de diagnóstico na saída de erro; tabela fixa de códigos de saída; segredos nunca por argumento nem por variável de ambiente.
- **Produção no headless (decisão do líder, 27/10/2026, opção escolhida: "Autorização prévia (Recomendado)").** Texto da opção, verbatim: "O senhor autoriza o headless uma vez por emissor, na tela, digitando o CNPJ, e pode revogar quando quiser. Em cada execução, o script entrega a senha por um canal protegido e repete o CNPJ para confirmar." A Produção Restrita, que não tem valor fiscal, dispensa a autorização (mesma decisão). É assim que o headless cumpre "sempre pedir senha antes de emitir em Produção" (4.5) e "digitar CPF ou CNPJ antes de emitir em Produção" (4.6 e 4.12): a senha chega a cada execução e a inscrição é repetida a cada execução, sobre uma autorização dada por uma pessoa. A autorização não substitui a trava de Produção nem a virada da 4.12 (NL-2.9, NL-12, L-70): o headless em Produção precisa das duas.
- **O que a autorização protege (decisão autônoma do CISO, a ratificar, DA-2.12):** engano operacional, como um script apontado para o emissor ou o ambiente errado, ou copiado de outro emissor. Não protege contra um programa malicioso que já roda na conta do usuário e tem os segredos: esse programa faz o que a própria CLI faz. É risco residual declarado.
- **Concessão (decisão autônoma do CISO, a ratificar, DA-2.12):** a autorização nasce só em modo interativo: na GUI, ou na CLI com terminal (`notalivre headless autorizar`), nunca no modo não interativo, onde o comando sai com o código 2. Exige o cofre aberto, a senha do certificado e a inscrição digitadas na tela ou no terminal, nunca por argumento. A tela avisa, com estas ideias: o script poderá emitir nota real deste emissor sem ninguém na frente do computador; quem tiver o que o script usa abre os dados da instalação e assina por este emissor; como revogar. **Leitura a ratificar pelo líder:** "na tela" inclui o terminal interativo. Sem isso, a paridade "Tudo que a GUI faz" ganharia uma exceção, e um servidor sem sessão gráfica nunca emitiria em Produção pelo headless.
- **Guarda e proteção contra adulteração (decisão autônoma do CISO, a ratificar, DA-2.12):** a autorização é um registro do cofre, cifrado como campo (AES-256-GCM sob K_campo, ADR-0003 regras 1 e 2), com AAD = rótulo `notalivre/headless-autorizacao/v1`, id interno do emissor, ambiente Produção, impressão SHA-256 do certificado e key_id. Sem a DEK, ninguém cria nem altera o registro; registro copiado de outro emissor, de outro ambiente ou de outro certificado não abre (a tag falha). A rotação da DEK recifra o registro junto com os outros campos.
- **Revogação (decisão autônoma do CISO, a ratificar, DA-2.13):** na GUI e por `notalivre headless revogar`, inclusive no modo não interativo, porque revogar só aperta. Revogar apaga o registro e não exige o cofre aberto. Revogação automática: troca do A1 (no mesmo ato, ADR-0002, decisão 3), recuperação do cofre (ADR-0002, decisão 7), remoção do emissor e o limite de tentativas abaixo. Sem prazo de validade: o líder disse "uma vez" e "pode revogar quando quiser".
- **Inscrição divergente (decisão autônoma do CISO, a ratificar, DA-2.14):** nada é reservado, assinado nem enviado; sai com o código 11 (tabela da NL-3.8), `erros[].codigo` igual a `headless.inscricao_divergente`, campo `confirmar-inscricao`, sem repetir o valor; conta como tentativa falha e vai para a auditoria pelo id interno. Comparação depois da mesma normalização que o Back usa para a inscrição do emissor. A autorização e a inscrição são conferidas antes de abrir o A1 e antes de reservar número (NL-3.7). Na Produção Restrita a confirmação é opcional, mas, se vier, tem que bater.
- **Canal da inscrição (decisão autônoma do CISO, a ratificar, DA-2.15):** `--confirmar-inscricao` aceita o valor literal ou `fd:N`. Para emissor CPF, a forma literal é recusada com o código 2, porque o argumento fica visível a outros usuários da máquina (lista de processos), no histórico do shell e no cron, e o CPF é dado pessoal (ADR-0005). A inscrição confirmada nunca aparece no JSON de saída nem no log.
- **Segredos (decisão autônoma do CISO, a ratificar, DA-2.16; corrige a proposta do CTO, que previa um descritor só):** um descritor por segredo, `--senha-certificado-fd N` e o descritor da chave do cofre (nome conforme o ponto do líder abaixo), para não depender de separador dentro do segredo. Cada um é lido uma vez até o fim, com teto de tamanho, em memória de segredo do libsodium, zerado logo após o uso e fechado. Descritor que é terminal é recusado (código 2), e os descritores 0, 1 e 2 também, para o modo não interativo nunca ler a entrada padrão (NL-3.8); descritor de arquivo comum legível por grupo ou por outros é recusado (código 2). Nunca por argumento, variável de ambiente ou arquivo de configuração. Em Produção, a senha do certificado vem sempre do descritor, a cada execução; o chaveiro só serve quando há sessão, e o headless nunca grava no chaveiro. Enquanto guarda segredo, o processo fica não despejável (além do `RLIMIT_CORE` 0 da ADR-0002). FATO: `flatpak run` passa ao app os descritores herdados; o padrão `flatpak run com.example.App --in-fd=3 3<foo` é suportado e testado pelo próprio Flatpak (https://github.com/flatpak/flatpak/pull/5626, integrado em 13/02/2024). Medição no NotaLivre: no contêiner do flatpak-builder, nunca na sessão do líder (L-50).
- **Limite de tentativas (decisão autônoma do CISO, a ratificar, DA-2.17):** contador por emissor, guardado no cofre como campo, conta senha do certificado errada e inscrição divergente no headless em Produção; 5 falhas seguidas revogam a autorização e vão para a auditoria; nova autorização só interativa; sucesso zera o contador. Falha de desbloqueio do cofre não conta (com o cofre fechado não há onde gravar; a proteção ali é o Argon2id, ADR-0006).
- **Política de senha no headless (decisão autônoma do CISO, a ratificar, DA-2.18):** cada execução é um processo novo que recebe o segredo pelo descritor e não guarda nada entre execuções. "Ao reabrir o app", "após 15 minutos parado" e "ao bloquear a tela" ficam cumpridos por construção: a chave só existe durante a execução. "Uma senha para o lote" (4.5) vale dentro da execução. "A cada X notas", se ligada, segue dentro da execução a mesma interação com o lote que a NL-4.3 fixar para a GUI; o headless nunca pede de novo e nunca guarda senha.
- **Auditoria (decisão autônoma do CISO, a ratificar, DA-2.19):** toda execução headless, aceita ou recusada, registra: modo headless, operação, emissor e autorização pelo id interno, ambiente, código de saída, origem de cada segredo (descritor ou chaveiro, nunca o valor) e se a inscrição conferiu. Concessão, revogação manual e revogação automática, com o motivo, também. Nunca: segredo, inscrição, argumentos, caminho de arquivo, nome da máquina ou do usuário (ADR-0005). A tela do emissor mostra o estado da autorização e a data do último uso headless.
- **Para o líder, no pacote L4 (não decidido; o que abre o cofre no headless):** emitir exige a senha do certificado e também algo que abra a DEK, porque sem sessão não há chaveiro e o desbloqueio pelo A1 é recuperação, não uso diário (ADR-0003, regra 4). (a) **Recomendada:** chave headless própria por autorização, 256 bits gerados na concessão, mostrados uma vez, com uma cópia própria da DEK envelopada sob ela (AAD da regra 9 da ADR-0003, tipo de fator headless, id do emissor); a senha-mestra nunca fica onde o script a busca, e revogar apaga essa cópia, o que torna inútil a chave que ficou com o script. Custo: um envelope a mais na hierarquia de chaves da P1. (b) Senha-mestra pelo descritor a cada execução: leitura literal de "a senha", sem mudar a P1. Custo: a senha que a pessoa memoriza passa a ficar gravada onde o script a busca, revogar a autorização não a invalida, e trocar a senha-mestra quebra todos os scripts. Nas duas, quem tiver o que o script usa abre os dados de todos os emissores da instalação (DEK única, se o líder ratificar) e assina por este emissor.

## 5. Fatos externos que condicionam o desenho

Cada item traz a URL da fonte. Quando a fonte é cópia, cópia e original estão indicados. Itens marcados como secundário ou terceiro não são texto oficial.

### 5.1 Leiaute, notas técnicas e API (oficial)

- Anexo I, DPS v1.01 (09/02/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_i-sefin_adn-dps_nfse-snnfse-v1-01-20260209.xlsx
- XSD v1.01 (09/02/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/nfse-esquemas_xsd-v1-01-20260209.zip
- Anexo I de Produção Restrita, v1.01 (20/10/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx
- XSD de Produção Restrita (20/10/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/esquemas_prod_restrita_20261020.zip
- Manual do contribuinte, API v1.2 (out/2025): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf
- Manual do contribuinte, ADN (fev/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf
- Anexo II, eventos (22/01/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_ii-sefin_adn-pedregevt_evt-snnfse-v1-01-20260122.xlsx
- NT 007 v1.0 (07/02/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nt-007-se-cgnfse-v1-0.pdf
- NT 008 v1.02, DANFSe (14/07/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nt-008-se-cgnfse-danfse-20260714-v1-02.pdf (cópia da Fenacon: https://fenacon.org.br/wp-content/uploads/2026/07/nt-008-se-cgnfse-danfse-20260714-v1-02.pdf)
- NT 009 v1.01 (11/09/2026): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nota-tecnica-009-se-cgnfs-e-v-1-01.pdf
- Anexo VI v1.04.01 (leiaute RN IBS/CBS, NT 009): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovi-leiautesrn_rtc_ibscbs-v1-04-01-nt009.xlsx (NÃO LIDO)
- Anexo VII, cIndOp (versão a confirmar, ver C8): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovii-indop_ibscbs_v1-03-00-nt009.xlsx (NÃO LIDO)
- Anexo VIII, correlação item, NBS, cIndOp e cClassTrib (v1.01.00, "trabalho inicial"): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexoviii-correlacaoitemnbsindopcclasstrib_ibscbs_v1-01-00.xlsx (NÃO LIDO)
- Portal da NFS-e: https://www.gov.br/nfse/

Fatos da leitura desses documentos:
- Ambientes e URLs: brief (fonte 2). O caminho base da API não foi confirmado.
- DANFSe: a API de geração seria suspensa em 03/08/2026 (NT 008 v1.02, item 1). Em tpAmb=2, a frase "NFS-e SEM VALIDADE JURÍDICA" é obrigatória.
- Cancelamento: eventos 101101 (cMotivo 1, 2 ou 9; xMotivo de 15 a 255 caracteres), 105102 (substituição), 101103 (análise fiscal), 105104 e 105105 (deferimento e indeferimento). O Id do evento é "PRE" mais a chave de 50 posições mais o código de 6 posições.
- Endpoints (manual v1.2): POST /nfse; GET /nfse/{chaveAcesso}; GET e HEAD /dps/{id}; POST e GET /nfse/{chaveAcesso}/eventos; GET /parametros_municipais/{cMun}/convenio, /{cMun}/{codigoServico} e /{cMun}/{CPF/CNPJ}. O último path tem duas descrições no manual (retenções e benefícios), relatado e não resolvido.
- ADN do contribuinte: GET /DFe/{NSU} e GET /NFSe/{Chave}/Eventos. A consulta usa certificado do mesmo CNPJ raiz.
- Id da DPS: "DPS" mais cLocEmi (7), tpInsc (1), inscrição (14), série (5) e nDPS (15), total de 45 posições.
- Campos do tomador (Anexo I v1.01, aba Leiaute): grupo toma 0-1; CNPJ, CPF, NIF ou cNaoNIF em escolha; xNome 1-1 (1 a 150, Anexo de 09/02, linha Excel 151; no de 20/10 é 0-1, linha Excel 153); endereço 0-1, com endNac (cMun, CEP, xLgr, nro, xBairro obrigatórios) ou endExt; fone 0-1 (6 a 20); email 0-1 (1 a 80).
- Regras de tomador (Anexo I, aba RN): E0187 (grupo obrigatório para alguns cIndOp), E0188 e E0206 (DV de CNPJ e CPF), E0190 e E0207 (existência no cadastro na data de competência, rejeição nível 2), E0234 (endereço obrigatório em casos específicos), E0236 e E0237 (endNac do tomador), E0238 e E0240 (IBGE e CEP), E0246 (cPais diferente de BR), E0202 (prestador diferente do tomador), E0228 a E0232 (inscrição municipal). Versões: E0187 e E0234 só no Anexo de 09/02 (linhas Excel 237 e 258); E0240 do de 09/02 (linha Excel 263) tem o mesmo texto de regra como E0653 no de 20/10 (linha Excel 282), e E0653 se repete na linha Excel 518 com outro texto; E0246 existe nos dois Anexos, com texto ampliado (linhas Excel 265 e 284).
- Ausência de canal de cadastro: o manual v1.2 e o ADN não têm endpoint de consulta ou escrita de cadastro de terceiro (ver R11).
- E9996 (Anexo I, aba RN): a DPS emitida pelo tomador ou intermediário (tpEmit 2 ou 3) é rejeitada nesta versão. Versão: só no Anexo de 09/02 (linha Excel 157). No Anexo de 20/10 o código E9996 não existe; o caminho infDPS/tpEmit tem E0027 (linha Excel 167) para importação de serviço.

### 5.2 Notas técnicas de reforma tributária (oficial, com ressalvas)

- NT 009 §2.1: CNPJ passa a alfanumérico (C) a partir de julho de 2026.
- NT 009 §2.4: opSimpNac ganha o valor 4 (optante pendente), e regApIBSCBSSN passa a existir (1, 2 ou 3).
- NT 009 §2.5: indFinal volta (0-1).
- NT 009 §2.10: validação de endereço do tomador contra cadastro CNPJ ou CPF, com rejeição por divergência de IBGE. Para o subitem 17.05, o município de incidência do ISSQN é o do endereço do tomador ou do destinatário.
- NT 009 §2.11: indDest e dest migram para infDPS.
- NT 009 §2.16: CST e cClassTrib migram para infDPS/IBSCBS/valores/trib. A obrigatoriedade do gIBSCBS vem do indicador ind_gIBSCBS da tabela CST e cClassTrib.
- NT 009 §2.3: vBC do IBS/CBS deduz vISSQN. Até 2026, a fórmula também deduz PIS e COFINS. De 2027 a 2032, só o vISSQN.
- NT 009 §1: "Será publicado no portal da NFS-e o cronograma de implantação" (cronograma não publicado na data da leitura).
- NT 007 p. 8: os novos fatos geradores têm cronograma "será publicado no portal da NFS-e".
- NT 007 p. 9: numeração com pulos (ver R4).

### 5.3 Qt, Flatpak e runtime

- Qt 6.12 LTS publicado em 30/09/2026: https://www.qt.io/blog/qt-6.12-released
- Downloads Qt 6.12: https://download.qt.io/official_releases/qt/6.12/
- Ciclo de releases Qt: https://doc.qt.io/qt-6/qt-releases.html
- Runtime KDE 6.10 (anúncio, 30/10/2025): https://discuss.kde.org/t/flatpak-org-kde-sdk-6-10-runtime-is-now-available/41158
- Branches do empacotamento KDE (qt6.9, qt6.10, qt6.11 como padrão, qt6.12): https://invent.kde.org/packaging/flatpak-kde-runtime/-/branches
- Permissões do sandbox Flatpak: https://docs.flatpak.org/en/latest/sandbox-permissions.html
- Requisitos do Flathub (permissões mínimas, portals): https://docs.flathub.org/docs/for-app-authors/requirements
- Secret portal: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Secret.html
- Falha de Secret em Flatpak (KDE Discourse, 2024): https://discuss.kde.org/t/kwallet-secrets-portal-cant-get-secret-for-flatpak/15566
- Falha de Secret em Flatpak (Red Hat Bugzilla 2491140, 21/06/2026): https://bugzilla.redhat.com/show_bug.cgi?id=2491140
- Print portal: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Print.html
- FileChooser portal: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.FileChooser.html
- Flatpak GitHub Action: https://github.com/flatpak/flatpak-github-actions
- Qt SQL, driver QMYSQL: https://doc.qt.io/qt-6.12/sql-driver.html (o driver usa a biblioteca cliente do MySQL em runtime; presença no runtime NÃO CONFIRMADA)
- Qt NetworkAuth, PKCE e loopback (doc.qt.io, classes QOAuth2AuthorizationCodeFlow e QOAuthHttpServerReplyHandler; PKCE desde Qt 6.8, callbackHost desde 6.9; URL exata NÃO registrada na pesquisa)

### 5.4 Banco e criptografia

- MySQL 8.4, cifra InnoDB: https://dev.mysql.com/doc/refman/8.4/en/innodb-data-encryption.html
- MySQL 9.4, keyring: https://dev.mysql.com/doc/refman/9.4/en/keyring-component-installation.html
- MySQL 8.0, retirada do libmysqld: https://dev.mysql.com/blog-archive/mysql-8-0-retiring-support-for-libmysqld/ (aviso de pré-lançamento)
- MariaDB, plugin file_key_management (referência, não escolhido): https://mariadb.com/kb/en/file-key-management-encryption-plugin/

### 5.5 CI e distros

- Runners do GitHub (só Ubuntu): https://github.com/actions/runner-images
- Fedora 44, qt6-base 6.11.2: https://packages.fedoraproject.org
- Ubuntu 26.04, qt6-base 6.10.2: https://launchpad.net/ubuntu
- Manjaro, comparação de branches: https://manjaristas.org/branch_compare
- Imagens Docker oficiais (fedora:44, ubuntu:26.04): https://hub.docker.com
- Linux Mint, versões: https://en.wikipedia.org/wiki/Linux_Mint
- Qt 6.11: data de 23/03/2026 apenas em resultado de busca, fonte exata NÃO CONFIRMADA.

### 5.6 E-mail e OAuth

- Google, cliente desktop (navegador do sistema, loopback, PKCE, OOB descontinuado): https://developers.google.com/identity/protocols/oauth2/native-app
- Escopos Gmail (gmail.send sensível; mail.google.com restrito): https://developers.google.com/workspace/gmail/api/auth/scopes
- Lista de escopos restritos (gmail.send não consta): https://support.google.com/cloud/answer/13464325
- Teto de 100 usuários e avaliação CASA: https://support.google.com/cloud/answer/13463817
- Verificação de escopo restrito (reavaliação a cada 12 meses): https://developers.google.com/identity/protocols/oauth2/production-readiness/restricted-scope-verification
- XOAUTH2 para IMAP, POP e SMTP (escopo https://mail.google.com/): https://developers.google.com/workspace/gmail/imap/xoauth2-protocol
- Senha de app Google (exige verificação em duas etapas): https://support.google.com/accounts/answer/185833
- Outlook.com, configurações POP, IMAP e SMTP (suporte Microsoft, URL abreviada na pesquisa): https://support.microsoft.com/.../pop-imap-and-smtp-settings-for-outlook-com-d088b986-291d-42b8-9564-9c414e2aa040
- Microsoft, OAuth para IMAP, POP e SMTP (escopo https://outlook.office.com/SMTP.Send, URL completa NÃO registrada na pesquisa): learn.microsoft.com, página "How to authenticate an IMAP, POP or SMTP application by using OAuth"
- Microsoft, desativação da autenticação básica (URL completa NÃO registrada): learn.microsoft.com, página "Deprecation of Basic Authentication in Exchange Online"
- Yahoo, SMTP (senha de app): https://help.yahoo.com/kb/SLN4075.html
- Apple, iCloud SMTP: https://support.apple.com/en-us/102525
- Locaweb, portas SMTP: https://www.locaweb.com.br/ajuda/wiki/portas-smtp/
- Hostinger, SMTP (resultado de busca, não lido): https://support.hostinger.com/1575756 (redireciona para hostinger.com)
- Terceiro, lista de hosts SMTP (não oficial): https://ajuda.alterdata.com.br/shopbase/smtp-e-porta-de-provedores-de-e-mail-185212695.html

### 5.7 Tomador, CNPJ e CPF

- Consulta de CPF (gov.br, consulta paga, só órgãos e PJ com e-CNPJ): https://www.gov.br/pt-br/servicos/obter-solucao-de-consulta-de-dados-de-cadastro-de-pessoa-fisica-cpf
- Serpro, consulta de CNPJ (HTTP 403, NÃO LIDO): https://loja.serpro.gov.br/consultacnpj
- Receita, dados abertos de CNPJ (conteúdo restrito na leitura): https://www.gov.br/receitafederal/pt-br/assuntos/orientacao-tributaria/cadastros/consultas/dados-publicos-cnpj
- CNPJ.ws, limitações da API pública: https://docs.cnpj.ws/referencia-de-api/api-publica/limitacoes
- CNPJ alfanumérico (Agência Gov, 03/07/2026): https://agenciagov.ebc.com.br/noticias/202607/implantacao-do-cnpj-alfanumerico-ocorrera-a-partir-de-31-de-julho
- Campinas, manual de emissão (municipal, recuperação de cadastro local, não nacional): https://portal-adm.campinas.sp.gov.br//sites/default/files/anexos_avulsos/Manual%20de%20Emiss%C3%A3o%20de%20NFSe%20RTC%20-%20altera%C3%A7%C3%B5es%20V2_14jul2026.pdf

### 5.8 Tabelas de classificação e calculadora

- Classificações tributárias (CST e cClassTrib): https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/classificacoes-tributarias
- Espelho da classificação (SVRS): https://dfe-portal.svrs.rs.gov.br/DFE/ClassificacaoTributaria
- Calculadora online: https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/regime-geral
- Calculadora offline (código aberto): https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/calculadora-offline
- Informe Técnico 2025.002 v1.60 (23/06/2026): citado na NT 009, p. 21.

## 6. Pendências fiscais e pontos para o contador

Pesquisa fiscal aprofundada concluída em 25/10/2026 (`docs/pesquisa/fiscal_aprofundada.md`). Legenda desta seção: **FATO PRIMÁRIO** = texto lido na fonte oficial; **FATO SECUNDÁRIO** = notícia ou fonte de terceiro, não lida no texto oficial; **REPRODUÇÃO DE TERCEIRO** = lei lida em site que copia o Planalto; **INFERÊNCIA** = conclusão da pesquisa. Nada desta seção é parecer jurídico ou contábil. A LC 214/2025 e a LC 227/2026 NÃO foram lidas no texto oficial: planalto.gov.br e LexML não devolveram o texto.

### 6.1 Prazos e regimes

- **Regimes da v1:** MEI e Simples Nacional (decisão do líder, rodada 8). Lucro Presumido fica fora (ver 4.11).
- **NFS-e nacional obrigatória para ME e EPP do Simples a partir de 01/11/2026.** FATO PRIMÁRIO, em cópia não oficial: Resolução CGSN 191, de 4/8/2026, publicada no DOU de 10/08/2026. Art. 3º, I: a partir de 01/11/2026, em relação ao art. 1º, que altera o art. 59 da Res. CGSN 140/2018 (uso obrigatório da NFS-e de padrão nacional). Art. 2º revoga a Res. CGSN 189. Cópia lida: https://economia.df.gov.br/documents/d/seec/resol-cgsn-n-191-2026-pdf. A Res. 191 NÃO trata do MEI e NÃO menciona IBS/CBS. Atenção de calendário: 01/11/2026 está a 7 dias da data deste documento.
- **IBS/CBS para optantes do Simples a partir de 2027.** FATO PRIMÁRIO: Anexo I DPS v1.01, linha I336 ("Para optantes dos Simples Nacional, os grupos IBSCBS só serão obrigatórios a partir de 2027"). Vem de dump local da pesquisa anterior, não reaberto nesta sessão. A NT 009 v1.01 traz opSimpNac 4 (optante pendente) e regApIBSCBSSN (1, 2 ou 3), sem data de obrigatoriedade.
- **Autorização sem IBS/CBS até 31/12/2026.** FATO SECUNDÁRIO (Legisweb, 17/09/2026, citando orientação do Portal NFS-e de 16/09/2026): a ausência de IBS e CBS não impede a autorização até 31/12/2026. Depois dessa data, pode haver não conformidade com sanção. Q&A oficial não localizado.
- **Cronograma do ISS:** 01/10/2026 para o ISS fora das alíneas a, b e c, e 01/12/2026 para plataformas digitais e alguns subitens (1.03, 1.05, 1.09, 16.01). FATO SECUNDÁRIO (Legisweb, 31/07/2026, citando a Prefeitura de BH; Ato Conjunto RFB/CGIBS 4/2026, DOU não lido). A data de 01/10/2026 já passou na data deste documento. A mesma fonte diz que o Simples começa em 01/01/2027, o que conflita com 01/11/2026 da Res. 191. Conflito não resolvido, para o contador.
- **MEI:** obrigatoriedade do IBS/CBS em 01/01/2027 ou 04/01/2027, conforme a fonte secundária. O I336 fala em "optantes dos Simples Nacional" sem separar o MEI. INFERÊNCIA: o código 2 de opSimpNac (MEI) segue a regra de 2027, sem texto normativo que diga isso. Fontes secundárias dizem que a opção de setembro de 2026 não se aplica ao MEI. Não lido no primário.
- **Opção do Simples pelo regime regular de IBS/CBS:** FATO SECUNDÁRIO. A Res. CGSN 186/2026 fixava a janela de 01 a 30/09/2026. A Res. CGSN 194/2026 (DOU de 28/09/2026, segundo quatro fontes) prorroga para 30/10/2026. Fontes divergem sobre o efeito da opção (primeiro semestre de 2027). Textos das resoluções 186 e 194 não lidos no primário. Prazo curto: 30/10/2026.

### 6.2 Campos da DPS e validação do tomador

- **Endereço do tomador validado contra o cadastro (NT 009 v1.01, §2.10).** FATO PRIMÁRIO (PDF pp. 13-14): a DPS é rejeitada quando o código IBGE do município informado diverge do cadastro do Tomador, ou do Destinatário, na Sefin Nacional. Base citada: art. 11 da LC 214/2025 e arts. 12 dos regulamentos do IBS e da CBS. O texto cobre CPF, e a aplicação ao tomador pessoa física é INFERÊNCIA. Não há código de erro para essa comparação no §2.10 nem no Anexo VI. Vigência da rejeição: NÃO ENCONTRADA. Conflita com a leitura de que a Sefin só valida existência (E0190, E0207). Efeito no produto: o IBGE do cadastro local do tomador precisa estar certo, senão a nota é rejeitada (ver C7).
- **cIndOp: 0-1 pelo Anexo VI v1.04.01.** FATO PRIMÁRIO (xlsx baixado e lido em 25/10/2026): cIndOp é ocorrência 0-1, com E0859 (não informar se ind_gIBSCBS = 0), E0860 (obrigatório se ind_gIBSCBS = 1) e E0901 (cIndOp incorreto para serviços, rejeição). O Anexo I v1.01 (09/02) diz 1-1 (aba LEIAUTE, linha Excel 339). No Anexo I de 20/10, cIndOp é 0-1 (aba LEIAUTE, linha Excel 338), e E0859 e E0860 estão na aba RN, linhas Excel 570 e 571, que não existem no de 09/02. INFERÊNCIA: o anexo mais novo prevalece. Confirmar com o contador.
- **Anexo VII (cIndOp), versão: não resolvida (C8).** O texto da NT 009 cita v1.02.01. A listagem da Biblioteca RTC cita v1.03.00. O arquivo aberto tem 40 códigos e nenhum carimbo de versão.
- **Código de rejeição por divergência de cadastro:** NÃO ENCONTRADO no Anexo VI. Os códigos próximos (E0238 e E0920) tratam da existência do código IBGE, não da comparação com cadastro.
- **Tabela CST e cClassTrib:** versão 1.60 do Informe Técnico 2025.002, de 23/06/2026, citada na NT 009 (informe não aberto). Versão posterior: NÃO ENCONTRADA.
- **Calculadora de Tributos:** motor oficial, online e offline (código aberto), citado na NT 009 §2.16 (não aberto). INFERÊNCIA: o app envia classificação (CST, cClassTrib, cIndOp), não alíquota. Confirmar com o Anexo VI e com a Calculadora offline.

### 6.3 Responsabilidade, multas e alíquotas de 2026 (ponto para Cláudio)

- **Responsabilidade solidária de desenvolvedores e fornecedores de software** (art. 24, IV, da LC 214/2025). REPRODUÇÃO DE TERCEIRO (ModeloInicial, lida em 25/10/2026) e FATO SECUNDÁRIO (Capital Aberto, 28/09/2026). A regra cobre "os desenvolvedores ou fornecedores de programas ou aplicativos utilizados para registro de operações com bens ou com serviços", inclusive quando contêm funções para descumprir a legislação tributária. NÃO LIDO NO PRIMÁRIO. O produto é distribuído a terceiros e a regra cita fornecedores de programas. Cláudio precisa confirmar se e como ela se aplica.
- **Multas de software, art. 341-G, incisos V e VI.** REPRODUÇÃO DE TERCEIRO, NÃO LIDO NO PRIMÁRIO. V: instalar ou manter software que permita emissão com valores suprimidos ou reduzidos, 100 UPF por equipamento. VI: desenvolver, fornecer ou instalar software fiscal "para terceiros", 150 UPF por equipamento. O inciso VI descreve a distribuição a terceiros em termos próximos do produto. Valor da UPF e enquadramento: para Cláudio, no texto primário.
- **Arts. 341-A e 341-C:** NÃO LIDOS. Fonte de terceiro descreve o 341-A como "qualquer ação ou omissão, mesmo involuntária", e diz que o 341-C fixa UPF de R$ 200,00, com atualização pelo IPCA: NÃO CONFIRMADO.
- **Responsabilidade do software por nota com valor errado:** NÃO ENCONTRADA em texto primário.
- **Multa do emitente por IBS/CBS errado ou ausente:** FATO SECUNDÁRIO (Legisweb, 17/09/2026): desconformidade com sanções. Valores de base (100% do tributo de referência no inciso XI; 6% para CBS e 12% para IBS em 2026): fonte secundária, NÃO CONFIRMADO no primário.
- **Alíquotas de teste de 2026 e período de cura:** arts. 343 (IBS 0,1%) e 346 (CBS 0,9%), REPRODUÇÃO DE TERCEIRO, NÃO LIDOS. Art. 348 (reprodução de terceiro): fatos geradores de 2026; §1º dispensa o recolhimento de quem cumprir as obrigações acessórias; §§3º e 4º dão 60 dias para sanar a falha após intimação. A inclusão dos §§3º e 4º pela LC 227/2026: NÃO CONFIRMADA.
- **ISS e IBS/CBS na mesma DPS.** A NT 009 mantém o vISSQN na base do IBS e da CBS até 2032. INFERÊNCIA: o ISS continua na DPS até 2032. Texto normativo não encontrado.
### 6.4 Lista "O QUE AINDA EXIGE CONTADOR"

Lista montada nesta revisão a partir da pesquisa fiscal. O arquivo de pesquisa não trazia lista pronta. Os itens são os marcados como CONFLITO, NÃO RESOLVIDO, NÃO LIDO NO PRIMÁRIO ou INFERÊNCIA a confirmar com o contador.

1. MEI (opSimpNac 2) entra na regra de 2027 do I336, e qual data vale: 01/01/2027 ou 04/01/2027. A regra E1302 (MEI só vLiq) é do grupo valores da NFS-e gerada pela Sefin, não da DPS (FATO, Anexo I de Produção Restrita de 20/10/2026, aba RN DPS_NFS-e 1.01, linha Excel 49: B49 = `NFSe/infNFSe/`, C49 = `valores`; DA-1.22, decisão autônoma do CTO, a ratificar pelo líder).
2. Combinação de regApIBSCBSSN com o MEI, e efeito da opção do Simples pelo regime regular (Res. 186 e 194; prazo até 30/10/2026).
3. Versão do Anexo VII (cIndOp): v1.02.01 ou v1.03.00 (C8).
4. cIndOp: 0-1 (Anexo VI v1.04.01) ou 1-1 (Anexo I v1.01).
5. Código de rejeição e vigência do §2.10: validação do endereço do tomador contra o cadastro, sem código no leiaute lido.
6. Regra oficial de pessoa física como emitente: NÃO ENCONTRADA. Pelo E0675, o CPF emitente não preenche PIS e COFINS [só no Anexo de 09/02, linha Excel 517; ver linha 87]. Pelo E0099, o CPF precisa de domicílio no município emissor.
7. Responsabilidade do software e do emitente (art. 24, IV; art. 341-A; art. 341-G, V e VI), com valor da UPF: exige leitura do texto primário.
8. Alíquotas de teste de 2026 (arts. 343 e 346), período de cura (art. 348, §§3º e 4º, com inclusão pela LC 227/2026 não confirmada) e guarda fiscal das notas. O prazo de guarda não foi pesquisado, e afeta a exclusão de tomador inativo (ver 3.2).

### 6.5 Condição para emissão em Produção para terceiros

- Liberar emissão em Produção para terceiros depende dos pontos 6.1 a 6.4, sobretudo: a data de 01/11/2026 (Res. 191), a validação do endereço (§2.10), a responsabilidade do software (art. 24, IV) e as multas (art. 341-G, V e VI).
- Encaminhamento proposto pelo mapa do Cósimo, citado pelo orquestrador (arquivo não lido nesta revisão): go/no-go do Celso, com assinatura de Narciso e Cláudio. Pela regra de governança, a decisão final de virar para Produção é do líder, e não do Celso. Esta seção registra a condição. Não decide nada.
- Referência secundária sobre o programa de conformidade RFB e CGIBS: https://tagdlaw.com.br/receita-federal-e-cgibs-preparam-programa-de-conformidade-para-evitar-multas-na-emissao-de-notas-com-cbs-e-ibs/

## 7. O que fica para o construtor

Itens não decididos pelo líder. O construtor propõe, com busca prévia (L-22), e a escolha é submetida ao C-level antes da implementação.

- **Estrutura.** Camadas conforme L-33 e L-34. TDD conforme L-35: teste falhando antes de código de comportamento. Cobertura é consequência, nunca meta numérica.
- **SMTP.** Qt não tem módulo SMTP nativo (FATO, fonte de fórum). Alternativas citadas na pesquisa: SimpleMail e libcurl com SMTP. Não escolhido.
- **OAuth.** Qt 6.12 tem QOAuth2 com PKCE e loopback. Alternativa para o Gmail: API REST, conforme decisão do líder.
- **XMLDSIG e assinatura.** Biblioteca não escolhida. xmlsec1 aparece como pacote nas distros (versões diferentes por distro).
- **GZip e Base64.** Biblioteca não escolhida. Ver C4.
- **Parser de CSV e XLSX** para o lote. Não escolhido.
- **Credenciais.** QtKeychain e portal Secret. Não escolhido; ver riscos em 4.4.
- **Driver MySQL.** QMYSQL ou biblioteca cliente direta. Presença no runtime KDE NÃO CONFIRMADA.
- **Validação do XML.** Contra o XSD, conforme Fase 2 do brief. XSD de Produção Restrita de 20/10/2026, decisão do orquestrador a confirmar (ver 4.13 e 4.14).
- **Esquema do banco.** Tabelas, índices, chaves. Não decidido. Depende de 4.4 (índice cego e chave mestra).
- **Atualização de tabelas.** Mecanismo de versão, download e fallback offline. Depende das fontes em 4.10.
- **Logs.** Política de campos. Ver 4.6.
- **Flatpak.** Manifesto, permissões mínimas, e o caminho de conexão ao MySQL (TCP em 127.0.0.1, decisão do orquestrador).
- **Testes.** Servidor falso da API com dados de Produção Restrita, certificado de teste e testes de Flatpak em contêiner.
- **Build pesado.** Em /var/tmp com TMPDIR exportado, nunca em /tmp, que é tmpfs nesta máquina (L-11).
- **Stack.** C++23 e Qt 6.12 (decisão do líder). Práticas de L-66.

## 8. Fora deste documento

- **Design visual** (paleta, tipografia, componentes, telas): próxima etapa, conduzida pelo visual-design-director, a partir deste documento de funcionalidades (decisão do líder, rodada 7).
- **Wireframes**: fase 3 do pipeline, depois da aprovação deste documento.
- **Plano de ondas e fatias**: C-level (L-16), depois da aprovação deste documento.
- **TODO.md**: ausente na data deste rascunho. Gerar via /tab_pendencias antes de prosseguir (ver aviso no relatório do orquestrador).
