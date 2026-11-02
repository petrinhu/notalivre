# Parâmetros municipais da API do Sistema Nacional NFS-e

**Fatia:** NL-2.10h, H2 (pesquisa, sem código). **Data:** 02/11/2026. **Base:** onda-2 (2eeda1c). **Ordem:** `/var/tmp/notalivre-logs/NL-2.10h-ordem.md`, seção H2. **Decisão relacionada:** DA-2.98 (a ratificar pelo líder).

**Rótulos usados.** FATO: o texto está em fonte citada com URL e trecho curto. INFERÊNCIA: conclusão de quem pesquisou, a partir de fatos ou de fontes secundárias. SECUNDÁRIO: relato de terceiro, não verificado em fonte oficial. NÃO ENCONTRADO: procurado e não achado, com a busca descrita.

**Regras desta pesquisa.** Nenhuma URL de rota `/parametros_municipais` foi chamada. Nenhum certificado foi usado. Nenhum arquivo do líder (.pfx, senha, arquivos de lembrar) foi aberto. A captura de tela do líder citada na ordem (fato F3) não foi aberta por mim; o fato consta aqui como "ordem H2, F3", sem verificação independente. Nada foi instalado e nenhum código do projeto foi executado.

---

## Resumo das respostas

| Pergunta | Resposta | Rótulo |
|---|---|---|
| Caminho | Manual dos contribuintes v1.2: `/parametros_municipais/{codigoMunicipio}/{codigoServico}`. Manual dos municípios v1.2: `/parametros_municipais/{codigoMunicipio}/aliquotas`. Os dois manuais divergem nesta rota. | FATO (divergência, seção 2) |
| Host, Produção Restrita | Oficial só para a documentação (Swagger): `adn.producaorestrita.nfse.gov.br`. Host da API de parâmetros: NÃO CONFIRMADO. Secundário: `sefin.producaorestrita.nfse.gov.br`. | FATO (Swagger) / SECUNDÁRIO / NÃO ENCONTRADO |
| Host, Produção | Oficial: NÃO ENCONTRADO. Secundário: `sefin.nfse.gov.br`. | NÃO ENCONTRADO / SECUNDÁRIO |
| Método | GET (nos dois manuais) | FATO |
| Certificado de cliente (mTLS) | Os manuais não dizem nada sobre certificado nesta rota. | NÃO ENCONTRADO. INFERÊNCIA: provável mTLS do solicitante, seção 3 |
| Formato da resposta | Não consta nenhum campo JSON. Não consta flag "administrado pelo município", lista de cTribMun, obrigatoriedade do código complementar nem alíquota. | NÃO ENCONTRADO |
| Códigos de erro e limite de taxa | Nenhum código de erro e nenhum limite de requisições em nenhum dos manuais. | NÃO ENCONTRADO |
| Rotas irmãs | Contribuinte: `/convenio`; `/{codigoMunicipio}/{CPF/CNPJ}` (descrita duas vezes, como retenções e como benefícios). Municípios: `/convenio`, `/aliquotas`, `/regimes_especiais`, `/retencoes`, `/beneficiomunicipal/{nBM}`, e três POST de manutenção com `{idManut}`. | FATO |
| Recife (2611606) e 040101 na Produção Restrita | Nenhum relato verificável. A portaria do Recife não traz tabela de serviços. | NÃO ENCONTRADO |
| E0312 | Texto oficial no Anexo I de Produção Restrita: trata do cTribNac, não do cTribMun. Relatos de terceiros associam o erro ao cTribMun em alguns casos. | FATO (texto) / SECUNDÁRIO (relatos) |

---

## 1. Fontes e acesso

