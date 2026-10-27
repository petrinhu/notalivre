# Onda 1: pesquisa de leiaute (itens b, c, d e e)

- **Data:** 25/10/2026
- **Autor:** backend-engineer
- **Natureza:** pesquisa de abertura da Onda 1, antes do planejamento (L-22). Sem código de produto.
- **Origem:** `docs/superpowers/plans/2026-10-25-notalivre-plano.md`, linha 240 (pesquisa de abertura da Onda 1).
- **Dados de trabalho (fora do repositório):** `/var/tmp/notalivre-pesquisa-onda1/`

Legenda: **FATO** (com arquivo:linha ou URL), **INFERÊNCIA** (marcada como tal), **NÃO ACHADO** (com a forma de conferência, L-41).

## 0. Fontes e integridade

| Item | Valor |
|---|---|
| ZIP do XSD de Produção Restrita | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/esquemas_prod_restrita_20261020.zip |
| Tamanho do ZIP | 36351 bytes |
| SHA-256 do ZIP | `25fdcecc651a67a216496ab10d7566631e6ed10f721e37d62c7845dadb0ea267` |
| Extração | `zipfile` da stdlib (`python3 -I`), script `/var/tmp/notalivre-pesquisa-onda1/bcde-scripts/extrai.py`, saída em `/var/tmp/notalivre-pesquisa-onda1/xsd-dl/x/` (10 arquivos) |
| Manual da API v1.2 (PDF de 6 páginas) | https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf |
| SHA-256 do PDF do manual | `ac2f36e34ff565cc36d67c5d67415c33cc09f27b2dea006e8da9bcd2ddce5581` (cópia em `/var/tmp/notalivre-pesquisa-onda1/manual-dl/manual.pdf`) |
| Texto do manual | extraído com `pdftotext -layout` (já instalado), em `/var/tmp/notalivre-pesquisa-onda1/manual-dl/manual.txt` |
| Anexo I de 09/02/2026 (v1.01) | `/var/tmp/builds/claude-1000/-home-petrus-IDrive-Documentos-projetos-claudebrain-Projects-nfse/38f29892-da53-4786-aca6-00ba009df597/scratchpad/xlsx_reader/anexo1.txt`, aba 4 "LEIAUTE DPS_NFS-e " (linhas 482 a 898) |
| XLSX oficial do Anexo I de Produção Restrita 20/10/2026 | URL: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx (listado em https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/ em 25/10/2026). Tamanho: 248110 bytes. SHA-256: `72b05981c8882cdd807f3acf901b669465360f9cf1b9d9e235ac4363dacfb2d8`. ZIP válido (`zipfile.testzip()` sem erro, 35 membros). Cópia: `/var/tmp/notalivre-nl111/dl/anexo_i_prodrest_20261020.xlsx`. Leitura por `/var/tmp/notalivre-nl111/scripts/ler_xlsx.py` (stdlib, texto integral das células, sem corte de 600 caracteres), saída em `/var/tmp/notalivre-nl111/xlsx/`. |
| Anexo I de Produção Restrita 20/10/2026 | `/var/tmp/builds/claude-1000/-home-petrus-IDrive-Documentos-projetos-claudebrain-Projects-nfse/38f29892-da53-4786-aca6-00ba009df597/scratchpad/xlsx_reader/anexo1_prodrest.txt`, aba 6 "LEIAUTE DPS_NFS-e 1.01" (linhas 537 a 940) e aba 7 "RN DPS_NFS-e 1.01" (linhas 941 a 1603) |

**Limitações dos dados (FATO, conferido):**

