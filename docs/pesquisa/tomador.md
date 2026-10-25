# Pesquisa de fatos: cadastro de tomador na NFS-e Nacional (25/10/2026)

Ordem: só coleta de fatos. FATO = com URL. INFERÊNCIA marcada. NÃO ENCONTRADO = com o que foi procurado.

## Q1 Consulta cadastral de tomador na API do SN NFS-e
- FATO: texto do manual v1.2 (out/2025) extraído com pdftotext de https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf (cópia local: <cópia local não versionada>). Seções 1.1 a 1.6. Endpoints: GET /parametros_municipais/{cMun}/convenio; GET /parametros_municipais/{cMun}/{codigoServico}; GET /parametros_municipais/{cMun}/{CPF/CNPJ} (descrito DUAS vezes: "retenções a que um contribuinte tem o dever de recolher" e, idêntico path, "benefícios municipais"); POST /nfse; GET /nfse/{chaveAcesso}; GET /dps/{id} e HEAD /dps/{id}; POST /nfse/{chaveAcesso}/eventos; GET /nfse/{chaveAcesso}/eventos[/{tipoEvento}[/{numSeqEvento}]].
- FATO: o manual NÃO lista endpoint de consulta de cadastro (nome, endereço, IM, contato) de tomador/contribuinte por CPF/CNPJ. O único endpoint com CPF/CNPJ como chave devolve retenções/benefícios municipais (parâmetros), não identidade.
- FATO: GET /dps/{id} devolve só a chave de acesso, e só se o certificado do solicitante for Prestador, Tomador ou Intermediário da NFS-e (manual 1.4.1-1.4.2). HEAD /dps/{id} informa só se a NFS-e foi gerada, sem chave.
- FATO: manual 1.3.1 lista como insumos do processamento da DPS "Informações do cadastro nacional de contribuintes NFS-e" e "Informações das integrações com os cadastros CNPJ, CPF e Simples Nacional" (manual 1.3.2 a, linhas 85-86). Ou seja, a Sefin consulta essas bases para validar a DPS, mas o manual não expõe endpoint para o contribuinte consultá-las.
- FATO: o histórico de versões do manual v1.2 traz só a linha "1.0 17/03/2025" (o próprio documento não registra a v1.2 no histórico extraído).
- INFERÊNCIA: a resposta de "dados cadastrais" pela API é NÃO no manual v1.2; a verificação por CPF/CNPJ via API ocorre dentro da validação da DPS (E-codes) e não é um recurso consultável.
- NÃO ENCONTRADO (procurei: versão 2026 posterior dos manuais da API e do ADN; não achei versão mais nova na biblioteca lida).

## Q1 complemento: ADN contribuinte (manual 1.0, 12/02/2026)
- FATO: manual ADN https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf (texto em <cópia local não versionada>) lista só 2 métodos: GET /DFe/{NSU} (documentos fiscais de serviço por NSU, emitente/tomador/intermediário) e GET /NFSe/{ChaveAcesso}/Eventos. Nenhum endpoint de cadastro. Diz que consultas de distribuição usam certificado de mesmo CNPJ raiz.
- NÃO ENCONTRADO: endpoint de cadastro de tomador em nenhum dos dois manuais oficiais da API (procurei: "cadastr", "tomador", "endere", "atualiz" nos textos extraídos; e os endpoints listados).
- Caveat: o manual v1.2 e o ADN são os de out/2025 e fev/2026. Não confirmei se há versão 2026 posterior do manual da API.

## Q6 Campos do tomador na DPS (Anexo I v1.01, 09/02/2026)
Fonte: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_i-sefin_adn-dps_nfse-snnfse-v1-01-20260209.xlsx (dump: <cópia local não versionada>; leitor: <cópia local não versionada>). Aba "LEIAUTE DPS_NFS-e" (linhas 144-166 da aba) e aba "RN DPS_NFS-e" para as regras.
- FATO, ocorrência (coluna F da aba Leiaute):
  - toma (grupo): 0-1 (opcional; tomador pode não ser identificado).
  - CNPJ (14), CPF (11), NIF (40), cNaoNIF: escolha (CE). Ocorrência 1-1 para cada item do grupo de escolha; CAEPF 0-1; IM 0-1 (15 chars).
  - xNome: 1-1 (1-150 chars).
  - end (grupo): 0-1. Dentro dele: endNac 1-1 (CG, escolha com endExt); cMun 1-1 (7); CEP 1-1 (8); xLgr 1-1 (1-255); nro 1-1 (1-60); xBairro 1-1 (1-60); xCpl 0-1 (1-156). endExt (exterior) tem cPais, cEndPost, xCidade, xEstProvReg.
  - fone 0-1 (6-20, DDD+número); email 0-1 (1-80).