| Fonte | Como foi acessada | Resultado |
|---|---|---|
| Manual dos contribuintes v1.2 (out/2025) | `https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf`. WebFetch não extraiu o texto (PDF com fluxo comprimido). Download por curl e `pdftotext -layout` deram o texto completo (6 páginas). | Lido |
| Manual dos municípios v1.2 (out/2025) | `https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-municipios-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out21025.pdf` (typo "out21025" no nome do arquivo). curl e `pdftotext` (6 páginas). | Lido |
| Swagger do contribuinte, Produção Restrita | `https://adn.producaorestrita.nfse.gov.br/contribuintes/docs/index.html` (link do manual, seção 1.6). curl e WebFetch falharam no TLS (`decryption failed or bad record mac`). | NÃO ACESSÍVEL nesta sessão |
| Swagger dos municípios, Produção Restrita | `https://adn.producaorestrita.nfse.gov.br/municipios/docs/index.html` (seção 1.5 do manual dos municípios). Não tentado de novo; é o mesmo host e o mesmo problema previsível. | NÃO ACESSADO |
| Swagger da Sefin, Produção Restrita | `https://sefin.producaorestrita.nfse.gov.br/API/SefinNacional/docs/index` (caminho citado por terceiro). curl respondeu HTTP 403 "Forbidden: Access is denied." | BLOQUEADO (403) |
| Anexo I (DPS), Produção Restrita, 20/10/2026 | Arquivo `anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx` (URL em `docs/pesquisa/dominio.md`). Usei uma cópia já baixada em `/var/tmp/notalivre-rev111/dl/`, lida só com `unzip -p` do `sharedStrings.xml`. | Lido (textos, sem ordem de células) |
| Portaria 12/2026 da Prefeitura do Recife | `https://recifeemdia.recife.pe.gov.br/sites/default/files/Portaria%20N%2012%2C%20de%2029%20de%20abril%20de%202026.pdf` (2 páginas, pdftotext). | Lido |
| Pacote PyPI `pynfse-nacional` 0.9.5 (14/07/2026) | `https://pypi.org/project/pynfse-nacional/`, via WebFetch (resumo). Biblioteca de terceiros. | SECUNDÁRIO |
| Repositórios com clientes da API | `https://github.com/ideia4it/nfse` (README, via WebFetch); `https://github.com/nfse-nacional/nfse-php` (README, via WebFetch). Lidos só para estrutura, sem código copiado (L-37). A busca por `parametros_municipais` no GitHub não retornou arquivo com a rota. | SECUNDÁRIO |
| Fóruns e blogs | ACBr (tópicos 75643, 88728, 92867), Omie, Asaas, nfe.io (Recife). Via WebFetch (resumo). | SECUNDÁRIO |

**Observações de acesso.** Os dois PDFs têm o mesmo autor e a mesma data de criação no metadado: Elisane Rodovanski, Microsoft Word 365, 06/10/2025 (manual dos contribuintes às 15:10:22 e manual dos municípios às 15:12:39, horário de -03). O histórico interno dos dois lista só a versão "1.0 17/03/2025". A "v1.2" aparece no nome do arquivo e nas referências do próprio repositório (`docs/pesquisa/dominio.md`), não no histórico do documento. FATO.

---

## 2. Caminho, método e host

### 2.1 Caminho (FATO, divergência entre os dois manuais)

**Manual dos contribuintes v1.2, seção 1.2.1 "Processamento"** (URL da seção 1). Trechos literais, sem o separador do original (traço entre o método e o caminho):

- `GET /parametros_municipais/{codigoMunicipio}/convenio`: "Consulta os parâmetros do convênio de um município."
- `GET /parametros_municipais/{codigoMunicipio}/{codigoServico}`: "Consulta as alíquotas, regimes especiais de tributação e deduções/reduções por subitens da lista de serviço."
- `GET /parametros_municipais/{codigoMunicipio}/{CPF/CNPJ}` (item c): "Consulta as retenções a que um contribuinte tem o dever de recolher para um município."
- `GET /parametros_municipais/{codigoMunicipio}/{CPF/CNPJ}` (item d): "Consulta os benefícios municipais a que um contribuinte tem direito em um município."

**Manual dos municípios v1.2, seção 1.1.2 "Métodos"** (URL na seção 1). Mesma seção de convênio, com rotas diferentes:

- `GET /parametros_municipais/{codigoMunicipio}/convenio`: "Consulta os parâmetros do convênio de um município."
- `GET /parametros_municipais/{codigoMunicipio}/aliquotas`: a mesma descrição de alíquotas, regimes especiais e deduções por subitem que o manual dos contribuintes atribui a `{codigoServico}`.
- `GET /parametros_municipais/{codigoMunicipio}/regimes_especiais`: com a descrição de retenções.
- `GET /parametros_municipais/{codigoMunicipio}/retencoes`: com a descrição de benefícios.
- `GET /parametros_municipais/{codigoMunicipio}/beneficiomunicipal/{nBM}`: com a descrição de benefícios.
- `POST /parametros_municipais/{codigoMunicipio}/beneficiomunicipal/{idManut}`: "Realiza manutenção em um benefício municipal já existente em um município."
- `POST /parametros_municipais/{codigoMunicipio}/retencoes/{idManut}`: "Realiza manutenção em uma retenção já existente em um município."
- `POST /parametros_municipais/{codigoMunicipio}/regimes_especiais/{idManut}`: "Realiza manutenção em um regime especial de tributação já existente em um município."

**Leitura.** FATO: o manual dos contribuintes atribui a descrição de alíquotas ao segundo parâmetro de path (`{codigoServico}`). O manual dos municípios atribui a mesma descrição à rota `/aliquotas`, e não traz a rota `{codigoServico}`. Os dois são o mesmo grupo de API (parâmetros municipais) na mesma versão de nome de arquivo.