- O dump (`dump_xlsx.py`) corta cada linha em 600 caracteres (`line[:600]`) e omite células vazias (`if x`). Na faixa do leiaute, 21 linhas do Anexo de 09/02 e 25 do de 20/10 têm 600 caracteres ou mais (no arquivo inteiro: 86 e 154). Os campos de caminho, nome, elemento, tipo, ocorrência e tamanho vêm antes da descrição, então sobrevivem ao corte; a descrição e as notas podem estar truncadas. Conferi o alinhamento: nenhuma linha de campo tem ocorrência fora do padrão `n-m` (script `valida.py`, zero casos).
- O XLSX do Anexo de 20/10/2026 foi baixado e hasheado na NL-1.11 (25/10/2026); URL, tamanho e SHA-256 estão na tabela acima. Deixou de ser pendência para NL-1.6.
- Convenção de linha: `linha N` nas citações desta pesquisa é o valor da coluna A (`#`), não o número da linha Excel (atributo `r` de `<row>`). No Anexo de 20/10, xDescServ tem `#` 200 na linha Excel 201, e E0031 tem `#` 166 na linha Excel 169. As citações novas da NL-1.11 dão as duas.
- Contagem: a aba RN de 20/10 tem 466 códigos distintos na coluna CÓD. ERRO. O código E1000 aparece só na coluna OBSERVAÇÕES (coluna T), e a contagem do texto integral de todas as células dá 467.
- O XSD de 09/02/2026 não foi baixado. A comparação (c) é só entre os dois Anexos I.

## 1. (b) Estrutura do XSD de Produção Restrita de 20/10/2026

### 1.1 Namespace, arquivos e imports

**FATO.** Todos os `.xsd` de domínio usam `targetNamespace="http://www.sped.fazenda.gov.br/nfse"`: DPS_v1.01.xsd:3, NFSe_v1.01.xsd:4, CNC_v1.00.xsd:3, pedRegEvento_v1.01.xsd:4, evento_v1.01.xsd:4, tiposCnc_v1.00.xsd:4, tiposComplexos_v1.01.xsd:2, tiposSimples_v1.01.xsd:2, tiposEventos_v1.01.xsd:4. O namespace de assinatura é `http://www.w3.org/2000/09/xmldsig#` (xmldsig-core-schema.xsd:22). Os esquemas usam `elementFormDefault="qualified"` e `attributeFormDefault="unqualified"`.

| Arquivo (SHA-256 na seção 5) | Elemento raiz | xs:import | xs:include |
|---|---|---|---|
| DPS_v1.01.xsd | `DPS`, type TCDPS (linha 9) | xmldsig (linha 7) | tiposComplexos_v1.01.xsd (linha 8) |
| NFSe_v1.01.xsd | `NFSe`, type TCNFSe (linha 10) | xmldsig (linha 8) | tiposComplexos_v1.01.xsd (linha 9) |
| CNC_v1.00.xsd | `CNC`, type TCNC (linha 9) | xmldsig (linha 7) | tiposCnc_v1.00.xsd (linha 8) |
| evento_v1.01.xsd | `evento`, type TCEvento (linha 10) | xmldsig (linha 8) | tiposEventos_v1.01.xsd (linha 9) |
| pedRegEvento_v1.01.xsd | `pedRegEvento`, type TCPedRegEvt (linha 10) | xmldsig (linha 8) | tiposEventos_v1.01.xsd (linha 9) |
| tiposComplexos_v1.01.xsd | nenhum | xmldsig (linha 3) | tiposSimples_v1.01.xsd (linha 4) |
| tiposEventos_v1.01.xsd | nenhum | xmldsig (linha 8) | tiposComplexos_v1.01.xsd (linha 9) |
| tiposCnc_v1.00.xsd | nenhum | xmldsig (linha 8) | tiposComplexos_v1.01.xsd (linha 9), tiposSimples_v1.01.xsd (linha 10) |
| tiposSimples_v1.01.xsd | nenhum | nenhum | nenhum |
| xmldsig-core-schema.xsd | nenhum | nenhum | nenhum |

**FATO (cadeia mínima).** Para validar uma DPS bastam quatro arquivos: DPS_v1.01.xsd, tiposComplexos_v1.01.xsd, tiposSimples_v1.01.xsd e xmldsig-core-schema.xsd. Os demais são de NFS-e, eventos e CNC.

**NÃO ACHADO.** O conteúdo do xmldsig-core-schema.xsd não foi comparado com a cópia oficial do W3C. Só o SHA-256 foi registrado.

### 1.2 Versão e atributo Id

