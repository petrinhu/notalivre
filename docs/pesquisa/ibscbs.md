# Pesquisa de fatos: IBS/CBS na NFS-e Nacional (25/10/2026)

Ordem: só coleta de fatos, sem desenho e sem parecer vinculante. FATO = com URL e data. INFERÊNCIA marcada. NÃO ENCONTRADO = com o que foi procurado.
Limitação de ferramenta: o agente não tem shell (sem Bash). Xlsx e PDFs chegam ao WebFetch como binário; PDF foi lido via Read (páginas renderizadas) e xlsx NÃO foi extraído.

## Q1 Grupo IBSCBS na DPS

### Q1.a Anexo I DPS v1.01 de 09/02/2026 (FATO, dump local)
- Fonte: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_i-sefin_adn-dps_nfse-snnfse-v1-01-20260209.xlsx (dump local: <cópia local não versionada>, aba "LEIAUTE DPS_NFS-e", linhas 336-417 do dump; aba "RN DPS_NFS-e").
- Grupo IBSCBS (infDPS/IBSCBS): ocorrência 0-1. Nota do leiaute (I336): "Para optantes dos Simples Nacional, os grupos IBSCBS só serão obrigatórios a partir de 2027."
- Campos diretos (tipo, tamanho, ocorrência):
  - finNFSe N1 1-1 (0 = NFS-e regular)
  - indFinal N1 0-1 (uso/consumo pessoal, art. 57; nota: "será descontinuado a partir da implantação do layout da NT 005"). NT 009 §2.5 reinsere indFinal (0-1, N1, 0 Não / 1 Sim).
  - cIndOp N6 1-1 (código indicador da operação, tabela "ANEXO C")
  - tpOper N1 0-1 (1 a 5; obrigatório em casos da LC 214/2025 art. 10 §2º e art. 254 III/IV)
  - gRefNFSe 0-1 (obrigatório se tpOper = 2 ou 3); refNFSe C50 1-99
  - tpEnteGov N1 0-1 (1 União, 2 Estado, 3 DF, 4 Município; só compras governamentais)
  - indDest N1 1-1 (0 = destinatário é o tomador; 1 = não é)
  - dest 0-1 (CNPJ N14 / CPF N11 / NIF C40 / cNaoNIF N1, escolha CE; xNome C1-150 1-1; end 0-1 com endNac [cMun N7, CEP N8] ou endExt [cPais C2, cEndPost C1-11, xCidade C1-60, xEstProvReg C1-60]; xLgr C1-255; nro C1-60; xCpl C1-156 0-1; xBairro C1-60; fone N6-20 0-1; email C1-80 0-1)
  - imovel 0-1 (inscImobFisc C1-30 0-1; cCIB C8 em escolha CE; end com CEP/endExt, xLgr, nro, xCpl, xBairro)
  - valores 1-1 com gReeRepRes 0-1 (documentos 1-1000; dFeNacional ou docFiscalOutro; docOutro; fornec; dtEmiDoc; dtCompDoc; tpReeRepRes N2; xTpReeRepRes 0-1; vlrReeRepRes N1-15V2)
  - trib 1-1 / gIBSCBS 1-1 (v1.01): CST N3 1-1; cClassTrib N6 1-1; cCredPres N2 0-1; gTribRegular 0-1 (CSTReg N3 1-1; cClassTribReg N6 1-1); gDif 0-1 (pDifUF, pDifMun, pDifCBS, N1-3V2 cada, 1-1)
  - DPS não traz alíquota (pIBS/pCBS), base de cálculo nem valor de IBS/CBS no v1.01: são calculados pela Calculadora (INFERÊNCIA a partir das regras "CALCULADORA" abaixo).
- Regras E no grupo (coluna K = emissores públicos Sefin/WEB/APP; M = ADN):
  - E0901 cIndOp inexistente (Rej; K=V, M=V)
  - E0905 gRefNFSe obrigatório se tpOper 2 ou 3 (K=V)
  - E0906 gRefNFSe proibido se tpOper não for informado ou for 1, 4 ou 5 (K=V; FATO, Anexo I de Produção Restrita de 20/10/2026, aba RN DPS_NFS-e 1.01, linha Excel 577; E0905 na linha 576)
  - E0907 chave de refNFSe com DV (K=V)
  - E0910 dest só pode ser identificado se indDest = 1 (K=V)
  - E0911 CNPJ do dest com DV; E0913 CPF do dest com DV
  - E0920 cMun do dest existe no IBGE; E0930 e-mail do dest válido
  - E0933 cCIB válido (7 caracteres + DV)
  - E0940 chaveDFe; E0942 docFiscalOutro só se dtCompDoc anterior a 31/12/2025; E0943 cMunDocFiscal IBGE; E0945 e E0947 CNPJ/CPF do fornecedor com DV; E0950 e E0951 datas do documento; E0952 xTpReeRepRes só se tpReeRepRes = 99; E0953 vlrReeRepRes ≤ valor do serviço
  - E0958 cClassTrib não suportado para prestação de serviço (validoParaSiglaDfeInformado = false) - depende da Calculadora (coluna O = CALCULADORA)
  - E0964 gTribRegular não deve ser informado quando exigeGrupoTributacaoRegular = false (CALCULADORA)
  - E0969 cClassTribReg não suportado para prestação de serviço (CALCULADORA)
  - E0971 gDif não deve ser informado quando permiteDiferimento = false; E0972 gDif obrigatório quando permiteDiferimento = true (CALCULADORA)
  - E0187 (K=V): grupo toma obrigatório para cIndOp de uma lista (030102, 050102, 100101, 100301, 100501, 030103, 050103, 100102, 100201, 100302, 100401, 100502, 100601) - já registrado na pesquisa anterior
  - E0675 (K=V): tribFed (PIS/COFINS) não pode ser preenchido quando o emitente é CPF (pessoa física) - FATO, relevante ao emissor PF, lembrando que é grupo tributos federais, não IBS/CBS