INFERÊNCIA: o manual dos municípios é o que descreve o estado mais novo da rota, porque traz verbos de manutenção e rotas nomeadas (`aliquotas`, `beneficiomunicipal/{nBM}`) que o manual dos contribuintes não tem. Não há como provar a data relativa. Esta é uma hipótese, não um fato.

INFERÊNCIA: o parâmetro `{codigoServico}` pode ser o cTribNac (6 dígitos) ou o cTribMun. O manual não diz qual. O texto "por subitens da lista de serviço" aponta para o cTribNac, mas não confirma. NÃO ENCONTRADO: definição de `{codigoServico}` no manual.

**Descrições trocadas no manual dos municípios (FATO).** A rota `regimes_especiais` tem a descrição de retenções, e a rota `retencoes` tem a descrição de benefícios. A descrição de benefícios aparece também em `beneficiomunicipal/{nBM}`. O manual dos contribuintes repete o mesmo erro com `{CPF/CNPJ}` (duas descrições para o mesmo path). Não se deve confiar nas descrições dos manuais sem teste.

### 2.2 Host (FATO parcial, NÃO CONFIRMADO para a API)

- FATO: manual dos contribuintes, seção 1.6: "Foi disponibilizada uma Swagger em ambiente de produção restrita destinada a testes por parte dos municípios conveniados." Link: `https://adn.producaorestrita.nfse.gov.br/contribuintes/docs/index.html`.
- FATO: manual dos municípios, seção 1.5: link `https://adn.producaorestrita.nfse.gov.br/municipios/docs/index.html`.
- Os manuais não mostram a URL base da API de parâmetros. Não há host de Produção (não restrita) em nenhum dos dois.
- SECUNDÁRIO: `pynfse-nacional` 0.9.5, via WebFetch: "Homologação (produção restrita): `sefin.producaorestrita.nfse.gov.br`" e "Produção: `sefin.nfse.gov.br`". O mesmo pacote cita documentação em `/API/SefinNacional/docs/index` nos dois hosts. A biblioteca `brans-nfe` (resultado de busca, não aberto) cita os mesmos hosts.
- INFERÊNCIA: a API de parâmetros fica no host `sefin.*` (com base `/API/SefinNacional/`), e o Swagger fica no host `adn.*`. Essa separação é a mais provável a partir das fontes, mas não está confirmada. O README do cliente `ideia4it/nfse` não usa o prefixo `/API/SefinNacional/` na documentação de parâmetros (leitura de resumo), o que enfraquece a inferência.
- NÃO ENCONTRADO: URL de produção (não restrita) da API de parâmetros em fonte oficial. Procurei no manual dos contribuintes, no manual dos municípios, no Anexo I de Produção Restrita e nos resultados de busca. Os únicos hosts de produção que apareceram são secundários.

**Ponto para a fatia H3.** O host que serve `/parametros_municipais` precisa ser confirmado por uma chamada real autorizada pelo líder. Esta pesquisa não resolve isso.

### 2.3 Método (FATO)

GET, nas quatro rotas de leitura, nos dois manuais.

---

## 3. Certificado de cliente (mTLS)

- NÃO ENCONTRADO: nenhum dos dois manuais diz se a rota de parâmetros exige certificado de cliente, nem de quem. A seção 1.2 descreve o serviço sem mencionar certificado.
- FATO, por contraste: o manual dos contribuintes exige certificado para os endpoints de DPS. Seção 1.4.1: "a chave de acesso será informada somente se a identificação do usuário do certificado digital da conexão solicitante for um ator (Prestador, Tomador ou Intermediário)". Seção 1.4.2: o HEAD "atende a qualquer usuário desde que realize a consulta com um certificado digital válido". Isso mostra que a Sefin identifica o solicitante pelo certificado nas rotas de DPS, não que a rota de parâmetros exija o mesmo.
- FATO: o README do cliente `ideia4it/nfse` diz que a biblioteca "assina a DPS com o certificado A1 do emitente". Não fala de consulta de parâmetros.
- SECUNDÁRIO: `pynfse-nacional` diz que cobre "autenticação mTLS com certificados ICP-Brasil", sem separar rota.
- SECUNDÁRIO, sem valor como prova: tópico ACBr 92867 (mensagem de "12 de junho", ano não visível). O autor diz ter consultado parâmetros do município 2304400 e depois ter recebido E999 no POST da DPS. O post não diz qual rota foi usada nem qual certificado, e não tem resposta.
- INFERÊNCIA: é provável que a rota de parâmetros use mTLS do solicitante, pelo padrão de identificação por certificado do resto da API. Não é fato, e não deve ser escrito como exigência até uma chamada real confirmar.
- Acesso ao Swagger e à Sefin nesta pesquisa foi bloqueado (TLS e 403). Essa falha não prova que o docs está atrás de um certificado; é só uma falha de acesso. Não tratar como evidência da exigência.

