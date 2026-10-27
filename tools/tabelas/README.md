# tools/tabelas: conversor das tabelas fiscais embutidas (NL-1.10)

Converte os Anexos A e B da Biblioteca da NFS-e Nacional (gov.br/nfse) para o formato embutido do NotaLivre.
Fonte dos fatos: `docs/pesquisa/tabelas.md`, seções Q2 e Q5.

## Fontes e âncora de confiança

`fontes.json` versiona URL, versão, data de publicação, tamanho e SHA-256 de cada XLSX. O conversor confere
o tamanho e o SHA-256 dos bytes antes de abrir o arquivo, e recusa com status 1 se não baterem.

| Anexo | Conteúdo | Versão | Publicação | Arquivo | Bytes |
|---|---|---|---|---|---|
| A | municípios IBGE, países ISO2, localidades (TAB.LOC.GERAL) | 1.00 | 10/12/2025 | `anexo_a.xlsx` | 218140 |
| B | cTribNac (lista de serviço nacional) e NBS 2.0 | 1.01 | 22/01/2026 | `anexo_b.xlsx` | 85008 |

Os XLSX não são versionados (são de terceiro). Ficam em `/var/tmp/notalivre-tabelas-fontes/` (fora da árvore,
persistente como o volume do Qt), porque o conversor precisa deles a cada build ou atualização.

## Formato embutido (decisão desta fatia)

**JSON UTF-8, um arquivo por tabela**, com um objeto `{"proveniencia", "contagens", "registros"}`: o JSON
carrega a proveniência de forma estruturada e legível pelo Qt, e a Onda 5 troca o arquivo de forma atômica.
Códigos ficam como texto (o XLSX perde o zero à esquerda do cTribNac), e a proveniência traz URL, data de
publicação, data de download, SHA-256 e bytes da origem.

Arquivos gerados: `municipios.json`, `paises.json`, `localidades.json`, `ctribnac.json` (código de 6 dígitos com
zero à esquerda, mais item, subitem e desdobro de 2 dígitos), `nbs.json` (código preservado como texto, pontuado
ou não).

Saída: proposta `${CMAKE_BINARY_DIR}/tabelas/` no build (fora de `src/`). Nesta fatia a saída de referência foi
gerada em `/var/tmp/notalivre-tabelas-saida/` (fora da árvore), pelo conversor rodando dentro do contêiner.
Ligar a conversão ao CMake como comando de build fica para a NL-5.2 e depende de decisão sobre embutir o
resultado no binário.

## Uso

```
python3 -I tools/tabelas/converter.py --xlsx-dir /var/tmp/notalivre-tabelas-fontes --saida DIR_DE_SAIDA
```

Só stdlib (`zipfile`, `xml.etree`, `json`, `hashlib`), sem pip (L-51).

## Guardas do XLSX (DA-1.12)

Antes de qualquer parse de XML vindo do ZIP, `xlsx_leitor.py` aplica:

- **DOCTYPE e ENTITY recusados pelo próprio parser expat** (`xml.parsers.expat`, `StartDoctypeDeclHandler` e
  `EntityDeclHandler`), antes do `xml.etree` construir a árvore. Mensagens: "DOCTYPE recusado" e "ENTITY recusado".
  A recusa é pelo parser, não por regex sobre bytes: em UTF-16 `<!ENTITY` não aparece como sequência contígua e a
  regex deixava passar (revisão 2). O expat decodifica a codificação real (UTF-8, UTF-8 com BOM, UTF-16), e a
  segunda passada do `xml.etree` só vê documento sem DTD. Como o DOCTYPE vem antes de qualquer ENTITY, na prática
  o DOCTYPE é que dispara; o handler de ENTITY é defesa a mais. XML malformado vira `ErroXlsx` (sem traceback).
- **Teto de tamanho descompactado por arquivo: 16 MiB** (`LIMITE_BYTES_ARQUIVO`). Confere o tamanho declarado no
  ZIP e o lido de fato, porque o declarado pode mentir. Os XLSX reais têm no máximo 1,1 MB por arquivo.
- **Teto de 256 entradas no ZIP** (`MAX_ARQUIVOS_ZIP`). Os XLSX reais têm 21 (Anexo B) e 27 (Anexo A).

Os tetos são valores escolhidos com folga de uns 15x (tamanho) e 9x (entradas) sobre o real; cada recusa tem
teste próprio com ZIP fabricado (`xlsxComDoctypeRecusado`, `xlsxComEntityRecusado`,
`xlsxArquivoDescompactadoGrandeRecusado`, `xlsxComArquivosDemaisRecusado`) e o conversor recusa o XLSX com rc 1 e
nada gravado (`conversorRecusaXmlComDoctypeRc1`).

A biblioteca padrão não traz `defusedxml`: é pacote pip, fica de fora por L-51. Por isso as guardas acima.

## Contagens fixadas (medidas em 25/10/2026, amarradas ao SHA-256 de `fontes.json`)

Outra versão de Anexo muda estes números e exige revisar `ESPERADO` em `converter.py`. Como a versão futura é
tratada na NL-5.3, aqui a contagem é exata.

| Tabela | Linhas de dados | Convertidos | Observação |
|---|---|---|---|
| ctribnac | 580 linhas de dados | 338 códigos | 220 de 6 dígitos e 118 de 5 (item 1 a 9, zero perdido no XLSX). As outras 242 linhas são de agrupamento, sem código (desdobro 0). A pesquisa dizia "580 linhas de cTribNac": o número de códigos é 338. |
| nbs | 1210 | 1210 | 1108 pontuados e 102 sem pontos (5 dígitos). Dois códigos sentinela, `99999` e `9.9999.99.99`, vêm sem descrição na fonte: ficam na tabela com descrição vazia, contados em `sem_descricao` (2). Não há texto para inventar. |
| municipios | 5570 | 5570 | UF derivada dos 2 primeiros dígitos (27 UFs). A coluna "Sigla UF" só vem preenchida nas 450 linhas do Norte: 450 da planilha, 5120 derivadas. |
| paises | 250 | 250 | Inclui `BR` e `ZZ` ("Município não possui a informação no sistema próprio;"). `ZZ` fica na tabela; a regra de que `ZZ` não vale no tomador exterior é do domínio (E0246), não desta tabela. |
| localidades | 1 | 1 | `TAB.LOC.GERAL` do Anexo A: `0000000` ÁGUAS MARÍTIMAS. Não é município; a linha vazia no fim da aba é contada em `vazias_finais` (1). Toca E0302 e E0304. |

## Falhas que param a conversão (status 1, nada gravado)

- SHA-256 ou tamanho do XLSX diferente de `fontes.json`.
- XLSX recusado pelas guardas do DA-1.12 (DOCTYPE, ENTITY, arquivo grande demais, ZIP com entradas demais).
- Cabeçalho de aba diferente do esperado.
- Código de cTribNac que não bate com item, subitem e desdobro, ou com desdobro zero.
- Código de município fora de 7 dígitos, UF desconhecida, nome da UF que não bate com o código, sigla dada que não bate.
- Código duplicado, descrição vazia, linha vazia no meio da aba.
- Qualquer contagem diferente da fixada acima.

## Confiança no primeiro uso

O SHA-256 dos dois XLSX foi registrado no primeiro download desta fatia (plano, Global Constraints, arquivos de
terceiros; ponto L9). A contagem de países (250) também é de primeiro uso: não há contagem oficial publicada na
pesquisa.