- FATO, regras da aba RN (coluna K = executada pelos emissores públicos SEFIN/WEB/APP, V=executa, X=não executa):
  - E0187 (K=V): grupo toma obrigatório se cIndOp ∈ {030102, 050102, 100101, 100301, 100501, 030103, 050103, 100102, 100201, 100302, 100401, 100502, 100601}.
  - E0188 (K=V): CNPJ inválido (DV). E0206 (K=V): CPF inválido (DV).
  - E0190 (K=V, L=V, M=X): "CNPJ do tomador não existe no cadastro CNPJ na data de competência" (rejeição, nível 2).
  - E0207 (K=V, L=V, M=X): "CPF do tomador inexistente no cadastro CPF na data de competência" (rejeição, nível 2).
  - E0204 (K=V): retenção pelo tomador (tpRetISSQN=2) exige CNPJ ou CPF.
  - E0233 (K=V): NIF do tomador exige xNome.
  - E0234 (K=V, L=V, M=V, N=V): endereço do tomador (grupo end) obrigatório quando (a) subitem da lista cujo ISSQN incide no local do tomador (planilha MUN.INCID_INFO.SERV.) ou (b) cIndOp da lista acima.
  - E0236 (K=V): se tpEmit=2 (o tomador emite), endNac do tomador NÃO deve ser informado.
  - E0237 (K=V, M=V): se tpRetISSQN=2, endNac do tomador obrigatório, exceto se tpEmit=2.
  - E0235 (K=X, M=V): se tpEmit=1 e tomador identificado por CNPJ, endNac obrigatório. Na coluna K (Sefin/WEB/APP) está X, na M (ADN) está V. Ou seja, a obrigatoriedade deste endNac é de regra do ADN, não dos emissores públicos (leitura literal das flags).
  - E0238 (K=V): cMun do endereço deve existir na tabela IBGE. E0240 (K=V): CEP existe e pertence ao município.
  - E0246 (K=V): cPais deve existir e ser diferente de BR.
  - E0202 (K=V): prestador não pode ser igual ao tomador (CNPJ completo).
  - E0212 (K=V, L=X): tpEmit=2, CPF do emitente tomador deve ter domicílio no município emissor (cadastro CPF e CNC). E0194: equivalente para CNPJ.
  - E0228-E0232 (K=V): regras de IM do tomador quando tpEmit=2 (IM exigido/proibido/inativo conforme CNC NFS-e).
  - E0667 (K=V): CPF do tomador precisa estar parametrizado para retenção no município de incidência.
- FATO, contraste no prestador: E0121 (K=V): se tpEmit=1 (prestador emite), xNome do prestador NÃO deve ser informado. E0129/E0125: endereço do prestador obrigatório quando o emitente é tomador/intermediário. Ou seja, quando o prestador emite, o leiaute pressupõe que o nome dele vem da base do governo, não do XML.
- FATO: nas regras de tomador não há NENHUMA regra que mande a Sefin devolver nome/endereço do tomador. As mensagens de erro (E0190, E0207, etc.) informam só existência/validade.
- INFERÊNCIA: o leiaute não prevê preenchimento do tomador pelo governo; o emissor é quem envia xNome, e o governo só valida existência (CNPJ/CPF) e formato/IBGE/CEP.
- INFERÊNCIA: a leitura literal de K=X em E0235 (endNac do tomador CNPJ) sugere que o emissor público não exige endereço nacional do tomador identificado por CNPJ, salvo E0234/E0237. Confirmar com a NT ou com o Emissor Web antes de depender disso.
- NÃO ENCONTRADO: fone/email do tomador sem regra de validação (além do leiaute 0-1).