---

## 4. Formato da resposta

Nenhum dos manuais descreve o JSON de resposta de nenhuma rota de parâmetros. NÃO ENCONTRADO para:

- campo de "administrado pelo município" (ou equivalente);
- lista de cTribMun (ou de códigos complementares) por serviço;
- indicador de obrigatoriedade do código complementar;
- campos de alíquota, de regime especial, de convênio e de benefício;
- nome de qualquer chave JSON de resposta.

**O que o Anexo I oficial diz sobre alíquota e parametrização (FATO).** Fonte: Anexo I de Produção Restrita, 20/10/2026, textos do `sharedStrings.xml`. A busca foi pelo texto, não por célula; as frases abaixo são literais.

- Alíquota: "Se o município de incidência pertence ao Sistema Nacional NFS-e a alíquota estará parametrizada e, portanto, será fornecida pelo sistema." Oposto: "Se o município de incidência não pertence ao Sistema Nacional NFS-e a alíquota não estará parametrizada e, por isso, deverá ser fornecida pelo emitente."
- Alíquota por código de serviço: "o percentual de alíquota previamente parametrizado pelo município de incidência no código de serviço, que foi indicado pelo emitente na DPS, quando o município de incidência do ISSQN identificado pelo sistema é conveniado ao Sistema Nacional NFS-e".
- Município emissor: "O município emissor informado na DPS deve estar parametrizado para utilizar os emissores públicos nacionais, conforme parametrização do município no Sistema Nacional NFS-e", com exceções para MEI e para ME/EPP no Simples Nacional.
- API própria: "O emitente poderá obter essa informação de parametrização do município através de API própria que dará publicidade às parametrizações dos municípios." O texto está na descrição do identificador de benefício municipal (nBM), que tem 14 posições: 7 do IBGE, 2 do tipo de parametrização (01 legislação, 02 regimes especiais, 03 retenções, 04 outros benefícios) e 5 sequenciais.
- cTribMun: "Código de tributação municipal do ISSQN." A definição não diz se o campo é obrigatório. NÃO ENCONTRADO: regra de obrigatoriedade por município no Anexo I de Produção Restrita. (Não consultei a aba `MUN.INCID_INFO.SERV` citada na linha de cTribNac, por não estar nos textos compartilhados.)

**O que o emissor web diz (FATO do repositório, não da API).** `docs/pesquisa/campos_emissor_web.md`, linhas 92 e 93: `serv/cServ/cTribMun` é "obrigatorio se flag CodigoComplementarMunicipalObrigatorio=='true' (hidden do servidor)". A flag é definida pelo servidor, e o repositório não diz como a API expõe essa informação. INFERÊNCIA: a obrigatoriedade do complementar é decidida pelo servidor da Sefin por município e por serviço. A API de parâmetros pode ou não expô-la.

**Descrição de rota e formato de chave (SECUNDÁRIO).**
- `ideia4it/nfse` (README, via WebFetch): lista "Parâmetros municipais do ADN: convênio, alíquotas, regimes especiais, retenções e benefícios". Diz que as chaves `beneficio` e `regimesEspeciais` "que o swagger não mostra" estão não confirmadas. Isso indica que o Swagger contém a definição das chaves de resposta, mas o acesso ao Swagger falhou aqui. O README não traz nome de chave confirmado. Data do último commit: não visível.
- `nfse-nacional/nfse-php` (README, via WebFetch): usa métodos `consultarParametrosConvenio()` e `consultarAliquota()`. Não mostra URL nem campos.

---

## 5. Códigos de erro e limite de taxa

- NÃO ENCONTRADO: códigos de erro HTTP ou de negócio da rota de parâmetros em nenhum dos manuais, no Anexo I ou nos repositórios lidos.
- NÃO ENCONTRADO: limite de requisições por minuto, por dia ou por certificado.
- Os códigos E0310 a E0315 do Anexo I são de validação da DPS (POST `/nfse`), não de parâmetros. Não se aplicam diretamente à rota de consulta. Ver seção 8.
- SECUNDÁRIO: `pynfse-nacional` 0.9.5 diz: "A API de parametrização (alíquotas por serviço) está com problemas no ambiente de homologação." (WebFetch, resumo). Isso é um risco para a fatia H3: a consulta pode falhar em Produção Restrita por motivo da própria API, sem relação com o serviço. O pré-voo de H3 deve degradar com aviso, não bloquear a emissão sem diagnóstico.