- **Versão.** Atributo `versao` do TCDPS, `use="required"`, type TVerNFSe (tiposComplexos_v1.01.xsd:747). TVerNFSe é `1\.00|1\.01` (tiposSimples_v1.01.xsd:3 e pattern na linha 10).
- **Valor da DPS.** INFERÊNCIA: o valor deve ser `1.01`, pelo nome do leiaute "LEIAUTE DPS_NFS-e 1.01" (aba 6 do Anexo de 20/10). O manual extraído não traz esse valor.
- **Id.** TCInfDPS (declarado em tiposComplexos_v1.01.xsd:750) tem o atributo `Id`, `use="required"`, type TSIdDPS (linha 859).
- **Assinatura.** FATO: TCDPS (linha 742) tem `infDPS` (linha 744) e `ds:Signature` opcional (linha 745), como irmão de `infDPS`, fora dele.
- **Referência da assinatura.** INFERÊNCIA: a assinatura envelopada fica em `DPS/Signature` e referencia o Id do infDPS (`URI="#DPS..."`). O XSD não fixa URI, algoritmo nem canonicalização. NÃO ACHADO no manual: o texto extraído cita assinatura só para eventos (manual.txt linhas 167 a 179, referentes a eventos: "assinatura digital do emissor do evento"). Pendente: Nota Técnica ou manual de assinatura do ADN. Sem isso, a assinatura da DPS não fecha em NL-1.6.

### 1.3 TSIdDPS (nome do tipo do Id da DPS)

FATO (tiposSimples_v1.01.xsd):

- Tipo `TSIdDPS`, linha 44. `maxLength` 45 (linha 53). Pattern (linha 54): `DPS[0-9]{7}(1[0-9]{14}|2[0-9A-Z]{14})[0-9]{20}`. `whiteSpace` preserve.
- Documentação (linhas 46 a 49): `"DPS" + Cód.Mun (7) + Tipo de Inscrição Federal (1) + Inscrição Federal (14 - CPF completar com 000 à esquerda) + Série DPS (5) + Núm. DPS (15)`.
- Soma das posições: 3 + 7 + 1 + 14 + 5 + 15 = 45.
- Tipo 1 é CPF e tipo 2 é CNPJ (FATO, anexo1_prodrest.txt:639, aba 6 linha 102: "Tipo de inscrição Federal = 1 / Inscrição Federal = CPF emitente da DPS; Tipo de inscrição Federal = 2 / Inscrição Federal = CNPJ emitente da DPS"). O mesmo texto aparece no Anexo de 09/02 (anexo1.txt:583).
- Pelo pattern, tipo 1 aceita 14 dígitos e tipo 2 aceita 14 caracteres `[0-9A-Z]`. CNPJ alfanumérico entra pelo tipo 2 (INFERÊNCIA pelo pattern; o Anexo de 20/10 diz "CNPJ (pode ser alfanumérico)" no campo prest/CNPJ, anexo1_prodrest.txt:656).

## 2. (c) Diferenças entre o Anexo I de 09/02/2026 e o de 20/10/2026

### 2.1 Método

Leiaute lido só nas faixas de aba indicadas na seção 0. Chave de comparação: caminho XML + nome do campo + repetição. Comparei elemento, tipo, ocorrência e tamanho. Script `compara2.py` em `/var/tmp/notalivre-pesquisa-onda1/bcde-scripts/`.

Como o tipo de campo e a ocorrência mudaram em alguns casos, usei a assinatura (campo, elemento, tipo, tamanho) para separar "movido de caminho" de "novo" e de "removido". O pareamento é ambíguo quando a mesma assinatura se repete (por exemplo, CEP), então os totais são confiáveis, mas o par nominal individual não.

### 2.2 Contagens

| Escopo | encontrados (antigo) | encontrados (novo) | novos | removidos | alterados | movidos de caminho |
|---|---|---|---|---|---|---|
| Aba inteira | 416 | 403 | 32 | 45 | 24 | 41 |
| Só campos DPS (caminho com `DPS/`) | 317 | 303 | 29 | 43 | 17 | 41 |

Lista completa: `/var/tmp/notalivre-pesquisa-onda1/bcde-scripts/comparacao2.txt`.

### 2.3 Principais diferenças (FATO)

**Removidos do leiaute (confirmados por `grep -ci` sem distinção de maiúsculas no dump inteiro do Anexo de 20/10):**