- Regras E de NFS-e (saída, não DPS) com referência a Calculadora: E1539 (alíquota UF IBS), E1560 (gIBSCredPres), E1583, E1602. Isto é FATO para o leiaute NFS-e, não DPS.

### Q1.b NT 009 v1.01 de 11/09/2026 (FATO, lida no PDF renderizado)
- Fonte: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nota-tecnica-009-se-cgnfs-e-v-1-01.pdf (23 páginas; cópia em <cópia local não versionada>). Data da capa: 11/09/2026.
- Anexos citados pela NT (p.3): AnexoVI-LeiautesRN_RTC_IBSCBS-V1.04.01 (leiaute, NT 009) e AnexoVII-IndOp_IBSCBS_V1.02.01 (tabela cIndOp, "baseada no art. 11 da LC 214/2025"). CONFLITO: a página da biblioteca lista o Anexo VII como "anexovii-indop_ibscbs_v1-03-00-nt009.xlsx" (URL: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovii-indop_ibscbs_v1-03-00-nt009.xlsx). Não extraído: qual versão vale, a NT diz v1.02.01 e a página diz v1.03.00.
- §2.1 CNPJ alfanumérico (p.3): "a ser implementado a partir de julho de 2026"; todos os campos CNPJ passam de N para C.
- §2.2 Notas de ajuste (p.3-4): gIBSCBSAjuste (vIBS, vCBS N15V2 1-1) e finNFSe 0 regular / 1 crédito / 2 débito; tpNFSeDebito (01 a 06), tpNFSeCredito (01 e 05).
- §2.3 (p.4-7): vDedRed e gReeRepRes unificados em vAjusteBC (pAjusteBCISSQN, vAjusteBCISSQN, docAjusteBC tpAjusteBC 1-1 N1-3 com tipos 2, 5 a 9, 99, 101 a 105, 199; dFeNacional; docFiscalOutro; docOutro; fornec com endereço e cNaoNIF). Tipos 1, 3 e 4 descontinuados. Esta regra é de DPS.
- §2.4 Simples Nacional na DPS (p.8-9): opSimpNac ganha valor 4 - "Optante Pendente". regApIBSCBSSN (0-1, N1): 1 = IBS e CBS apurados pelo SN; 2 = CBS apurada pelo SN e IBS pelo regime regular; 3 = IBS e CBS pelo regime regular. cAtvSN (0-1, N2): 7, 8, 9, 10, 11, 12, 13, 14, 90 (códigos da LC 123/2006). Nota: regras de cAtvSN passam a ser bicondicionais.
- §2.5 indFinal reinserido (0-1, N1; 0 Não, 1 Sim) (p.10).
- §2.6 imóvel (p.10-12): grupo gLocacao (pCoPropriedade N1-3V2 1-1; vTotOper N1-15V2 1-1; vDescIncondTot 0-1; vDescCondTot 0-1; dVencOrig 0-1) e gUnidImob 0-99 (cMun, inscImobFisc, cCIB, end, gAjusteBCLocImoveis 0-10 com tpAjusteBCLocImoveis, xTpAjusteBCLocImoveis, vAjusteBCLocImoveis).
- §2.7 bensMoveis 0-1000 (renomeado de gLocBensMoveis) (p.12).
- §2.8 gPgtoVinc 0-1 (vinculação com transação de pagamento) (p.13): pgto 1-99 com nPag N3 1-1, idTransacao C2-35, tpMeioPgto N2 (15 Boleto; 17 Pix QRCode dinâmico; 18 TED; 20 Pix chave/QRCode estático; 23 Pix automático; 24 TEF/booktransfer), CNPJReceb C14 1-1, CNPJBasePSP C8 1-1.
- §2.10 (p.13-14) - FATO RELEVANTE: "Na Sefin Nacional, os endereços do Tomador/Adquirente e do Destinatário deverão ser validados com base nas informações constantes dos cadastros CNPJ/CPF. Essa exigência advém do art. 11 da LC 214/2025, bem como dos arts. 12 dos regulamentos do IBS e da CBS: Resolução CGIBS nº 6, de 30 de abril de 2026; e Decreto nº 12.955, de 29 de abril de 2026." E: "Para todas as operações (todos os subitens), a DPS, ou a NFS-e no caso dos compartilhamentos pelos municípios, deverá ser rejeitada quando o código IBGE do município informado divergir daquele constante no respectivo cadastro do Tomador/Adquirente e/ou do Destinatário." Para o subitem 17.05 define o município de incidência do ISSQN pelo endereço do tomador ou do destinatário (p.14).
  - INFERÊNCIA: isto contradiz a leitura anterior de que a Sefin só valida existência e formato do CNPJ/CPF do tomador. A NT diz que o endereço é validado contra o cadastro. O texto não diz se a Sefin devolve o cadastro ao emissor.
