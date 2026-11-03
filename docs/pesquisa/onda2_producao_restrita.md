# Onda 2: resultado da emissão de teste em Produção Restrita (NL-2.10)

**Data:** 03/11/2026.
**Escopo:** condição (9) da definição de fechamento da Onda 2 (plano, linha 665): cada INFERÊNCIA da seção da Onda 2 com o estado medido na emissão de teste do líder.
**Leis aplicadas:** L-18 (FATO separado de INFERÊNCIA), L-28 (nenhum dado pessoal no documento), L-32 (sem travessão).

**Frase-guarda (L-21):** "Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição."

**Legenda.** CONFIRMADA: a emissão real mostrou o que a INFERÊNCIA afirma. REFUTADA: a emissão real mostrou o contrário. NÃO EXERCIDA: terceiro estado, autorizado pelo líder por AskUserQuestion em 03/11/2026, com a resposta "Não exercida (Recomendado)". Uma INFERÊNCIA que a emissão não exercitou fica NÃO EXERCIDA, com a fatia onde será medida, em vez de CONFIRMADA ou REFUTADA. Quando uma INFERÊNCIA tem uma parte medida e outra não, a tabela mostra os dois estados.

**Fonte da emissão.** Os fatos da pasta de devolução, lidos pelo main e sem dado pessoal, estão em `FECH-2-0` (fatos da devolução, itens (a) a (g)). O registro de decisões autônomas da onda cita a execução e as recusas; cada citação traz a linha.

## 1. O que foi enviado e o que voltou

- **Execução de 09:31 de 03/11/2026 (horário de Recife), DPS número 9.** O POST /nfse respondeu status 201 e o log gravou o evento `emissao.aceita`. Nenhum código E na execução (FECH-2-0, itens (a) e (g); registro de decisões, linha 556).
- **Ambiente.** Produção Restrita, pela decisão do líder de 27/10/2026 (plano, linha 445).
- **Base da API.** `/SefinNacional`, padrão do script (`tools/producao_restrita/emitir_teste.sh:93`). O script troca para `/API/SefinNacional` só depois de 404 (`:1016`). O FECH-2-0 não registra 404 em nenhuma execução (item (c)).
- **Algoritmo da assinatura.** `rsa-sha1`, padrão do script (`tools/producao_restrita/emitir_teste.sh:94`). O script troca para `rsa-sha256` só depois de E0714 (`:1024`). O FECH-2-0 não registra E0714 em nenhuma execução (item (c)).
- **Regime e município.** Regime ME/EPP, com `pTotTribSN` 0,00 e aviso, sem `--ptottribsn` (registro de decisões, linha 556). Município 2611606, lido das posições 1 a 7 da chave mascarada (FECH-2-0, item (e)).
- **cTribMun.** O registro informa cTribMun 501 para a DPS 8, na execução de 02/11 às 20:06 (registro de decisões, linha 528). O FECH-2-0 não traz o valor da execução de 09:31, e nenhuma fonte permitida o confirma para ela.
- **Chave de acesso de 50 posições, na forma mascarada da DA-2.102** (FECH-2-0, item (e)):

  `261160622##############000000000000126115284999651`

- **Linhas JSON do log da execução de 09:31** (FECH-2-0, item (g), copiadas como estão):

```
{"level":"info","ts":"2026-11-03T12:31:23Z","service":"notalivre-cli","evento":"certificado.caminho_configurado","campos":{},"descartados":0}
{"level":"info","ts":"2026-11-03T12:31:28Z","service":"notalivre-cli","evento":"sefin.requisicao","campos":{"metodo":"POST","status":"201"},"descartados":0}
{"level":"info","ts":"2026-11-03T12:31:28Z","service":"notalivre-cli","evento":"emissao.aceita","campos":{"idDps":"DPS26116062##############00001000000000000009","chaveAcesso":"261160622##############000000000000126115284999651"},"descartados":0}
```

**Recusas anteriores** (FECH-2-0, item (a); registro de decisões):