---

## 6. Rotas irmãs

| Rota | Manual | Descrição no manual | Rótulo |
|---|---|---|---|
| `GET /parametros_municipais/{codigoMunicipio}/convenio` | contribuintes e municípios | "Consulta os parâmetros do convênio de um município." | FATO |
| `GET /parametros_municipais/{codigoMunicipio}/{CPF/CNPJ}` | contribuintes | retenções (item c) e benefícios (item d), mesmo path | FATO (descrição duplicada) |
| `GET /parametros_municipais/{codigoMunicipio}/aliquotas` | municípios | alíquotas, regimes especiais e deduções por subitem | FATO |
| `GET /parametros_municipais/{codigoMunicipio}/regimes_especiais` | municípios | descrição de retenções (trocada) | FATO |
| `GET /parametros_municipais/{codigoMunicipio}/retencoes` | municípios | descrição de benefícios (trocada) | FATO |
| `GET /parametros_municipais/{codigoMunicipio}/beneficiomunicipal/{nBM}` | municípios | descrição de benefícios | FATO |
| `POST /parametros_municipais/{codigoMunicipio}/beneficiomunicipal/{idManut}` | municípios | manutenção de benefício municipal (é do município, não do contribuinte) | FATO |
| `POST /parametros_municipais/{codigoMunicipio}/retencoes/{idManut}` | municípios | manutenção de retenção | FATO |
| `POST /parametros_municipais/{codigoMunicipio}/regimes_especiais/{idManut}` | municípios | manutenção de regime especial | FATO |

INFERÊNCIA: as três rotas POST são de manutenção do município e não devem ser chamadas pelo app. O README de `ideia4it/nfse` diz o mesmo, em resumo: as alterações "pertencem ao município". SECUNDÁRIO.

Rotas de DPS, para contexto (FATO, manual dos contribuintes 1.4): `GET /dps/{id}` e `HEAD /dps/{id}`. O `{id}` é formado por código IBGE do município emissor (7), tipo de inscrição (1), inscrição federal (14, "CPF completar com 000 à esquerda"), série DPS (5) e número DPS (15).

---

## 7. Recife (IBGE 2611606) e código 040101 na Produção Restrita

- FATO, Portaria 12/2026 da Prefeitura do Recife (29/04/2026): "Regulamenta a emissão da Nota Fiscal de Serviços Eletrônica (NFS-e)." e cita "a adesão do Município do Recife ao Emissor Nacional de NFS-e após a aprovação da Reforma Tributária". A portaria não traz tabela de serviços nem de códigos. O artigo 8 lista regimes especiais (autônomos, sociedades simples, MEI, cooperativas, notariais). NÃO ENCONTRADO: tabela de cTribMun do Recife na portaria.
- FATO, nfe.io, página "Recife (PE, 2611606)", via WebFetch: a coluna "Emissor Nacional" tem "Sim" para Recife. A página diz "Classificação do serviço (CNAE e/ou código do serviço conforme o município)". Não menciona cTribMun, código complementar, 040101, 04.01.01, 501, E0312 nem parametrização. Data da página: não consta.
- FATO, ordem H2 (F3): captura do líder do Emissor Nacional de Produção, com "04.01.01 Medicina" e "04.01.01.501 Medicina" como códigos obrigatórios. Não verifiquei esta captura, por regra de não abrir arquivos do líder. Trato como fato da ordem, não como fato desta pesquisa.
- FATO, Anexo I, Produção Restrita: os códigos cTribNac e cTribMun são validados contra "a lista de serviços nacional" e contra "o município de incidência". O Anexo não traz a lista do Recife.
- NÃO ENCONTRADO, procurado em buscas por "Recife NFS-e nacional código complementar municipal medicina 04.01.01 501 cTribMun" e por "Recife 040101 Produção Restrita" e variantes: nenhum relato sobre Recife com 040101 ou com o código complementar 501. O resultado mais próximo é a tabela de Recife de 2012 em notícia COAD (item 4.01 "Medicina e biomedicina"), que é antiga e não traz o código atual.
- NÃO ENCONTRADO: qualquer informação sobre se Recife administra 040101 na Produção Restrita. Essa é a pergunta H1 da ordem e continua aberta. O único jeito de respondê-la é a consulta real da rota, pelo líder.

---

## 8. E0312 (texto oficial e relatos de terceiros)

**Texto oficial (FATO), Anexo I de Produção Restrita, 20/10/2026.** O texto aparece com a regra antes do código e a mensagem depois. Pelo padrão das linhas, que se repete em E0310 e E0312, a regra vem antes do código e a mensagem depois. Essa disposição é INFERÊNCIA de layout.

