# Onda 2: pesquisa de abertura da API da Sefin Nacional (envio da DPS)

**Data:** 27/10/2026.
**Escopo:** perguntas (b), (c) e (g) da pesquisa de abertura da Onda 2 (plano, seção Onda 2, linha 444). As perguntas (a) e (d), e o reforço do líder sobre assinatura e CNPJ no certificado, não foram pesquisados aqui (fora desta ordem de serviço; ver "Em aberto").
**Leis aplicadas:** L-22 (pesquisa antes do planejamento), L-18 (FATO separado de INFERÊNCIA), L-37 (leitura só para aprender a técnica; nenhum código de terceiro copiado), L-51 (nada instalado), L-57 (nenhuma tentativa de contornar o 403 do swagger; nenhuma chamada autenticada à Sefin; nenhum browser).

**Frase-guarda (L-21):** "Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição."

**Legenda.** FATO: lido nesta sessão, com URL e trecho. INFERÊNCIA: fonte secundária, dedução ou trecho de busca não aberto na íntegra; a confirmar na primeira chamada real à Produção Restrita, feita pelo líder com o certificado dele.

## 0. Achado de versão: os PDFs "v1.2" baixam conteúdo v1.0

- FATO: a URL oficial do manual de contribuintes com nome "v1-2-out2025" (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf) devolveu, em 25/10 e em 27/10/2026, um PDF com SHA-256 `ac2f36e34ff565cc36d67c5d67415c33cc09f27b2dea006e8da9bcd2ddce5581`. O histórico do documento diz "1.0 17/03/2025". O hash é igual ao da cópia local `/var/tmp/notalivre-pesquisa-onda1/manual-dl/manual.pdf`.
- FATO: o "Manual dos Municípios ... Emissor Público" com nome "v1-2-out21025" (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-municipios-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out21025.pdf) também tem histórico "1.0 17/03/2025" (SHA-256 `3b5e982be9298ecc72ee254e26eb6824dc13d2e05704a83f01aa7a8fd321916e`).
- FATO: o manual de emissão por decisão administrativa ou judicial (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-emissao-decisao-administrativa-e-judicial.pdf) tem histórico "1.0 26/01/2026 Versão inicial" (SHA-256 `f6d39849f57ed48893fa435f69069edfaa1154b067c286e73514ff870814ce69`).
- FATO: o manual de contribuintes das APIs do ADN (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf) tem histórico "1.0 12/02/2026 Versão inicial" (SHA-256 `9ffc97d8b1be4c80a440b3569b0027df61265b9e75ef69437f21f9481a9a1d6d`). Lido integralmente em texto extraído.
- FATO: a página de manuais da Biblioteca (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual, lida em 27/10/2026) lista o manual de contribuintes como "Versão 1.2 · out/2025", pelo nome do arquivo. O conteúdo baixado não confirma essa versão.
- FATO: cópias dos PDFs, dos textos extraídos e a lista de SHA-256 estão em `/var/tmp/notalivre-pesquisa-onda2/` (arquivo `SHA256SUMS`), para reproduzir os hashes desta seção.
- **Consequência:** a especificação de POST /nfse que a Onda 2 precisa não foi lida em nenhum manual com versão de 2025 ou 2026 além da 1.0. Esta é a lacuna central desta pesquisa.

## 1. (b) Corpo do POST de envio da DPS

### 1.1 O que os textos oficiais lidos dizem

