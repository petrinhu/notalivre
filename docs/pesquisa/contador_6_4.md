<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa -->

# Os 8 pontos de "O QUE AINDA EXIGE CONTADOR" (spec 6.4): pesquisa na web (27/11/2026)

Ordem do líder, por escrito, em 27/11/2026: "busque na web! sites de contabilidade".

Mudança de regra pelo líder, por escrito, em 27/11/2026, verbatim (repassada pelo main): "nao deixe nada para o contador. Ele vai buscar na web também e você é mais eficiente nisso". Efeito, pela ordem do main: a categoria "continua para o contador" deixa de existir. Cada ponto fecha em um de três níveis:

- **Resolvido pela fonte primária** (FATO).
- **Resolvido por fontes secundárias concordantes** (com quantas e quais).
- **Resolução recomendada, a ratificar pelo líder**: quando as fontes divergem ou não existem, a interpretação mais conservadora para o emitente (a que evita rejeição na Sefin e risco fiscal), com o porquê e o que o app faz.

Escopo: os 8 itens da seção 6.4 do spec (`docs/superpowers/specs/2026-10-25-notalivre-design.md`), sem refazer o que já está em `docs/pesquisa/fiscal_aprofundada.md`. Papel: `compliance-legal` como pesquisador fiscal. Nada aqui é parecer jurídico ou contábil (recomendação técnica; a decisão é do líder, e o texto jurídico, como termos de uso e aviso LGPD, passa pelo Cláudio, CLO).

Frase-guarda (L-21), verbatim:

> Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição.

Consumidor (spec, seção 1): MEI, autônomos e pequenas empresas do Simples Nacional, emitindo NFS-e pelo Sistema Nacional NFS-e (Sefin Nacional e ADN).

## Legenda e rodadas

- **FATO**: lido na fonte primária, com URL e data da leitura.
- **FONTE SECUNDÁRIA**: site de contabilidade, de software ou de comunidade, ou página oficial vista só pelo resumo do buscador, com URL e o que diz.
- **CONFLITO**: fontes que divergem, com as duas.
- **NÃO ENCONTRADO**: procurado e não achado.
- Nas citações literais, o travessão e a meia-risca das fontes foram trocados por hífen (NF-18); o resto do texto citado está como na fonte.

**Rodada 1 (27/11/2026, manhã, commit d30ff59).** A rede da sessão só deixava passar a busca: nenhuma página foi aberta, tudo veio do resumo do buscador e ficou como FONTE SECUNDÁRIA.

**Rodada 2 (27/11/2026, a partir das 11h50 UTC).** O líder aprovou um ambiente novo. Abriram: `www.gov.br` (inclui gov.br/nfse), `www2.camara.leg.br`, `www8.receita.fazenda.gov.br/SimplesNacional` e a API de pesquisa de `normas.receita.fazenda.gov.br` (só a listagem com ementa e datas; o texto do ato fica em outro host, `normasinternet2.receita.fazenda.gov.br`, fora da lista, não tentado). `www.planalto.gov.br` não responde e `legis.senado.leg.br` dá 403: as leis vieram do texto compilado da Câmara. O texto do Ato Conjunto RFB/CGIBS nº 7/2026 está em `cgibs.gov.br`, fora da lista, não tentado: ele foi lido pela notícia oficial do CGNFS-e que o resume. Downloads em pasta de rascunho da sessão, fora do repositório; os XLSX foram lidos com `tools/tabelas/xlsx_leitor.py`, os PDFs com `pdftotext`. Todas as leituras abaixo marcadas "lido em 27/11/2026" são desta rodada.