- E0312, regra: "Verificar se o código de tributação nacional informado está administrado pelo município de incidência do ISSQN na data de competência informada na DPS, conforme a lista de serviços nacional do Sistema Nacional NFS-e." Com a exceção: "Exceto quando o emitente da DPS for MEI na data de competência da emissão da NFS-e."
- E0312, mensagem: "O código de tributação nacional informado não está administrado pelo município de incidência do ISSQN na data de competência informada na DPS, conforme a lista de serviços nacional do Sistema Nacional NFS-e."
- E0313, regra: "Não é permitido informar o código de tributação municipal quando o emitente da DPS for MEI na data de competência da emissão da NFS-e."
- E0313, mensagem (texto seguinte à linha): "Verificar se o código de tributação municipal informado existe e está administrado pelo município de incidência do ISSQN na data de competência informada na DPS."
- E0314, mensagem: "O código de tributação municipal informado não existe ou não está administrado pelo município de incidência do ISSQN na data de competência informada na DPS".
- E0315, regra: "Não é permitido informar 000 para o codigo de tributação municipal na DPS."

INFERÊNCIA: a ordem H2 descreve E0313 como "cTribMun proibido para MEI", E0314 como "cTribMun, se informado, existe e é administrado" e E0315 como "Não é permitido informar 000". Essa leitura bate com o layout. Não há divergência entre a ordem e o Anexo.

**Fato central.** O texto oficial de E0312 fala de cTribNac. O cTribMun aparece nas mensagens E0313 a E0315. Portanto, a hipótese H2 da ordem (que E0312 apareça por falta de cTribMun obrigatório) não é a leitura literal do Anexo. É uma hipótese com peso secundário, não oficial.

**Relatos de terceiros (SECUNDÁRIO).**
- ACBr, tópico 88728, "Erro E0312 ao emitir NFSe": resposta de "farnetani" em 04/01/2026, marcada como solução. A resposta diz que não estava informando o cTribMun, "que no caso de Belo Horizonte tem que ser 001". O mesmo autor, em 23/02/2026, escreve que o cTribMun "não deve ser fixo em 001 para todos os serviços: cada serviço tem código definido pela prefeitura", e anexa planilhas da própria prefeitura de BH. Os trechos vêm de resumo do WebFetch, não foram conferidos literalmente no navegador.
- Artigo do Asaas, título "O Código Complementar Municipal não foi preenchido ou está preenchido incorretamente E0312". Só o título e o trecho do resultado de busca foram vistos. A página retornou HTTP 403 ao WebFetch.
- Artigo da Omie (`ajuda.omie.com.br`, E0312), lido por resumo do WebFetch. O resumo diz que "alguns municípios exigem que seja informado, além do código de serviço nacional", e a data "29 de setembro de 2026". O resumo também menciona um módulo "OneFlow", o que sugere que a página lida não é o artigo esperado. Confiabilidade baixa; não usar como fonte sem conferir.
- Resultados de busca não abertos (só o trecho da busca): Domínio sugere testar "001" primeiro; Alterdata descreve cTribMun como "usually 3 digits (e.g. 000, 001)"; CIGAM diz que cTribMun é opcional quando a cidade não exige. Não foram abertos.

INFERÊNCIA: há corroboração secundária de que E0312 pode ser disparado por falta ou erro de código municipal em alguns municípios, mas isso não é o texto oficial e não vale para Recife sem teste. Pela leitura do Anexo, o caminho de diagnóstico do E0312 é o cTribNac contra a lista do município. A consulta de parâmetros da fatia H3 é o discriminador certo entre as duas hipóteses.

---

## 9. Zeros à esquerda (fato colateral para a fatia H4)

- FATO, manual dos contribuintes, seção 1.4.1, e manual dos municípios, seção 1.3.1: "Inscrição Federal (14 - CPF completar com 000 à esquerda)". Ou seja, um CPF de 11 dígitos entra no identificador da DPS como 14 posições, com três zeros à esquerda.
- FATO, Anexo I de Produção Restrita: o mesmo texto aparece em "Inscrição Federal (14 - CPF completar com 000 à esquerda)".
- Esse fato reforça a ordem F8 para a H4: o CPF e o CNPJ com zero inicial precisam sobreviver ao caminho inteiro, inclusive ao identificador da DPS.

---

## 10. Divergências entre fontes