- FATO, manual 1.0 (17/03/2025), seção 1.3.2, método a: "Geração síncrona de Nota Fiscal de Serviço eletrônica. Através de um método POST a API recepciona uma DPS emitida pelo contribuinte e enviada pelo solicitante do serviço." Cópia local `/var/tmp/notalivre-pesquisa-onda1/manual-dl/manual.txt`, texto integral (10,8 KB).
- FATO, mesmo manual: nenhum termo de corpo de POST /nfse. Busca por gzip, base64, json, content-type e nome de campo não achou nada do envio. A menção a JSON está na seção 1.5.2 (eventos), que diz: "As mensagens de comunicação com a API utilizarão o padrão JSON". Refere-se a POST /nfse/{chaveAcesso}/eventos, não à DPS.
- FATO, manual de decisão judicial (1.0, 26/01/2026), seção 1.1.2, método a, POST /decisao-judicial/nfse: "recepciona uma NFS-e emitida pelo contribuinte". Nenhum formato de corpo; busca por gzip e base64 sem resultado.
- FATO, Anexo I (DPS) e XSD DPS_v1.01 lidos nas pesquisas da Onda 1 (`docs/pesquisa/onda1_leiaute.md`): busca por base64 e gzip nas abas dos XLSX e nos XSD não achou envelope de transporte. A única ocorrência de base64 está em `xmldsig-core-schema.xsd` (tipo da assinatura), que não é envelope de envio.
- FATO, manual de contribuintes das APIs do ADN (1.0, 12/02/2026), lido integralmente: não contém base64, gzip nem descrição de codificação. Descreve só GET /DFe/{NSU} e GET /NFSe/{ChaveAcesso}/Eventos, sem formato de retorno.
- **FATO (conflito com o canon):** a spec (`docs/superpowers/specs/2026-10-25-notalivre-design.md`, C4, linha 126) diz "FATO: o Base64 está confirmado no texto oficial", e o plano (linha 133) repete "Base64 confirmado". A origem dessa afirmação é `docs/pesquisa/dominio.md`, linha 43 ("gzip não confirmado em texto oficial (base64 sim)"), que não cita fonte para o "base64 sim". Nos textos oficiais que consegui ler (manual 1.0 de contribuintes, manual de decisão judicial, manual do ADN, Anexo I e XSD), nenhum traz Base64 no envio. A hipótese que resta é o manual v1.2 real, que não consegui ler. A afirmação da spec está sem fonte verificada. Precisa de correção ou de fonte antes de NL-2.4 (L-67, decisão do líder).

### 1.2 Fontes secundárias sobre o formato

