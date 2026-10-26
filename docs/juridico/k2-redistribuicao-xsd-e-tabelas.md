# K2: redistribuição dos XSD da NFS-e Nacional e das tabelas embutidas

Orientação técnica do CLO (Cláudio), 25/10/2026, para o ponto K2 do plano (`docs/superpowers/plans/2026-10-25-notalivre-plano.md`, seção NL-1.6 e tabela de gatilhos K2). Não é parecer jurídico vinculante: o documento inteiro fica marcado para revisão de advogado humano antes da publicação no Flathub (Onda 9).

Convenção (L-18): **FATO** traz a fonte (URL lida em 25/10/2026, ou arquivo:linha); **INFERÊNCIA** é leitura do CLO e pode estar errada.

## 0. O que muda a pergunta

- FATO (https://docs.flathub.org/docs/for-app-authors/requirements): "There is no network access during the build process"; toda dependência "must be present in the manifest as sources with publicly accessible URLs"; "All content hosted on Flathub must allow legal redistribution"; fonte não redistribuível tem de usar o tipo `extra-data`.
- FATO (https://docs.flatpak.org/en/latest/module-sources.html): "Sources are copied or downloaded to the source directory before the build starts"; a fonte `archive` aceita `zip` (extraído com `unzip`) e exige `sha256` ou `sha512`.
- INFERÊNCIA: o `ValidadorXsd` (NL-1.6) valida a DPS dentro do app, então os XSD precisam estar no bundle em tempo de execução.

Consequência: a opção (a), "baixar no build", tira os XSD do git, mas não do Flatpak. O flatpak-builder baixa o zip antes do build e o conteúdo vai para dentro do pacote que o Flathub hospeda. Logo o K2 tem de liberar a redistribuição no bundle em qualquer opção; a DA-1.15 é decisão sobre a árvore do repositório, não uma saída de licença.

Correção de premissa (L-18): o briefing diz que as tabelas da NL-1.10 "já são convertidas e embutidas no app". FATO (`tools/tabelas/README.md:16-17` e `:30-33`): os XLSX não são versionados, a saída JSON foi gerada fora da árvore (`/var/tmp/notalivre-tabelas-saida/`), e embutir o resultado no binário "fica para a NL-5.2 e depende de decisão". Hoje nada derivado dos Anexos está no repositório nem em bundle.

## 1. XSD e Anexos da biblioteca gov.br/nfse

### Fatos

- Rodapé da página de Produção Restrita (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita): "Todo o conteúdo deste site está publicado sob a licença Creative Commons Atribuição-SemDerivações 3.0 Não Adaptada", com link para https://creativecommons.org/licenses/by-nd/3.0/deed.pt_BR. A mesma página lista o arquivo `NFSe-ESQUEMAS_XSD-PRODREST-v1.01-20261020` (zip do briefing), "Atualizado em 20/10/2026 18h10".
- Termos de uso do gov.br (https://www.gov.br/pt-br/termos-de-uso): a única disposição sobre conteúdo é o mesmo rodapé CC BY-ND 3.0; o termo de uso não trata de propriedade intelectual nem de reutilização de dados.
- Os nove XSD da NFS-e não trazem aviso de copyright nem de licença: busca por `copyright|licen[cçs]|direitos autorais|all rights` nos dez arquivos extraídos em `/var/tmp/notalivre-pesquisa-onda1/xsd-dl/x/` casou só em `xmldsig-core-schema.xsd` (4 ocorrências, 1 arquivo).
- Lei 9.610/98, texto compilado da Câmara (https://www2.camara.leg.br/legin/fed/lei/1998/lei-9610-19-fevereiro-1998-365399-normaatualizada-pl.pdf, página 3):
  - Art. 7º, XIII: protege "as coletâneas ou compilações, antologias, enciclopédias, dicionários, bases de dados e outras obras, que, por sua seleção, organização ou disposição de seu conteúdo, constituam uma criação intelectual".
  - Art. 7º, § 2º: "A proteção concedida no inciso XIII não abarca os dados ou materiais em si mesmos [...]".
  - Art. 8º: "Não são objeto de proteção como direitos autorais de que trata esta Lei: I - as idéias, procedimentos normativos, sistemas, métodos, projetos ou conceitos matemáticos como tais; II - os esquemas, planos ou regras para realizar atos mentais, jogos ou negócios; III - os formulários em branco para serem preenchidos por qualquer tipo de informação, científica ou não, e suas instruções; IV - os textos de tratados ou convenções, leis, decretos, regulamentos, decisões judiciais e demais atos oficiais; V - as informações de uso comum tais como calendários, agendas, cadastros ou legendas; VI - os nomes e títulos isolados; VII - o aproveitamento industrial ou comercial das idéias contidas nas obras."
- CC BY-ND 3.0 Unported, código legal (https://creativecommons.org/licenses/by-nd/3.0/legalcode):
  - Seção 1: uma Collection (coletânea em que a obra entra inteira e sem modificação, ao lado de outras obras independentes) "will not be considered an Adaptation".
  - Seção 2: "Nothing in this License is intended to reduce, limit, or restrict any uses free from copyright" (nem usos que decorram de limitações e exceções da lei).
  - Seção 3: os direitos valem em qualquer mídia e formato e incluem "the right to make such modifications as are technically necessary" para exercê-los em outros meios e formatos.
  - Seção 4(a): distribuir sob os termos da licença, com cópia ou URI dela; manter os avisos intactos; não impor termos nem medidas técnicas que restrinjam os direitos do destinatário; não sublicenciar.
  - Seção 4(b): crédito com o nome do autor (quando fornecido), o título (quando fornecido) e a URI associada à obra; o crédito não pode sugerir endosso do licenciante.
- GPL-3.0, seção 5, último parágrafo (`LICENSE:235-243`): compilação com obras "separate and independent" que não formam "a larger program" é um "aggregate", e "Inclusion of a covered work in an aggregate does not cause this License to apply to the other parts of the aggregate."
- FSF (https://www.gnu.org/licenses/license-list.html): as licenças Creative Commons NoDerivatives "does not qualify as free, because there are restrictions on distributing modified versions".
- Lei de Acesso à Informação (Lei 12.527/2011): não consultada nesta rodada (teto de buscas atingido).

### Leitura do CLO

- INFERÊNCIA (fundamento 1): os XSD são o anexo técnico do leiaute normativo da NFS-e publicado pela SE-CGNFS-e, que o contribuinte é obrigado a seguir. Cabem no art. 8º, IV ("demais atos oficiais"), e no III ("formulários em branco [...] e suas instruções"), com apoio do I e do II. Se cabem, não há direito autoral sobre eles, e a seção 2 da própria CC confirma que a licença não restringe esse uso. Os Anexos (leiaute e regras de validação) têm a mesma natureza.
- INFERÊNCIA (fundamento 2, independente do 1): mesmo que um juiz visse obra protegida nos XSD, a CC BY-ND 3.0 permite copiar e distribuir sem modificação, para qualquer fim, com atribuição. Pôr os XSD inteiros e intactos num repositório e num bundle com o NotaLivre é Collection (seção 1), não Adaptation, e na GPL é agregado (seção 5): a GPL não se estende aos XSD, e os XSD não precisam de exceção de licença.
- Condição que fecha o risco do "SemDerivações": os XSD ficam byte a byte iguais aos publicados. O `SHA256SUMS` da NL-1.6a já impõe isso. Se um dia for preciso corrigir um XSD oficial com defeito, a correção não entra no arquivo da Receita: a fatia para e o ponto sobe ao líder.
- Condição da seção 4(a): o aviso tem de dizer que esses arquivos NÃO estão sob a GPL, senão o NotaLivre estaria impondo ao destinatário termos que a CC não permite. É o papel do aviso da seção 5 abaixo.
- INFERÊNCIA (LAI): a LAI rege o acesso à informação pública, não concede licença de reutilização; não é usada aqui como fundamento.

**Resposta 1:** sim, os XSD e os Anexos podem ser redistribuídos num repositório GPL e num bundle Flatpak, sem modificação, com atribuição à origem (gov.br/nfse, Receita Federal e SE-CGNFS-e; nome do autor conforme o briefing, a página lida não o declara, confirmar no cabeçalho da documentação antes do commit), título e versão do arquivo, URL de origem, menção e URI da CC BY-ND 3.0, aviso de que não estão sob a GPL e de que não há endosso da Receita. Nenhuma exceção à GPL é necessária. Recomendação técnica (INFERÊNCIA): carregar os XSD como arquivos de dados em `share/` do Flatpak, e não compilá-los dentro do binário (recurso qrc), para que a separação "obra independente" fique evidente.

## 2. xmldsig-core-schema.xsd (W3C)

- FATO (`xmldsig-core-schema.xsd:7-16`, arquivo extraído do zip oficial): "Copyright 2001 The Internet Society and W3C (Massachusetts Institute of Technology, Institut National de Recherche en Informatique et en Automatique, Keio University). All Rights Reserved." e "This document is governed by the W3C Software License [1]", com [1] = http://www.w3.org/Consortium/Legal/copyright-software-19980720.
- FATO (https://www.w3.org/Consortium/Legal/copyright-software-19980720): permissão para "use, copy, modify, and distribute this software and its documentation, with or without modification, for any purpose and without fee or royalty", desde que todas as cópias incluam: (1) "The full text of this NOTICE in a location viewable to users of the redistributed or derivative work"; (2) "Any pre-existing intellectual property disclaimers, notices, or terms and conditions"; (3) "Notice of any changes or modifications to the W3C files, including the date changes were made". A página não indica substituição por outra licença.
- FATO (https://www.gnu.org/licenses/license-list.html): a W3C Software Notice and License "is a free software license and is GPL compatible".
- INFERÊNCIA: o identificador SPDX desta versão é `W3C-19980720` (a conferir na lista SPDX antes do commit).

**Resposta 2:** licença W3C Software Notice and License de 1998, livre e compatível com a GPL. Obrigações: manter o cabeçalho do arquivo intacto, incluir o texto completo do aviso da W3C num local visível ao usuário (arquivo em `LICENSES/` no repositório e em `share/licenses/` no Flatpak) e, se o arquivo for alterado, registrar a alteração com data. Arquivo sem modificação e com cabeçalho preservado, mais uma linha no aviso e o texto da licença, cumpre tudo.

## 3. Dados derivados (NL-1.10), IBGE e ISO 3166

- FATO (`tools/tabelas/README.md:11-14` e `:69-75`): as tabelas vêm dos Anexos A (1.00, 10/12/2025) e B (1.01, 22/01/2026) da biblioteca gov.br/nfse; conteúdo: municípios (código IBGE e nome), países (código ISO2 e nome em português, 250 linhas, incluindo `ZZ`, que não é código ISO), a localidade `0000000`, cTribNac e NBS. Cada JSON leva a proveniência (URL, data, SHA-256).
- INFERÊNCIA: código e nome de município, de país e de item de serviço são dados e cadastros (art. 7º, § 2º; art. 8º, V e VI). A lista de serviços e a NBS são anexos de atos normativos (art. 8º, IV); não conferi o ato de origem de cada uma nesta rodada. Uma lista completa de todos os municípios ou países em ordem de código não tem seleção nem disposição criativa (art. 7º, XIII). A Lei 9.610 não traz direito específico de base de dados além do XIII (não pesquisei lei esparsa). Se é assim, extrair os dados não toca a CC BY-ND (seção 2), e a mudança de XLSX para JSON é modificação "technically necessary" de formato (seção 3).
- Risco residual (INFERÊNCIA): alguém sustentar que a planilha é compilação protegida e que o JSON normalizado (zero à esquerda, UF derivada) é Adaptation proibida pelo "SemDerivações". Avalio como baixo; a decisão de aceitar sobe ao líder.
- IBGE: FATO (`tools/tabelas/README.md:13`): os municípios do app vêm do Anexo A da gov.br, não da API do IBGE. FATO (`docs/design/spec-visual.md:250`; `docs/pesquisa/tabelas.md`, Q5): a API de localidades (https://servicodados.ibge.gov.br) aparece como fonte de atualização da tela de tabelas, ligada à Onda 5. NÃO ENCONTRADO: termos de uso da API de localidades (uma busca web). O único termo do IBGE que a busca apontou é de outro produto, a base cartográfica BC250, que pediria crédito "Fonte: IBGE/DGC" (https://geoftp.ibge.gov.br/cartas_e_mapas/bases_cartograficas_continuas/bc250/versao2019/informacoes_tecnicas/BC250_termo_uso_2019.pdf; resumo de busca, página não lida). Recomendação: crédito "Fonte: IBGE" na tela de tabelas quando a API entrar (INFERÊNCIA, prudência), e leitura dos termos da API como item K antes da fatia da Onda 5 que a usa. Não bloqueia a Onda 1.
- ISO 3166: FATO: https://www.iso.org/iso-3166-country-codes.html devolveu HTTP 403. Só achei fontes secundárias de 2003 (declaração do IAB, https://datatracker.ietf.org/doc/statement-iab-2003-09-23-isocodes/, e documentos do Unicode, https://www.unicode.org/L2/L2003/03340-iso-ref871.html; as duas por resumo de busca, páginas não lidas), que divergem segundo esse resumo: uma diz que o uso dos códigos é livre, inclusive comercial; outra diz que o uso em produto comercial pode exigir licença. INFERÊNCIA: o app distribui a tabela da Receita (nomes em português dela, mais `ZZ`), não a publicação da ISO; os códigos de duas letras são identificadores curtos (art. 8º, VI); o app é gratuito. Risco residual baixo; aceitar sobe ao líder.

**Resposta 3:** as tabelas derivadas podem ser embutidas, com o mesmo crédito à gov.br/nfse e a nota de que só o formato mudou; IBGE e ISO entram no aviso como origem dos códigos. Fica risco residual baixo (tabelas como "derivação" e uso comercial da lista ISO), que é do líder aceitar ou não.

## 4. Recomendação e Flathub

Juridicamente, (a) e (b) se equivalem na redistribuição: nas duas o bundle leva os XSD, e nas duas o aviso precisa existir. O desempate é técnico.

- (a) Fora da árvore, baixado no build (como hoje). No manifesto, os XSD entram como fonte `archive` com `url` do zip da gov.br, `sha256` do zip (o mesmo do `SHA256SUMS`), `dest` na pasta da versão (por exemplo `third_party/nfse-xsd/prodrest-20261020`) e `strip-components` conforme a estrutura do zip (INFERÊNCIA: o zip traz os arquivos na raiz; conferir). `mirror-urls` existe, mas um espelho mantido pelo projeto já é redistribuição (coberta pelo item 1). Risco (INFERÊNCIA): a gov.br troca o zip quando sai versão nova, a URL some e o build do Flathub, que roda de novo a cada atualização, quebra.
- (b) Na árvore, intactos, com aviso e declaração de licença por arquivo (estilo REUSE). O Flathub constrói só a partir do git, sem depender da gov.br no dia do build.
- (c) Recomendação do CLO: (a) agora, como decidido na DA-1.15, e (b) antes da Onda 9 (empacotamento Flathub), quando o aviso abaixo entra junto com os arquivos. A decisão sobre a árvore é do CTO (DA-1.15) e do líder; o K2 não a bloqueia.

Obrigações do Flathub para a Onda 9 (FATO, página de requisitos): instalar os arquivos de licença de cada módulo em `$FLATPAK_DEST/share/licenses/$FLATPAK_ID` (aqui: `LICENSE` da GPL, o aviso abaixo, o texto da CC BY-ND 3.0 ou a URI dela e o aviso da W3C), e declarar no MetaInfo uma licença que "must match with license information available in its source". INFERÊNCIA: a expressão do MetaInfo passa a ter de citar os dados de terceiros (por exemplo `GPL-3.0-or-later AND CC-BY-ND-3.0 AND W3C-19980720`), a confirmar no K3.

## 5. Texto pronto do aviso (para `NOTICE` ou equivalente)

```
Esquemas XSD da NFS-e Nacional (NFSe-ESQUEMAS_XSD-PRODREST-v1.01-20261020) e tabelas derivadas dos Anexos A e B: Sistema Nacional NFS-e, Receita Federal do Brasil e SE-CGNFS-e, https://www.gov.br/nfse/pt-br/biblioteca
Licença de origem: Creative Commons Atribuição-SemDerivações 3.0 Não Adaptada, https://creativecommons.org/licenses/by-nd/3.0/deed.pt_BR
Os XSD vão sem modificação (SHA-256 em SHA256SUMS); as tabelas só mudaram de formato (XLSX para JSON), com a origem gravada em cada arquivo. Códigos de município: IBGE. Códigos de país: ISO 3166-1 alfa-2, pela tabela da Receita.
Estes arquivos não estão sob a GPL: estão agregados ao NotaLivre (GPL-3.0-or-later, seção 5) sem mudar os termos deles. A Receita Federal não endossa o NotaLivre.
xmldsig-core-schema.xsd: Copyright 2001 The Internet Society and W3C (Massachusetts Institute of Technology, Institut National de Recherche en Informatique et en Automatique, Keio University). All Rights Reserved.
Licença: W3C Software Notice and License, http://www.w3.org/Consortium/Legal/copyright-software-19980720 (texto completo em LICENSES/).
```

## Sobe ao líder (não decidido aqui)

1. Aceitar o risco residual de as tabelas JSON serem lidas como "derivação" proibida pela CC BY-ND (avaliado como baixo).
2. Aceitar a ambiguidade sobre uso comercial da lista ISO 3166 (avaliada como baixa; app gratuito, tabela da Receita).
3. Pedir confirmação escrita à Receita ou SE-CGNFS-e (e ao IBGE, antes da Onda 5) ou seguir sem ela.
4. Revisão por advogado humano antes da publicação no Flathub (Onda 9).
5. A FSF classifica a CC "SemDerivações" como não livre: o líder aceita que o bundle leve dados sob essa licença (afeta a forma de apresentar o app como "software livre")?
6. Opção de árvore (a), (b) ou (c): decisão do CTO e do líder; o CLO recomenda (c).