- Grupo `IBSCBS/valores/gReeRepRes` (linhas 861 a 884 do antigo) e campos `tpReeRepRes`, `vlrReeRepRes`, `xTpReeRepRes`. `ReeRep` e `gReeRep` têm zero ocorrências no dump novo (contagem 0).
- Grupo `valores/vDedRed` (dedução e redução por documento, linhas 735 a 773 do antigo). `DedRed` tem zero ocorrências no dump novo (contagem 0; no antigo, 91).
- `NFSe/infNFSe/valores/vCalcDR` (linha 519 do antigo) e `IBSCBS/valores/vCalcReeRepRes` (linha 534 do antigo). No dump novo, `vCalcDR` aparece uma vez fora do leiaute e `vCalcReeRepRes` não aparece.
- Ressalva: o par do CEP do imóvel (linha 851 do antigo) pode ser movimento para `gUnidImob/end/CEP`, e não remoção. Pelo pareamento ambíguo, não afirmo qual dos dois.

**Novos no leiaute (FATO):**

- `IBSCBS/bensMoveis` (0-1000), com `cNCMBemMovel` (N, 8), `qtdNCMBemMovel` (N, 3) e `xNCMBemMovel` (C, 1-150) (linhas 922 a 925 do novo).
- `valores/vAjusteBC` (0-1), com `pAjusteBCISSQN`, `vAjusteBCISSQN` e `documentos/docAjusteBC` (1-1000), que tem `fornec` (CNPJ, CPF, NIF, xNome, endereço) (linhas 792 a 822 do novo).
- `IBSCBS/imovel/gLocacao` (pCopropriedade, vTotOper, vDescIncondTot, vDescCondTot, dVencOrig) e `IBSCBS/imovel/gUnidImob` (0-99), com `gAjusteBCLocImoveis` (linhas 903 a 921 do novo).
- `NFSe/infNFSe/valores/vCalcAjusteBCISSQN`, `IBSCBS/valores/vCalcAjusteBCLocImoveis` e `vCalcAjusteBCIBSCBS` (linhas 574, 589 e 590 do novo).

**Movidos de caminho (41 casos, FATO):** `inscImobFisc` e `cCIB` (e o endereço do imóvel) passaram de `IBSCBS/imovel/...` para `IBSCBS/imovel/gUnidImob/...`. Exemplos: antigo linha 848 para novo linha 910 (`inscImobFisc`), antigo linha 849 para novo linha 911 (`cCIB`).

**Alterados (24 casos, FATO):**

- **CNPJ passa de numérico (N) para alfanumérico (C):** prest/CNPJ, toma/CNPJ, interm/CNPJ, emit/CNPJ, dest/CNPJ e chNFSeRej. Conferido à mão: antigo linha 599 ("Número da inscrição federal (CNPJ) do prestador do serviço", tipo N) e novo linha 656 ("Número Código da inscrição federal (CNPJ - pode ser alfanumérico) do prestador do serviço", tipo C).
- **CEP passa de C para N (8 posições numéricas):** prest, toma, interm, serv/atvEvento, serv/obra. Conferido à mão: antigo linha 609 (tipo C) e novo linha 666 (tipo N, "Código numérico do Endereçamento Postal nacional (CEP)").
- **Tamanho:** `serv/cServ/xDescServ` de 1000 para 2000. Conferido à mão: antigo linha 678 (1000) e novo linha 737 (2000).
- **Tamanho:** `subst/cMotivo` de 1 para 2. Conferido à mão: antigo linha 596 (1) e novo linha 653 (2).
- **Tamanho:** `atvEvento/end/endExt/cEndPost` de 11 para 1-11. Conferido à mão: antigo linha 714 (11) e novo linha 771 (1-11).
- **Ocorrência de 1-1 para 0-1 (obrigatório deixou de ser):** `IBSCBS/cIndOp`, `IBSCBS/cLocalidadeIncid`, `IBSCBS/xLocalidadeIncid`, `IBSCBS/valores`, `IBSCBS/valores/vBC`, `IBSCBS/totCIBS/gIBS`, `IBSCBS/totCIBS/gCBS`, `IBSCBS/valores/trib/gIBSCBS`, `interm/xNome`, `toma/xNome`.

### 2.4 NÃO ACHADO

- Nenhum campo do leiaute novo que tenha sido apenas renomeado sem assinatura igual. Essa verificação depende do pareamento por assinatura, que é ambíguo nos casos repetidos (ver 2.1).