| Ponto | Fonte A | Fonte B | Situação |
|---|---|---|---|
| Rota de alíquotas | Contribuintes: `/{codigoMunicipio}/{codigoServico}` | Municípios: `/{codigoMunicipio}/aliquotas` | Não resolvida. Teste real necessário |
| Descrição de regimes e retenções | Municípios: `regimes_especiais` com descrição de retenções | Idem: `retencoes` com descrição de benefícios | Erro de descrição no manual. Não usar a descrição como guia |
| Descrição de `{CPF/CNPJ}` | Contribuintes: duas descrições para o mesmo path (retenções e benefícios) | Idem | Erro de descrição no manual |
| Host de Swagger e host de API | Manual: Swagger em `adn.producaorestrita.*` | Pacote de terceiro: docs em `sefin.producaorestrita.*/API/SefinNacional/docs` | Não resolvida |
| Estratégia para E0312 | Relatos de terceiros: cTribMun 001 resolve em BH | Anexo I: E0312 trata do cTribNac; cTribMun tem E0313 a E0315 | A leitura oficial tem prioridade para o diagnóstico |
| Formato de cTribMun | ACBr 75643 (2023): portal mostra 9 caracteres, XML 3, "Sempre são os 3 ultimos numeros" | Captura do líder (ordem F3, não verificada): "04.01.01.501" | Compatíveis: 9 dígitos com pontos no portal e os 3 finais no XML. INFERÊNCIA |

---

## 11. Relato de ACBr 75643 (formato do cTribMun, secundário)

- Tópico "campo cTribMun no NFSe nacional", 21/12/2023: o portal "busca os códigos, no tamanho 9, com suas respectivas descrições, mas no XML o tamanho do campo é 3". "Sempre são os 3 ultimos numeros do código." "são as prefeituras que cadastram os códigos dentro do próprio portal nacional." "o portal nacional não disponibiliza nenhuma consulta destes códigos." "esses códigos podem mudar a qualquer momento."
- Resposta de 22/12/2025: município Bento Gonçalves (RS). Um usuário tentou informar os 3 dígitos e o sistema "não acatou". Deixou o campo em branco. Sem solução marcada.
- Esse relato, se confirmado, diz que o portal nacional não tem consulta pública de cTribMun. Isso é relevante para H3: a lista de cTribMun pode não estar disponível pela API. NÃO ENCONTRADO: confirmação oficial.

---

## 12. NÃO ENCONTRADO (lista consolidada)

Cada item indica o que foi procurado e onde.

1. **Host de produção (não restrita) da API de parâmetros.** Procurado nos dois manuais, no Anexo I de Produção Restrita, na portaria do Recife e nos resultados de busca. Só apareceu `sefin.nfse.gov.br` em pacote de terceiro.
2. **Host da API de parâmetros em Produção Restrita, confirmado.** Os únicos hosts oficiais são os de Swagger em `adn.producaorestrita.*`.
3. **Base path da API** (por exemplo, `/API/SefinNacional/`). Só aparece em documentação de terceiro e em caminho de Swagger de terceiro, nunca em manual oficial.
4. **Certificado de cliente exigido na rota de parâmetros, e de quem.** Manuais silenciosos. Swagger inacessível.
5. **Formato JSON da resposta** de qualquer rota de parâmetros. Swagger inacessível. Clientes de terceiros não trazem exemplo verificado.
6. **Flag "administrado pelo município"** por código de serviço. Não encontrada em nenhuma fonte.
7. **Lista de cTribMun (códigos complementares)** por município ou por serviço. Não encontrada. O relato de 2023 diz que o portal nacional não tem consulta pública.
8. **Obrigatoriedade do código complementar** por município e serviço, na API. Só existe a flag hidden do emissor web (`docs/pesquisa/campos_emissor_web.md`).
9. **Campos de alíquota** na resposta. Não encontrados. O Anexo I diz que a alíquota "será fornecida pelo sistema" para município conveniado, sem dizer em qual campo de resposta.
10. **Códigos de erro HTTP ou de negócio** da rota de consulta de parâmetros. Não encontrados.
11. **Limite de taxa** (requisições por tempo). Não encontrado em manual, Anexo, README ou fórum.
12. **Recife (2611606) e 040101 na Produção Restrita.** Não encontrados em fonte oficial nem em relato verificável. A portaria 12/2026 não tem tabela. O nfe.io não traz o código.
13. **Recife, código complementar 501 para 04.01.01.** Não encontrado em fonte independente. Só na captura do líder citada na ordem, não verificada aqui.
14. **Data da página do nfe.io sobre Recife.** Não consta.
15. **Conteúdo do Swagger de contribuinte e de municípios.** Inacessível (TLS). Não tentado com outro cliente, para não gerar tráfego repetido contra o host de docs.
16. **Conteúdo do Swagger da Sefin.** Inacessível (403).
17. **Conteúdo do Asaas sobre E0312.** Inacessível (403).
18. **Dados de Recife e de qualquer município no Swagger.** Não lidos.
19. **Limite de versão de API:** se o manual dos municípios é posterior ao dos contribuintes. Não há data interna distinta; os dois têm o mesmo histórico e o mesmo metadado de criação.