- §2.11 indDest e dest REPOSICIONADOS para infDPS, raiz (após o grupo toma) (p.14-15).
- §2.12 comExt (p.15-17): nFatura, vFatura, nContCambio, nDUIMP, nDUE, mdic, movTempBens, mdPrestacao (inclui 5 = consumo concomitante), vincPrest (9 = desconhecido), mecAFComexP e mecAFComexT, tpMoeda (validação pela tabela do BCB). FATO: "a partir de 01/01/2027, o domínio 'Desconhecido (tipo não informado na nota de origem)' ... não será admitido nas NFS-e compartilhadas pelos municípios ao ADN" (p.16).
- §2.13 vAjusteBCIBSCBSComExt (NFS-e, p.17-18).
- §2.14 cMun do local do evento (atvEvento) para cTribNac 12.13 e 17.10 (p.18).
- §2.15 condomínios (p.19-20): novo código 99.05.01 (Operações Condominiais). Grupo condominios (0-1) com gCobranca (1-20) e gDescont (0-10) (p.20).
- §2.16 CST e cClassTrib REPOSICIONADOS (p.21-22): CST (N3, 1-1) e cClassTrib (N6, 1-1) passam para infDPS/IBSCBS/valores/trib/. O grupo gIBSCBS passa a 0-1. FATO (texto da NT): "Essa alteração decorre da necessidade de permitir a identificação prévia do enquadramento tributário ... por meio do CST"; a relação de quando gIBSCBS é exigido vem do indicador "ind_gIBSCBS" da Tabela CST e cClassTrib, citada como Informe Técnico 2025.002 v1.60 (publicado 23/06/2026). Os campos passam a ter OCOR 0-1 (lista do texto: NFS-e cLocalidadeIncid, xLocalidadeIncid, valores, vBC, totCIBS/gIBS, totCIBS/gCBS; DPS cIndOp e valores/trib/gIBSCBS). Casos de não exigência de IBS/CBS citados: CST 400 (isenção), 410 (imunidade e não incidência), 820 (tributação em documento específico).
  - INFERÊNCIA: a lista do §2.16 inclui cIndOp como 0-1 na DPS, enquanto o Anexo I v1.01 (20260209) traz cIndOp 1-1. Confirmar no Anexo VI v1.04.01 antes de implementar.
- Tabelas e ferramentas indicadas pela NT (p.22-23) (FATO):
  - Tabela CST e cClassTrib: https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/classificacoes-tributarias ; espelho: https://dfe-portal.svrs.rs.gov.br/DFE/ClassificacaoTributaria
  - Calculadora de Tributos (motor oficial, "versão online, na Plataforma da Reforma Tributária, e offline, em código aberto, para ser embarcado nos ERPs via API"): online https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/regime-geral ; offline https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/calculadora-offline
  - Na NFS-e, o processo de geração tem integração online com a calculadora para validação de campos.
- §1 Introdução (p.3), cronograma: "Será publicado no portal da NFS-e o cronograma de implantação das funcionalidades e evoluções presentes nesta Nota Técnica." A NT não traz datas de obrigatoriedade de IBS/CBS.

### Q1.c Anexo VI v1.04.01 / Anexo VII (cIndOp) - NÃO EXTRAÍDO
- Anexo VI v1.04.01 (leiaute RN RTC IBSCBS, vinculado à NT 009): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovi-leiautesrn_rtc_ibscbs-v1-04-01-nt009.xlsx
- Anexo VII (tabela cIndOp): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovii-indop_ibscbs_v1-03-00-nt009.xlsx (ver conflito de versão acima)
- Anexo VIII v1.01.00 (correlação item de serviço / NBS / cClassTrib / cIndOp): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexoviii-correlacaoitemnbsindopcclasstrib_ibscbs_v1-01-00.xlsx ; a página informa que é "trabalho inicial, sem regras de negócio no Piloto RTC nem em Produção".
- Motivo de NÃO extração: WebFetch devolve xlsx como binário ZIP; sem shell o agente não descompacta. O delta 20261020 (produção restrita, https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx) NÃO FOI LIDO. NÃO ENCONTRADO: lista de diferenças entre 20260209 e 20261020.
- Consequência para Q1: a lista de campos acima é do v1.01 de 09/02/2026 + as alterações da NT 009 v1.01 (texto). Não é o Anexo I 20261020.