## 3. (d) Como o Id da DPS preenche as 14 posições para CPF

**Resposta:** os 11 dígitos do CPF ocupam as posições finais, precedidos de três zeros à ESQUERDA: `000` + CPF de 11 dígitos. Não há zeros à direita.

Fontes, que concordam entre si:

- Anexo de 20/10 (anexo1_prodrest.txt:639, aba 6 linha 102) e Anexo de 09/02 (anexo1.txt:583): "Inscrição Federal (14 - CPF completar com 000 à esquerda)".
- XSD (tiposSimples_v1.01.xsd:48): mesmo texto, "CPF completar com 000 à esquerda".
- Manual da API v1.2, seção 1.4.1 (manual.txt linhas 98 e 99): mesmo texto.

**Atenção (FATO, testado):** o pattern do XSD NÃO distingue zeros à esquerda de zeros à direita, porque `[0-9]{14}` aceita os dois. Testei com `re.fullmatch` em Python: `000` + CPF e CPF + `000` são aceitos. A regra de posição é semântica e tem que ficar no código e no teste (`idDpsComCpfPreenchidoConformeManual`, NL-1.3).

Exemplo verificado contra o pattern (CPF de exemplo 12345678909, município de exemplo 4305108, série 1, número 1):

`DPS4305108` + `1` + `00012345678909` + `00001` + `000000000000001` = 45 posições, aceito pelo pattern.

## 4. (e) Mapa de campos do Emissor Web contra o Anexo I de 20/10/2026

Mapa lido: `/home/petrus/IDrive/Documentos/projetos_claudebrain/Projects/nfse/docs/pesquisa/campos_emissor_web.md` (somente leitura).

### 4.1 D5: ChaveAcessoFornPagRealizado

**NÃO ACHADO por nome.** Conferido com `grep -ci` (sem distinção de maiúsculas) de `FornPag`, `AcessoForn`, `PagRealiz`, `pagRealiz`, `Realizado` e `ForncPag` no dump de 20/10 (zero para todos, exceto "Realizado", que aparece em três textos sem relação, linhas 138, 150 e 875). No XSD, nenhum nome de elemento ou tipo contém "pag" nem "realiz" (`grep -oi` sobre todos os `.xsd`, sem resultado).

**FATO que reforça o candidato `IBSCBS/gRefNFSe/refNFSe`:**

- Anexo de 20/10, linha 876 (aba 6 linha 339): `gRefNFSe`, G, ocorrência 0-1, "Grupo de NFS-e referenciadas. Obrigatório para tpOper = 2 ou 3".
- Anexo de 20/10, linha 877 (aba 6 linha 340): `refNFSe`, E, C, ocorrência 1-99, tamanho 50, "Chave da NFS-e referenciada".
- Anexo de 20/10, linha 875 (aba 6 linha 338): `tpOper` 0-1, com valores 2 ("Recebimento do pagamento com fornecimento já realizado") e 3 ("Fornecimento com pagamento já realizado").
- XSD: tiposComplexos_v1.01.xsd:2345 (`gRefNFSe`, type TCInfoRefNFSe, minOccurs 0), linha 2397 (declaração de TCInfoRefNFSe), linha 2399 (`refNFSe`, type TSChaveNFSe, maxOccurs 99).

**Conclusão (INFERÊNCIA):** a condição de aparecer com tpOper 2 ou 3 é a mesma nos dois lados (FATO). O nome "ChaveAcessoFornPagRealizado" não aparece em lugar nenhum (NÃO ACHADO). Mantenho o candidato como [inf]. Duas dúvidas abertas: (i) o site pode ter um único campo, enquanto o Anexo permite até 99 chaves; (ii) não sei se o campo do site corresponde ao grupo inteiro ou a uma ocorrência. Isso exige confirmação do líder ou de outra fonte, não dedução minha.

### 4.2 Seção 10 do mapa (não mapeados com certeza)

