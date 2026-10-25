# Pesquisa de fatos: NFS-e Nacional (agente backend-engineer/haiku, 25/10/2026)

Transcrição fiel dos fatos com fonte. INFERÊNCIA marcada como tal.

## Q1 Leiaute/XSD
- FATO: Anexo I (DPS) v1.01 de 09/02/2026: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_i-sefin_adn-dps_nfse-snnfse-v1-01-20260209.xlsx ; XSD v1.01: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/nfse-esquemas_xsd-v1-01-20260209.zip
- FATO: Produção restrita: Anexo I v1.01 20261020 https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx ; XSD https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/esquemas_prod_restrita_20261020.zip
- FATO: NT 009 v1.01 (11/09/2026) https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nota-tecnica-009-se-cgnfs-e-v-1-01.pdf ; NT 007 v1.0 (07/02/2026) https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nt-007-se-cgnfse-v1-0.pdf ; NT 008 DANFSe v1.02 (14/07/2026, cópia Fenacon) https://fenacon.org.br/wp-content/uploads/2026/07/nt-008-se-cgnfse-danfse-20260714-v1-02.pdf

## Q2 Endpoints (manual v1.2 out/2025: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf)
- POST /nfse; GET /nfse/{chaveAcesso}; GET|HEAD /dps/{id}; POST|GET /nfse/{chaveAcesso}/eventos[/{tipoEvento}[/{numSeqEvento}]]; GET /parametros_municipais/{cMun}/convenio | /{cMun}/{cServ} | /{cMun}/{CPF/CNPJ}
- ADN contribuinte: GET /DFe/{NSU}, GET /NFSe/{Chave}/Eventos (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf)
- Cancelamento/substituição = eventos via POST /nfse/{chave}/eventos; substituição também via POST /nfse com chave a substituir.
- Swagger produção restrita deu 403 (não lido). Base path da API NÃO CONFIRMADA.

## Q3 DANFSe
- FATO: NT 008 v1.02 item 1: API de geração do DANFSe (adn.nfse.gov.br/danfse) "será sobrestada (suspensa) na data de 03 de agosto de 2026". Modelo DANFSe v2.0 (Anexo I da NT). tpAmb=2 exige "NFS-e SEM VALIDADE JURÍDICA".
- INFERÊNCIA: o PDF precisa ser gerado localmente a partir do XML, conforme NT 008.

## Q4 Cancelamento (Anexo II https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_ii-sefin_adn-pedregevt_evt-snnfse-v1-01-20260122.xlsx)
- 101101 cancelamento (cMotivo 1 erro emissão, 2 serviço não prestado, 9 outros; xMotivo 15-255 chars); 105102 substituição; 101103 análise fiscal; 105104/105105 deferimento/indeferimento. Id = "PRE"+chave(50)+código(6).
- Prazo: parametrizado por município (E0822); exemplo municipal de 60 dias.

## Q5 Pessoa física
- FATO: Id da DPS aceita tpInsc=1 (CPF). E0099: CPF precisa de domicílio no município emissor (CNC). Certificado com CPF aceito (E1209). E0815 casa CPF do autor com certificado. MEI = opSimpNac=2 (E1302: só vLiq).
- NÃO ENCONTRADO: regra oficial de quem pode emitir como PF.

## Q6 Teste sem certificado
- FATO: transmissão exige raiz ICP-Brasil, validade, cadeia, LCR, não revogado (E1203-E1208). NÃO ENCONTRADO teste oficial sem certificado.

## Q7 Numeração e lote
- FATO: Id = "DPS"+cLocEmi(7)+tpInsc(1)+inscrição(14)+série(5)+nDPS(15) = 45 posições. Série 00001-49999 = aplicativo próprio. E0014 unicidade série+número+município+inscrição.
- FATO: não há lote no leiaute; POST síncrono, uma DPS por chamada (INFERÊNCIA forte).

## Q8 Reforma tributária
- FATO: grupo IBSCBS na DPS; NT 009: CNPJ passa de N para C (alfanumérico) a partir de jul/2026, já no Anexo I 20261020.
- Obrigatoriedade (fontes secundárias, conflitantes): DF 01/10/2026 (COAD); Simples ME/EPP 01/11/2026 (Fenacon, CGSN 191/2026); cronograma até 01/01/2027 (Inventti).

## Q9 Libs abertas (só para aprender técnica)
- gildonei/nfse-nacional (PHP, MIT); gsse/nfse-core-client (PHP, sem licença); CristianoMZn/danfse-nacional (PHP); ACBr (Delphi, licença não confirmada). Nada em C++/Python/Java/Go/Node encontrado.

## Surpresas contra o prompt-emissor-nfse-nacional.md
DANFSe por API suspenso; CPF aceito como prestador; CNPJ alfanumérico; base path não confirmado; gzip não confirmado em texto oficial (base64 sim).