## Q2 Emissor Web Nacional: preenchimento do tomador por CPF/CNPJ
- NÃO ENCONTRADO em fonte oficial. Procurei: biblioteca gov.br (lista de documentos: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica , sem guia do usuário do Emissor Web); página de acesso https://www.nfse.gov.br/EmissorNacional (só login, certificado, gov.br; "Portal de Gestão NFS-e - Contribuinte | Versão 1.6.0.0"; sem texto sobre tomador); Londrina NT 2025/001 (repositorio.londrina.pr.gov.br, arquivo em <cópia local não versionada>; sem menção a consulta RFB para tomador).
- FATO (fonte municipal, NÃO oficial federal): Manual do Usuário "NFS-e" da IMA para Campinas, 2026 (https://portal-adm.campinas.sp.gov.br//sites/default/files/anexos_avulsos/Manual%20de%20Emiss%C3%A3o%20de%20NFSe%20RTC%20-%20altera%C3%A7%C3%B5es%20V2_14jul2026.pdf ; texto em <cópia local não versionada>, p. 6-7 e 24): "Ao informar um tomador identificado, será necessário o preenchimento dos seus dados cadastrais, incluindo CPF/CNPJ, Nome/Razão Social e endereço... Caso o tomador já esteja previamente cadastrado no sistema, suas informações serão recuperadas automaticamente". Ou seja, a recuperação é de cadastro LOCAL do próprio sistema, não consulta a Receita. O mesmo manual diz que os dados do EMITENTE são preenchidos automaticamente a partir do contribuinte autenticado.
- FATO: Londrina NT 2025/001 §2.3.3.1: acesso ao Portal de Gestão do Contribuinte por "cadastramento de usuário autorizado (responsável legal), certificado digital ou, para o MEI, pelo gov.br".
- INFERÊNCIA: o Emissor Web nacional, pelo que está documentado, não consulta Receita para tomador. Não confirmado por teste; a tela real não foi observada.

## Q3 Fontes de CNPJ
- Serpro Consulta CNPJ (loja.serpro.gov.br/consultacnpj): HTTP 403 ao WebFetch, página NÃO lida. Fontes secundárias (WebSearch, não oficiais): produto é pago; preço por faixa, sem valor oficial encontrado; contratação exige e-CNPJ (gov.br: https://www.gov.br/pt-br/servicos/obter-solucao-de-consulta-de-dados-de-cadastro-de-pessoa-fisica-cpf lista e-CNPJ para CPF; para CNPJ NÃO CONFIRMADO). Sem termos de uso lidos. NÃO ENCONTRADO: preço atual; termos; se PF pode contratar.
- Receita Federal dados abertos CNPJ: https://www.gov.br/receitafederal/pt-br/assuntos/orientacao-tributaria/cadastros/consultas/dados-publicos-cnpj retorna "Conteúdo Restrito" ao WebFetch; https://dadosabertos.rfb.gov.br/CNPJ/ deu ECONNREFUSED; dados.gov.br sem texto útil. Único trecho legal visto: licença "Creative Commons Atribuição-SemDerivações 3.0" citada em rodapé do gov.br (site em geral, não dos dados CNPJ), https://creativecommons.org/licenses/by-nd/3.0/deed.pt_BR. Periodicidade "mensal" só via fonte municipal secundária (dados.mogidascruzes.sp.gov.br). NÃO ENCONTRADO: licença específica do conjunto CNPJ.
- Terceiros (não oficiais):
  - ReceitaWS (receitaws.com.br): página WebFetch só com título. Via docs de terceiros: plano gratuito 3 consultas/min. NÃO LIDO diretamente.
  - CNPJ.ws (docs.cnpj.ws/referencia-de-api/api-publica/limitacoes, FATO lido): API pública 3 requisições/min por IP; 360 erros 429 por hora = bloqueio de 1 hora; dados sensíveis (endereço completo, contatos, sócios) vêm null/reduzidos na API pública; "observacao" quando há omissão a pedido do representante. Sem texto sobre fonte/termos nessa página.
  - CNPJá (cnpja.com): página /subscription/tos retornou 429, NÃO LIDA. Via busca (secundário): API pública gratuita sem cadastro, 5 consultas/min por IP; plano gratuito com cadastro 50 créditos/mês e 10/min; termos dizem que a CNPJá não é fonte original (dados de Receita Federal e Sefaz). NÃO CONFIRMADO em página oficial.
  - BrasilAPI (brasilapi.com.br/docs): página só "Loading...". Via busca (secundário): fonte Receita; sem limite confirmado. NÃO ENCONTRADO termos oficiais.
- NÃO ENCONTRADO: nenhum termo de uso lido diretamente para ReceitaWS, BrasilAPI ou CNPJá.

## Q4 Consulta de CPF
- FATO: https://www.gov.br/pt-br/servicos/obter-solucao-de-consulta-de-dados-de-cadastro-de-pessoa-fisica-cpf : "API que consulta a base da Receita Federal (RFB) a partir do número do CPF"; retorno = "informações do Cadastro Nacional de Pessoas Físicas com a respectiva situação cadastral perante a RFB". Página NÃO lista nome/endereço como campos.
- FATO: quem pode contratar: "Órgãos e entidades públicas (em todas as esferas)" e pessoa jurídica; "Para contratação, é necessário o certificado digital e-CNPJ da contratante." Pessoa física não aparece como público elegível.
- FATO: contratação pela Loja Serpro; custo "depende da faixa de preços, conforme o seu volume de consumo"; valores não na página. Cancelamento pela Área do Cliente.
- FATO: restrições citadas pela página: só títulos de Portaria MF 457/2016 e Portarias RFB 167/2022 e 220/2022 (conteúdo não aparece). LGPD NÃO citada. Frase: "Esse acesso às informações públicas ajuda conhecer melhor clientes e fornecedores."
- FATO (secundário, notícia Tecnoblog, https://tecnoblog.net/noticias/receita-autoriza-serpro-a-compartilhar-cpf-telefone-e-mais-com-terceiros/): portaria da Receita diz que o compartilhamento com terceiros é lícito à luz da LGPD e que a informação só é liberada após justificativa da consulta. Texto da portaria NÃO lido.
- NÃO ENCONTRADO: fonte oficial pública e gratuita de nome/endereço por CPF. Procurei: gov.br CPF (acima), busca Serpro CPF (só preço antigo e notícias).

## Q5 Atualização de cadastro de terceiro (tomador)
- FATO: manual v1.2 e manual ADN (ver Q1) não têm endpoint de escrita/atualização de cadastro. Únicos POST: /nfse (DPS), /nfse/{chave}/eventos.
- FATO: Londrina NT 2025/001 §2.6.12.2: substituição "não sendo permitido substituir uma NFS-e onde tomador não foi identificado, nem alterar informações dos não emitentes (não pode alterar um tomador já identificado por outro)". Prazo de 35 dias (parametrizado pelo município, nota de Londrina).
- FATO: Londrina NT §2.6.9.2: "Está em implantação a criação de dado complementar ao cadastro nacional da RFB e do Comitê Gestor do IBS que indique a condição de imune... Quando disponível, será feita integração com o Emissor Nacional." Mostra que há planos de dado complementar; não é mecanismo de terceiro.
- FATO (gov.br via Senior, secundário): "o CNC passará a complementar o cadastro CNPJ/CPF", sem descrever canal de atualização por terceiro. Fonte oficial da notícia não localizada.
- FATO: Londrina NT §2.3.3.1: Portal de Gestão acessado por usuário autorizado do próprio contribuinte, certificado ou gov.br (MEI).
- INFERÊNCIA: o único registro de dados do tomador que chega ao governo é o XML da própria DPS/NFS-e emitida pelo prestador (ou o tomador/intermediário, pelo tpEmit 2/3). Não há canal documentado para o prestador atualizar cadastro RFB/CNC de terceiro.
- NÃO ENCONTRADO: endpoint ou serviço de atualização cadastral de terceiro. Procurei: manuais API v1.2 e ADN, Anexo II (eventos: ver abaixo), Londrina NT, busca "CNC contribuinte atualização".

## Q5 complemento: Anexo II (eventos), 16 tipos (FATO)
- Fonte: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_ii-sefin_adn-pedregevt_evt-snnfse-v1-01-20260122.xlsx (dump: <cópia local não versionada>, aba "TIPO EVENTOS DE NFSe").
- FATO: cancelamento (1 01 1 01), cancelamento por substituição (1 05 1 02), análise fiscal (1 01 1 03), deferimento/indeferimento (1 05 1 04/1 05), manifestações de confirmação/rejeição do PRESTADOR, TOMADOR e INTERMEDIÁRIO (2 02-2 04 e 2 05-2 07), confirmação tácita (2 05 2 04), anulação da rejeição (2 05 2 08), cancelamento/bloqueio/desbloqueio por ofício (3 05 ...).
- FATO: nenhum dos 16 tipos altera dados cadastrais de tomador ou de outra pessoa. Eventos de manifestação são de ciência/aceite do próprio autor (o tomador manifesta sobre a própria nota).
- INFERÊNCIA: o único canal de escrita de dados do tomador é a DPS (POST /nfse) que o emitente envia. Alterar tomador exige substituir a nota (cancelamento por substituição + nova NFS-e), sujeito a prazo parametrizado e, segundo Londrina, sem alterar tomador identificado por outro.

## Q3 complemento
- NÃO ENCONTRADO: página oficial gov.br do serviço Serpro de CNPJ e regra de e-CNPJ para CNPJ (procurei: busca "gov.br serviço consulta dados cadastrais pessoa jurídica CNPJ Serpro", só notícias de 2017-2020 e site Serpro).

## Veredito por pergunta (resumo para handback)
- Q1: NÃO ENCONTRADO endpoint de consulta cadastral de tomador por CPF/CNPJ nos manuais oficiais da API (v1.2) e do ADN.
- Q2: NÃO ENCONTRADO em fonte oficial. A recuperação automática do tomador descrita no manual da IMA é de cadastro LOCAL do próprio sistema municipal (Campinas), não de Receita. Não se aplica ao emissor nacional.
- Q3: Serpro não lido (403); RFB não lido (restrito); licença específica do CNPJ aberto NÃO ENCONTRADA; ReceitaWS, BrasilAPI e CNPJá só via fontes secundárias ou páginas sem texto (ver seção Q3).
- Q4: fonte oficial gov.br lida; fonte de nome/endereço por CPF NÃO ENCONTRADA; elegibilidade restrita a órgãos públicos e PJ com e-CNPJ (FATO).
- Q5: NÃO ENCONTRADO canal de atualização de cadastro de terceiro. Anexo II: 16 eventos, nenhum altera tomador. Londrina (municipal, não nacional): "não pode alterar um tomador já identificado por outro".
- Q6: campos e regras do Anexo I v1.01 registrados acima (leituras de flags cruas em E0235: J=1 K=X L=X M=V N=X; E0234: J=2 K=V L=V M=V N=V; E0190: J=2 K=V L=V M=X N=X; E0207: J=2 K=V L=V M=X N=X).
- Nota Q5: regra E9996 (Anexo I, aba RN, linha 157; J=1 K=V L=V M=V N=V): "Se a DPS for emitida pelo tomador ou intermediário (tpEmit = 2 ou 3), então a DPS deve ser rejeitada... Nesta versão da aplicação, não é permitida a emissão de NFS-e pelo tomador ou intermediário." Fecha a via de o tomador emitir/atualizar pela DPS dele.
- Oddidade do manual v1.2: GET /parametros_municipais/{codigoMunicipio}/{CPF/CNPJ} aparece com duas descrições (retenções e benefícios) sob o mesmo path. Relatado, não resolvido.

## Arquivos auxiliares criados no scratchpad (extrações, referenciadas acima)
- <cópia local não versionada> (texto do manual v1.2)
- <cópia local não versionada> (texto do manual ADN)
- <cópia local não versionada> (dump do Anexo I) e <cópia local não versionada> (stderr vazio)
- <cópia local não versionada> (dump do Anexo II) e <cópia local não versionada> (stderr vazio)
- <cópia local não versionada> (leitor de xlsx, executado com python3 -I)
- <cópia local não versionada> (texto da NT Londrina)
- <cópia local não versionada> (texto do manual IMA Campinas)