| Item da seção 10 | Resultado | Fonte | Situação |
|---|---|---|---|
| Obrigatoriedade real (asteriscos e `required` do HTML) | O Anexo traz a ocorrência legal (1-1, 0-1), não a obrigatoriedade da tela | Anexo, coluna OCOR | NÃO RESOLVÍVEL pelo Anexo. Continua aberto |
| Rótulos visíveis dos campos | O Anexo traz descrição, não rótulo de tela | Anexo, coluna DESCRIÇÃO | NÃO RESOLVÍVEL pelo Anexo. Continua aberto |
| Parâmetros de VerificarIncidencia, RecuperarListaDeIndOp e modais | São endpoints e JS do site | Mapa, seção 7 (L-57) | NÃO RESOLVÍVEL pelo Anexo. Fora da Onda 1 |
| Mapa valor para tribISSQN (MotivoNaoTributacao, HaExportacaoImunidadeNaoIncidencia) | O domínio foi confirmado: 1 Operação tributável, 2 Imunidade, 3 Exportação de serviço, 4 Não incidência (anexo1_prodrest.txt:837, aba 6 linha 300). Os valores 2, 3 e 4 do mapa batem com isso. A regra "HaExportacao=0 gera tribISSQN=1" e o default "não incidência" continuam só no JS | Anexo, linha 837 | PARCIALMENTE RESOLVIDO |
| Semântica de LocalDomicilio 0, 1 e 2 | NÃO ACHADO: `grep -ci` de `localdomic`, `local de domic` e `LocDom` dá zero nos dois dumps. O par NIF e endExt (exterior) existe no Anexo (prest/NIF, linha 658), o que é coerente com "2 = exterior", mas isso é INFERÊNCIA. O valor 0 não tem contrapartida no Anexo | Anexo | NÃO ACHADO. Mapa segue [inf] |
| Semântica de TipoEmitente (tpEmit) | RESOLVIDO: 1 Prestador, 2 Tomador, 3 Intermediário (anexo1_prodrest.txt:646, aba 6 linha 109). O XSD aceita os três (tiposSimples_v1.01.xsd:165 a 178) | Anexo e XSD | RESOLVIDO na semântica. O bloqueio de 2 e 3 está em outro item (ver 4.3) |
| Ramo não-RFB de ListarEstabelecimentosParaEmissao (IM obrigatória) | Endpoint do site, não aparece no Anexo | Mapa, seção 10 | NÃO RESOLVÍVEL pelo Anexo |
| Regras de exclusão retenção x suspensão | Parcial: E1311 (aba 7 linha 49, dump linha 993) proíbe os demais campos do grupo valores quando há suspensão, exceto vLiq. E0585 (linha 489, dump linha 1433) proíbe suspensão quando tribISSQN é 2, 3 ou 4. A exclusão retenção x suspensão em si: NÃO ACHADO. Nenhuma linha da aba RN tem "reten" e "suspens" juntos (`grep -i`, zero) | Anexo, aba 7 | PARCIAL |
| Obra x Evento | NÃO ACHADO: nenhuma linha da aba RN tem "obra" e "evento" juntos (`grep -i`, zero). Obra tem regra própria por cTribNac (regras 360 e 361 da aba RN) | Anexo, aba 7 | NÃO ACHADO para o par Obra x Evento |

### 4.3 Divergência nova: regra E9996 (tpEmit 2 ou 3 bloqueado)

**ALERTA (FATO):**

- O mapa diz, na seção 6, "Sem divergência (alinhados)": "tpEmit 2 e 3 bloqueados (E9996)". Também diz que a regra "E9996" entra em NL-1.5 do plano.
- No Anexo de 09/02, a regra existe: anexo1.txt:1056, RN linha 154, código E9996, "Nesta versão da aplicação, não é permitida a emissão de NFS-e pelo tomador ou intermediário".
- No Anexo de 20/10, o código E9996 não aparece (zero ocorrências no dump inteiro). A linha 154 da aba RN passou a ser E0014 (anexo1_prodrest.txt:1098). A regra de bloqueio por tpEmit 2 ou 3 não existe mais.
- No Anexo de 20/10 existe apenas uma regra que trata tpEmit 2 ou 3 de outro jeito: não permite imunidade, exportação ou não incidência quando o emitente é tomador ou intermediário (anexo1_prodrest.txt:1426). Isso é uma restrição sobre tribISSQN, não um bloqueio de emissão.
- O XSD aceita 2 e 3 (tiposSimples_v1.01.xsd:176 a 178).