### Q1.d Campos adicionais da NT 007 v1.0 (07/02/2026) - FATO
- Fonte: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nt-007-se-cgnfse-v1-0.pdf (cópia lida: <cópia local não versionada>, p.3-9).
- Anexos da NT 007 (p.3): AnexoVI-LeiautesRN_RTC_IBSCBS-V1.03.00 e AnexoVII-IndOp_IBSCBS_V1.01.00 (cIndOp, baseado no art. 11 da LC 214/2025).
- indZFMALC (DPS, infDPS/IBSCBS, 0-1, N1: 0 Não / 1 Sim): operação de fornecimento favorecida com alíquota zero de CBS (arts. 451 e 466 da LC 214/2025) (p.3).
- PIS/COFINS na DPS (p.4-6): vPis e vCofins são débitos de apuração própria, não valores retidos (NT 007 §2.c); domínio de CST PIS/COFINS atualizado (00 a 99); tpRetPisCofins passa a aceitar 0 e 3 a 9; códigos 1 e 2 serão suprimidos quando os grupos IBSCBS forem obrigatórios; valores retidos de PIS, COFINS e CSLL são somados em vRetCSLL. Disponível em produção e produção restrita em 09/02/2026 (p.4).
- Esclarecimentos (p.7-8) sobre novos fatos geradores (cTribNac 99.02.01, 99.03.01 a 99.03.05, 99.04.01; 99.01.01 só quando incidir IBS/CBS mas não ISSQN): "Todas as pessoas, físicas ou jurídicas, estarão autorizadas a emitir esses documentos na plataforma nacional"; "inicialmente todo CPF e todo CNPJ será autorizado a utilizar os emissores públicos nacionais (API, Web ou APP), mesmo que o seu Município ... tenha optado por não utilizar os emissores públicos nacionais". As evoluções "ainda não estão disponíveis e o cronograma dessas implantações será publicado no portal da NFS-e" (p.8).
- Numeração (p.9): nNFSe é atribuído pela Sefin ao receber a DPS; número reservado e não persistido gera "pulos" na numeração; NT afirma que o número reservado não pode ser reutilizado.

### Q1.e Campos e regras de IBS/CBS que o app precisa tratar (síntese FATO das fontes acima)
- Tipos: CNPJ passa a C (alfanumérico) em todos os campos (NT 009 §2.1). CPF continua N11. cIndOp N6. CST N3. cClassTrib N6 (Anexo I v1.01 e NT 009 §2.16). cCredPres N2. cNBS N9 0-1 (Anexo I, "seguindo a versão 2.0, conforme Anexo B"). cTribNac N6 1-1 (Anexo I v1.01 linha 195; obrigatório). cMun N7 (IBGE). CEP N8.
- FATO (dump Anexo I v1.01, grupo gIBSCBS da DPS): os filhos são CST, cClassTrib, cCredPres, gTribRegular (CSTReg, cClassTribReg) e gDif (pDifUF, pDifMun, pDifCBS). A DPS v1.01 NÃO tem campo de alíquota pIBS/pCBS nem valor de IBS/CBS.
- FATO (NT 009 p.22-23, §2.16): "A Calculadora é o motor de cálculo oficial dos tributos da Reforma Tributária sobre o Consumo ... Na NFS-e, no processo de autorização/geração do documento, há uma calculadora para validação de campos, verificação de regras e retorno de valores em conformidade." §2.9 (p.13): verCalcIBSCBS é a versão da Calculadora que gerou a NFS-e, "gerada pela SEFIN Nacional ... ambGer = 2".
- INFERÊNCIA: o app de emissão por API (emissores públicos) provavelmente precisa informar classificação (CST, cClassTrib, cIndOp, indicadores), não alíquota. Confirmar com o Anexo VI v1.04.01 e com a Calculadora offline antes de desenhar.
- Cálculo ISSQN x IBS/CBS: NT 009 §2.3/§2.13 (p.8, p.18): vBC do IBS/CBS deduz vISSQN; a fórmula inclui PIS e COFINS "até 2026" e deixa de incluí-los "a partir de 2027 até 2032".

## Q2 Cronograma e obrigatoriedade