- INFERÊNCIA, fórum ProjetoACBr (thread sobre E999, https://www.projetoacbr.com.br/forum/topic/92867-nfs-e-nacional-sefin-produ%C3%A7%C3%A3o-restrita-%E2%80%94-e999-mesmo-com-xmldsig-ok-e-dps-equivalente-ao-emissor-web/, lida em 27/10): o autor escreve que o XML vai "compactado em GZip e codificado em Base64" e cita a chave `dpsXmlGZipB64`. O JSON completo não aparece. A thread não tem resposta marcada.
- INFERÊNCIA, README de pedrocasado/nfse-php (https://github.com/pedrocasado/nfse-php, lido em 27/10), seção "Changelog / v2 para v3": "REST, JSON body, gzip+base64 XML", e na seção "Create NFS-e (DPS)": "sends it gzip+base64 to the Sefin endpoint". Não informa o nome da chave nem o Content-Type.
- INFERÊNCIA, blog Nota Gateway (https://notagateway.com.br/blog/api-nfse-nacional/, lido em 27/10): XML "compactado em GZip, codificado em Base64 e assinado". Blog comercial, sem citação de seção do manual.
- INFERÊNCIA, TabNews (https://www.tabnews.com.br/CesarMasserati/nfs-e-nacional-implementacao-via-webservice-api, lido em 27/10): o XML vai "em um arquivo GZIP para os endpoints". Não cita Base64 nem chave. O autor diz que as páginas do gov.br ainda marcavam a API como "EM DESENVOLVIMENTO" (data não determinada).
- INFERÊNCIA, trechos de busca não abertos na íntegra: thread ACBr marcada como resolvida com `{"dpsXmlGZipB64": "..."}` e Content-Type `application/json`; daash.io citando `dpsXmlGZipB64`. A página do daash, quando aberta, devolveu 404 (https://daash.io/doc/integracao-direta-com-o-sistema-nacional-nfs-e-adn-govbr-631ecb), então a afirmação dela não foi confirmada.
- Referência interna, não oficial: o brief do líder (`docs/origem/prompt-emissor-nfse-nacional.md`, linha 44) diz "JSON contendo o XML da DPS compactado em GZip + Base64".

**Resumo do que as fontes secundárias concordam:** corpo JSON, uma chave, GZip antes do Base64. O nome `dpsXmlGZipB64` aparece em três fontes (uma lida na íntegra, duas em trecho). Nenhuma fonte oficial lida confirma nada disso.

### 1.3 Compressão: GZip ou zlib

- INFERÊNCIA técnica, padrão não lido nesta sessão: gzip (RFC 1952) e zlib (RFC 1950) são formatos diferentes. Um zlib cru não é gzip. O plano (linha 96) escreve "Gzip (zlib)", o que mistura os dois. O formato precisa ser fixado por teste de contrato, não por hipótese.
- INFERÊNCIA, já registrada no plano (linha 133): `qCompress` do Qt não gera GZip. Não verifiquei na documentação Qt nesta sessão.

### 1.4 Content-Type

- FATO: nenhuma fonte oficial lida informa Content-Type.
- INFERÊNCIA: `application/json` (trecho de busca de thread resolvida; a thread ACBr lida não informa).

### 1.5 Resposta à pergunta (b)

Hipótese de trabalho, sem fonte oficial: `POST {base}/nfse` com `Content-Type: application/json` e corpo `{"dpsXmlGZipB64": "<base64 de gzip do XML da DPS>"}`. INFERÊNCIA com três fontes secundárias. Não sabemos se o XML enviado é o assinado com envelope, nem se o nome da chave é o mesmo nos dois ambientes.

## 2. (c) Caminho base da API

### 2.1 Fatos

- FATO, página Apis Prod. Restrita e Produção (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/apis-prod-restrita-e-producao, atualizada em 20/08/2026, lida em 27/10/2026). Os links de documentação são: SEFIN Nacional em Produção Restrita, `https://sefin.producaorestrita.nfse.gov.br/API/SefinNacional/docs/index`; SEFIN Nacional em Produção, `https://sefin.nfse.gov.br/SefinNacional/docs/index`. A página não diz qual é a base de envio, só as URLs de documentação.
- FATO, manual de decisão judicial 1.0, seção 1.4: "Foi disponibilizada uma Swagger em ambiente de produção restrita destinada a testes por parte dos municípios conveniados", com link `https://sefin.producaorestrita.nfse.gov.br/API/SefinNacional/docs/index`.
- FATO, manual 1.0, seção 1.6: o link de Swagger de Produção Restrita aponta para o ADN (`https://adn.producaorestrita.nfse.gov.br/contribuintes/docs/index.html`), não para a SEFIN. Não diz a base de POST.
- FATO: o Swagger SEFIN de Produção Restrita respondeu HTTP 403 Forbidden em 27/10/2026 (mesmo resultado de 25/10, `docs/pesquisa/tabelas.md`). Não tentei outro caminho nem cabeçalho (L-57).
- FATO, brief (`docs/origem/prompt-emissor-nfse-nacional.md`, linhas 17 a 22, não oficial): Produção Restrita `https://sefin.producaorestrita.nfse.gov.br/SefinNacional`; Produção `https://sefin.nfse.gov.br/SefinNacional`; Swagger de Produção Restrita `/API/SefinNacional/docs/index`.
- FATO, página Apis Prod. Restrita e Produção: o link de Painel Administrativo Municipal exibe um domínio e aponta para outro. Ambos os links levam à documentação de Produção Restrita. Vale conferir antes de usar.

### 2.2 Inferências

- INFERÊNCIA: base de envio de Produção Restrita `https://sefin.producaorestrita.nfse.gov.br/SefinNacional`. Dois fatos a sustentam: o brief, e a thread ACBr, que cita POST para `.../SefinNacional/nfse` (relato de terceiro, o servidor respondeu com JSON de erro, não com 404 de rota, o que sugere que a rota existia naquela data).
- INFERÊNCIA: o Swagger de Produção Restrita usa `/API/SefinNacional`. Pode ser que o prefixo `/API` faça parte da base real de Produção Restrita, e a documentação oficial lida não diz qual. **Não resolvido.**
- INFERÊNCIA forte: base de Produção `https://sefin.nfse.gov.br/SefinNacional`, pelo padrão das URLs de documentação e do brief.
- Recomendação para o código: base e prefixo em configuração por ambiente, nunca constante no código. Teste que monta `{base}/nfse` a partir da configuração.

### 2.3 Não achei

- Conteúdo do Swagger SEFIN de Produção Restrita (403).
- Texto oficial que diga a base de envio, em qualquer dos dois ambientes.
- Manual com versão 2025 ou 2026 que confirme a base (ver seção 0).

## 3. (g) Resposta de sucesso e de erro, e consulta GET /dps/{id}

### 3.1 Fatos do manual oficial (1.0, seções 1.3.2 e 1.4.2; decisão judicial 1.1.2)

- FATO, POST /nfse: "O sistema retorna para o solicitante a mensagem de erro com o motivo da rejeição da validação ou o arquivo XML da NFS-e gerada com a DPS." O texto não traz status HTTP, nome de campo, código E no corpo, nem chave de acesso separada.
- FATO, decisão judicial 1.1.2 a: "O sistema retorna para o solicitante a mensagem de erro com o motivo da rejeição da validação, se for o caso, ou o arquivo XML da NFS-e autorizada." Mesma omissão.
- FATO, GET /dps/{id} (seção 1.4.1 e 1.4.2 a): a API "retorna a chave de acesso da NFS-e correspondente" somente se o certificado do solicitante for Prestador, Tomador ou Intermediário da NFS-e. "Caso contrário a solicitação será negada." Não há status nem formato.
- FATO, HEAD /dps/{id} (seção 1.4.2 b): "informar se uma NFS-e foi gerada ou não" e "atende a qualquer usuário desde que realize a consulta com um certificado digital válido". Não há status nem formato.
- FATO, GET /nfse/{chaveAcesso} (seção 1.3.2 b): sem descrição da resposta.
- Não verificado nesta pesquisa: tamanho da chave de acesso (a spec, seção 5.1, diz 50 posições, citando o manual v1.2).

**Implicação para NL-2.8:** o NotaLivre consulta com o certificado do próprio emissor, que é o Prestador da DPS (desenho da spec R1, INFERÊNCIA de desenho). Nesse caso a condição de ator vale, e o ramo "negada" não se aplica à autoemissão. O ramo "negada" existe só para certificado que não é ator, e o app não deve usá-lo. Para o caminho de ator, a fonte secundária ACBr (INFERÊNCIA) dá HTTP 404 com E2404 "Não foi gerada uma NFS-e com o identificador de DPS informado" como sinal de DPS não gerada. HEAD /dps/{id} é alternativa mais leve, que informa só se a NFS-e foi gerada, sem chave. Os status HTTP de HEAD não estão nas fontes oficiais lidas.

### 3.2 Inferências de fontes secundárias

- INFERÊNCIA, fórum ProjetoACBr (lido em 27/10): erro na recepção da DPS com HTTP 400 e corpo JSON `{"Codigo":"E999","Descricao":"Erro não catalogado", "tipoAmbiente":2, "versaoAplicativo":"SefinNacional_1.6.0", "idDPS": ...}`. Consulta posterior `GET .../SefinNacional/dps/{idDPS}` com HTTP 404 e `E2404`, "Não foi gerada uma NFS-e com o identificador de DPS informado". É um relato isolado, sem resposta nem causa confirmada.
- INFERÊNCIA, README de pedrocasado/nfse-php (lido em 27/10), seção "Response parsing": sucesso traz `chaveAcesso` e o XML da NFS-e já decodificado de gzip+base64. Erro vem em `erro` ou `erros`, com `codigo`/`Codigo` e `descricao`/`Descricao`. A biblioteca diz que normaliza as duas formas, ou seja, o autor também não sabe qual vem sempre.
- INFERÊNCIA, trecho de busca não aberto: sucesso com HTTP 201 e campos `chaveAcesso` e `nfseXmlGZipB64` (daash.io e um cliente PHP no GitHub). Não confirmado.
- INFERÊNCIA, blog Nota Gateway (lido em 27/10, seção "Erros comuns"; não é manual): 403 para problemas de mTLS (certificado inválido, cadeia incompleta, CN ou OU divergente), 400 para payload fora do schema ou sem GZip+Base64, 409 para DPS duplicada, 429 citado só na checklist de teste.

### 3.3 Não achei

- Status HTTP oficial de sucesso (200 ou 201) e de rejeição de validação.
- Estrutura oficial do JSON de sucesso e de erro, nomes de campos, e se o XML de retorno vem em gzip+base64.
- Forma de devolução do código E. O catálogo de códigos existe no Anexo I, mas não o formato em que a API o entrega.
- Status e corpo oficiais de GET /dps/{id} para certificado sem vínculo, e para DPS ainda não gerada.
- Lista de erros de HEAD /dps/{id}.

### 3.4 Implicação para o servidor falso (NL-2.6)

O plano (linha 482) já marca o formato de resposta como INFERÊNCIA até a Onda 11. A forma do ACBr (`Codigo`/`Descricao`, E2404 no GET) pode servir de fixture candidato, marcada como INFERÊNCIA no código e no teste. Não inventar schema além do que as fontes trazem.

## 4. Em aberto (decisões e fontes que não consegui fechar)

1. **Manual de Produção Restrita com versão real.** O URL oficial baixa a v1.0 (17/03/2025). Sem o manual real, o corpo do POST e as respostas continuam INFERÊNCIA. Caminho para o líder (L-01, ele decide): baixar o PDF pelo próprio navegador e informar o arquivo, ou confirmar que a Biblioteca não tem a versão nova. Nenhuma automação de browser (L-57).
2. **Swagger SEFIN de Produção Restrita (403 para agentes).** O líder pode exportar o JSON pelo navegador dele, se o site permitir.
3. **Base de envio: `/SefinNacional` ou `/API/SefinNacional`.** Só a primeira chamada real resolve. Decisão de quem faz a chamada: o líder, com o certificado dele.
4. **Afirmação "Base64 confirmado" (spec C4 e plano linha 133).** Correção de canon, a decidir com o líder (L-67).
5. **Perguntas (a) e (d), e o reforço do líder sobre assinatura e CNPJ no certificado.** Não pesquisados aqui. Precisam de pesquisa própria, porque a assinatura é outro assunto.
6. **Textos extraídos e PDFs** estão em `/var/tmp/notalivre-pesquisa-onda2/` (com `SHA256SUMS`). O texto do manual de contribuintes de 2025 também está em `/var/tmp/notalivre-pesquisa-onda1/manual-dl/manual.txt`.

## Fontes consultadas

| Fonte | Tipo | Resultado |
|---|---|---|
| https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/apis-prod-restrita-e-producao | Oficial | URLs de documentação; sem base de envio |
| https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual | Oficial | Lista de manuais com "v1.2" no nome |
| URL "manual v1.2" de contribuintes (SHA-256 ac2f36...) | Oficial | Conteúdo v1.0 de 17/03/2025 |
| URL "manual municípios v1.2" (SHA-256 3b5e98...) | Oficial | Conteúdo v1.0 de 17/03/2025 |
| URL manual de decisão judicial (SHA-256 f6d398...) | Oficial | v1.0 de 26/01/2026; POST /decisao-judicial/nfse sem formato |
| https://sefin.producaorestrita.nfse.gov.br/API/SefinNacional/docs/index | Oficial | 403 (27/10/2026) |
| https://github.com/pedrocasado/nfse-php | Secundária | gzip+base64, JSON, sucesso e erro (INFERÊNCIA) |
| https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf | Oficial | v1.0 de 12/02/2026; sem base64 nem gzip; sem formato de retorno |
| https://github.com/LibreCodeCoop/nfse-php | Secundária | docs/integracao.md lido sem os campos; sem dados de envio |
| https://www.projetoacbr.com.br/forum/topic/92867-... | Secundária | dpsXmlGZipB64; E999 e E2404 (INFERÊNCIA) |
| https://notagateway.com.br/blog/api-nfse-nacional/ | Secundária | GZip+Base64; erros 403/400/409 (INFERÊNCIA) |
| https://www.tabnews.com.br/CesarMasserati/nfs-e-nacional-implementacao-via-webservice-api | Secundária | GZip; sem Base64 nem chave |
| https://daash.io/doc/integracao-direta-com-o-sistema-nacional-nfs-e-adn-govbr-631ecb | Secundária | Página 404 ao abrir; só trecho de busca |

**Não achei:** corpo oficial do POST; Content-Type oficial; base de envio oficial; status e corpo oficiais de sucesso e de erro; forma oficial de GET /dps/{id}; conteúdo do Swagger SEFIN; manual 2025/2026 com a especificação da API. Procurei nos manuais oficiais de contribuintes, municípios e decisão judicial, no Anexo I, no XSD, na página de APIs, no Swagger (403), em três repositórios e em quatro textos secundários. Nada disso mudou o resultado.

## Implicações para o plano

1. NL-2.4 não fecha o corpo do POST com fonte oficial. A hipótese `{"dpsXmlGZipB64": ...}`, gzip mais Base64 e `application/json` fica INFERÊNCIA, atrás de uma interface de envelope com teste de contrato marcado.
2. Corrigir C4 (spec linhas 126 e 207) e o plano (linha 133): "Base64 confirmado no texto oficial" não tem fonte encontrada (L-67, decisão do líder).
3. Fixar gzip (cabeçalho RFC 1952) por teste, não zlib cru; corrigir "Gzip (zlib)" do plano (linha 96).
4. Base e prefixo (`/SefinNacional` ou `/API/SefinNacional`) em configuração por ambiente; confirmar na primeira chamada real do líder.
5. NL-2.8, caminho de retomada: com o certificado do emissor (Prestador), GET /dps/{id} é o caminho de ator. 404 com E2404 (INFERÊNCIA, ACBr) pode liberar o reenvio, após confirmação na Produção Restrita. HEAD /dps/{id} é alternativa mais leve. Uma resposta 409 para DPS duplicada (INFERÊNCIA, blog Nota Gateway) deve ser tratada como "já existe, consultar", e nunca como reenvio. Isso reforça o POST que não repete sozinho (plano, NL-2.8).
6. NL-2.6: respostas são INFERÊNCIA marcadas; catálogo E do Anexo I continua o único código oficial.
7. Parser de erro tolerante a `erro`/`erros` e `Codigo`/`codigo` (INFERÊNCIA, medir na Onda 11).
8. Pedir ao líder o manual v1.2 real (ou o JSON do Swagger) antes de NL-2.4 (L-01).