- **E0116**, HTTP 400, nas execuções 20261101-210221 e 20261101-210339. O registro (linha 451) atribui essas execuções às DPS 5 e 6. Fatia aberta: NL-2.10g (linha 453).
- **E0312**, HTTP 400, na execução 20261102-081300, DPS 7 (registro, linha 506). Fatia aberta: NL-2.10h (linha 509).
- **E0712**, HTTP 400, na execução 20261102-200710, DPS 8 (registro, linha 528). Fatia aberta: NL-2.10j (linha 529).
- Forma das quatro recusas: HTTP 400, `application/json; charset=utf-8`, lista `erros` com objetos que trazem `Codigo` e um texto de 88 caracteres.
- Falha de transporte: as execuções de 01/11 entre 15:31 e 15:34 saíram como `sefin.falha` sem status HTTP, com causa não medida, antes da correção da NL-2.10e (FECH-2-0, itens (a) e (c)). Outras execuções foram recusadas antes do envio, com código de 15 letras (FECH-2-0, item (a)).

## 2. Inferências

Uma linha por INFERÊNCIA do plano, de INF-01 a INF-21. Nenhuma INFERÊNCIA nova: as linhas 650 e 665 do plano citam a palavra só para se referir a este documento.

| ID | Onde (blob 2c10320) | Inferência | Estado | Evidência | Destino |
|---|---|---|---|---|---|
| INF-01 | plano, 450 | Referência `URI="#"` com o `Id` do `infDPS` | CONFIRMADA | DPS 9 aceita com status 201 e sem código E (FECH-2-0, item (a)) | não se aplica |
| INF-02 | plano, 450 e 462 | `rsa-sha1`, digest sha1, C14N 20010315, `enveloped-signature`, `KeyInfo` só com `X509Certificate` | CONFIRMADA (não prova que `rsa-sha256` seria recusado) | `tools/producao_restrita/emitir_teste.sh:94`; a troca só ocorre em E0714 (`:1024`), que não apareceu (FECH-2-0, item (c)) | sem destino (a configuração já cobre) |
| INF-03 | plano, 451, 464, 466, 541 e 542; spec, 126 e 212 | Corpo `{"dpsXmlGZipB64": ...}`, GZip RFC 1952 antes do Base64, `application/json` | CONFIRMADA | Status 201 com o envelope (FECH-2-0, item (a)); nome do campo em `src/mid/include/nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h:21`; o Content-Type é fixado no código (`src/mid/sefin/leiaute/prodrest_20261020/envelope_dps.cpp:27` a `:44`; `src/mid/sefin/cliente/cliente_sefin.cpp:360`) e a Sefin aceitou a requisição (201) | não se aplica |
| INF-04 | plano, 452 e 465 | Base `/SefinNacional` ou `/API/SefinNacional` | CONFIRMADA (`/SefinNacional`); NÃO EXERCIDA (`/API/SefinNacional`) | `tools/producao_restrita/emitir_teste.sh:93`; a troca só ocorre em 404 (`:1016`), e o FECH-2-0 não registra 404 (item (c)) | NL-12.1 (dossiê de go/no-go; a base de Produção tem outro host e é conferida ali) |
| INF-05 | plano, 453 e 505 | Inscrição do A1 pelo OID 2.16.76.1.3.3 (CNPJ) | CONFIRMADA | A posição 9 da chave mascarada é tipo 2, que é CNPJ (FECH-2-0, item (e); `docs/adr/0005-*.md:12`); OID em `src/foundation/pkcs12/extrator_inscricao.cpp:19`; carga do A1 em `src/mid/composicao/emissao_cli.cpp:204-207`. A comparação entre A1 e DPS não foi medida (FECH-2-0, item (d)) | não se aplica |
| INF-06 | plano, 453 e 505 | CPF nas posições 9 a 19 do OID 2.16.76.1.3.1 | NÃO EXERCIDA | O A1 usado é de CNPJ (FECH-2-0, item (e)) | NL-4.4 |
| INF-07 | plano, 453 e 505 | "Mesma raiz" pelos 8 primeiros caracteres, como texto | NÃO EXERCIDA | Sem caso de raiz na emissão (FECH-2-0) | NL-4.4 |
| INF-08 | plano, 456, 552 e 567 | Erro HTTP 400 com `Codigo` e `Descricao`; leitor aceita `erro`/`erros` e `Codigo`/`codigo` | CONFIRMADA (400, `erros`, `Codigo` com C maiúsculo); NÃO EXERCIDA (nome do campo de texto e variantes `erro` e `codigo`) | FECH-2-0, item (a), nas quatro recusas de HTTP 400. O nome do campo de texto aparece mascarado, e `Descricao` não foi visto | NL-8.10 (microcópia do catálogo E; se o `Descricao` da Sefin serve de reserva, por exemplo no E1204, é decisão dessa fatia) |
| INF-09 | plano, 456, 552, 568 e 575 | GET /dps/{id} sem nota devolve 404 com E2404 | NÃO EXERCIDA | Nenhuma execução registrou GET /dps/{id} (FECH-2-0, item (c)) | NL-3.7 |
| INF-10 | plano, 456 e 552 | Sucesso com `chaveAcesso` de 50 posições | CONFIRMADA (e status 201 como FATO novo) | FECH-2-0, itens (a), (e) e (g) | não se aplica |
| INF-11 | plano, 456, 552 e 572 | XML da NFS-e em GZip e Base64 no corpo do sucesso (`nfseXmlGZipB64`) | CONFIRMADA (presença do campo); NÃO EXERCIDA (decodificação) | `nfseXmlGZipB64` com 6892 caracteres (FECH-2-0, item (b)); `src/mid/sefin/cliente/cliente_sefin.cpp:156-160` | NL-4.2 |
| INF-12 | plano, 572 | Teto de 5 MiB do XML descompactado cobre uma NFS-e | NÃO EXERCIDA | Só o Base64 foi visto, não o XML descompactado (FECH-2-0, item (b)) | NL-4.2 |
| INF-13 | plano, 456 e 568 | 409 para DPS duplicada | NÃO EXERCIDA | Nenhuma execução registrou 409 (FECH-2-0, item (c)) | NL-3.7 |
| INF-14 | plano, 568 | Número de tentativas do retry medido na NL-2.10 | NÃO EXERCIDA | Nenhuma falha transitória com resposta da Sefin (FECH-2-0, item (c)) | NL-11.3 (critério de pronto com lote em Produção Restrita; até lá o valor segue configurável) |
| INF-15 | plano, 570 | Inscrição nas posições 10 a 23 da chave de 50; 13 a 26 do Id da NFS-e de 53 posições | CONFIRMADA (chave de 50); NÃO EXERCIDA (Id da NFS-e de 53 posições) | A máscara da chave está nas posições 10 a 23 (FECH-2-0, item (e)); o leiaute bate nas posições 1 a 9 e 37 a 40 (`docs/adr/0005-*.md:33`). A inscrição real não foi comparada, por ser dado pessoal | NL-4.2 |
| INF-16 | plano, 571 | Forma como a Sefin devolve E1207 (certificado revogado) | NÃO EXERCIDA | Nenhum certificado revogado na emissão (FECH-2-0) | NL-4.4 |
| INF-17 | plano, 575 e 580 | Classificação de antes e depois do envio; recusa do certificado sem entrega à aplicação | NÃO EXERCIDA | Nenhuma execução registrou `sefin.transporte_falhou` (FECH-2-0, item (c)); as falhas de 01/11 saíram como `sefin.falha` sem status | NL-11.2 (comparação entre o servidor falso e a Sefin real para o TLS) |
| INF-18 | plano, 578 | A NFS-e devolvida traz o `infDPS` sem alteração | NÃO EXERCIDA | O caminho do E0014 e do 409 não rodou (FECH-2-0, item (c)) | NL-3.7 |
| INF-19 | plano, 648 | Sem adesão do município, a Sefin rejeita | NÃO EXERCIDA (o município aceitou) | A DPS 9 foi aceita para o município 2611606 (FECH-2-0, itens (a) e (e)) | NL-5.4 (parâmetros municipais; a consulta mostra a adesão e o app pode avisar antes de enviar) |
| INF-20 | plano, 531 | O codec não emite `xmlns:xsi` | CONFIRMADA | `git grep` no blob, em `src`: nenhum código emite o namespace. Há dois comentários do assinador, `src/foundation/xmldsig/assinador_xml.cpp:115` e `:466`, que descrevem herança | não se aplica |
| INF-21 | spec, 344 | O servidor falso valida o que o app faz, não a aceitação pela Sefin | CONFIRMADA | A aceitação real foi medida numa DPS (FECH-2-0, item (a)); o servidor falso não a substitui | não se aplica |