- FATO (primário, lido): NT 009 v1.01 (11/09/2026) §1 (p.3): "Será publicado no portal da NFS-e o cronograma de implantação das funcionalidades e evoluções presentes nesta Nota Técnica." O cronograma não estava publicado no que foi lido (www.gov.br/nfse "Plano de Implantação" lido em 25/10/2026: só marcos de 2022, sem IBS/CBS; URL https://www.gov.br/nfse/pt-br/municipios/cronograma-de-implantacao-da-nfs-e).
- FATO (primário, lido): NT 007 (07/02/2026) p.8: "o cronograma dessas implantações será publicado no portal da NFS-e".
- FATO (Anexo I v1.01, 09/02/2026, I336): "Para optantes dos Simples Nacional, os grupos IBSCBS só serão obrigatórios a partir de 2027."
- FATO (NT 009 §2.4, p.8-9): opSimpNac ganha valor 4 (Optante Pendente) e regApIBSCBSSN (1 = IBS e CBS apurados pelo SN; 2 = CBS pelo SN e IBS pelo regime regular; 3 = IBS e CBS pelo regime regular), para optante ou cuja opção esteja pendente.
- FATO (secundário, não verificado no DOU): Ato Conjunto RFB/CGIBS nº 4/2026, assinado 30/07/2026, publicado DOU 31/07/2026, regulamenta art. 112 do Regulamento do IBS (CGIBS Res. 6/2026) e da CBS (Decreto 12.955/2026). Fontes: https://www.estrategiaconcursos.com.br/blog/ato-conjunto-rfb-cgibs-4-2026/ ; https://coad.com.br/home/noticias-detalhe/138948/ibs-e-cbs-novas-regras-para-emissao-de-notas-fiscais-entram-em-vigor-a-partir-de-3-de-agosto ; https://www.economia.df.gov.br/documents/d/seec/_notas-fiscais_-conheca-o-cronograma-da-obrigatoriedade-de-emissao_-1-
  - Datas segundo fontes secundárias (CONFLITANTES entre si): 03/08/2026 NF-e, NFC-e, CT-e, MDF-e; 01/10/2026 NFS-e (um blog inclui também NFCom; outra fonte diz que a NFS-e do ISS da LC 116 entra nesta fase); 01/12/2026 plataformas digitais e subitens 1.03, 1.05, 1.09 da LC 116 (divergência sobre quais subitens); 01/01/2027 Simples Nacional. Fonte: https://simplifique.contmatic.com.br/blogs/obrigatoriedade-ibs-cbs-documentos-fiscais-4-ondas-2026
  - Hoje é 25/10/2026: a data de 01/10/2026 já passou segundo as fontes secundárias. Confirmar no DOU antes de qualquer decisão.
- Ano de teste 2026 (FATO secundário): IBS 0,1% (estadual) e CBS 0,9%, "finalidade exclusivamente operacional", recolhimento dispensado se cumpridas as obrigações acessórias (art. 348 §1º). Fontes: https://www.bmalaw.com.br/conteudo/direito-tributario/bma-review/bma-review-89/bma-review-89-estreia-do-ibs-e-cbs-papel-estrategico-das-obrigacoes-acessorias/pdf ; https://www.reformatributaria.com/opiniao/o-periodo-de-cura-do-art-348-3o-e-4o-da-lc-214-2025-alcance-temporal-e-limites-interpretativos/
  - Três artigos complementares citados pelas fontes secundárias, NÃO CONFIRMADOS no texto da LC 214 (Planalto inacessível, ver Limitações): art. 343 (alíquota estadual do IBS de 0,1% em 2026), art. 346 (alíquota da CBS de 0,9% em 2026) e art. 348 §1º (dispensa de recolhimento se cumpridas as obrigações acessórias). Não são versões conflitantes do mesmo dado; são três dispositivos diferentes.
  - Não-encontrado: data de início do IBS/CBS no XSD como obrigatório. Os campos do grupo IBSCBS aparecem como 0-1 no leiaute v1.01 (ocorrência), com obrigatoriedade condicionada a cClassTrib/Simples (NT 009 §2.16).
- Simples Nacional, NFS-e nacional (FATO secundário): Resolução CGSN nº 191/2026 torna a NFS-e padrão nacional obrigatória para ME e EPP optantes a partir de 01/11/2026, altera a Res. CGSN 140/2018 e revoga a Res. CGSN 189/2026 (que previa 01/09/2026). Data de publicação diverge: 04/08/2026 (Fenacon, COAD) e 10/08/2026 (CRCMA). Fontes: https://fenacon.org.br/reforma-tributaria/simples-nacional-nfs-e-nacional-sera-obrigatoria-para-me-e-epp-a-partir-de-1o-de-novembro-de-2026/ ; https://coad.com.br/home/noticias-detalhe/139271/nfs-e-nacional-sera-obrigatoria-para-optantes-do-simples-nacional-a-partir-de-1-11-2026 ; https://crcma.org.br/noticias/simples-nacional-me-e-epp-terao-nfs-e-padrao-nacional-obrigatoria-a-partir-de-novembro
  - Campos IBS/CBS para SN: obrigatórios a partir de 01/01/2027 (secundário; coincide com Anexo I "a partir de 2027").
  - Opção de regime SN (IBS/CBS pelo SN ou regime regular) a partir de 2027, com opção "entre 1º e 30/09/2026" (secundário, Poder360: https://www.poder360.com.br/poder-economia/reforma-tributaria-saiba-o-que-muda-para-o-mei/). NÃO CONFIRMADO em texto oficial.
- MEI, leitura do primário (FATO + INFERÊNCIA): o Anexo I v1.01 (I336) diz "Para optantes dos Simples Nacional, os grupos IBSCBS só serão obrigatórios a partir de 2027". A NT 009 §2.4 (p.8) define opSimpNac com "2 - Optante - Microempreendedor Individual (MEI)" e "3 - Optante - Microempresa ou Empresa de Pequeno Porte (ME/EPP)". INFERÊNCIA: como o próprio domínio do leiaute classifica o MEI como optante do Simples (código 2), a regra de 2027 do I336 cobre o MEI. A NT não diz isso em texto. Confirmar com contador.
- MEI (FATO secundário com conflito): IBS/CBS exigidos na nota do MEI a partir de 01/01/2027 (Meu Contador Online: https://www.meucontadoronline.com.br/blog/nota-fiscal-do-mei-ibs-cbs-2027/) ou 04/01/2027 (Poder360 e Bling: https://ajuda.bling.com.br/hc/pt-br/articles/35704385019031-Quem-pode-emitir-notas-fiscais-com-a-Reforma-Tribut%C3%A1ria). O MEI não faz opção de regime, segue no SIMEI (Poder360). Em 2026 o MEI não precisa preencher os campos (secundário).
  - Não encontrado: dispensa de IBS/CBS para MEI na NT 009 ou no Anexo I (procurei "opSimpNac 2", "MEI", "E1302" no Anexo I: só E1302, vLiq, para MEI). Atualização de 26/10/2026: a E1302 trata do grupo valores da NFS-e que a Sefin gera, não da DPS (FATO, Anexo I de Produção Restrita de 20/10/2026, aba RN DPS_NFS-e 1.01, linha Excel 49: B49 = `NFSe/infNFSe/`, C49 = `valores`; DA-1.22 do plano, decisão autônoma do CTO, a ratificar pelo líder). Regra de MEI em NFS-e: NÃO ENCONTRADO no texto primário lido.
- Caveat: o Anexo I v1.01 traz E1302 (MEI só vLiq) (pesquisa anterior, FATO). Conferido no Anexo I de 20/10/2026 (lido na NL-1.11): a E1302 está na linha Excel 49, sobre o grupo valores da NFS-e (`NFSe/infNFSe/valores`), não sobre a DPS; não há regra local (DA-1.22, decisão autônoma do CTO, a ratificar pelo líder).

## Q3 ISS municipal e IBS/CBS na mesma DPS (2026-2027)

- FATO (NT 009 p.8, §2.3, lido na imagem da página): vBC do IBS/CBS deduz vISSQN. Fórmula: vBC = vServ − descIncond − (vCalcAjusteBCIBSCBS ou vCalcAjusteBCLocImoveis) − vISSQN − vPIS − vCOFINS (até 2026); vBC = vServ − descIncond − (vCalcAjusteBCIBSCBS ou vCalcAjusteBCLocImoveis) − vISSQN (de 2027 até 2032). Ou seja, PIS e COFINS saem da fórmula em 2027; o ISSQN continua sendo deduzido até 2032.
- FATO (NT 009 p.18, §2.13): mesma lógica com vAjusteBCIBSCBSComExt ("a partir de 2027 até 2032") para operações de comércio exterior.
- FATO (Anexo I v1.01, aba RN): regras de ISSQN na DPS seguem em vigor junto com IBS/CBS: tribMun/pAliq ≤ 5% (E0595), tpImunidade (E0592), tribFed não permitido para emitente CPF (E0675), e xLocIncid (E1327 para NFS-e).
- FATO (NT 009 §2.10, p.13-14): para subitem 17.05, o município de incidência do ISSQN é definido pelo endereço do tomador ou do destinatário; a DPS é rejeitada se o IBGE do endereço divergir do cadastro CNPJ/CPF do tomador/destinatário.
- FATO (secundário): ISS e ICMS reduzidos de 2029 a 2032 e extintos em 2033 (art. 129 ADCT, EC 132/2023), com coexistência 2029-2032 (fontes contábeis, não verificadas no texto). Primário NÃO LIDO (Planalto inacessível).
- INFERÊNCIA: o leiaute de 2026-2027 mantém os campos de ISSQN na mesma DPS, com dedução do vISSQN na base do IBS/CBS até 2032 (fórmulas acima). A leitura "ISS segue na DPS até 2032" é inferência a partir dessa fórmula, não texto normativo lido.
- NÃO ENCONTRADO: texto normativo que diga literalmente "ISSQN continua na DPS até 2032". Procurei: NT 009 (sumário e §2.3, §2.13), NT 007 (esclarecimentos), Anexo I RN (campos tribMun), secundárias.
- Secundário (não verificado, resultado de busca sem URL individual confirmada): em 2026 IBS/CBS aparecem destacados sem efeito financeiro; cobrança efetiva em 2027 (CBS integral; IBS gradual). Confirmar com a LC 214 (art. 348 e transição) antes de usar.

## Q4 Tabelas que o app precisa embarcar ou atualizar

| Tabela | Onde está (FATO) | Formato | Versão/data (FATO) | Frequência observada |
|---|---|---|---|---|
| CST e cClassTrib | NT 009 p.22: https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/classificacoes-tributarias (online e interativa); espelho https://dfe-portal.svrs.rs.gov.br/DFE/ClassificacaoTributaria | Consulta online; download NÃO CONFIRMADO | Informe Técnico 2025.002 v1.60, publicado 23/06/2026 (NT 009 p.21) | Secundário: v1.30 (19/11/2025), v1.40 (jan/2026), v1.50 (abr/2026, homologação 04/05/2026), v1.60 (jun/2026). Ver https://documentacao.senior.com.br/exigenciaslegais/noticias/federal/2026/2026-04-16-df-e-informe-tecnico-no-2025-002-v-1-50-tabelas-codigo-de-classificacao-tributaria-cst-e-classificacao-do-credito-presum |
| cIndOp | Anexo VII, xlsx: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovii-indop_ibscbs_v1-03-00-nt009.xlsx (listado na biblioteca); NT 009 cita v1.02.01 | xlsx | CONFLITO: NT 007 = v1.01.00 (07/02/2026); NT 009 texto = v1.02.01; página da biblioteca = v1.03.00 | 3 versões em 2026 |
| cNBS / Anexo B | Anexo I v1.01 cNBS "conforme Anexo B" (dump linha 678) e E0316 cita "ANEXO_B-NBS2-LISTA_SERVICO_NACIONAL-SNNFSe" | Não identificado (xlsx não extraído) | NÃO ENCONTRADO URL de download | NÃO ENCONTRADO |
| cTribNac (lista de serviço nacional) | Anexo I, aba MUN.INCID_INFO.SERV. (dump) | xlsx dentro do Anexo I | Anexo I v1.01 (09/02/2026): cTribNac N6 1-1 | Segue o Anexo I |
| Correlação item / NBS / cIndOp / cClassTrib | Anexo VIII v1.01.00: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexoviii-correlacaoitemnbsindopcclasstrib_ibscbs_v1-01-00.xlsx | xlsx | v1.01.00 (v1.00.00 com NT 006) | Página: "trabalho inicial, sem regras de negócio" |
| Municípios IBGE | Campos "Tabela do IBGE" (cMun N7; E0920, E0238 valida) | NÃO ENCONTRADO URL consultado |: |: |
| Calculadora (motor) | NT 009 p.22-23: online https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/regime-geral ; offline (código aberto, para ERP via API): https://consumo.tributos.gov.br/servico/calcular-tributos-consumo/calculadora/calculadora-offline | código aberto (offline) | NT 009 (11/09/2026) |: |
| Anexo VI (leiaute RN IBSCBS) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/anexovi-leiautesrn_rtc_ibscbs-v1-04-01-nt009.xlsx | xlsx | v1.04.01 (NT 009; NT 009 texto também cita "v1.04.00") | Anexo do leiaute |

- Observações FATO: a NT 009 §2.16 diz que o "ind_gIBSCBS" (da tabela CST e cClassTrib) decide se o grupo gIBSCBS é exigido; e que a Calculadora traz a memória de cálculo com a base legal (p.22-23).
- Observação de tabela (FATO, Anexo I RN): E0958, E0964, E0969, E0971 e E0972 dependem de atributos do cClassTrib (validoParaSiglaDfeInformado, exigeGrupoTributacaoRegular, permiteDiferimento), que vêm da tabela/Calculadora, não do leiaute.
- Acesso por API (secundário, não verificado, terceiro): Tecnospeed descreve API SVRS para cClassTrib (cff.svrs.rs.gov.br), com mTLS por certificado ICP-Brasil: https://blog.tecnospeed.com.br/portal-conformidade-facil-nova-plataforma/ .
- Atualização: a frequência do cClassTrib, por Informe Técnico, foi de quatro versões em 2026 (secundário e NT 009). Recomendação técnica do agente: versionar a tabela e registrar a versão do Informe no registro de cada DPS (INFERÊNCIA).
- NÃO ENCONTRADO: a tabela IBGE com URL de download, o formato de download da tabela cClassTrib no portal consumo.tributos.gov.br e o arquivo oficial do Anexo B (NBS). Procurei: biblioteca gov.br (página documentação-tecnica e RTC), busca web "Anexo B NBS2 lista serviço nacional download", página de códigos de tributação nacional (HTTP 404).

## Q5 Outras mudanças fiscais 2026 na NFS-e

- CNPJ alfanumérico (FATO): implantação "a partir do dia 31 de julho de 2026"; formatos numérico e alfanumérico coexistem e ambos são válidos (notícia Receita/EBC de 03/07/2026): https://agenciagov.ebc.com.br/noticias/202607/implantacao-do-cnpj-alfanumerico-ocorrera-a-partir-de-31-de-julho ; NT 009 §2.1 (p.3): todos os campos CNPJ passam de N para C "a ser implementado a partir de julho de 2026".
  - A notícia não cita NFS-e nem NF-e. Secundário: NT 2026.004 da NF-e (não lida).
  - INFERÊNCIA: tratar o campo CNPJ como texto de 14 caracteres alfanuméricos (o tipo passa de N para C). A regra de DV para o formato alfanumérico NÃO foi verificada nas notas lidas.
- DANFSe (FATO, NT 008 v1.02, 14/07/2026, p.5-6, lida no PDF): https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nt-008-se-cgnfse-danfse-20260714-v1-02.pdf . "a API de geração do DANFSe (https://adn.nfse.gov.br/danfse/docs/index.html) será sobrestada (suspensa) na data de 03 de agosto de 2026". Versão 1.01 (30/06/2026) já alterara a data de suspensão. DANFSe de NFS-e gerada em tpAmb=2 deve conter "NFS-e SEM VALIDADE JURÍDICA". Novos fatos geradores terão nota técnica específica de DANFSe.
  - INFERÊNCIA: o PDF do DANFSe passa a ser gerado localmente a partir do XML (NT 008 §2.1.10 lista IBS/CBS: CST/cClassTrib, indicador de operação/IBGE do município de incidência, exclusões e reduções da base, base após exclusões).
  - Modelo DANFSe v2.0: NÃO CONFIRMADO. A NT 008 v1.02 §2.2.4 (p.12) diz apenas "Deverá ser utilizado o modelo de DANFSe conforme Anexo I" da própria NT; o número de versão do modelo não aparece nas páginas 1-13 lidas (o Anexo I da NT não foi localizado; não varri as páginas finais). A pesquisa anterior afirmou "v2.0": não reproduzir sem ver o Anexo I.
- Simples Nacional (secundário): NFS-e nacional obrigatória a partir de 01/11/2026 (CGSN 191/2026), ver Q2.
- MEI (secundário): sem resposta primária. "Padrão nacional obrigatório para MEI prestador desde 2023" (investnews, fonte única, não confirmada): NÃO CONFIRMADO.
- Outras mudanças lidas no primário (NT 007 e NT 009):
  - Domínio "Desconhecido (tipo não informado na nota de origem)" de comExt: não será admitido nas NFS-e compartilhadas com o ADN a partir de 01/01/2027 (NT 009 §2.12, p.16).
  - Endereço do tomador/destinatário validado no cadastro CNPJ/CPF; DPS rejeitada por divergência de IBGE (NT 009 §2.10, p.13-14). Este item conflita com a leitura anterior (pesquisa_nfse_tomador.md, INFERÊNCIA de que a Sefin só valida existência): registrar.
  - indDest e dest migram para infDPS (NT 009 §2.11).
  - opSimpNac 4 - Optante Pendente; cAtvSN (NT 009 §2.4).
  - Numeração com "pulos" (NT 007 p.9).
  - indFinal reinserido (NT 009 §2.5); E0942 proíbe docFiscalOutro com dtCompDoc após 31/12/2025.
  - E0675: PIS/COFINS (tribFed) não pode ser preenchido por emitente CPF (Anexo I RN).
  - Todo CPF e todo CNPJ podem emitir os novos fatos geradores no emissor nacional, mesmo com município que não adere (NT 007 p.8).

## Q6 Responsabilidades e penalidades (só citação de norma, sem parecer)

- Texto primário NÃO LIDO (Planalto inacessível, ECONNRESET em 7 tentativas). As citações abaixo vêm de cópia de terceiro (https://modeloinicial.com.br/lei/LCP-214-2025/infracoes-penalidades-relativas-ao-ibs-a-cbs-@__I_VII_IV, que diz reproduzir o Planalto, não conferida) e de fontes secundárias. Confirmar no DOU/Planalto antes de qualquer uso.
- Art. 341-A (cópia de terceiro): infração é qualquer ação ou omissão, ainda que involuntária, que descumpra obrigação principal ou acessória.
- Art. 341-C (cópia de terceiro): UPF de R$ 200,00 reajustada anualmente pelo IPCA.
- Art. 341-G (cópia de terceiro), incisos de documento fiscal: IV, entrega de documento em desacordo com a lei: 20 UPF por período (ou 30 UPF por período e por intimação); V, software que suprime valores: 100 UPF por equipamento; VI, desenvolver ou fornecer esse software a terceiros: 150 UPF por equipamento; XI, operar com bem ou serviço desacobertado de documento fiscal: 100% do tributo de referência; XIII, documento não idôneo: 66%; XVI, não emitir documento de aquisição no prazo: 100%.
- Tributo de referência (§5º do art. 341-G, cópia de terceiro): a partir de 2027 para CBS e de 2033 para IBS, base = alíquota de referência x valor da operação; em 2026: 6% (CBS) e 12% (IBS). Secundário.
- Período de cura (secundário): art. 348 §§3º e 4º, incluídos pela LC 227/2026, criam período de cura para operações de 2026 (fonte: https://www.reformatributaria.com/opiniao/o-periodo-de-cura-do-art-348-3o-e-4o-da-lc-214-2025-alcance-temporal-e-limites-interpretativos/).
- Saneamento (secundário, resultado de busca sem URL individual confirmada): LC 227/2026 prevê intimação para saneamento em 60 dias para infrações do art. 341-G, com extinção da penalidade se corrigido. Confirmar no texto da LC 227.
- Programa de conformidade RFB/CGIBS (secundário): https://tagdlaw.com.br/receita-federal-e-cgibs-preparam-programa-de-conformidade-para-evitar-multas-na-emissao-de-notas-com-cbs-e-ibs/
- Responsabilidade do emitente e do software por nota com IBS/CBS errado: NÃO ENCONTRADO em texto primário lido (procurei: LC 214 cópia de terceiro, "responsabilidade solidária", "emitente", "software", "documento fiscal").
- Multa municipal de ISSQN por nota errada: NÃO ENCONTRADO (fora do escopo nacional, depende de lei municipal).

## Limitações desta pesquisa (declaradas)

- Contagem de chamadas: 19 WebFetch (incluindo 6 falhas planalto.gov.br e 1 falha nfe.fazenda.gov.br "too many redirects" e 1 HTTP 404 do NBS) + 14 WebSearch = 33 chamadas. Teto 35: sobram 2 se busca contar, 16 se não contar busca.
- Canal de extração de xlsx: as instruções citam o MCP desktop-commander (start_process), mas a sonda de 25/10/2026 retornou "No such tool available: start_process". Não há ferramenta de shell nesta sessão. Canal testado e indisponível. Xlsx de Anexo I 20261020 e demais anexos não extraídos.
- Resposta primária que NÃO EXISTE no que foi lido: se prestador fora do Simples (Lucro Presumido) já deve enviar o grupo IBSCBS na DPS em 25/10/2026. NT 007 e NT 009 dizem que o cronograma "será publicado no portal". Fontes secundárias dizem 01/10/2026 para NFS-e do ISS (Ato Conjunto 4/2026, não lido). Decisão para contador.

- Planalto (lcp214, lcp227, emc132): ECONNRESET em todas as tentativas (7 no total). Texto da LC 214/2025, LC 227/2026 e EC 132/2023 NÃO LIDO em primário.
- Ato Conjunto RFB/CGIBS nº 4/2026 (DOU), Resolução CGSN 191/2026 (CGSN): só fontes secundárias. Não encontrei URL oficial.
- Xlsx (Anexo I 20261020, Anexo VI v1.04.01, Anexo VII, Anexo VIII) não extraídos: WebFetch devolve binário ZIP e não há shell (sem Bash) para descompactar. Pela regra L-51 não instalei nada.
- PDFs lidos via Read (NT 007, NT 008, NT 009 partes). NT 004 e NT 005 NÃO lidas.
- A página da Receita (CNPJ alfanumérico) retornou "conteúdo restrito". Usei a notícia EBC/Agência Gov.
- Fetches usados: cerca de 23 de 35.