---

## 13. Implicações para as fatias H1 e H3 (INFERÊNCIA, para o main decidir)

- **H3 não deve escrever a rota como fato.** O caminho pode ser `/{codigoServico}` ou `/aliquotas`, e o host é desconhecido. A primeira consulta real precisa dizer qual delas responde, e isso exige autorização do líder (a consulta real é do líder, pelo script; L-01).
- **A consulta real, se autorizada, deve ser de uma única rota**, sem gastar número de DPS, e registrar status HTTP e corpo sem dados do líder (CNPJ, CPF, IM). O objetivo é separar as duas hipóteses da ordem: se a rota de parâmetros diz que o serviço não é administrado para 040101, H1 se confirma sem gastar número.
- **O pré-voo de H3 precisa degradar.** O aviso de `pynfse-nacional` sobre problemas de homologação na API de parametrização, e a falta de códigos de erro documentados, pedem que a ausência de resposta da rota não bloqueie a emissão por si só. A decisão de bloquear ou seguir é do main, por L-01.
- **Não há base oficial para a lista de cTribMun.** Se a rota de parâmetros não devolver a lista, a opção `--ctribmun` continua sendo a única via, e o relato de 2023 sugere que o portal nacional não tem consulta pública desses códigos.
- **O acesso ao Swagger pelo líder** resolve as lacunas de formato e de certificado sem consumir a API: o líder pode abrir os dois links do manual no navegador dele (`adn.producaorestrita.nfse.gov.br/contribuintes/docs/index.html` e `/municipios/docs/index.html`) e copiar a definição dos campos e do cabeçalho de autenticação. Isso é leitura de documentação, não chamada à API. Decisão do líder.
- **H4 (zeros à esquerda)** tem base oficial na seção 9. Fixture com CPF de 11 dígitos precisa mostrar o preenchimento de 14 posições.

---

## Referências

- Manual dos contribuintes v1.2: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf
- Manual dos municípios v1.2: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-municipios-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out21025.pdf
- Swagger contribuintes (Produção Restrita, inacessível): https://adn.producaorestrita.nfse.gov.br/contribuintes/docs/index.html
- Swagger municípios (Produção Restrita, não acessado): https://adn.producaorestrita.nfse.gov.br/municipios/docs/index.html
- Anexo I DPS, Produção Restrita 20/10/2026: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx
- Portaria 12/2026 Recife: https://recifeemdia.recife.pe.gov.br/sites/default/files/Portaria%20N%2012%2C%20de%2029%20de%20abril%20de%202026.pdf
- Recife no nfe.io: https://nfe.io/docs/prefeituras-integradas/pernambuco/recife-pe-2611606/
- pynfse-nacional (PyPI, secundário): https://pypi.org/project/pynfse-nacional/
- ideia4it/nfse (GitHub, secundário): https://github.com/ideia4it/nfse
- nfse-nacional/nfse-php (GitHub, secundário): https://github.com/nfse-nacional/nfse-php
- ACBr, E0312 (tópico 88728, secundário): https://www.projetoacbr.com.br/forum/topic/88728-erro-e0312-ao-emitir-nfse/
- ACBr, cTribMun no NFS-e nacional (tópico 75643, secundário): https://www.projetoacbr.com.br/forum/topic/75643-campo-ctribmun-no-nfse-nacional/
- ACBr, E999 (tópico 92867, sem valor como prova): https://www.projetoacbr.com.br/forum/topic/92867-nfs-e-nacional-sefin-produ%C3%A7%C3%A3o-restrita-%E2%80%94-e999-mesmo-com-xmldsig-ok-e-dps-equivalente-ao-emissor-web/
- Omie, E0312 (secundário, conferir antes de usar): https://ajuda.omie.com.br/pt-BR/articles/17221186-e0312-rejeicao-nfs-e-o-codigo-de-tributacao-nacional-informado-nao-esta-administrado-pelo-municipio-de-incidencia-do-issqn
- Asaas, E0312 (403, só o título visto): https://central.ajuda.asaas.com/hc/pt-br/articles/47342463979803-O-C%C3%B3digo-Complementar-Municipal-n%C3%A3o-foi-preenchido-ou-est%C3%A1-preenchido-incorretamente-E0312

Repositório, sem URL: `docs/pesquisa/dominio.md` (URLs de Anexo I e manuais) e `docs/pesquisa/campos_emissor_web.md`, linhas 92 e 93.