## 3. Achados novos

- **Sucesso com status 201.** Primeira medição do status de sucesso da Sefin (FECH-2-0, itens (a) e (g)). O Content-Type da resposta de sucesso é `application/json; charset=utf-8` (FECH-2-0, item (a)).
- **Corpo de sucesso.** Traz cinco campos escalares, um deles nulo, além de `chaveAcesso` e `nfseXmlGZipB64`. Os nomes dos outros campos estão mascarados no FECH-2-0 (item (a)).
- **Corpo de erro.** Traz a lista `erros`, com `Codigo` (C maiúsculo) e um texto de 88 caracteres. O nome do campo de texto não aparece no FECH-2-0 (item (a)).
- **Falha de transporte antes da correção.** Três execuções de 01/11 entre 15:31 e 15:34 saíram como `sefin.falha` sem status HTTP, antes da NL-2.10e (FECH-2-0, item (c)).
- **Ausência de consultas.** Nenhuma execução fez GET /dps/{id} ou GET /nfse/{chave}, e nenhuma recebeu 404, 409, E2404 ou E0014 (FECH-2-0, item (c)).
- **Número de DPS das recusas de E0116.** O registro de decisões (linha 451) atribui E0116 às DPS 5 e 6, versão adotada neste documento. A ordem do CTO trazia só a DPS 6.
- **cTribMun da execução de 09:31.** Não consta de fonte permitida para essa execução (ver seção 1).