**Peso das perguntas frequentes oficiais.** A página "Perguntas Frequentes da NFS-e" (https://www.gov.br/nfse/pt-br/perguntas-frequentes , publicada em 24/11/2026 18h45, atualizada em 26/11/2026 09h57, "Versão atual: 1.5 - 23/11/2026", lida em 27/11/2026) diz de si mesma: "A elaboração desse material foi realizada com auxílio de inteligência artificial e revisado para aprimorar os esclarecimentos". É orientação oficial do Portal, não norma. Onde ela diverge do leiaute ou do ato, vale o leiaute ou o ato. Exemplo de divergência interna: a pergunta 15.1 ainda traz o cronograma antigo do Ato Conjunto nº 4 (01/10/2026 e 01/12/2026) e fala em Simples "que aderirem voluntariamente", enquanto a 15.4 diz que para ME, EPP e MEI o IBS/CBS é obrigatório a partir de 01/01/2027.

## Fontes primárias lidas na rodada 2 (27/11/2026)

| Fonte | URL | Data do documento | SHA-256 do arquivo |
|---|---|---|---|
| Perguntas Frequentes da NFS-e, v1.5 | https://www.gov.br/nfse/pt-br/perguntas-frequentes | 23/11/2026 (página atualizada 26/11/2026) | página HTML |
| Notícia CGNFS-e: Ato Conjunto RFB/CGIBS nº 7/2026 | https://www.gov.br/nfse/pt-br/noticias/cgnfs-e-informa-ato-conjunto-rfb-cgibs-no-7-2026-atualiza-o-cronograma-de-obrigatoriedade-de-emissao-da-nfs-e-para-ibs-e-cbs | 13/11/2026 | página HTML |
| Notícia: Ato Técnico Conjunto nº 7 aprova a NT 009 v1.01 | https://www.gov.br/nfse/pt-br/noticias/publicado-o-ato-tecnico-conjunto-rfb-cgibs-no-7-aprovando-a-versao-1-01-da-nota-tecnica-009-nfs-e-e-a-nota-tecnica-010-da-nfs-e-via | 09/10/2026 (atualizada 15/10/2026) | página HTML |
| Notícia: Release 2 da NT 009 e cronograma | https://www.gov.br/nfse/pt-br/noticias/cgnfs-e-informa-disponibilizado-no-ambiente-de-testes-o-release-2-das-evolucoes-da-nota-tecnica-009 | 25/11/2026 | página HTML |
| Notícia: Release 1 da NT 009 | https://www.gov.br/nfse/pt-br/noticias/cgnfs-e-informa-disponibilizadas-no-ambiente-de-testes-o-release-1-das-evolucoes-da-nota-tecnica-009 | 20/10/2026 | página HTML |
| Biblioteca RTC (listagem) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc | atualizada 23/11/2026 | página HTML |
| Documentação de Produção Restrita (listagem) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita | atualizada 25/11/2026 | página HTML |
| NT 009 v1.01 (PDF, 23 páginas, capa 11/09/2026) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nota-tecnica-009-se-cgnfs-e-v-1-01.pdf | 11/09/2026 | cc86c9402e317e84eb3a9f48bbd5e0b8e3dcd84f2645700190c84d45bf1ea58e |
| Anexo VI v1.04.01 (NT 009) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovi-leiautesrn_rtc_ibscbs-v1-04-01-nt009.xlsx | NT 009 v1.01 | 103a150dd6f56ec8edcaa3a453b59a1b0e096d7cd5025d7bd06a387f141ddf39 |
| Anexo VII v1.03.00 (NT 009) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovii-indop_ibscbs_v1-03-00-nt009.xlsx | NT 009 v1.01 | 95f30a44ee94adeea57fb92f3a0337b1e62fdc106f393e73894087889be9e5aa |
| Anexo I de Produção Restrita, 24/11/2026 | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261124.xlsx | 24/11/2026 | cf4d1d36daae5600060db2b36d9a84a703aa209cb099a0abbd7d3793c3e7e928 |
| Portal do Simples: notícia da Res. CGSN 186/2026 | https://www8.receita.fazenda.gov.br/simplesnacional/noticias/NoticiaCompleta.aspx?id=c739e03c-8482-473f-8e82-f38ec3b13637 | 17/04/2026 | página HTML |
| Normas da Receita, API de pesquisa (atos do CGSN de 2026: número, data, publicação, ementa) | https://normas.receita.fazenda.gov.br/api/indexacao/ato/pesquisar | consulta de 27/11/2026 | resposta JSON |
| LC 214/2025, texto compilado (Câmara, norma atualizada) | https://www2.camara.leg.br/legin/fed/leicom/2025/leicomplementar-214-16-janeiro-2025-796905-normaatualizada-pl.html | compilado com a LC 227/2026 | página HTML |
| CTN, Lei 5.172/1966, texto compilado (Câmara) | https://www2.camara.leg.br/legin/fed/lei/1960-1969/lei-5172-25-outubro-1966-358971-normaatualizada-pl.html | compilado com a LC 236/2026 | página HTML |
| LGPD, Lei 13.709/2018, texto compilado (Câmara) | https://www2.camara.leg.br/legin/fed/lei/2018/lei-13709-14-agosto-2018-787077-normaatualizada-pl.html | compilado | página HTML |

Não lidos no primário (e por quê): texto integral do Ato Conjunto RFB/CGIBS nº 7/2026 (em `cgibs.gov.br`, fora da lista); texto das Res. CGSN 194, 196 e 198/2026 (texto em `normasinternet2.receita.fazenda.gov.br`, fora da lista; só ementa e datas pela API); LC 123/2006 compilada (não foi preciso: o art. 26, II, vigente está no art. 516 da LC 214/2025, lido); LC 227/2026 avulsa (não foi preciso: as inclusões aparecem anotadas no texto compilado da LC 214).

## Fatos que valem para mais de um ponto

- **Ato Conjunto RFB/CGIBS nº 7, de 11/11/2026.** FATO (notícia do CGNFS-e de 13/11/2026, lida em 27/11/2026): publicado no DOU de 13/11/2026; altera o Ato Conjunto nº 4, de 30/07/2026; para a NFS-e, posterga os prazos que estavam em 01/12/2026 e escalona entre 01/01/2027 e 01/03/2027. Texto, verbatim, sobre o Simples: "As empresas do Simples Nacional também entram no rol de escalonamento: 1º de janeiro de 2027 Para as hipóteses previstas nas alíneas "d" a "h" do inciso III (serviços gerais da LC nº 116/2003, bens imateriais, receitas condominiais, locações/arrendamentos de bens móveis e imóveis e demais serviços residuais). 1º de março de 2027 Para as demais hipóteses previstas no caput do art. 1º (incluindo plataformas digitais, serviços de TI/streaming dos subitens 1.03, 1.05 e 1.09, transporte coletivo do subitem 16.01, além dos demais documentos fiscais eletrônicos)." A rodada 1 tinha esse ponto em CONFLITO; a notícia oficial o resolve.
- **Ato Técnico Conjunto RFB/SUFIS/CGIBS/Diretoria Executiva nº 7, de 28/09/2026.** FATO (notícia do CGNFS-e de 09/10/2026, lida em 27/11/2026): publicado em 01/10/2026; "aprova a versão 1.01 da Nota Técnica 009 e a Nota Técnica 010 da NFS-e"; "Também foram aprovados a versão 1.04.01 do anexo de Leiaute e Regras de Negócio e a versão 1.03.00 da tabela de Indicadores de Operação". São dois atos de séries diferentes com o número 7 (Ato Conjunto, de cronograma; Ato Técnico Conjunto, de leiaute).
- **CONFLITO residual de nome.** A FAQ v1.5, pergunta 15.1, diz: "o Ato Técnico Conjunto RFB/CGIBS nº 1/2026 ratificou formalmente as Notas Técnicas 008 e 009". A notícia de 09/10/2026 diz que a NT 009 v1.01 foi aprovada pelo Ato Técnico Conjunto nº 7. As duas coisas podem ser verdadeiras (o nº 1 para a v1.0, o nº 7 para a v1.01), mas nenhuma fonte lida diz isso. Sem efeito no app.
- **Cronograma dos releases da NT 009.** FATO (notícia do CGNFS-e de 25/11/2026, lida em 27/11/2026), tabela "Cronograma de Implantações": Release 1, Produção Restrita 20/10/2026 (implantado), Produção 07/12/2026; Release 2, Produção Restrita 24/11/2026 (implantado), Produção 07/12/2026; Release 3, 07/12/2026 nos dois; Release 4, 24/12/2026 nos dois. O texto chama de "cronograma inicial" e avisa que "Eventuais alterações do cronograma proposto serão publicadas no Portal da NFS-e". Conteúdo: o Release 2 traz o item 2.10 (validação de endereços); o Release 3 traz "AnexoVII-IndOp_IBSCBS_v1.03.00 - Nova tabela de códigos cIndOp"; o Release 4 traz o item 2.4, "Evoluções para o Simples Nacional. Reflexos nos Emissores, DANFSe, layout e regras". A notícia de 20/10/2026 dizia Produção em 01/12/2026; a de 25/11/2026 a substitui por 07/12/2026.
- **Atos do CGSN de 2026.** FATO (API de pesquisa de normas.receita, consulta de 27/11/2026; número, data do ato, data de publicação e ementa): Res. 186 (09/04, publicada 17/04), Res. 190 e 191 (04/08, publicadas 10/08), Res. 194 (25/09, publicada 28/09; "Altera a Resolução CGSN nº 186, de 9 de abril de 2026, que dispõe sobre os prazos e condições para o exercício de opção pelo Simples Nacional e, em relação aos optantes, pelo regime regular do IBS e da CBS"), Res. 195 (21/10, publicada 22/10), Res. 196 (16/11, publicada 24/11; altera as Res. 140/2018 e 190/2026), Res. 197 (16/11, publicada 24/11; regimento interno) e **Res. 198 (16/11, publicada 25/11; altera a Res. 140/2018)**. A Res. 198 não apareceu em nenhuma fonte da rodada 1. Texto de nenhuma delas lido.
- **LC 227/2026.** FATO (texto compilado da LC 214 na Câmara, lido em 27/11/2026): os dispositivos incluídos aparecem anotados "(Artigo acrescido pela Lei Complementar nº 227, de 13/1/2026)". A lei é de 13/01/2026.

---

## Ponto 1. O MEI entra na regra de 2027 do grupo IBS/CBS, e em que data?

**Pergunta simples.** O MEI (opSimpNac 2) também preenche IBS e CBS na NFS-e a partir de 2027? Em que dia?

**O que achei.**

- FATO (FAQ v1.5, pergunta 15.4, lida em 27/11/2026): "Regime do Simples Nacional: para ME, EPP e MEI, o preenchimento de IBS/CBS só é obrigatório a partir de 1º de janeiro de 2027 - se a empresa não optar por destacar voluntariamente antes disso, a nota é autorizada sem o grupo". A pergunta 15.5 repete: "Para o Simples Nacional, o preenchimento do grupo IBS/CBS continua facultativo em 2026, tornando-se obrigatório apenas em 2027". A 20.4: o uso do Emissor Nacional desde 01/11/2026 e o IBS/CBS em 2027 são "cronogramas independentes".
- FATO (Anexo VI v1.04.01, aba "LEIAUTE DPS_NFS-e - RT", linhas 48 e 374, lido em 27/11/2026): os grupos IBSCBS da NFS-e e da DPS são 0-1, com a observação "Para optantes dos Simples Nacional, os grupos IBSCBS só serão obrigatórios a partir de 2027". Na linha 152, opSimpNac 2 = "Optante - Microempreendedor Individual (MEI)": o MEI é optante pelo próprio leiaute.
- FATO (Anexo VI v1.04.01, aba "RN DPS_NFS-e", regra 104): "O valor calculado de receita bruta para os optantes do simples nacional deve ser informado se opSimpNac = 2, 3 ou 4" (Rej., código "EXXX"). O leiaute já prevê informação de IBS/CBS ligada ao MEI.
- FATO (Ato Conjunto nº 7/2026, pela notícia oficial; ver acima): Simples Nacional, 01/01/2027 para as alíneas "d" a "h" do inciso III (serviços gerais da LC 116 e residuais) e 01/03/2027 para plataformas digitais, subitens 1.03, 1.05, 1.09 e 16.01.
- FATO (Release 4, notícia de 25/11/2026): as "Evoluções para o Simples Nacional" (item 2.4 da NT 009, onde estão regApIBSCBSSN, gTribSN e cAtvSN) entram em Produção Restrita e em Produção só em 24/12/2026.
- FATO (FAQ v1.5, 15.1): até 31/12/2026 "a ausência do IBS/CBS não rejeita o documento fiscal - mas evidencia desconformidade". Para o Simples, o art. 348, III, "c", da LC 214 (ver ponto 8) tira as alíquotas de teste de 2026.
- NÃO ENCONTRADO: qualquer fonte, primária ou secundária, com 04/01/2027.
- Rodada 1 (mantido como corroboração): seis fontes secundárias com 01/01/2027 para o MEI (Meu Contador Online, Conta Azul, Radar da Reforma, Contabilidade Clara, Nota Gateway, NFS-e Downloader). O CONFLITO da rodada 1 ("o MEI informa ou não o grupo") se resolve: a FAQ diz que o MEI preenche em 2027; o "MEI não informa o grupo IBSCBS" (rejeição 1107) é regra da NF-e (NT 2026.005), não da NFS-e; e o que o MEI não informa na NFS-e são campos específicos (ponto 2), não o grupo.

**Conclusão: resolvido pela fonte primária.** O MEI entra: o grupo IBS/CBS é facultativo em 2026 e obrigatório em 2027 para optantes, MEI incluído (FAQ v1.5, 15.4; Anexo VI, linhas 48 e 374). A data é 01/01/2027 para serviços gerais e 01/03/2027 para plataformas digitais e subitens 1.03, 1.05, 1.09 e 16.01 (Ato Conjunto nº 7/2026). A data de 04/01/2027 não tem fonte e sai. Ressalva: o texto do ato foi lido pela notícia oficial, não no DOU.

**O que o app faz.** Vigência do grupo IBS/CBS em tabela por regime e por subitem: 01/01/2027 por padrão, 01/03/2027 para 1.03, 1.05, 1.09, 16.01 e plataformas. Em 2026, MEI e Simples emitem sem o grupo. As regras do Simples na DPS só vão a Produção em 24/12/2026 (Release 4): o teste de DPS de MEI com o grupo na Produção Restrita fica para depois de 24/12/2026 e antes de 01/01/2027, como verificação da implementação, não como decisão de regra.

## Ponto 2. regApIBSCBSSN no MEI e o efeito da opção do Simples pelo regime regular

**Pergunta simples.** Que valor de regApIBSCBSSN o MEI informa? O MEI pode optar pelo regime regular de IBS/CBS? O que a opção da ME e da EPP muda?

**O que achei.**

- FATO (Anexo VI v1.04.01, aba "RN DPS_NFS-e", regra 314, lido em 27/11/2026): "O regime de apuração tributária pelo Simples Nacional para IBS e CBS não deve ser preenchido quando o prestador de serviço não for optante do simples nacional ou for MEI, ou seja, o campo opSimpNac = 1 (Não Optante) ou 2 (MEI)." Efeito: Rej.; código "EXXX" (ainda sem número). Regra 313: o campo é obrigatório quando opSimpNac = 3 (ME/EPP) ou 4 (Pendente).
- FATO (mesmo anexo, regra 197): o grupo gTribSN "não deve ser destacado quando opSimpNac = 1 (Não Optante) ou 2 (MEI)". Regra 198: deve ser destacado quando opSimpNac = 3 ou 4 e regApIBSCBSSN = 1 ou 2.
- FATO (mesmo anexo, linha 154, observação do campo): a opção "2 - CBS apurada pelo SN e IBS apurado pelo regime regular" "deverá ser utilizada pelos contribuintes que ultrapassarem os sublimites estabelecidos na Lei Complementar nº 123/2006".
- FATO (NT 009 v1.01, §2.4, p. 8, lida em 27/11/2026): regApIBSCBSSN serve "para o prestador de serviços optante pelo regime do Simples Nacional ou cuja opção esteja pendente".
- FATO (LC 214/2025, art. 41, texto compilado da Câmara, lido em 27/11/2026): "§ 2º Os contribuintes optantes pelo Simples Nacional ou pelo MEI ficam sujeitos às regras desses regimes. § 3º Os optantes pelo Simples Nacional poderão exercer a opção de apurar e recolher o IBS e a CBS pelo regime regular". O §3º não inclui o MEI; o §2º prende o MEI ao próprio regime.
- FATO (Portal do Simples Nacional, notícia de 17/04/2026 sobre a Res. CGSN 186/2026, lida em 27/11/2026): a escolha de IBS/CBS "dentro do boleto único do Simples ou pelo regime regular (por fora)" tem "validade [...] para os meses de janeiro a junho de 2027"; quem não optar recolhe "ambos os tributos na guia única do Simples Nacional" no primeiro semestre, e "no mês de março/2027 haverá uma nova oportunidade para realizar essa opção, que produzirá efeitos no segundo semestre do ano de 2027". E: "essa nova regra de setembro não se aplica ao MEI"; a opção pelo SIMEI "continua sendo realizada em janeiro de cada ano, até o seu último dia útil".
- FATO (API de normas.receita): a Res. CGSN 194/2026 (25/09, DOU 28/09) altera a Res. 186. Que ela levou o prazo da opção a 30/10/2026: FONTE SECUNDÁRIA concordante da rodada 1 (Contábeis, CRCRS, Simtax, Molina, NFe.io; cinco), texto não lido.
- O CONFLITO da rodada 1 sobre o primeiro semestre se desfaz: a opção vale de janeiro a junho de 2027, e quem não optou fica no Simples nesse semestre; as duas leituras eram metades da mesma regra.

**Conclusão: resolvido pela fonte primária.** O MEI não informa regApIBSCBSSN (Anexo VI, regra 314, com rejeição) nem gTribSN (regra 197), e não tem a opção pelo regime regular (LC 214, art. 41, §§2º e 3º; Portal do Simples). Para ME e EPP, o campo é obrigatório (regra 313), e a opção vale por semestre, com nova janela em março de 2027 para o segundo semestre. Só a data de 30/10/2026 da Res. 194 segue em fonte secundária, e não muda o app.

**O que o app faz.** Para opSimpNac 2, o codec nunca envia regApIBSCBSSN nem gTribSN, e a interface não mostra o campo; para opSimpNac 3 ou 4, o campo é obrigatório e vem do cadastro do emissor (1, 2 ou 3), com data de vigência, porque pode mudar no segundo semestre de 2027. O app não sugere a opção. Como o código da regra ainda é "EXXX", a mensagem de rejeição sai pelo texto, sem código fixo.

## Ponto 3. Versão do Anexo VII (cIndOp)

**Pergunta simples.** Qual tabela de códigos de indicador de operação (cIndOp) vale?

**O que achei.**

- FATO (notícia de 09/10/2026 do Ato Técnico Conjunto nº 7): foi aprovada "a versão 1.03.00 da tabela de Indicadores de Operação".
- FATO (Biblioteca RTC, atualizada em 23/11/2026, lida em 27/11/2026): sob "Nota Técnica SE/CGNFS-e nº 009 versão 1.01 (01/10/2026)" estão só "AnexoVI-LeiautesRN_RTC_IBSCBS-v1.04.01 - NT009 v1.01" e "AnexoVII-IndOp_IBSCBS_v1.03.00 - NT009 v1.01".
- FATO (NT 009 v1.01, PDF, lida em 27/11/2026): a lista de anexos do texto cita "AnexoVII-IndOp_IBSCBS_V1.02.01.xlsx". É o mesmo PDF de capa 11/09/2026; a aprovação de 28/09/2026 e a Biblioteca indicam a 1.03.00.
- FATO (arquivo v1.03.00 baixado em 27/11/2026, SHA-256 acima): 40 códigos de 6 dígitos, do 010101 ao 130201, aba única "cIndOp Public", sem carimbo de versão. Não há código "99" (a FONTE SECUNDÁRIA da rodada 1 que falava em código 99 não se confirma neste arquivo).
- FATO (notícia de 25/11/2026): a v1.03.00 entra no Release 3, Produção Restrita e Produção em 07/12/2026.
- FATO (Produção Restrita, listagem de 25/11/2026): "Anexo C IndOp IBSCBS SNNFSe Produção Restrita versão 1.01 20260122 (referente ao AnexoVII-IndOp_IBSCBS_V1.00.00 publicado na seção RTC)". E o Anexo I de 24/11/2026 traz uma aba "CÓDIGO_INDICADOR_OPERAÇÃO_IBS" com 32 códigos e colunas de vigência "01/01/2026 até DD/MM/AAAA" e "à partir de DD/MM/AAAA". Comparação feita nesta rodada: 4 códigos só na aba do Anexo I (030103, 030104, 050103, 050104) e 12 só na v1.03.00 (010104, 010105, 010201, 010202, 020102, 090101, 090102, 110101, 110201, 120101, 130101, 130201).

**Conclusão: resolvido pela fonte primária.** A versão aprovada e publicada para a NT 009 v1.01 é a 1.03.00; o "V1.02.01" do PDF é referência anterior à aprovação. Ela vale em Produção e em Produção Restrita a partir de 07/12/2026 (Release 3). Até lá, a Produção Restrita valida pela tabela antiga (32 códigos), que diverge da nova em 16 códigos.

**O que o app faz.** Tabela de cIndOp como dado, por ambiente e com data de vigência: v1.03.00 (hash acima) a partir de 07/12/2026; tabela antiga só para teste em Produção Restrita antes disso. O usuário escolhe o código na tabela, nunca digita. Rejeição E0901 sugere atualizar a tabela. Se o cronograma mudar, troca a data, não o código.

## Ponto 4. cIndOp: 0-1 ou 1-1

**O que achei.**

- FATO (já registrado em 25/10/2026 e relido em 27/11/2026 no Anexo VI v1.04.01): cIndOp 0-1, com E0859 e E0860 conforme o ind_gIBSCBS; o 1-1 é do Anexo I de 09/02/2026, superado.
- FATO (FAQ v1.5, 15.3, lida em 27/11/2026): "O grupo gIBSCBS passou a ser facultativo. A regra vem do indicador de tributação IBS/CBS (ind_gIBSCBS) da Tabela CST e Classificação Tributária (cClassTrib) [...] Quando o indicador é 1, o grupo é obrigatório (erro E0960); quando é 0 [...] ele não pode ser informado (erro E0961). A mesma regra vale para o cIndOp (erros E0859 e E0860)." A mesma resposta diz que essas atualizações "entrarão no ambiente de Produção nas próximas semanas" (Release 1: 07/12/2026).

**Conclusão: resolvido pela fonte primária.** Sem mudança de nível; agora com a FAQ confirmando o leiaute.

**O que o app faz.** cIndOp e gIBSCBS enviados só com ind_gIBSCBS = 1 e omitidos com 0 (E0859, E0860, E0960, E0961), a partir de 07/12/2026 em Produção.

## Ponto 5. Código de rejeição e vigência do §2.10 (endereço do tomador contra o cadastro)

**Pergunta simples.** Quando a Sefin começa a recusar a nota porque o município do tomador ou do destinatário não bate com o cadastro do CNPJ ou do CPF, e com que código?

**O que achei.**

- FATO (Anexo I de Produção Restrita de 24/11/2026, aba "RN DPS_NFS-e 1.01", lido em 27/11/2026), os quatro códigos que faltavam:
  - **E0244** (regra 281): "Quando o endereço nacional do tomador for informado na DPS e o mesmo for identificado pelo CNPJ, o código do município do endereço do tomador deve existir e corresponder ao município do seu endereço no cadastro CNPJ, na data de competência informada na DPS." Rej.
  - **E0245** (regra 282): o mesmo para tomador identificado pelo CPF, contra o "cadastro CPF". Rej.
  - **E0921** (regra 308): o mesmo para o destinatário CNPJ, com a exceção do subitem 17.05 quando tomador e destinatário têm CNPJ de mesma raiz. Rej.
  - **E0922** (regra 309): o mesmo para o destinatário CPF, com a exceção do 17.05 quando é o mesmo CPF. Rej.
- FATO (mesmo anexo): nas linhas de E0244 e E0245 as colunas de nível da regra e de execução por ambiente estão vazias; nas de E0921 e E0922 estão preenchidas (nível 1, executadas). O anexo não explica a diferença.
- FATO (mesmo anexo): E0238 e E0920 (código IBGE inexistente), E0653 (CEP do tomador inexistente ou fora do município), E0924 (o mesmo para o destinatário), E0243 e E0919 (tomador e destinatário obrigatoriamente identificados, com endereço, quando o cIndOp aponta o município deles como local do fornecimento e há IBS/CBS).
- FATO (Anexo VI v1.04.01, relido em 27/11/2026): não traz E0244, E0245, E0921 nem E0922; só E0238 e E0920. Por isso a rodada anterior não achou os códigos.
- FATO (notícia de 25/11/2026): o item 2.10 está no Release 2, em Produção Restrita desde 24/11/2026 e com Produção prevista para 07/12/2026 ("cronograma inicial").
- FATO (notícia de 09/10/2026): "Em todas as operações, a DPS será rejeitada se o município informado para o tomador ou o destinatário divergir do cadastro do CNPJ ou do CPF."
- FATO (FAQ v1.5, 4.5): o MEI na Emissão Simplificada pode deixar o endereço do tomador em branco quando o serviço não exige retenção nem local de incidência que dependa do endereço; a regra E0244/E0245 só roda "Quando o endereço nacional do tomador for informado".

**Conclusão: resolvido pela fonte primária.** Códigos E0244, E0245, E0921 e E0922; vigência em Produção prevista para 07/12/2026, já ativa na Produção Restrita. Resta uma incerteza menor, sem fonte: o Anexo I deixa em branco o nível e a execução de E0244 e E0245. Leitura conservadora, a ratificar pelo líder: tratar as quatro como ativas a partir de 07/12/2026.

**O que o app faz.** Mensagens próprias para E0244, E0245, E0921, E0922, E0653 e E0924, que pedem conferir o endereço contra o cartão CNPJ ou o cadastro do CPF; conferência local de CEP contra IBGE antes de emitir; origem do endereço marcada no cadastro do tomador. A emissão em Produção a partir de 07/12/2026 depende disso pronto. Para MEI, endereço do tomador opcional só quando o serviço não exige local de incidência pelo endereço (FAQ 4.5). Teste de cada código na Produção Restrita, que já os executa.

## Ponto 6. Pessoa física como emitente (CPF)

**Pergunta simples.** Um autônomo sem CNPJ pode emitir NFS-e nacional com o CPF? Em que condições?

**O que achei.**

- FATO (FAQ v1.5, 2.2, lida em 27/11/2026): "O CNC se torna obrigatório (independente da escolha acima) em 3 situações: Emissão por Pessoa Física (CPF) - profissionais autônomos precisam estar cadastrados no CNC pelo município para emitir".
- FATO (FAQ v1.5, 11.1): "Isso não é erro - é a Regra E0675, que bloqueia campos de tributos federais sempre que o emitente é identificado por CPF. Emita pelo valor bruto; a retenção deve ser calculada e recolhida pelo tomador fora do sistema." E: "existe uma regra irmã, E0676, que bloqueia os mesmos campos para MEI". O valor retido pode ir só em "Informações Complementares" (xInfComp).
- FATO (FAQ v1.5, 11.5): o mesmo indivíduo pode ter cadastro de MEI (CNPJ) e de autônomo (CPF), "cada um com sua própria Inscrição Municipal".
- FATO (FAQ v1.5, 15.1): "profissionais autônomos e serviços notariais/cartorários podem continuar se identificando pelo CPF até que exista um CNPJ técnico próprio para essas atividades". A identificação por CPF é chamada de "transitória".
- FATO (já registrado no spec R7): E0099, domicílio do CPF no município emissor.

**Conclusão: resolvido pela fonte primária.** Era fontes secundárias concordantes na rodada 1. O CPF emite se estiver no CNC do município; não informa tributos federais; e a identificação por CPF é transitória, até um "CNPJ técnico" que ainda não existe.

**O que o app faz.** No cadastro de emissor CPF: aviso de que é preciso estar no CNC do município; primeira emissão em Produção Restrita como confirmação; grupo de tributos federais bloqueado (E0675), com o valor retido só em xInfComp, se o usuário quiser. O modelo de dados do emissor CPF deve aceitar a troca futura por um "CNPJ técnico" sem perder o histórico. O autônomo continua no escopo (L-21).

## Ponto 7. Responsabilidade do software e do emitente; valor da UPF

**Pergunta simples.** Quem distribui o NotaLivre responde junto pelo IBS/CBS se a nota sair errada? Quais multas há para software, e quanto vale a UPF?

**O que achei.**

- FATO (LC 214/2025, art. 24, IV, texto compilado da Câmara, lido em 27/11/2026): "são solidariamente responsáveis pelo pagamento do IBS e da CBS: [...] IV - os desenvolvedores ou fornecedores de programas ou aplicativos utilizados para registro de operações com bens ou com serviços que contenham funções ou comandos inseridos com a finalidade de descumprir a legislação tributária". A leitura "inclusive quando" do spec 6.3 está errada.
- FATO (art. 341-A, incluído pela LC 227/2026): "Constitui infração toda ação ou omissão, ainda que involuntária, que importe inobservância, por parte do sujeito passivo, de obrigação tributária principal ou acessória."
- FATO (art. 341-C, incluído pela LC 227/2026): "É instituída a Unidade Padrão Fiscal dos Tributos sobre Bens e Serviços (UPF), no valor de R$ 200,00 (duzentos reais), a ser atualizada anualmente pela variação do IPCA ou de outro índice que vier a substituí-lo. Parágrafo único. Ato conjunto do CGIBS e da RFB divulgará o valor atualizado da UPF".
- FATO (art. 341-G, incluído pela LC 227/2026), verbatim dos incisos de software:
  - "V - instalar ou manter instalado programa, software, aplicativo fiscal ou solução tecnológica que possibilite a emissão de documentos fiscais com supressão ou redução de valores do tributo ou da operação ou que não atenda aos requisitos estabelecidos na legislação tributária: 100 (cem) UPF por equipamento;"
  - "VI - desenvolver, fornecer ou instalar programa, software, aplicativo fiscal ou solução tecnológica para terceiros que possibilite a emissão de documentos fiscais com supressão ou redução de valores do tributo ou da operação ou que não atenda aos requisitos estabelecidos na legislação tributária: 150 (cento e cinquenta) UPF por equipamento;"
  - Achado novo: os dois incisos têm uma segunda hipótese, "ou que não atenda aos requisitos estabelecidos na legislação tributária", que a reprodução de terceiro da pesquisa de 25/10/2026 não trazia. Com ela, o inciso VI não exige função de fraude: basta software para terceiros que não atenda aos requisitos legais.
- FATO (art. 341-G, IV): 20 UPF por período, independentemente de intimação, e 30 UPF por período e a cada intimação, por entregar, registrar ou manter "em desacordo com a legislação tributária [...] arquivos eletrônicos decorrentes da emissão de documentos fiscais"; §4º: não se aplica "em caso de mera falha ou erro material que não prejudique o conhecimento acerca da natureza, da discriminação, da procedência e do destino da operação".
- FATO (art. 341-G, §5º, III): tributo de referência em 2026, 6% para a CBS e 12% para o IBS sobre o valor da operação; XI, 100% do tributo de referência por operação sem documento; XIII, 66% por documento não idôneo; XVII, 66% ou 33% por cancelamento após o fato gerador ou após o prazo.
- FATO (art. 341-F): 75% sobre o tributo não declarado ou declarado a menor no lançamento de ofício; 100% ou 150% em sonegação, fraude ou conluio.
- FATO (art. 341-H): redução de 50%, 40%, 30% ou 20% da multa conforme o momento do pagamento ou do parcelamento.
- FATO (art. 337 da LC 214): "O sujeito passivo usuário de sistema de processamento de dados deverá manter documentação técnica completa e atualizada do sistema, suficiente para possibilitar a sua auditoria".
- NÃO ENCONTRADO: valor da UPF de 2027 (ato conjunto do CGIBS e da RFB ainda não visto).

**Conclusão.** Textos (art. 24, IV; 341-A; 341-C; 341-G, V e VI; 341-H; 337): **resolvidos pela fonte primária**. Enquadramento do NotaLivre: **resolução recomendada, a ratificar pelo líder**. Sem norma nem jurisprudência sobre o que são "os requisitos estabelecidos na legislação tributária" para software emissor. A leitura conservadora é que o distribuidor responde por 150 UPF por equipamento sempre que o app permitir emitir documento que não atenda ao leiaute e às regras vigentes, mesmo sem intenção. A solidariedade do art. 24, IV, só alcança função inserida com a finalidade de descumprir a lei, que o app não terá.

**O que o app faz.** (1) Nenhuma função que suprima, reduza ou altere valor depois de calculado, nem modo oculto. (2) Validação local completa do leiaute vigente antes de enviar, com bloqueio de emissão se falhar, e tabelas oficiais com vigência (pontos 1, 3 e 5). (3) Aviso ao usuário quando o app estiver com leiaute ou tabela vencidos, com bloqueio de emissão em Produção até atualizar. (4) Trilha de auditoria da origem de cada valor. (5) Documentação técnica do sistema publicada e versionada, para o emitente cumprir o art. 337. (6) Termos de uso que digam que o emitente responde pelo que informa, com texto do Cláudio. A emissão em Produção para terceiros continua presa ao spec 6.5, agora com o inciso VI como risco central.

## Ponto 8. Alíquotas de teste de 2026, período de cura e prazo de guarda das notas

**Pergunta simples.** (a) O Simples paga as alíquotas de teste de 2026? (b) Há prazo para corrigir sem multa uma falha de obrigação acessória de 2026? (c) Por quanto tempo é preciso guardar as notas, e isso trava o "apagar tomador inativo"?

**O que achei.**

- (a) FATO (LC 214, arts. 343, 346 e 348, texto compilado da Câmara, lido em 27/11/2026): art. 343, IBS "à alíquota estadual de 0,1% (um décimo por cento)" em 2026; art. 346, CBS "de 0,9% (nove décimos por cento)" em 2026; art. 348, III: as alíquotas dos arts. 343 e 346 "c) não serão aplicadas em relação às operações dos contribuintes optantes pelo Simples Nacional"; §1º: "Fica dispensado o recolhimento do IBS e da CBS [...] em relação aos sujeitos passivos que cumprirem as obrigações acessórias previstas na legislação".
- (b) FATO (art. 348, §§3º e 4º, cada um anotado "Parágrafo acrescido pela Lei Complementar nº 227, de 13/1/2026"): "§ 3º Durante o período a que se refere o caput deste artigo, caso seja lavrado auto de infração por descumprimento das obrigações acessórias relativas ao IBS e à CBS previstas no art. 341-G desta Lei Complementar, o sujeito passivo será intimado para, no prazo de 60 (sessenta) dias contado da intimação, suprir a omissão apontada pela fiscalização. § 4º O atendimento à intimação a que se refere o § 3º deste artigo importa extinção da penalidade imposta ao sujeito passivo." O texto não tem a palavra "exclusivamente": o CONFLITO da rodada 1 fecha pela redação sem ela.
- (c) FATO (LC 214, art. 516, que dá nova redação ao art. 26, II, da LC 123/2006): as optantes do Simples devem "manter em boa ordem e guarda os documentos que fundamentaram a apuração dos impostos e contribuições devidos e o cumprimento das obrigações acessórias a que se referem os arts. 25 e 25-B desta Lei Complementar enquanto não decorrido o prazo decadencial e não prescritas eventuais ações que lhes sejam pertinentes". O art. 25-B citado é a declaração anual do MEI: a regra alcança o MEI.
- (c) FATO (CTN, texto compilado da Câmara, lido em 27/11/2026): art. 173, I, o direito de constituir o crédito "extingue-se após 5 (cinco) anos, contados: I - do primeiro dia do exercício seguinte àquele em que o lançamento poderia ter sido efetuado"; art. 174, a cobrança "prescreve em 5 (cinco) anos, contados da data da sua constituição definitiva"; art. 195, parágrafo único, livros e comprovantes "serão conservados até que ocorra a prescrição dos créditos tributários decorrentes das operações a que se refiram".
- (c) FATO (LC 214, art. 336): comprovantes da escrituração "serão conservados até que se opere a decadência do direito de a Fazenda Pública constituir os créditos tributários relativos a esses exercícios". Art. 341-G, IV: deixar de "manter" arquivos eletrônicos da emissão é infração (20 ou 30 UPF por período).
- (c) FATO (FAQ v1.5, 17.8): "O documento fiscal oficial, para fins de fiscalização, escrituração e validade jurídica, é exclusivamente o arquivo XML da NFS-e autorizada e armazenado no Ambiente de Dados Nacional (ADN)". A FAQ não diz por quanto tempo o ADN guarda nem por quanto tempo o emitente pode baixar de novo (NÃO ENCONTRADO).
- (c) FATO (LGPD, texto compilado da Câmara, lido em 27/11/2026): art. 16, "Os dados pessoais serão eliminados após o término de seu tratamento [...] autorizada a conservação para as seguintes finalidades: I - cumprimento de obrigação legal ou regulatória pelo controlador"; art. 7º, II, base legal "para o cumprimento de obrigação legal ou regulatória pelo controlador".

**Conclusão.** (a) e (b): **resolvidos pela fonte primária**. (c) As regras estão **resolvidas pela fonte primária**: guarda enquanto não decair, em 5 anos contados de 1º de janeiro do ano seguinte, e enquanto não prescrever ação, em 5 anos contados da constituição definitiva do crédito; a LGPD autoriza conservar para cumprir essa obrigação. O número de anos de cada caso é **resolução recomendada, a ratificar pelo líder**, porque depende de fatos que o app não conhece (lançamento, processo, parcelamento).

**O que o app faz.** Em 2026, nenhum IBS/CBS a pagar para MEI e Simples. O texto de ajuda pode citar a cura de 60 dias. O app nunca apaga por conta própria o XML autorizado nem o PDF de NFS-e emitida. O "apagar tomador inativo após N anos" apaga só o cadastro do tomador; a nota guarda a própria cópia dos dados da época (decisão do líder, rodada 6). N conta de 1º de janeiro do ano seguinte ao da última nota do tomador, nunca da data da nota, e o app não apaga tomador ligado a nota marcada pelo usuário como "em fiscalização, processo ou parcelamento". O N = 5 do spec (4.14) fica compatível com o art. 173, I. Apagar uma nota emitida é ato manual do usuário, com aviso do art. 26, II, da LC 123 e do art. 195 do CTN. O aviso LGPD cita o art. 16, I, e o art. 7º, II, como base para manter os dados das notas, com texto do Cláudio.

---

## Tabela-resumo (rodada 2)

| # | Ponto | Resposta curta | Nível | O que o app faz |
|---|---|---|---|---|
| 1 | MEI na regra de 2027 do grupo IBS/CBS; data | O MEI entra. Grupo facultativo em 2026 e obrigatório em 2027 para optantes, MEI incluído (FAQ v1.5, 15.4; Anexo VI, linhas 48 e 374). 01/01/2027 para serviços gerais e 01/03/2027 para plataformas e subitens 1.03, 1.05, 1.09 e 16.01 (Ato Conjunto nº 7/2026). 04/01/2027 sem fonte | Resolvido pela fonte primária | Vigência em tabela por regime e subitem; em 2026 emite sem o grupo; teste do MEI com o grupo na Produção Restrita entre 24/12/2026 (Release 4) e 01/01/2027 |
| 2 | regApIBSCBSSN do MEI; opção pelo regime regular | O MEI não informa regApIBSCBSSN (Anexo VI, regra 314, Rej.) nem gTribSN (regra 197) e não tem a opção (LC 214, art. 41, §§2º e 3º; Portal do Simples). ME e EPP: campo obrigatório (regra 313); opção por semestre, nova janela em março de 2027 | Resolvido pela fonte primária | Para opSimpNac 2, nunca envia regApIBSCBSSN nem gTribSN; para 3 e 4, campo obrigatório vindo do cadastro com data de vigência |
| 3 | Versão do Anexo VII | v1.03.00, aprovada pelo Ato Técnico Conjunto nº 7 e única publicada para a NT 009 v1.01 (40 códigos); em Produção e Produção Restrita a partir de 07/12/2026 (Release 3); até lá a Produção Restrita usa tabela antiga de 32 códigos | Resolvido pela fonte primária | Tabela por ambiente e com vigência; v1.03.00 a partir de 07/12/2026; escolha só da tabela; E0901 sugere atualizar |
| 4 | cIndOp 0-1 ou 1-1 | 0-1, condicionado ao ind_gIBSCBS (Anexo VI v1.04.01; FAQ v1.5, 15.3) | Resolvido pela fonte primária | Envia com ind_gIBSCBS = 1 e omite com 0 (E0859, E0860, E0960, E0961) |
| 5 | Código e vigência do §2.10 | E0244 e E0245 (tomador CNPJ e CPF), E0921 e E0922 (destinatário, com exceção do 17.05), no Anexo I de Produção Restrita de 24/11/2026; Release 2, Produção prevista para 07/12/2026 | Resolvido pela fonte primária. Tratar E0244 e E0245 como ativas, embora o anexo deixe em branco o nível delas: resolução recomendada, a ratificar pelo líder | Mensagens próprias para os quatro códigos e para E0653 e E0924; CEP contra IBGE antes de emitir; pronto antes de 07/12/2026 |
| 6 | Pessoa física como emitente | CPF emite se estiver no CNC do município; sem tributos federais (E0675); identificação por CPF transitória, até um "CNPJ técnico" (FAQ v1.5, 2.2, 11.1 e 15.1) | Resolvido pela fonte primária | Aviso do CNC no cadastro; tribFed bloqueado; retenção só em xInfComp; modelo pronto para a troca por CNPJ técnico |
| 7 | Responsabilidade do software; UPF | Art. 24, IV: só função inserida "com a finalidade de" descumprir. UPF R$ 200,00 com IPCA (art. 341-C). Art. 341-G, VI: 150 UPF por equipamento para software para terceiros que permita suprimir valor "ou que não atenda aos requisitos estabelecidos na legislação tributária" | Textos: resolvidos pela fonte primária. Enquadramento do NotaLivre no inciso VI: resolução recomendada, a ratificar pelo líder | Sem função de suprimir valor; validação local completa e bloqueio de emissão com leiaute ou tabela vencidos; auditoria de valores; documentação técnica (art. 337); termos pelo Cláudio |
| 8 | Teste de 2026, cura e guarda | Simples fora das alíquotas de teste (art. 348, III, "c"); cura de 60 dias sem "exclusivamente" (art. 348, §§3º e 4º, LC 227); guarda até decair e prescrever (LC 123, art. 26, II; CTN, arts. 173, 174 e 195); LGPD, art. 16, I | (a) e (b): resolvidos pela fonte primária. (c): regras resolvidas pela fonte primária; anos por caso em resolução recomendada, a ratificar pelo líder | Sem IBS/CBS a pagar em 2026 para MEI e Simples; nunca apaga NFS-e emitida; "apagar inativo" só no cadastro, com N contado de 1º/01 do ano seguinte à última nota |

## O que mudou da rodada 1 para a rodada 2

| # | Rodada 1 | Rodada 2 | Por quê |
|---|---|---|---|
| 1 | Data por fontes secundárias; MEI com o grupo em CONFLITO; resolução recomendada | Resolvido pela fonte primária | FAQ v1.5 (15.4) inclui o MEI; Anexo VI (linhas 48, 152 e 374); notícia oficial do Ato Conjunto nº 7 com as datas por subitem |
| 2 | Resolução recomendada (MEI não envia, por analogia) | Resolvido pela fonte primária | Anexo VI, regra 314, proíbe com rejeição; LC 214, art. 41, lido; Portal do Simples sobre o semestre e o MEI |
| 3 | Resolução recomendada (vale o arquivo publicado) | Resolvido pela fonte primária | Ato Técnico Conjunto nº 7 aprovou a v1.03.00; Release 3 em 07/12/2026; achado novo: a Produção Restrita ainda usa tabela antiga, com 16 códigos de diferença |
| 4 | Resolvido pela fonte primária | Igual, reforçado | FAQ v1.5, 15.3 |
| 5 | Resolução recomendada (sem código nem data) | Resolvido pela fonte primária (resta uma ressalva menor sobre o nível de E0244 e E0245) | Anexo I de 24/11/2026 com E0244, E0245, E0921 e E0922; Release 2 com Produção em 07/12/2026 |
| 6 | Fontes secundárias concordantes | Resolvido pela fonte primária | FAQ v1.5, 2.2, 11.1 e 15.1; novo: CPF é identificação transitória |
| 7 | Fontes secundárias concordantes; VI em resolução recomendada | Textos resolvidos pela fonte primária; enquadramento em resolução recomendada | Texto compilado da Câmara. Achado novo e mais grave: os incisos V e VI também punem software "que não atenda aos requisitos estabelecidos na legislação tributária" |
| 8 | (a) e (b) por fontes secundárias; (c) resolução recomendada | (a), (b) e regras de (c) resolvidas pela fonte primária; anos por caso em resolução recomendada | LC 214, arts. 343, 346, 348 e 516; CTN, arts. 173, 174 e 195; LGPD, arts. 7º e 16; o "exclusivamente" não existe no texto |

## O que muda no app

Propostas técnicas para o CTO e o líder. Itens "a ratificar pelo líder" não viram código antes da ratificação.

1. **Calendário (pontos 1, 3, 5).** Três datas oficiais mandam no app: 07/12/2026 (Releases 1, 2 e 3 em Produção: cIndOp e gIBSCBS condicionados, validação de endereço com E0244, E0245, E0921 e E0922, Anexo VII v1.03.00); 24/12/2026 (Release 4: regras do Simples na DPS); e 01/01/2027 e 01/03/2027 (grupo IBS/CBS obrigatório para MEI e Simples, por subitem). O cronograma dos releases é "inicial": as datas ficam em tabela, não no código.
2. **Regime (ponto 2).** MEI: sem regApIBSCBSSN e sem gTribSN. ME e EPP: regApIBSCBSSN obrigatório, do cadastro, com vigência por semestre.
3. **Tabelas (ponto 3).** cIndOp por ambiente e por vigência, com hash; a de Produção Restrita difere da v1.03.00 até 07/12/2026.
4. **Endereço do tomador (ponto 5).** Mensagens para os códigos novos, CEP contra IBGE antes de emitir, cadastro local atualizado quando o usuário corrige (rodada 6). Pronto antes de 07/12/2026.
5. **CPF emitente (ponto 6).** Aviso do CNC, tribFed bloqueado, modelo pronto para o futuro CNPJ técnico.
6. **Software (ponto 7).** O inciso VI do art. 341-G (150 UPF por equipamento para software para terceiros que não atenda aos requisitos legais) passa a ser o risco central do spec 6.5: validação local completa, bloqueio de emissão com leiaute ou tabela vencidos, documentação técnica do sistema (art. 337), auditoria de valores e termos de uso.
7. **Guarda (ponto 8).** O app nunca apaga NFS-e emitida por conta própria; o "apagar inativo" só apaga o cadastro do tomador, com N contado de 1º de janeiro do ano seguinte à última nota.
8. **Correções no spec (não feitas por este agente).** O spec 6.3 deve trocar "inclusive quando", no art. 24, IV, por "que contenham funções ou comandos inseridos com a finalidade de descumprir a legislação tributária", e incluir a segunda hipótese dos incisos V e VI do art. 341-G. O spec 6.4, item 1, perde a data de 04/01/2027.

## Fontes secundárias da rodada 1 que continuam úteis

Mantidas como corroboração; nenhuma conclusão acima depende só delas.

- MEI em 01/01/2027: Meu Contador Online (https://www.meucontadoronline.com.br/blog/mei-sera-obrigado-a-preencher-ibs-cbs/), Conta Azul (https://ajuda.contaazul.com/hc/pt-br/articles/49970205052301-NFS-e-Reforma-Tribut%C3%A1ria-o-MEI-precisa-adotar-as-mudan%C3%A7as), NFS-e Downloader (https://www.nfsedownloader.com.br/preencher-ibs-cbs-nfse).
- Res. CGSN 194 com prazo de 30/10/2026: Contábeis (https://www.contabeis.com.br/noticias/79714/simples-nacional-2027-opcao-e-prorrogada-ate-15-de-outubro/), CRCRS (https://www.crcrs.org.br/noticias/?p=35065), NFe.io (https://nfe.io/blog/reforma-tributaria/novo-prazo-simples-nacional-ibs-cbs/).
- Res. CGSN 196 (MEI não gera crédito ao adquirente, ressalvado o art. 169 da LC 214): Legisweb (https://www.legisweb.com.br/legislacao/?id=504837), Barbieri (https://www.barbieriadvogados.com/resolucao-cgsn-196-2026/). Texto não lido; ementa e datas confirmadas pela API de normas.receita.
- Doutrina sobre o art. 24, IV: Papo Fiscal (https://papofiscal.blog/posts/reforma-tributaria-responsabilidade-tributaria-solidaria/), Capital Aberto (https://legislacaoemercados.capitalaberto.com.br/reforma-aumenta-a-responsabilidade-tributaria-solidaria/).
- Guarda de 5 anos contados de 1º de janeiro: Avalara/Oobj (https://oobj.com.br/gestao/por-quanto-tempo-guardar-notas-fiscais/), Jus.com.br (https://jus.com.br/artigos/10687/decadencia-e-conservacao-de-documentos-fiscais).

## Limitações

- Não lidos no primário: o texto do Ato Conjunto RFB/CGIBS nº 7/2026 (host `cgibs.gov.br`) e o texto das Res. CGSN 194, 196 e 198/2026 (host `normasinternet2.receita.fazenda.gov.br`). Os dois hosts estão fora da lista aprovada e não foram tentados.
- `www.planalto.gov.br` não respondeu e `legis.senado.leg.br` deu 403; as leis vieram do texto compilado da Câmara, que é norma atualizada, não a publicação no DOU.
- `normas.receita.fazenda.gov.br` só devolveu listagem pela API; as páginas antigas (`sijut2consulta/link.action`) caem no aplicativo novo sem texto.
- A FAQ v1.5 é feita "com auxílio de inteligência artificial e revisado", e a pergunta 15.1 está desatualizada em relação à 15.4 e ao Ato Conjunto nº 7.
- Valor da UPF de 2027 e tempo de guarda no ADN: NÃO ENCONTRADOS.
- Houve resets de conexão intermitentes em `www.gov.br` e `normas.receita.fazenda.gov.br`; todo download foi repetido até 200, e os hashes estão na tabela de fontes.
