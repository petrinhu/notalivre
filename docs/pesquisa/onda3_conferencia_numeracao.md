# Onda 3: conferência da numeração pela Sefin e pelo ADN (pesquisa NL-3.11P)

**Data:** 24/11/2026.
**Escopo:** as quatro perguntas da ordem do CTO para a NL-3.11P (`/var/tmp/notalivre-logs/onda3-ordem-cto-NL-3.11P-v1.md`, md5 `429b74aa1d07617454e6c49eaaea592d`), que abre a fatia NL-3.11 (DA-2.93).
**Leis aplicadas:** L-22 (pesquisa antes do plano; ler para aprender e reescrever do zero, sem copiar), L-18 (FATO separado de INFERÊNCIA), L-32 (sem travessão nem meia-risca), L-11 (prosa para leitor humano), NF-05 (nada instalado; só documentos oficiais e leitura de repositórios pela API), NF-18. A ordem cita a L-37; não achei seção L-37 em `~/.claude/GODS_LAWS.md` nesta data, e a regra de ler sem copiar está na L-22.
**Nenhuma chamada** foi feita à Sefin nem ao ADN com certificado (plano, 6.1, L8). Onde o servidor recusou o acesso, não tentei contornar.

**Frase-guarda (L-21):** "Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição."

**Legenda.**
- **FATO:** lido nesta sessão no texto citado, com URL (e SHA-256, quando é arquivo) ou arquivo:linha no commit citado.
- **FATO (cópia de terceiro):** o texto está numa cópia do Swagger oficial guardada no repositório de outro projeto. Prova o que a cópia diz, não que ela é igual ao Swagger publicado hoje. Para o NotaLivre vale como INFERÊNCIA forte.
- **INFERÊNCIA:** dedução minha, relato de terceiro, fórum ou blog. Fica assim até a medição na Produção Restrita confirmar.
- **Leitura por resumo:** página lida pela ferramenta de busca, que devolve um resumo e não o texto inteiro. Uma negativa vinda de resumo ("a página não fala disso") é fraca.

## 0. Fontes oficiais lidas e achados de versão

| Documento | URL | Versão no texto | SHA-256 |
|---|---|---|---|
| Manual dos Contribuintes, API do Emissor Público (nome do arquivo "v1-2-out2025") | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf | 1.0, 17/03/2025 | `ac2f36e34ff565cc36d67c5d67415c33cc09f27b2dea006e8da9bcd2ddce5581` |
| Manual dos Contribuintes, APIs do ADN | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf | 1.0, 12/02/2026 | `9ffc97d8b1be4c80a440b3569b0027df61265b9e75ef69437f21f9481a9a1d6d` |
| Arquivo listado como "Manual de Municípios, ADN v1.1, 03/11/2026" | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-municipios-apis-adn-sistema-nacional-nfs-e-v1-1-20261103.pdf | 1.1, 03/11/2026 | `031dde7e6440f48a0541193fb38c264d131c2d906d7ada3db2ce6ba54452dd51` |
| Manual Integrado do Sistema Nacional NFS-e | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/manualintegradosnnfse_v1-00-02-producao.pdf | 1.00.02, 14/09/2022 | `82096b7f8023d8a775e8d013a63cbe6782b24f13d953fb721a59211f7c2e6818` |
| Perguntas Frequentes da NFS-e | https://www.gov.br/nfse/pt-br/perguntas-frequentes/perguntas-frequentes-nfs-e-v1-5-20261123.pdf | 1.5, 23/11/2026 | `5c138adb85dca8a8b7dd1a5bdad19316ca6b9b1911744432f4081d24a5225afc` |
| NT 007 SE/CGNFS-e | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nt-007-se-cgnfse-v1-0.pdf | 1.0, 07/02/2026 | `24e46b39fcf73d40b8241147b2c0607159893fd5934f16f63f3103343972cf05` |
| Anexo I da Produção Restrita (cópia local da pesquisa de 17/11) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx | v1.01, 20/10/2026 | `72b05981c8882cdd807f3acf901b669465360f9cf1b9d9e235ac4363dacfb2d8` |

Achados de versão:

- FATO: os dois primeiros PDFs têm o mesmo SHA-256 registrado em 27/10 (`docs/pesquisa/onda2_api_sefin.md:13` e `:16`). Não mudaram.
- FATO: o arquivo publicado com nome de manual de **municípios** v1.1 tem, na capa, o título "Manual dos Contribuintes" e "Guia para utilização das API´s do ADN". O histórico diz: "1.1 03/11/2026, Atualização do método GET /DFe/{NSU} (item a), para atender a nova funcionalidade de alternância de perfis e procurações" e "Inclusão do item 1.1.2, Mensagem de erro". É a versão mais nova do manual de contribuintes do ADN, publicada com o nome trocado.
- FATO: a página "Documentação atual" (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual, atualizada em 04/11/2026, leitura por resumo) lista esse arquivo e não lista o Manual Integrado. O Manual Integrado continua publicado no gov.br, mas fora da lista atual. É texto de 2022.
- FATO: o Swagger da Sefin de Produção (https://sefin.nfse.gov.br/SefinNacional/docs/index) respondeu 403 em 24/11/2026. O Swagger do ADN, nos dois ambientes (https://adn.producaorestrita.nfse.gov.br/contribuintes/docs/index.html e https://adn.nfse.gov.br/contribuintes/docs/index.html), falhou no aperto de mão TLS, o que é compatível com exigência de certificado de cliente até na documentação (INFERÊNCIA). Não tentei outro caminho.
- FATO: as páginas https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/apis-prod-restrita-e-producao (atualizada em 20/08/2026) e https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/atualizacoes-e-implantacoes/atualizacoes-e-implantacoes (atualizada em 18/11/2026) não falam de limite de requisição, bloqueio nem de mudança no `/dps` ou na distribuição por NSU (leitura por resumo).

## 1. Pergunta 1: HEAD /dps/{id} e GET /dps/{id} na Sefin Nacional

### 1.1 Códigos HTTP

FATO, texto oficial (Manual de Contribuintes 1.0, seção 1.4.2): o GET "Recupera a chave de acesso da NFS-e a partir do identificador do DPS"; o HEAD "é uma consulta para informar se uma NFS-e foi gerada ou não a partir do identificador da DPS". O manual não traz nenhum código HTTP.

FATO (cópia de terceiro): o repositório `nfse-nacional/nfse-php`, commit `ac855fa750f074865a4df55c419d7e204abeb2ca`, guarda cópias dos Swagger oficiais em `references/api-specs/production/prod-API-NFS-e-Sefin-Nacional-(v1).json` e `references/api-specs/homologation/API-NFS-e-Sefin-Nacional-(v1).json`. As cópias entraram no commit `1da72b7` de 01/01/2026 e foram renomeadas no `d1f5ec3` de 09/04/2026 (`gh api repos/nfse-nacional/nfse-php/commits?path=references/api-specs`). Nas duas cópias, a rota `/dps/{id}` é igual:

| Situação | GET /dps/{id} | HEAD /dps/{id} |
|---|---|---|
| Id existente (a DPS virou NFS-e) | 200, "Foi gerada uma NFS-e com o identificador de DPS informado", corpo `DpsGetResponse` com `tipoAmbiente`, `versaoAplicativo`, `dataHoraProcessamento`, `idDps` e `chaveAcesso` | 200, "Identificador do DPS encontrado", sem corpo |
| Id inexistente (nenhuma NFS-e com esse Id) | 404, "Não foi gerada uma NFS-e com o identificador de DPS informado", corpo `ResponseErro` | 404, "Identificador do DPS não encontrado", sem corpo |
| Id mal formado | 400, "Identificador de DPS inválido", corpo `ResponseErro` | 400, "Identificador inválido", sem corpo |
| Id de outro contribuinte | não documentado na cópia (ver 1.2) | o mesmo que para qualquer Id: 200 ou 404 (ver 1.2) |

- FATO (cópia de terceiro): `ResponseErro` traz `tipoAmbiente`, `versaoAplicativo`, `dataHoraProcessamento` e um objeto `erro` (singular) do tipo `MensagemProcessamento`, com `mensagem`, `codigo`, `descricao` e `complemento`, em camelCase. É outro formato que o do ADN (seção 2.3), que usa listas `Erros` e `Alertas` em PascalCase.
- INFERÊNCIA (relato de terceiro, já registrado em `docs/pesquisa/onda2_api_sefin.md:96`): o código no corpo do 404 do GET é E2404. Continua NÃO EXERCIDA no NotaLivre (INF-09, `docs/pesquisa/onda2_producao_restrita.md:55`).
- FATO, texto oficial (manual 1.0, seção 1.4.2 a, e Manual Integrado 1.00.02, seção 14.4.2 a): "Somente podem ser consultados identificadores de DPS emitidos através Sefin Nacional NFS-e". O NotaLivre só emite pela Sefin Nacional, então isso não o limita.
- INFERÊNCIA: "inexistente" para a Sefin quer dizer "não há NFS-e com esse Id". Uma DPS recusada por regra de validação não gera NFS-e, então devolve o mesmo 404 que um número nunca usado. Base: o texto do HEAD fala em "se uma NFS-e foi gerada", e no projeto as DPS 5 a 8 foram recusadas com HTTP 400 e só a 9 virou nota (`docs/pesquisa/onda2_producao_restrita.md:15` e `:35` a `:37`). Isso pesa no K (seção 3).
- INFERÊNCIA importante para o desenho: o HEAD não tem corpo. Um 404 de **rota** (base errada, prefixo `/API` trocado, `docs/pesquisa/onda2_producao_restrita.md:17`) é indistinguível de um 404 de "Id não encontrado". O GET distingue os dois, porque o 404 de negócio traz o `ResponseErro`.

### 1.2 Certificado e ser parte da DPS

- FATO, texto oficial (manual 1.0, seção 1.4.1): por sigilo fiscal, a chave de acesso só é informada se o usuário do certificado da conexão "for um ator (Prestador, Tomador ou Intermediário) que consta na NFS-e gerada a partir da DPS consultada". Seção 1.4.2 a, sobre o GET: "Caso contrário a solicitação será negada."
- FATO, texto oficial (manual 1.0, seção 1.4.2 b): o HEAD "atende a qualquer usuário desde que realize a consulta com um certificado digital válido".
- FATO (cópia de terceiro): a descrição do Swagger da Sefin exige TLS "com autenticação mútua" e certificado ICP-Brasil "tipo A1 ou A3, CNPJ ou CPF, com 'Autenticação Cliente'". Os dois métodos exigem certificado; só o GET exige que o certificado seja ator.
- NÃO ACHEI o código HTTP do GET negado a quem não é ator. A cópia do Swagger só lista 200, 400 e 404. Pode ser 403 ou 404 (INFERÊNCIA, sem fonte). Para o NotaLivre isso não pesa: o app consulta com o certificado do Prestador, que é ator da própria DPS (desenho da spec, R1).

### 1.3 Limite de taxa

- NÃO ACHEI limite de taxa oficial para `/dps`. Procurei nos dois manuais de contribuintes, no Manual Integrado, no FAQ v1.5, nas cópias dos Swagger (que não declaram 429 nem `Retry-After`), na página de APIs e no histórico de implantações.
- INFERÊNCIA (afirmação de terceiro): a documentação do projeto `Fm-s/open-nfse`, commit `18d70dd2acac6fcc5c4a918a451c61c548b20596`, arquivo `specs/ruleset/transporte.md:180-181`, diz a mesma coisa: os manuais e os OpenAPI capturados "não documentam 429 nem `Retry-After`", e a biblioteca trata 429 de forma defensiva.
- INFERÊNCIA (blogs comerciais, leitura por resumo, sem número): há relatos de 429 em consultas repetidas ao ADN. Não uso esses relatos como base. O bloqueio por "consumo indevido" da NF-e (rejeição 656) é de outro sistema e não vale aqui sem fonte da NFS-e.
- O único número oficial de cadência que achei é do ADN, não da Sefin: esperar pelo menos 1 hora quando a distribuição não tiver documento novo (seção 2.3).
- O que acontece ao passar do limite: NÃO ACHEI fonte.

### 1.4 Produção Restrita e Produção

- FATO (cópia de terceiro): as cópias de Produção e de "homologation" (Produção Restrita) trazem a rota `/dps/{id}` idêntica, com os mesmos códigos e esquemas. A única diferença visível é o domínio do logotipo (`sefin.nfse.gov.br` e `sefin.producaorestrita.nfse.gov.br`). O campo `tipoAmbiente` da resposta diz o ambiente (1 Produção, 2 Homologação).
- FATO: são hosts diferentes (página de APIs; `docs/pesquisa/onda2_api_sefin.md:61`).
- INFERÊNCIA: as bases de dados são separadas, e a série e o número usados num ambiente não existem no outro. Base: os hosts separados e o `tipoAmbiente` em cada resposta. Nenhuma fonte lida diz isso com todas as letras. A chave de acesso não serve de prova: o campo dela é o "Ambiente Gerador" (FAQ v1.5, assunto 18.4), que é outra coisa.
- NÃO ACHEI diferença documentada de limite de taxa entre os ambientes.

### 1.5 Resposta curta

200 para Id que virou NFS-e, 404 para Id sem NFS-e (inclusive DPS recusada), 400 para Id mal formado: FATO na cópia de terceiro do Swagger, INFERÊNCIA forte para o NotaLivre até medir. GET exige que o certificado seja ator; HEAD aceita qualquer certificado válido: FATO no texto oficial. Limite de taxa: não documentado. Comportamento igual nos dois ambientes, pela cópia.

## 2. Pergunta 2: ADN, GET /DFe/{NSU}

### 2.1 O ADN distribui ao emitente as NFS-e que ele mesmo emitiu?

As fontes oficiais se contradizem. Não há resposta oficial única.

- FATO, texto oficial de 2022 (Manual Integrado 1.00.02, seção 17.2.2 a, p. 112): "Documentos emitidos pela própria empresa não estarão disponíveis para consulta." Na tabela da p. 113, a linha "NFS-e" tem "Não" na coluna "Emitente da NFS-e" e "Sim" na coluna "Não Emitente". A nota da tabela diz que o sistema "não fará a distribuição de um documento para o próprio emitente do documento".
- FATO, texto oficial de 2026 (manual do ADN 1.0, seção 1.1, e 1.1 de 03/11/2026, seção 1.1): as APIs "permitem que o contribuinte consulte documentos fiscais de serviços em que figure como emitente, tomador ou intermediário". Na v1.1, item 1.1.1 a: "sem informar cnpjConsulta/cpfConsulta, são retornados os documentos do contribuinte identificado pelo certificado de transmissão".
- FATO, FAQ v1.5 (23/11/2026), assunto 19.3: o título fala das notas "em que sou tomador ou intermediário", mas o requisito citado é a inscrição constar "nos campos apropriados da nota (Prestador, Tomador ou Intermediário)". No assunto 19.1, o NSU de distribuição por ator "distribui os documentos para cada Prestador/Tomador/Intermediário interessado". O FAQ não diz, com todas as letras, se o emitente recebe a própria nota.
- INFERÊNCIA (relato de terceiro com medição): o projeto `RicardoBiazin/nfse-nacional`, commit `a8e9b5de438499ac6ef9e5c8916db042e1d0a300`, registra em `HISTORICO.md:33-36` que foi "Validado contra o ADN real" com "leitura de 28 documentos (NFS-e emitidas, recebidas e eventos de cancelamento/substituição)". O `README.md:89` diz que a sincronização baixa as notas "emitidas pelo portal também". O código separa a nota em "emitida" ou "recebida" pelo documento do emitente (`nfse_nacional/servico.py:454`).
- INFERÊNCIA (terceiros sem medição publicada): `Fm-s/open-nfse`, `docs/guide/integracao.md:353` (commit `18d70dd`), diz que a distribuição por NSU "reconstrói todas as NFS-e emitidas e recebidas pelo CNPJ"; post de 29/10/2026 no fórum Contábeis (https://www.contabeis.com.br/forum/tecnologia-contabil/416021/download-em-lote-de-nfs-e-do-padrao-nacional-como-automatizar-pela-api-do-adn/, leitura por resumo) diz o mesmo.
- **Conclusão (INFERÊNCIA):** é provável que a regra de 2022 tenha sido superada e que hoje o ADN distribua ao emitente as próprias NFS-e. Não há texto oficial que diga isso sem ambiguidade. Enquanto não houver medição, o ramo da DA-2.93 que usa o ADN "para descoberta sem registro local" fica sem base confirmada.

### 2.2 Com que atraso

- FATO, FAQ v1.5, assunto 6.7: não existe prazo fixo garantido para a nota ficar disponível a quem tem interesse. O texto trata de notas que o **município** compartilha com o ADN, e diz que a distribuição não tem envio automático ao interessado: ele precisa consultar.
- FATO, FAQ v1.5, assunto 18.5: menciona a Sefin Nacional que "falha ao gravar no ADN por timeout". Daí a INFERÊNCIA de que a NFS-e gerada pela Sefin Nacional é gravada no ADN dentro do próprio processo de emissão, com atraso pequeno. Não achei número.
- INFERÊNCIA (relato de terceiro): `RicardoBiazin/nfse-nacional`, `nfse_nacional/servico.py:411-413`, conta que o NSU 30 de uma empresa ficou de fora numa passada em que o 31 e o 32 já tinham chegado. O projeto relê os últimos 50 NSU a cada sincronização (`servico.py:437`, `REVISAR_NSU = 50`). Ou seja: um NSU pode aparecer atrasado e fora de ordem.
- FATO, Manual Integrado, p. 112, e FAQ v1.5, assunto 19.2: a ordem do NSU é a ordem de recepção pelo ADN, não a ordem de emissão.

### 2.3 Tamanho de página e retomada

- FATO, texto oficial de 2022 (Manual Integrado, seção 17.2.2 a, p. 112): "Obtém até 50 DF-e a partir do último NSU informado"; "Quantidade máxima de documentos no lote: 50 documentos"; o sistema informa "o último NSU da sequência encontrada" para a consulta seguinte. O FAQ v1.5, assunto 19.2, repete "até 50 documentos" para a distribuição a municípios. Os manuais de contribuintes de 2026 não falam de tamanho.
- FATO, texto oficial de 2022 (mesma página): o interessado "deverá aguardar um tempo mínimo de uma hora para efetuar uma nova solicitação" quando receber a indicação de que não há mais documentos, isto é, quando o NSU informado é igual ao maior NSU do ambiente. O FAQ v1.5, assuntos 6.7 e 19.2, repete a espera mínima de 1 hora.
- FATO (cópia de terceiro): o Swagger do ADN Contribuinte (`references/api-specs/production/prod-API-NFS-e-ADN-Contribuinte-(v1).json`, commit `ac855fa`) define `NSU` inteiro de 64 bits no caminho, `cnpjConsulta` texto e `lote` booleano com padrão `true`. A resposta `LoteDistribuicaoNSUResponse` tem `StatusProcessamento` (`REJEICAO`, `NENHUM_DOCUMENTO_LOCALIZADO`, `DOCUMENTOS_LOCALIZADOS`), `LoteDFe` (cada item com `NSU`, `ChaveAcesso`, `TipoDocumento`, `TipoEvento`, `ArquivoXml` e `DataHoraGeracao`), `Alertas`, `Erros`, `TipoAmbiente`, `VersaoAplicativo` e `DataHoraProcessamento`. Não há campo de último NSU nem de maior NSU.
- FATO, texto oficial (manual do ADN 1.1, item 1.1.2): erros com HTTP. E2220, 404, "não existem documentos fiscais para o contribuinte a partir do NSU informado"; E2230, 404, "não existe documento fiscal para o contribuinte com o NSU informado"; E2210 a E2214, 400, problemas do certificado de transmissão; E2243 a E2247, 400, problemas de `cnpjConsulta`, `cpfConsulta` e procuração; E999, 400, falha não tratada. Pela redação, o E2220 é o fim da leitura em lote e o E2230 é a consulta de um NSU só (`lote=false`), INFERÊNCIA pela diferença entre "a partir do" e "com o".
- **Conflito sobre o último e o maior NSU:** o Manual Integrado cita `ultNSU` e `maxNSU`; a cópia do Swagger não tem esses campos; o projeto `RicardoBiazin/nfse-nacional` afirma, depois de chamar o ADN real, que o ADN "não manda `UltimoNSU`/`MaiorNSU`; o fim é o 404 com E2220" (`HISTORICO.md:35-36`, INFERÊNCIA); a documentação do projeto `vasfvitor/nanci` (commit `8b31aa8b2bc2e0c1a4878f1b2b14255a69a2e573`, `docs/NFSE_ADN.md:17`) diz que a resposta traz `ultNSU` e `maxNSU` (INFERÊNCIA). O cursor seguro é o maior `NSU` dos itens recebidos.
- **Conflito sobre o NSU ser exclusivo ou inclusivo:** o projeto `renan20553/baixar_nfse_portal_nacional` (commit `c0e68e42c6a0be37d887fc880c1138ae6eceac8e`) diz que "o NSU da distribuição em lote é exclusivo" e envia o anterior ao próximo desejado (`README.md:101`; `nfse/downloader.py:199-201`). O `RicardoBiazin/nfse-nacional` recua uma janela de 50 NSU a partir do último visto, pede o início da janela mais um e descarta o que for menor ou igual ao início (`servico.py:417` e `:420-421`). Os dois são INFERÊNCIA. O desenho que funciona nos dois casos é deduplicar por NSU.

### 2.4 O que muda para o emitente CPF

- FATO, texto oficial (manual do ADN 1.1, item 1.1.1 a): sem `cnpjConsulta` e sem `cpfConsulta`, o ADN devolve os documentos do titular do certificado. `cpfConsulta`, com 11 dígitos, diferente do CPF do certificado, exige procuração (E2245). Os dois parâmetros juntos dão E2244. CPF inválido dá E2247. A consulta pela "mesma raiz" vale só para CNPJ.
- FATO, FAQ v1.5, assunto 19.3: "Certificado exigido: e-CPF ou e-CNPJ correspondente ao ator interessado". O Manual Integrado, seção 17.2.1, diz o mesmo: "certificado digital de PF com seu CPF ou PJ com seu CNPJ".
- FATO (cópia de terceiro): a descrição do Swagger do ADN copiado em 01/01/2026 cita certificado "CNPJ" e só o parâmetro `cnpjConsulta`. É anterior à v1.1 do manual, que trouxe o `cpfConsulta`.
- **Consequência (INFERÊNCIA):** o emitente CPF (autônomo com e-CPF) consulta as próprias notas sem parâmetro. Não existe para ele o atalho da raiz do CNPJ. Para o NotaLivre, o caminho é o mesmo do CNPJ, sem `cnpjConsulta`.

### 2.5 Resposta curta

Se o ADN entrega ao emitente as próprias NFS-e: as fontes oficiais se contradizem (2022 diz não; 2026 diz que o contribuinte consulta onde figure como emitente); um terceiro relata ter recebido as emitidas em chamada real. INFERÊNCIA: hoje entrega. Atraso: sem número oficial; um NSU pode chegar fora de ordem. Página: até 50 (FATO de 2022). Retomada: informar o último NSU conhecido, avançar pelo maior NSU recebido, fim com 404 E2220 e espera mínima de 1 hora (FATO). CPF: consulta sem parâmetro com e-CPF (FATO, v1.1 de 03/11/2026).

## 3. Pergunta 3: valor de K e custo da varredura

### 3.1 O que a varredura procura

A varredura para a frente (DA-2.93) parte do último número que o registro local conhece, N, e pergunta à Sefin por N+1, N+2 e assim por diante, até K respostas "não há NFS-e" seguidas. O que ela encontra é o **maior número da série que virou NFS-e**. Pela seção 1.1, a Sefin não distingue número recusado de número nunca usado. Logo, nenhuma consulta à Sefin recupera o registro de uma DPS recusada depois do backup (INFERÊNCIA forte). Isso vale também para o ADN, que só distribui documentos gerados.

Onde a varredura entra, pelo canon da branch `onda-3` (FATO):

- restaurar sem o banco atual marca a contagem de cada emissor como não confiável e pede a série no login (decisão do líder de 04/11/2026, "Cópia e série nova (Recomendado)"; `docs/adr/0003-envelope-e-hierarquia-de-chaves.md:96`; `plano:704`, "restaurar sem banco atual leva à escolha de série e não trava em conferência");
- na falha de integridade, o usuário escolhe entre conferir a numeração, se a conferência pela Sefin estiver confirmada, ou escolher outra série (`docs/adr/0003-envelope-e-hierarquia-de-chaves.md:96`);
- a NL-6.6 chama a mesma conferência antes de um lote (`plano:704`).

Então a varredura roda em dois casos: quando o usuário escolhe "conferir" em vez de série nova, e antes de cada lote.

### 3.2 De onde vêm os buracos

- FATO: a P3 do plano manda reservar e gravar o número antes de enviar e "nunca reutilizar" (`docs/superpowers/plans/2026-10-25-notalivre-plano.md:152`). Com essa regra, cada DPS recusada deixa um número sem NFS-e na série.
- FATO, no próprio projeto: na primeira emissão real, as DPS 5, 6, 7 e 8 foram recusadas com HTTP 400 e a 9 foi aceita (`docs/pesquisa/onda2_producao_restrita.md:15` e `:35` a `:37`); o registro de decisões diz que as DPS 1 a 6 foram recusadas (`/var/tmp/notalivre-logs/decisoes_autonomas_onda2.md`, linha das 21:43:06 de 01/11). Foram até 8 números seguidos sem NFS-e antes da primeira nota. Um K de 8 ou menos, partindo do zero, teria parado antes da DPS 9.
- INFERÊNCIA: rajadas de recusa acontecem quando o usuário briga com uma regra de validação nova (troca de leiaute, código de serviço do município, regime). O consumidor declarado (MEI, autônomo e pequena empresa do Simples) emite com pouco suporte técnico, o que torna a rajada mais provável, não menos.

### 3.3 Achado: a NT 007, p. 8 e 9, fala do nNFSe, não do nDPS

- FATO, NT 007 v1.0 (07/02/2026), item "b) Números da NFS-e", p. 8 e 9: o texto trata do campo "nNFSe", numeração "atribuída exclusivamente pela Sefin Nacional"; diz que números reservados pela plataforma que não viram NFS-e geram "pulos" e que "o número previamente reservado não pode ser reutilizado". O FAQ v1.5, assunto 18.5, fala do mesmo nNFSe.
- FATO: o canon da `onda-3` já registra isso. A R4 da spec diz que a NT 007, p. 9 e 10, é "sobre o nNFSe" e que "Para o nDPS não há regra de sequência" (`docs/superpowers/specs/2026-10-25-notalivre-design.md:84`); a P3 do plano diz o mesmo e justifica o "nunca reutilizar" por outro motivo: "Não reutilizar impede duplicar nota às cegas" (`plano:152`).
- FATO: resta uma frase antiga na spec, na "Mecânica": "Número reservado e não usado gera pulo, que não pode ser reutilizado (NT 007, p. 9)" (`docs/superpowers/specs/2026-10-25-notalivre-design.md:231`). Ela atribui à NT uma regra sobre o nDPS que a NT não traz.
- FATO, Anexo I da Produção Restrita: a única regra de unicidade que achei para o número da DPS é a E0014, "Conjunto de Série, Número, Código do Município Emissor e CNPJ/CPF informado nesta DPS já existe em uma NFS-e gerada a partir de uma DPS enviada anteriormente". Procurei por regra de ordem crescente ou de sequência sem buraco no nDPS e não achei.
- FATO, FAQ v1.5, assunto 18.6: reutilizar "a numeração de uma nota já emitida", mesmo cancelada, é vedado (Decreto 12.955, art. 124, §3º). O texto fala de nota emitida.
- INFERÊNCIA: não achei fonte oficial que proíba reenviar uma DPS recusada com o mesmo nDPS. O "nunca reutilizar" da P3 é escolha de desenho do projeto, não regra fiscal. Ele é o que cria os buracos que o K precisa atravessar.

### 3.4 Fórmula do custo

Chamadas de uma varredura = 1 + E + B + K, em que:

- 1 é a chamada âncora num Id que se sabe existente (o último emitido que o registro local conhece), para provar que a base, o prefixo e o certificado estão certos antes de contar ausências (seção 1.1);
- E é o número de NFS-e emitidas na série depois do ponto que o registro local conhece;
- B é o número de buracos entre elas (DPS recusadas ou reservas abandonadas);
- K é o número de ausências seguidas que encerra a varredura.

Cenários, todos com premissas minhas (INFERÊNCIA) e sem ancorar no uso de nenhum usuário identificado:

| Cenário | Premissa | Chamadas com K = 20 |
|---|---|---|
| Usuário escolhe "conferir" com um registro de uma semana atrás | E de 0 a 5; B de 0 a 2 | de 21 a 28 |
| Usuário escolhe "conferir" com um registro de três meses atrás | 20 notas por mês; E = 60; B = 5 | 86 |
| Conferência antes de um lote | registro local íntegro; E = 0 e B = 0 | 21 por lote |
| Série nova escolhida depois de restauração (canon) | série nunca usada por aquele emissor nesta instalação | nenhuma é necessária; uma âncora não existe, porque a série não tem nota |

Com uma chamada por vez e um ritmo de 1 por segundo (proposta minha, INFERÊNCIA, sem limite oficial a respeitar), 28 chamadas levam cerca de meio minuto. O custo da conferência escolhida pelo usuário é baixo porque ela é rara. O que pesa é o custo fixo antes de cada lote: 21 chamadas a cada lote, mesmo com tudo certo.

Uma nota sobre a âncora: numa série que nunca teve nota, não há Id existente para a chamada âncora. Nesse caso a âncora pode ser outra nota do mesmo emissor em outra série, ou a varredura começa sem âncora e só conta ausência com o 404 de corpo do GET (recomendação 1 da seção 7).

### 3.5 K proposto

**Proposta: K = 20, configurável, com piso de 10.** Raciocínio:

1. K precisa ser maior que a maior sequência de números sem NFS-e que se espera depois do último emitido. A única medida que temos é a do próprio projeto: 8 seguidos (seção 3.2). 20 dá mais que o dobro de margem.
2. O custo de K grande é linear e pequeno (seção 3.4). O custo de K pequeno é uma recusa E0014 depois (seção 3.6), não uma nota em duplicidade.
3. O piso de 10 evita que uma configuração ruim volte abaixo da rajada já medida.
4. Se o CTO e o líder decidissem que a DPS recusada pode reusar o número (seção 3.3), os buracos passariam a vir só de reservas abandonadas, e K poderia cair. Com a P3 como está, não recomendo menos que 20.

### 3.6 A rede de segurança é a E0014

- FATO: a Sefin recusa com E0014 a DPS cuja série, número, município e inscrição já existem numa NFS-e (Anexo I, regra E0014). A duplicidade fiscal não acontece por erro de K.
- FATO: o plano já trata a E0014 em qualquer tentativa, comparando o digest do `infDPS` enviado com o da NFS-e encontrada (`plano:578`).
- FATO: o canon já trata a E0014 de DPS diferente. O plano: "Diferente: estado novo `MotivoFalhaSefin::DpsDuplicada`, que nunca renumera sozinho e vai ao usuário, porque indica colisão de numeração" (`plano:578`). A ADR-0003: "E0014 com DPS diferente: o número reservado vira pulo, o app mostra o código e volta à escolha, sem trocar sozinho" (`docs/adr/0003-envelope-e-hierarquia-de-chaves.md:99`), com o teste `dpsDuplicadaViraPuloEVoltaAEscolha` (`:162`).
- INFERÊNCIA: se a varredura parar cedo e o app reutilizar um número já emitido, o resultado é uma E0014 com digest diferente, ou seja, número ocupado por outra DPS. O tratamento do canon (o número vira pulo e o usuário volta à escolha) cobre o caso. O custo de um K curto é, então, uma recusa e uma volta ao usuário, não uma nota duplicada.

## 4. Pergunta 4: como outros emissores livres tratam a retomada

Li para aprender a técnica, sem copiar código (L-22). Cito arquivo:linha no commit lido.

| Projeto | Commit | Licença | O que faz com a numeração |
|---|---|---|---|
| https://github.com/Fm-s/open-nfse (TypeScript, biblioteca) | `18d70dd2acac6fcc5c4a918a451c61c548b20596` | MIT | Contador local atômico por CNPJ e série, que o integrador implementa no banco dele (`src/nfse/dps-counter.ts:1-28`; `docs/guide/integracao.md:43-68`). GET e HEAD `/dps` só para reconciliar uma DPS pendente depois de timeout (`src/nfse/fetch-dps-status.ts:31-74`). Para banco perdido, manda reconstruir as notas pela distribuição por NSU (`docs/guide/integracao.md:353`). Não faz varredura para a frente. |
| https://github.com/RicardoBiazin/nfse-nacional (Python, desktop e servidor) | `a8e9b5de438499ac6ef9e5c8916db042e1d0a300` | AGPL-3.0 | Reserva atômica, "Número reservado nunca volta" (`nfse_nacional/banco.py:446-458`). O usuário informa o próximo número no cadastro da empresa, se já emitia por outro sistema (`README.md:83`; `nfse_nacional/interface/dialogo_empresa.py:496`). Falha de rede consulta GET `/dps` e, sem resposta, deixa a nota "indefinida" (`nfse_nacional/servico.py:4-7`). Sincroniza pelo ADN as notas emitidas e recebidas e relê os últimos 50 NSU (`servico.py:407-437`). Não faz varredura para a frente. |
| https://github.com/cleitonleonel/nfse_manager (Python, FastAPI) | `09dcfc9482f2931024cd95f90dccc822e41117b3` | sem licença na API do GitHub | Reserva com trava por cliente e coluna `ultimo_numero_dps` (`backend/app/services/adn_service.py:91-148`). Se o usuário informa um número maior, o contador sobe até ele (`adn_service.py:123-128` e `:920`). Não consulta a Sefin para conferir. |
| https://github.com/nfse-nacional/nfse-php (PHP, biblioteca) | `ac855fa750f074865a4df55c419d7e204abeb2ca` | MIT | Expõe GET e HEAD `/dps` (`src/Http/Client/SefinClient.php:154-207`) e a distribuição com `cpfConsulta` para inscrição de 11 dígitos (`src/Http/Client/AdnClient.php:121-131`). Não achei contador de número na lista de arquivos de `src/`; a numeração fica com quem usa a biblioteca (INFERÊNCIA). |
| https://github.com/renan20553/baixar_nfse_portal_nacional (Python, só distribuição) | `c0e68e42c6a0be37d887fc880c1138ae6eceac8e` | MIT | Não emite. Guarda o próximo NSU por CNPJ e por ambiente (`README.md:101`) e trata 404 com E2220 ou E2230 como fim (`nfse/downloader.py:225-229`). |
| https://github.com/gildonei/nfse-nacional (PHP, biblioteca) | `0e49c5f5d3f5a44da8150d46c00a60f1ca435f0e` | MIT | Expõe GET e HEAD `/dps`; o comentário do HEAD diz 200 se encontrado e 404 se não (`src/Application/Service/SefinNacionalService.php:122-156`). Não achei contador de número na árvore de arquivos (INFERÊNCIA). |
| https://github.com/gsse/nfse-core-client (PHP, biblioteca) | `cc2b65a6edf3a541f55c5c986e9539581b5ec54d` | sem licença na API do GitHub | Só a consulta da DPS na interface do cliente (`packages/nfse-client/src/Contracts/NfseClientInterface.php:20`). Não achei contador de número na árvore (INFERÊNCIA). |
| https://github.com/pedrocasado/nfse-php (PHP, biblioteca) | `f3f49d91693bf4bad471bfa2c8e31cb4422411a3` | sem licença na API do GitHub | Só tipos de resposta da DPS em `src/DTO/`. Não achei contador de número na árvore (INFERÊNCIA). |
| https://github.com/LibreCodeCoop/nfse-php (PHP, biblioteca) | `107b0971122c6d7dc293bf865b275b2607d4c619` | AGPL-3.0 | O HEAD `/dps` serve para "recovery after ambiguous network failures without re-sending the DPS" (`src/Contracts/DpsLookupInterface.php:19-25`). Não achei contador de número na árvore (INFERÊNCIA). |
| Webmania, artigo de ajuda de 28/10/2025 (comercial): https://ajuda.webmania.com.br/pt-BR/articles/12680564-mei-como-localizar-a-serie-e-numero-do-proximo-dps-da-nfs-e-padrao-nacional | não se aplica | não se aplica | O usuário abre a última nota no portal, lê a série e o número da DPS no DANFSe ou no XML e digita no painel (leitura por resumo). |

O que se aprende (INFERÊNCIA a partir do lido):

1. Nos nove repositórios e no artigo lidos, quem emite tem a fonte do número num contador local com reserva atômica, como a NL-3.7 já prevê; as cinco bibliotecas sem contador deixam o número com quem as usa.
2. Nos projetos lidos que tratam disso, a correção depois de perda é **manual**: o usuário informa o próximo número. Nenhum deles faz varredura para a frente na Sefin. A DA-2.93 está à frente do que li, não atrás.
3. GET e HEAD `/dps` aparecem só para resolver uma DPS específica cujo resultado ficou em dúvida.
4. Dois projetos usam o ADN para reconstruir as notas emitidas depois de perder o banco, e um deles relê uma janela de NSU para pegar documento atrasado.

Procurei e não achei:

- GitHub: buscas de código por uso de HEAD em `/dps`, por "proximoNumeroDps", "ultimoNumeroDps", "dpsExiste" e "Dps_Head" voltaram vazias ou sem relação. A busca de código do GitHub atingiu o limite de 10 por minuto no meio da pesquisa; continuei pela árvore de arquivos de cada repositório.
- GitLab (API pública de projetos, termo "nfse"): `harissonmatos/nfse-nacional-el`, commit `a7a4ca70978a1d44310ad150ad654193a8f7582c`, é para um provedor municipal (padrão ABRASF), não para a Sefin Nacional; `nfse-nacional/nfse-nacional-sdk` não tem ramo padrão nem arquivos pela API. Nenhum dos dois trata a retomada.
- Codeberg: a API de busca respondeu 429 em 24/11/2026. Não insisti. Fica em aberto.
- ACBr: a busca no fórum não achou tópico sobre recuperar o número pela API. Não li o código do ACBr (repositório SVN), então não afirmo nada sobre ele.

## 5. Repositórios da Receita (NL-I.307)

- FATO (árvore de arquivos pela API do GitHub, nos SHAs fixados em 17/11): `calculadora-rtc-regime-regular` em `150cccbe414d4a5edea703e4f7a424af5aa222c7` tem `NfseController.java` e modelos de base de cálculo de IBS e CBS para NFS-e; `calculadora-rtc-simples-nacional` em `c5b0f01c8351c3f0d0795cc617b29d233e9f98df` não tem nenhum arquivo com Sefin, ADN, DPS ou NSU no nome; `calculadora-rtc-gerador-danfse` em `8d79e6b2f7fad6375340afcb51c85ed370c64a14` (ainda o topo do `main` em 24/11) gera o DANFSe a partir do XML.
- FATO, pesquisas de 17/11: o único endereço do sistema nacional achado nesses repositórios é a URL da Consulta Pública no QR do DANFSe (`/var/tmp/notalivre-logs/receita-gerador-danfse.md:31`).
- Conclusão: o FATO do CTO se confirma. Os três não falam com a API da Sefin nem com o ADN, e nenhum muda a conclusão desta pesquisa.

## 6. Medição que fecharia as lacunas

O que só uma chamada real responde: os códigos do `/dps` no NotaLivre (INF-09), se o ADN entrega ao emitente a própria nota, e se o fim da distribuição vem como 404 E2220. Tudo isso cabe numa execução só, na Produção Restrita, com o certificado do líder, porque lá já existe uma NFS-e da série 00001 (DPS 9, `docs/pesquisa/onda2_producao_restrita.md:15`). O pedido ao líder deve ser um comando único, sem roteiro de desenvolvedor (memória do projeto "Líder age como usuário"). O comando faria, uma chamada por vez:

1. HEAD e GET na DPS 9 (esperado 200; o GET traz a chave);
2. HEAD e GET na DPS 8, recusada (esperado 404; confirma que recusa e ausência são iguais);
3. HEAD e GET no primeiro número ainda sem uso (esperado 404);
4. HEAD e GET num Id mal formado (esperado 400);
5. GET `/DFe/0` no ADN da Produção Restrita, sem parâmetro e com `lote=true` (esperado: a NFS-e da DPS 9 no `LoteDFe`, se o ADN entrega a nota própria; ou 404 E2220, se não entrega).

Ele grava status, cabeçalhos e corpo de cada resposta, sem dado pessoal, para o main ler.

## 7. O que isso muda na NL-3.11

Recomendações ao CTO. Nenhuma decisão foi tomada aqui.

1. **Usar o GET, não o HEAD, na varredura.** O certificado do emitente é ator da própria DPS, então o GET é permitido (seção 1.2). Ele custa uma chamada, como o HEAD, e devolve a chave. Com a chave, o GET `/nfse/{chave}` traz o XML de cada nota emitida depois do backup e reconstrói o registro local que se perdeu. O 404 do GET também traz corpo, o que separa ausência de rota errada. O HEAD continua útil só para quem não é ator, o que não é o caso. Isso muda a letra da DA-2.93, que fala em HEAD.
2. **Âncora antes de contar ausências.** Começar a varredura por uma chamada a um Id que se sabe existente e exigir 200. Contar como ausência só o 404 de negócio. 400, 403, 5xx, 429, timeout e erro de transporte nunca contam como ausência: param a varredura com resultado indeterminado.
3. **K = 20, configurável, piso 10** (seção 3.5). Uma chamada por vez, com intervalo configurável, e respeito a `Retry-After` se ele vier, porque não há limite oficial publicado.
4. **Conferência antes do lote mais barata que a escolhida pelo usuário.** Com o registro local íntegro, as 21 chamadas por lote protegem contra casos que a P3 (série por instalação) e a escolha de série depois de restauração já tratam. O CTO pode avaliar um K menor só antes do lote, mantendo o K cheio quando o usuário escolhe "conferir", com a E0014 como rede de segurança.
5. **E0014 de digest diferente continua virando pulo e volta à escolha**, como o canon já prevê (seção 3.6). A NL-3.11 só precisa garantir que a varredura e esse tratamento usam o mesmo registro de "número ocupado".
6. **Frase residual na spec (L-67).** A linha 231 da spec ainda atribui à NT 007, p. 9, a proibição de reutilizar o número da DPS, enquanto a R4 (linha 84) e a P3 (`plano:152`) já dizem que a NT trata só do nNFSe (seção 3.3). Corrigir é decisão de canon, a levar ao líder. A escolha de não reutilizar o nDPS depois de uma recusa é de desenho, e é ela que define o tamanho dos buracos e, com isso, o K.
7. **ADN só depois de medir.** O ramo "ADN por NSU sem registro local" depende de o ADN entregar ao emitente a própria nota, e as fontes oficiais se contradizem (seção 2.1). Recomendo marcar esse ramo como INFERÊNCIA na fatia e pedir a medição da seção 6 antes de implementá-lo. Se a medição confirmar, o ADN também dá o maior nDPS emitido da série (está no `infDPS` de cada nota), sem depender de K.
8. **Se o ADN entrar:** cursor pelo maior `NSU` recebido, deduplicação por NSU (o NSU pode ser exclusivo ou inclusivo, seção 2.3), releitura de uma janela de NSU para pegar documento atrasado, fim no 404 E2220, espera mínima de 1 hora antes de nova leitura quando não houver novidade, e, para emitente CPF, consulta sem `cpfConsulta`.
9. **Parsers diferentes para Sefin e ADN.** A Sefin devolve `erro` em camelCase; o ADN devolve `Erros` e `Alertas` em PascalCase, e usa 400 e 404 com corpo de negócio (seções 1.1 e 2.3). O servidor falso (NL-2.6) precisa dos dois formatos, marcados como INFERÊNCIA até a medição.
10. **Fixtures do servidor falso para a NL-3.11:** conferência escolhida pelo usuário com buraco de 8 números antes da última nota; varredura que para cedo e recebe E0014 de digest diferente; âncora com 404 de rota; 429 com `Retry-After`; 5xx no meio da varredura.

## Fontes consultadas

| Fonte | Tipo | Resultado |
|---|---|---|
| Manual de Contribuintes, API do Emissor Público (SHA-256 `ac2f36...`) | Oficial | Seções 1.4.1 e 1.4.2: GET exige ator; HEAD aceita qualquer certificado válido; sem códigos HTTP |
| Manual de Contribuintes, APIs do ADN 1.0 (SHA-256 `9ffc97...`) | Oficial | Emitente, tomador ou intermediário; raiz do CNPJ |
| Manual dos Contribuintes, ADN 1.1 de 03/11/2026, publicado com nome de manual de municípios (SHA-256 `031dde...`) | Oficial | `cnpjConsulta`, `cpfConsulta`, procuração; tabela de erros E2210 a E2247 e E999 com HTTP |
| Manual Integrado 1.00.02 de 2022 (SHA-256 `82096b...`, igual à cópia de terceiro) | Oficial, antigo | Até 50 por lote; espera de 1 hora; emitente não recebe a própria nota |
| FAQ v1.5 de 23/11/2026 (SHA-256 `5c138a...`) | Oficial | Assuntos 6.7, 18.1 a 18.6, 19.1 a 19.5 e 26.5 |
| NT 007 v1.0 (SHA-256 `24e46b...`) | Oficial | p. 8 e 9 tratam do nNFSe |
| Anexo I da Produção Restrita v1.01 de 20/10/2026 (SHA-256 `72b059...`) | Oficial | E0014; sem regra de ordem do nDPS |
| https://sefin.nfse.gov.br/SefinNacional/docs/index | Oficial | 403 em 24/11/2026 |
| https://adn.producaorestrita.nfse.gov.br/contribuintes/docs/index.html e https://adn.nfse.gov.br/contribuintes/docs/index.html | Oficial | Falha no aperto de mão TLS |
| https://github.com/nfse-nacional/nfse-php @ `ac855fa` | Cópia de terceiro dos Swagger; biblioteca | Códigos 200, 400 e 404 do `/dps`; esquemas do ADN |
| https://github.com/Fm-s/open-nfse @ `18d70dd` | Terceiro | Contador local; `/dps` para reconciliação; sem 429 documentado |
| https://github.com/RicardoBiazin/nfse-nacional @ `a8e9b5d` | Terceiro | ADN real com notas emitidas; sem último e maior NSU; NSU atrasado |
| https://github.com/cleitonleonel/nfse_manager @ `09dcfc9` | Terceiro | Contador local com número informado pelo usuário |
| https://github.com/renan20553/baixar_nfse_portal_nacional @ `c0e68e4` | Terceiro | NSU exclusivo; fim com E2220 ou E2230 |
| https://github.com/vasfvitor/nanci @ `8b31aa8` | Terceiro | Afirma `ultNSU` e `maxNSU` na resposta |
| gildonei/nfse-nacional, gsse/nfse-core-client, pedrocasado/nfse-php e LibreCodeCoop/nfse-php, nos commits da seção 4 | Terceiro | GET e HEAD `/dps`; sem contador de número |
| https://www.contabeis.com.br/forum/tecnologia-contabil/416021/download-em-lote-de-nfs-e-do-padrao-nacional-como-automatizar-pela-api-do-adn/ | Fórum, leitura por resumo | ADN devolve as notas emitidas |
| https://ajuda.webmania.com.br/pt-BR/articles/12680564-mei-como-localizar-a-serie-e-numero-do-proximo-dps-da-nfs-e-padrao-nacional | Comercial, leitura por resumo | Número da DPS digitado pelo usuário |
| GitLab (API pública) e Codeberg (API, 429) | Busca | Nada sobre retomada; Codeberg em aberto |
| ReceitaFederal, os 3 repositórios, nos SHAs de 17/11 | Oficial (código) | Não cobrem a API da Sefin nem o ADN |

**Não achei:** limite de taxa oficial da Sefin ou do ADN além da espera de 1 hora; o código HTTP do GET negado a quem não é ator; texto oficial atual que diga, sem ambiguidade, se o emitente recebe a própria nota pelo ADN; atraso medido entre a emissão na Sefin e a chegada ao ADN; o Swagger publicado hoje (403 e falha de TLS); projeto livre que faça varredura para a frente na Sefin.