## 4. Marcadores no código

Os comentários "INFERÊNCIA até a NL-2.10" destes arquivos continuam no código e são atualizados pela próxima fatia que tocar cada arquivo (DA-2.105, INBOX). Os 11 arquivos são:

- `src/mid/include/nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h`
- `src/back/include/nl/back/ports/gateway_sefin.h`
- `src/mid/sefin/cliente/leitor_erro_sefin.cpp`
- `src/mid/include/nl/mid/sefin/cliente/leitor_erro_sefin.h`
- `src/mid/sefin/leiaute/prodrest_20261020/envelope_dps.cpp`
- `tests/unit/mid/sefin/CMakeLists.txt`
- `tests/unit/mid/sefin/tst_envelope_dps.cpp`
- `tests/tools/servidor_falso/test_servidor_falso.py`
- `tools/servidor_falso/__init__.py`
- `tools/servidor_falso/envelope.py`
- `tools/servidor_falso/servidor.py`

## 5. Fontes

- `docs/superpowers/plans/2026-10-25-notalivre-plano.md` (blob 2c10320): linhas 445, 450 a 456, 462 a 466, 505, 531, 541, 542, 552, 567, 568, 570 a 572, 575, 578, 580, 648 e 665.
- `docs/superpowers/specs/2026-10-25-notalivre-design.md` (blob 2c10320): linhas 126, 212 e 344.
- `tools/producao_restrita/emitir_teste.sh:93`, `:94`, `:1016` e `:1024`.
- `src/mid/composicao/emissao_cli.cpp:204-207`; `src/mid/sefin/cliente/cliente_sefin.cpp:156-160`.
- `src/foundation/pkcs12/extrator_inscricao.cpp:19`; `src/foundation/xmldsig/assinador_xml.cpp:115` e `:466`.
- `src/mid/include/nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h:21`.
- `docs/adr/0005-*.md`, linhas 12 e 33.
- FECH-2-0 (fatos da devolução, lidos pelo main, itens (a) a (g)) e registro local da emissão, conferido pelo orquestrador, fora do repositório. O registro de decisões autônomas da onda cita as linhas 451, 453, 506, 509, 528, 529 e 556.