**Conclusão:** o mapa e a NL-1.5 estão baseados no Anexo de 09/02, não no de 20/10. O mapa precisa ser corrigido e a regra E9996 precisa de decisão do líder antes de NL-1.5. Não decido isso aqui (L-01).

## 5. SHA-256 dos arquivos do ZIP (para `third_party/nfse-xsd/prodrest-20261020/SHA256SUMS`, NL-1.6)

ZIP de origem: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/esquemas_prod_restrita_20261020.zip (SHA-256 `25fdcecc651a67a216496ab10d7566631e6ed10f721e37d62c7845dadb0ea267`).

```
7032188bb6f137d52b16512583b739a361e6b1434c42cb539a29b8efa32f8321  CNC_v1.00.xsd
fe45e5250a48e519aba89fc6a472863b8e602ed957778fb64692804933a00d0c  DPS_v1.01.xsd
986d0a1c4d27454f712169849aa7c2380aaa4560bd9c920e0ff03ba6599ae28b  evento_v1.01.xsd
af0bd2d8c50acba3d9c3f3f515426eca083b3f66e3211ce8dd48a7cf101818b8  NFSe_v1.01.xsd
e90b6816d29cca0bd5ed8f86ad98d3b7b0d8bbfc11a7583e6ff8f98170fd4009  pedRegEvento_v1.01.xsd
af606b7317824fa8fa7ad8e44bac2ad8530cd5d162ebcdec647205473c41d525  tiposCnc_v1.00.xsd
4f66f7c15bd3b05b3a16cbe5fa671b90c10e4c9c74276038730be1c8c794b721  tiposComplexos_v1.01.xsd
1b32bea21089dc232d78b62d805e9c07382f440d4e0ea2cb3fb6b82ebde68c11  tiposEventos_v1.01.xsd
562e326724f85cd7c41176d35e67c542376784d4c950ec1719704d480a8af226  tiposSimples_v1.01.xsd
8c9792a056aedcadfc95bf16f2d7448a35da40df71533e20aea37d0329980c48  xmldsig-core-schema.xsd
```

## 6. Impacto nas fatias (NL-1.3, NL-1.4, NL-1.6)

- **NL-1.3:** Id da DPS com 45 posições. CPF usa tipo 1 e `000` + 11 dígitos à esquerda (seção 3). Teste de posição obrigatório, porque o pattern não cobre isso. CNPJ usa tipo 2, com 14 caracteres `[0-9A-Z]`.
- **NL-1.4:** versão do leiaute 1.01. CNPJ alfanumérico entra como texto; CEP passa a ser numérico de 8 posições. Grupos removidos (gReeRepRes, vDedRed) não entram no modelo. Grupos novos (bensMoveis, vAjusteBC, gLocacao, gUnidImob) precisam de decisão de escopo (fora da v1 ou dentro), pelo rito da NL-1.4.
- **NL-1.4 e NL-4.5:** D5 segue como candidato `IBSCBS/gRefNFSe/refNFSe`, com o tpOper 2 ou 3 como condição, até confirmação do líder.
- **NL-1.5:** a regra E9996 não existe no Anexo de 20/10. Não implementar sem decisão do líder (seção 4.3).
- **NL-1.6:** o bundle mínimo para validar DPS são 4 arquivos (seção 1.1). O SHA-256 do XLSX de origem do Anexo de 20/10 precisa ser registrado. A regra de Reference da assinatura não está no manual (seção 1.2), então a assinatura fica pendente.
- **NL-1.6:** a regra de tamanho de `cEndPost` (1-11) e `xDescServ` (2000) precisa estar no codec. O teste `cnpjAlfanumericoSaiComoTexto` tem base no XSD (TSCNPJ, `[0-9A-Z]{14}`, tiposSimples_v1.01.xsd:364) e no Anexo (tipo C).

## 7. Perguntas abertas para o líder (não decididas nesta pesquisa)

1. A regra E9996 (tpEmit 2 ou 3 bloqueado) continua valendo na v1, já que o Anexo de 20/10 não a tem? (seção 4.3)
2. D5: o candidato `IBSCBS/gRefNFSe/refNFSe` é aceito como provisório? (seção 4.1)
3. A regra de Reference da assinatura (URI do Id do infDPS) precisa de Nota Técnica ou manual de assinatura do ADN. Quem fornece? (seção 1.2)
