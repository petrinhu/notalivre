<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa -->

# Plano da fatia NL-3.L: leiaute de Produção Restrita de 24/11/2026 (Release 2 da NT 009) no codec e nas regras locais

Autor: Caetano (CTO), 27/11/2026, 12:30 UTC. Repositório `/home/user/notalivre`, branch `onda-3`, HEAD `a48fbb3`, lido sem editar, sem commit e sem build.

Frase-guarda (L-21), verbatim:

> Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição.

Decisão que abre a fatia: líder, 27/11/2026, por AskUserQuestion, "Fatia agora na Onda 3 (Recomendado)". Ratificações do mesmo dia: ponto 5, "Como rejeição ativa (Recomendado)"; ponto 7, "Bloquear o que estiver vencido (Recomendado)".

**ID.** `NL-3.L` está livre: `grep -o "NL-3\.[A-Za-z0-9]*"` no `TODO.md` e no plano lista NL-3.0 a NL-3.13, NL-3.1a a NL-3.1v, NL-3.2a a NL-3.2c, NL-3.A, NL-3.C, NL-3.D2 a NL-3.D5, NL-3.E, NL-3.K, NL-3.K3 e NL-3.P; nenhum NL-3.L.

**Leis que disparam.** NF-05 (downloads da lista L9, medidos abaixo), NF-17 (a fatia não toca login, permissão, token, segredo, apagar dado, schema de banco nem chave, e a Fase 0 ainda não contou: sem as 7 fases; a mudança é do leiaute XML, não do schema do banco), NF-18 (mensagens novas em pt-br, sem travessão), NF-19 (nada de dinheiro muda; `Valor` intocado), NF-25 (pesquisa feita antes deste plano, com fonte), NF-31 (o dourado de digest SHA-1 do arnês muda, ver 5.4), NF-32 (átomos), NF-33 (executor, revisor e orquestrador distintos; portão contado só depois de visto vermelho), NF-34 (sem SQL). NF-12: sem job de mutação, exceto se entrar o átomo opcional do log (pergunta 4).

Legenda: **FATO** (lido ou medido, com fonte), **INFERÊNCIA** (conclusão minha, com a base), **NÃO MEDIDO**.

---

## 0. O que foi baixado e lido (NF-05, lista L9)

Pasta: `pasta de rascunho da sessão de 27/11/2026, fora do repositório (os arquivos se rebaixam do gov.br pelas URLs e SHA-256 desta seção)` (downloads em `dl/`, XSD extraídos em `xsd_20261124/` e `xsd_prod_20260209/`; scripts de leitura fora dela, em `scratchpad/scripts/`). Planilhas lidas com `tools/tabelas/xlsx_leitor.py` (`python3 -B`), XSD com `lxml` e `xmllint --nonet`.

| Arquivo | URL (gov.br, 27/11/2026, 12:00 a 12:25 UTC) | Bytes | SHA-256 |
|---|---|---|---|
| ZIP do XSD de Produção Restrita 24/11/2026 | `.../producao-restrita/esquemas-xsd-producao-restrita-20261124.zip` | 39728 | `1ca83c1a7929f6d5684c9a17700fe3d3a375e4ea12b7ccb832bfc369768902c0` |
| Anexo I de Produção Restrita 24/11/2026 | `.../producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261124.xlsx` | 245045 | `cf4d1d36daae5600060db2b36d9a84a703aa209cb099a0abbd7d3793c3e7e928` (igual ao de `contador_6_4.md`) |
| Anexo VII v1.03.00 (NT 009) | `.../rtc/anexovii-indop_ibscbs_v1-03-00-nt009.xlsx` | 27475 | `95f30a44ee94adeea57fb92f3a0337b1e62fdc106f393e73894087889be9e5aa` (igual) |
| Anexo VI v1.04.01 (NT 009) | `.../rtc/anexovi-leiautesrn_rtc_ibscbs-v1-04-01-nt009.xlsx` | 296297 | `103a150dd6f56ec8edcaa3a453b59a1b0e096d7cd5025d7bd06a387f141ddf39` (igual) |
| Anexo C de Produção Restrita 22/01/2026 | `.../producao-restrita/anexo_c-indop_ibscbs-snnfse-prodrest-v1-01-20260122.xlsx` | 17966 | `f13b08dc802e82c3c43d591d06575922095595a17c94e61bc4a37c41633a744b` |
| ZIP do XSD de Produção (documentação atual) 09/02/2026 | `.../documentacao-atual/esquemas-xsd-v1.01-20260209.zip` | 65640 | `e7935cbd9470527c6cc32984c1b2263e614183bf0139ce2733eaaed2de9a8072` |
| Manual contribuintes, API Emissor Público v1.0 | `.../documentacao-atual/manual-contribuintes-emissor-publico-api-snnfse-v1.0-20250317.pdf` | 188158 | `ac2f36e34ff565cc36d67c5d67415c33cc09f27b2dea006e8da9bcd2ddce5581` |
| Manual municípios, API Emissor Público v1.2 (PR) | `.../producao-restrita/prod-res-manual-municipios-emissor-publico-api-snnfse-v1-2-20261123.pdf` | 294900 | `79d2832343c5cffa6d178824e5029a115a04f4ba72753e310e4d36113700695e` |
| Tentativa: ZIP antigo de 20/10 | `.../producao-restrita/esquemas_prod_restrita_20261020.zip` | 58116 | `f1dab801...` **é HTML, não ZIP** (ver D3) |
| Tentativa: Anexo I antigo de 20/10 | `.../producao-restrita/anexo_i-...-20261020.xlsx` | 1198 | `1b09f476...` **é texto de redirecionamento, não XLSX** (ver D3) |

Prefixo das URLs: `https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica`. Páginas de listagem lidas e guardadas em `leiaute_2611/pagina/`: "Documentação Técnica (Homologação/testes)" (atualizada em 25/11/2026 15h59, lista só "Esquemas XSD Produção Restrita 20261124" e "Anexo I ... versão 1.01 20261124"), "Leiaute e esquemas antigos", "RTC", "Documentação atual", "Atualizações e implantações".

Hash por arquivo do ZIP de 24/11 contra `third_party/nfse-xsd/prodrest-20261020/SHA256SUMS:7-16` (FATO, `sha256sum`):

| Arquivo | 20/10 (SHA256SUMS) | 24/11 (medido) | Bytes 24/11 |
|---|---|---|---|
| CNC_v1.00.xsd | 7032188b... | igual | 674 |
| DPS_v1.01.xsd | fe45e525... | igual | 691 |
| evento_v1.01.xsd | 986d0a1c... | igual | 737 |
| NFSe_v1.01.xsd | af0bd2d8... | igual | 752 |
| pedRegEvento_v1.01.xsd | e90b6816... | igual | 746 |
| tiposCnc_v1.00.xsd | af606b73... | igual | 7201 |
| **tiposComplexos_v1.01.xsd** | `4f66f7c15bd3b05b3a16cbe5fa671b90c10e4c9c74276038730be1c8c794b721` | **`3e72f4d443dfefb6c9eb97c97719261e2c0bba7a1542193729a1799ac3abdb96`** | 121266 |
| tiposEventos_v1.01.xsd | 1b32bea2... | igual (data do membro 23/11, conteúdo igual) | 31544 |
| **tiposSimples_v1.01.xsd** | `562e326724f85cd7c41176d35e67c542376784d4c950ec1719704d480a8af226` | **`e20074f552b2000227adcc89492ed9485b776ea96695b693444001d70c441e0c`** | 77131 |
| xmldsig-core-schema.xsd | 8c9792a0... | igual | 10312 |

Mudaram 2 dos 10 arquivos. O texto antigo dos dois não pôde ser comparado linha a linha porque o gov.br tirou o ZIP de 20/10 do ar (D3); a diferença foi medida pelo efeito, validando as 16 fixtures do repositório (que passavam no XSD de 20/10 pelo teste `vereditoDoValidadorBateComONomeDaFixture`) contra o XSD de 24/11.

---

## 1. Diferenças medidas, por gravidade

### Gravidade 1: bloqueia a emissão de teste

**D1. `indDest` saiu do grupo IBSCBS e virou campo obrigatório da raiz de `infDPS`, logo depois de `toma`.**
- FATO, XSD 24/11: `tiposComplexos_v1.01.xsd:845` (`<xs:element name="indDest" type="TSRTCIndDest">`, sem `minOccurs`, logo 1-1), entre `toma` (`:840`, 0-1) e `dest` (`:850`, 0-1), antes de `serv` (`:860`); `TCRTCInfoIBSCBS` (`:2392`) não tem mais `indDest` nem `dest` (sequência medida: indFinal, cIndOp, tpOper, gRefNFSe, tpEnteGov, imovel, bensMoveis, condominios, valores). `TSRTCIndDest`, `tiposSimples_v1.01.xsd:1793`, valores 0 e 1.
- FATO, Anexo I 24/11, aba "LEIAUTE DPS_NFS-e 1.01", linha Excel 170 (nº 169): `NFSe/infNFSe/DPS/infDPS/` `indDest`, OCOR 1-1; linha 171 (nº 170): `dest` 0-1 no mesmo caminho.
- FATO, já previsto: NT 009 v1.01 §2.11, "indDest e dest REPOSICIONADOS para infDPS, raiz (após o grupo toma)" (`docs/pesquisa/ibscbs.md:55`; spec `:437`). O XSD de 20/10 ainda não aplicava; o de 24/11 (Release 2) aplica.
- FATO, app: o codec escreve `indDest` dentro de IBSCBS (`src/mid/sefin/leiaute/prodrest_20261020/grupo_ibscbs.cpp:31`) e só quando o grupo existe (`codec_dps.cpp:74-76`); a raiz não tem o campo (`codec_dps.cpp:51-78`). O domínio guarda o campo em `Ibscbs` (`src/back/include/nl/back/dps/ibscbs.h:29,34,41-42,49`; `src/back/domain/dps/ibscbs.cpp:25-38`) e `DadosDps` não o tem (`src/back/include/nl/back/dps/dados_dps.h:33-48`). A CLI lê `indDest` dentro de `ibscbs` (`src/mid/composicao/ler_ibscbs.cpp:48-54,76`).
- FATO, medido com `xmllint --nonet --schema xsd_20261124/DPS_v1.01.xsd`: as 10 fixtures válidas de `tests/fixtures/dps/` e `tests/fixtures/xsd/valida_dps_minima.xml` **reprovam**, todas com "Element 'serv': This element is not expected. Expected is one of ( toma, indDest )" (ou "Expected is ( indDest )" quando há `toma`). Com `indDest` movido para depois de `toma` (ou de `prest`, sem tomador) e tirado de IBSCBS, por script em cópias (`scratchpad/fixtures_movidas/`), **as 11 passam**, e as 5 inválidas reprovam pelo motivo de antes (xDescServ 2001, só espaços, campo obrigatório ausente, Id fora do pattern, XML malformado). Conclusão medida: para o que as fixtures cobrem, a única quebra de esquema é o `indDest`.
- Efeito na emissão de teste: `tools/producao_restrita/emitir_teste.sh:273-299` monta a DPS sem tomador e sem IBSCBS; o XML sai sem `indDest` e reprova no XSD de 24/11. INFERÊNCIA (base em D2 e na seção 2): resolvido o 403, a Sefin recusa com E1235.

**D2. O app valida localmente contra um leiaute que a Produção Restrita não usa mais.**
- FATO: o build fixa o ZIP de 20/10 (`third_party/nfse-xsd/prodrest-20261020/CMakeLists.txt:17-20`; `SHA256SUMS:4-5`, ZIP `25fdcecc...`, 36351 bytes), e a Mid embute o hash do manifesto (`src/mid/composicao/CMakeLists.txt:34`, `NL_XSD_MANIFESTO_SHA256`). A DPS sem `indDest` passa no validador local e no pin, e a Sefin valida pelo de 24/11.
- FATO: Anexo I 24/11, aba "RN_RECEP_DPS", linha Excel 17 (nº 16): "Falha no esquema XML do DF-e.", Rej., **E1235**.
- Efeito no ponto 7 ratificado ("Bloquear o que estiver vencido"): hoje o app está, ele mesmo, com leiaute vencido em Produção Restrita desde 24/11/2026.

**D3. O gov.br tirou do ar o ZIP do XSD de 20/10 e o Anexo I de 20/10.**
- FATO, medido em 27/11/2026: `GET .../esquemas_prod_restrita_20261020.zip` devolve 302 para `https://www.gov.br/nfse/acl_users/credentials_cookie_auth/require_login?came_from=...`; com `-L`, chega uma página HTML de 58116 bytes. O mesmo para o Anexo I de 20/10 (302). As listagens de Produção Restrita e de "Leiaute e esquemas antigos" não citam mais nenhum dos dois.
- Efeito: sem o cache do CI, o configure reprova (`third_party/nfse-xsd/prodrest-20261020/CMakeLists.txt:55-75`, hash divergente vira FATAL_ERROR); o manifesto Flatpak (`flatpak/io.github.petrinhu.notalivre.yml:93-95`) não baixa mais a fonte. A única cópia do ZIP de 20/10 é o cache do GitHub Actions com a chave `xsd-prodrest-20261020-25fdcecc...` (`.github/workflows/ci.yml:157,250,438,948`). É o risco (a) do parecer K2 (`docs/juridico/k2-redistribuicao-xsd-e-tabelas.md:71`, citado no `TODO.md`, NL-I.084) acontecendo: 35 dias depois de publicado, o arquivo sumiu.

### Gravidade 2: DPS que o app aceita e a Sefin recusa

**D4. `indDest = 1` sem o grupo `dest` passa no app e é recusado na Sefin (E0910).**
- FATO, Anexo I 24/11, aba "RN DPS_NFS-e 1.01": linha Excel 299 (nº 296), E0908, "Não é permitido identificar o destinatário quando o indicador do destinatário for igual a zero (indDest = 0).", executada (coluna K = V); linha Excel 300 (nº 297), E0910, "É obrigatório identificar o destinatário quando o indicador do destinatário for igual a um (indDest = 1).", coluna K vazia.
- FATO, app: `IndicadorDestinatario::de(u"1")` aceita (`src/back/domain/dps/indicador_destinatario.cpp:57-58`), o domínio não tem `dest`, e o caso de teste "completa" usa `indDest = 1` (`tests/unit/mid/sefin/casos_dps.h:346,356`; fixture `tests/fixtures/dps/valida_dps_completa.xml:89`). O grupo `dest` está na pendência de Capitolino (NL-I.065, "IBS/CBS especial").
- Leitura conservadora, como a ratificada no ponto 5 para E0244 e E0245: coluna K vazia não quer dizer regra desligada.

**D5. E0390 passou a valer também para o subitem 17.10.**
- FATO, Anexo I 24/11, linha Excel 414 (nº 411): "Se o código de tributação nacional pertencer ao item 12 ou subitem 17.10 da lista de serviços, então o grupo de informações de Atividade/Evento é obrigatório." Em 20/10 era só o item 12 (plano, `docs/superpowers/plans/2026-10-25-notalivre-plano.md:383`, "mesma regra (item 12 exige evento)"; cabeçalho `src/back/include/nl/back/regras/regra_e0390.h`). NT 009 §2.14 cita os subitens 12.13 e 17.10 (`docs/pesquisa/ibscbs.md`, §2.14).
- FATO, app: `src/back/domain/regras/regra_e0390.cpp` só confere `startsWith("12")`. Um cTribNac `1710xx` passa localmente e cai na Sefin.

**D6. Códigos novos sem mensagem própria no app.**
- FATO, Anexo I 24/11: E0244 (linha Excel 284, nº 281) e E0245 (285, nº 282), colunas J e K vazias; E0921 (311, nº 308) e E0922 (312, nº 309), nível 1, coluna K vazia (o `contador_6_4.md` dizia "executadas"; na coluna K, que é a do emitente prestador, estão vazias; a diferença não muda a decisão, já ratificada como rejeição ativa); E0653 do tomador (286) e E0924 do destinatário (313), E0238 (283) e E0920 (310); e E1235 (RN_RECEP_DPS, linha 17).
- FATO, app: o catálogo tem 7 entradas (`src/mid/sefin/cliente/catalogo_erros_sefin.cpp:16-24`); código fora dele recebe "A Sefin recusou o envio sem um código conhecido pelo app." (`:26`). O fluxo de recusa já está certo para a "rejeição ativa": 4xx com código E válido vira `MotivoFalhaSefin::Rejeitada`, sem nova tentativa e sem consulta (`src/mid/sefin/cliente/cliente_sefin.cpp:395-410`), exceto E0014.

**D7. E0238 (município do tomador inexistente no IBGE) dá para checar sem a Sefin e não é checada.**
- FATO, Anexo I 24/11, linha Excel 283 (nº 280): "O código do município informado para o endereço do tomador do serviço não existe, conforme tabela de municípios do IBGE.", caminho `infDPS/toma/end/endNac/cMun`, nível 1, K = V.
- FATO, app: a porta de catálogo já responde `municipioExiste` (usada por `regra_e0302.cpp`, registrada em `validador_local.cpp:35`) e nenhuma regra confere o `cMun` do tomador. E0244 e E0245 (cadastro CNPJ e CPF na data de competência) e E0653 (CEP no município) não dão para checar localmente: não há base de CEP nem consulta ao cadastro (plano, `:388`, E0653 "fica para a Sefin").

### Gravidade 3: média, informativa ou documental

**D8. cIndOp: a Produção Restrita ainda usa a tabela antiga até 07/12/2026; o app só confere o formato.**
- FATO, medido nesta rodada: Anexo VII v1.03.00 tem 40 códigos; a aba "CÓDIGO_INDICADOR_OPERAÇÃO_IBS" do Anexo I 24/11 tem 32; só no VII: 010104, 010105, 010201, 010202, 020102, 090101, 090102, 110101, 110201, 120101, 130101, 130201; só no Anexo I: 030103, 030104, 050103, 050104 (bate com `contador_6_4.md`, ponto 3). Release 3 (v1.03.00 nos dois ambientes) em 07/12/2026 (notícia de 25/11/2026, lida em `contador_6_4.md`).
- FATO, app: `src/back/domain/dps/codigo_indicador_operacao.cpp:16` confere só `[0-9]{6}`. As fixtures usam `000001`, que não está em nenhuma das duas tabelas (medido): uma DPS com IBSCBS assim levaria E0901 na Sefin (Anexo I 24/11, linha Excel 605). A emissão de teste não manda IBSCBS (`emitir_teste.sh:273-299`), então não é afetada.

**D9. Linhas novas e opcionais na raiz de infDPS, sem efeito na v1.** `cMotivoEmisTI` (XSD `:798`; Anexo I linha 112) e `chNFSeRej` (XSD `:815`; linha 114), 0-1, só para tpEmit 2 ou 3; o app emite só tpEmit 1 (DA-1.3). `prest` aceita NIF, cNaoNIF e CAEPF (`TCInfoPrestador`, XSD `:911-985`): superconjunto do que o codec escreve. NÃO MEDIDO: se isso já estava em 20/10 (sem o arquivo antigo).

**D10. Release 4 ainda não está no XSD de 24/11 (confirma a Onda 5).** FATO: `TSOpSimpNac` continua 1, 2 e 3 (`tiposSimples_v1.01.xsd:1029`); zero ocorrências de `regApIBSCBSSN`, `cAtvSN`, `gTribSN`, `gPgtoVinc`, `gIBSCBSAjuste` em `tiposComplexos`. `condominios` (`:2585`), `bensMoveis` e `indFinal` existem e são 0-1 (o app não os emite).

**D11. Todas as "linhas Excel" citadas no código mudaram.** O Anexo de 24/11 inseriu linhas. Medido, 20/10 para 24/11: E0202 257 para 259; E0204 258 para 260; E0237 277 para 279; E0241 278 para 280; E0246 284 para 288; E0302 333 para 365; E0313 340 para 372; E0390 380 para 414; E0532 484 para 521; E0583 513 para 550; E0850 566 para 599; E0903 574 para 608; E0905 576 para 610; E0906 577 para 611; E0188 254 para 256; E0206 260 para 262; E0676 539 para 572; E0710 565 para 598. Textos conferidos sem mudança de efeito para tpEmit 1: E0202, E0204, E0241, E0246, E0302, E0313, E0532, E0583, E0850, E0903, E0905, E0906; E0237 ganhou "exceto se o emitente da DPS é o tomador do serviço (tpEmit = 2)", sem efeito. Os cabeçalhos `src/back/include/nl/back/regras/regra_e*.h` e as mensagens em `src/back/domain/regras/regra_e*.cpp` citam "linha Excel" e "Anexo I de 20/10/2026".

**D12. Contagem de códigos.** FATO: a aba RN de 24/11 tem 476 códigos distintos em 477 linhas com código (E0653 aparece duas vezes, linhas 286 e 555, como já em 20/10). O plano registra 466 distintos em 20/10 (`plano:363`). Saldo +10: identificados E0244, E0245, E0921, E0922. NÃO MEDIDO: os outros 6 (o Anexo de 20/10 saiu do ar; a cópia de `/var/tmp/notalivre-nl111/` é da máquina do líder).

**D13. Códigos citados no app e ausentes no Anexo I 24/11.** E1204 (comentário de `catalogo_erros_sefin.cpp:5`) não está na aba RN_RECEP_DPS de 24/11, que tem E1200, E1203, E1205 a E1209, E1225, E1226, E1228, E1229, E1235 e E1242. E2404 (`catalogo_erros_sefin.cpp:23`) não está no Anexo I (é código da API de consulta, já marcado como INFERÊNCIA no próprio arquivo).

**D14. Incoerência interna do Anexo I 24/11.** A aba "CÓDIGO_INDICADOR_OPERAÇÃO_IBS" ainda cita `NFSe/infNFSe/DPS/infDPS/IBSCBS/indDest` (por exemplo, a linha do código 010103), caminho que o XSD e a aba LEIAUTE do mesmo arquivo já mudaram. Vale o XSD. Sem efeito no código; registro.

**D15. Produção (tpAmb 1) continua noutro leiaute.** FATO: no ZIP de Produção de 09/02/2026, `Schemas/1.01/tiposComplexos_v1.01.xsd` tem `finNFSe`, `cIndOp` (1-1) e `indDest` dentro de IBSCBS e não tem `finNFSe` na raiz. O codec do app não serve para Produção hoje nem serviu; a Produção está travada (`src/mid/composicao/politica_emissao.cpp:60`, `producaoLiberada`) até a NL-12. INFERÊNCIA: em 07/12/2026 a Produção recebe os Releases 1 a 3, e um XSD de Produção novo deve sair; não publicado na data da leitura.

---

## 2. O 403 de hoje: a mudança de leiaute explica?

**Resposta curta: não. A mudança de leiaute explica o que vem depois do 403, não o 403.**

Base:
1. FATO: as quatro recusas por leiaute e por regra da Onda 2 vieram como HTTP 400, `application/json`, lista `erros` com `Codigo` (`docs/pesquisa/onda2_producao_restrita.md:35-38`, E0116, E0312 e E0712).
2. FATO: o Anexo I 24/11, aba "RN_RECEP_DPS", põe a falha de esquema como regra com código (linha Excel 17, nº 16, E1235, Rej.), e as regras do certificado de transmissão vêm antes, nas linhas 2 a 9 (E1200, E1203, E1205 a E1209), também com código.
3. FATO: o app não manda a DPS se o A1 estiver vencido, ainda não válido, com data ilegível ou sem `clientAuth` (`src/foundation/pkcs12/carregador_pkcs12.cpp:28-42` e a função `verificarEku` logo abaixo). Um 403 depois do POST quer dizer que, pelo relógio da máquina, o certificado estava dentro da validade.
4. FONTE SECUNDÁRIA (resumo do buscador; o fórum do Projeto ACBr não abriu, host `www.projetoacbr.com.br` fora da rede da sessão): a tabela de status do Swagger da Sefin Nacional diria "403 Certificado digital da transmissão inválido ou fora dos padrões da NFS-e". Os manuais oficiais lidos (API do Emissor Público, contribuintes v1.0 e municípios v1.2 de 23/11/2026) não trazem tabela de status HTTP; o Swagger da Sefin de Produção Restrita responde 403 a agentes (`docs/pesquisa/onda2_api_sefin.md:64`).

INFERÊNCIA: o 403 sem código E vem de antes da aplicação que valida a DPS (porta de entrada, mTLS ou autorização do certificado), porque um erro de esquema tem código (E1235) e as recusas de aplicação da Onda 2 tinham código e status 400. Causas candidatas, nenhuma medida: certificado revogado (o app não consulta CRL nem OCSP, NF-06); cadeia ou AC não aceita pela porta de entrada; certificado de outra pessoa que não a do prestador; bloqueio da porta de entrada (o próprio CI já levou 403 do gov.br por cliente, `.github/workflows/ci.yml`, comentário do passo do ZIP: "o urllib do Python levou 403 do gov.br"); habilitação do contribuinte em Produção Restrita. O contador (série 00001, nDPS 1) não explica um 403: número repetido dá E0014, que o app reconcilia (`cliente_sefin.cpp:392-403`).

**O que é preciso para a emissão de teste voltar a ser aceita:**
1. Fora da fatia, com o líder: achar a causa do 403 (validade e revogação do A1 no site da AC, o CNPJ ou CPF do certificado contra o `--cnpj`/`--cpf` do script, outra máquina ou rede). Se quiser medir de qual camada vem, ver a pergunta 4 (registrar o tipo e o tamanho do corpo do 4xx sem código, sem o corpo).
2. Na fatia: `indDest` na raiz (D1) e o XSD de 24/11 embutido (D2). Sem isso, resolvido o 403, a próxima resposta esperada é E1235 (INFERÊNCIA pela base 1 e 2 acima).
3. Na fatia: E1235 e os códigos de D6 com mensagem própria, para o próximo erro sair claro.
4. A DPS do script fica com `<indDest>0</indDest>` depois de `prest` (sem tomador). E0908 não dispara (sem `dest`); E0243 e E0239 só valem com IBS/CBS pela calculadora, que a DPS de teste do MEI não manda em 2026.

---

## 3. O que entra na fatia e o que fica na Onda 5

**Entra na NL-3.L** (o que faz a DPS de hoje passar no leiaute em vigor na Produção Restrita e recusa localmente o que a Sefin já recusa):

| # | Item | Diferença | Motivo |
|---|---|---|---|
| E1 | XSD de 24/11 no build, no manifesto e no CI | D2, D3 | sem ele o validador local aprova o que a Sefin recusa (ponto 7) e o build depende de um cache |
| E2 | `indDest` na raiz: domínio (`DadosDps`), codec e leitor da CLI | D1 | quebra de esquema em toda DPS |
| E3 | Regra local E0910: `indDest = 1` recusado enquanto o app não tiver o grupo `dest` | D4 | mesma forma da E0390 de hoje (recusa até o grupo existir) |
| E4 | E0390 também para o subitem 17.10 | D5 | texto de 24/11 |
| E5 | Regra local E0238 (município do tomador no IBGE), pela porta que já existe | D7 | checável sem a Sefin, custo baixo |
| E6 | Catálogo: mensagens próprias para E0244, E0245, E0921, E0922, E0653, E0924, E0238, E0920, E0910, E0908, E1235, E1200, E1209 e E0901 (esta sugere atualizar o app e as tabelas) | D6, D8, D13 | ponto 5 ratificado: recusa ativa com texto que diz o que corrigir |
| E7 | Fixtures, dourado de digest e e2e regenerados; `emitir_teste.sh` com o `indDest` | D1 | o teste de igualdade fixture-codec e o dourado dependem do XML |
| E8 | Comentários "linha Excel" e "Anexo I de 20/10/2026" trocados pelas linhas de 24/11 | D11 | fonte citada certa (L-18) |
| E9 | Pisos e portões (seção 5) | todos | teste novo sem piso atualizado reprova |

**Fica na Onda 5**, com o motivo:

| Item | Destino | Motivo |
|---|---|---|
| Tabela de cIndOp em dado, por ambiente e vigência (32 códigos até 07/12, v1.03.00 depois) e E0901 local | NL-5.3, NL-5.6 | decisão do líder: "tabelas em dado ficam na Onda 5"; a emissão de teste não manda IBSCBS (D8) |
| CEP contra o IBGE antes de emitir (E0653 local) | NL-5.2 | precisa de base de CEP, que o app não tem (plano `:388`) |
| E0244, E0245, E0921, E0922 locais | NL-6.3 (consulta de CNPJ) e NL-5.x | dependem do cadastro CNPJ e CPF na data de competência; na NL-3.L só a mensagem |
| Grupo `dest` e E0919 a E0930 | pendência de Capitolino, NL-I.065 | o grupo não está no modelo; até lá E3 recusa `indDest = 1` |
| Release 4 (opSimpNac 4, regApIBSCBSSN, gTribSN, cAtvSN) | NL-I.386 e NL-I.388 | não está no XSD de 24/11 (D10); vai a 24/12/2026 |
| Grupo IBS/CBS obrigatório em 2027 | NL-I.387, NL-5.6 | decisão do líder |
| Detector automático de "leiaute ou tabela vencidos" com bloqueio | NL-5.3 com NL-5.1 | precisa ler a listagem da Biblioteca (pesquisa (a) da Onda 5) e datas em dado (NL-I.389); a NL-3.L tira o app do vencimento de hoje trocando o leiaute. Ver pergunta 5 |
| Leiaute de Produção | NL-12 | Produção travada (D15) |

---

## 4. Testes primeiro (L-35, L-36)

Cada teste nasce vermelho antes do código, e a sabotagem mostra que ele morde. O executor grava o vermelho no relatório (rodada local opcional em contêiner, NF-04; o CI seletivo é o portão). Base do seletivo: último SHA verde da `onda-3` (NF-04). Como a fatia muda `CMakeLists.txt` e o manifesto, roda tudo.

### 4.1 Back (domínio e regras), `tests/unit/back/`

| Teste (arquivo) | Asserção | Sabotagem que fica vermelha |
|---|---|---|
| `tst_dps.cpp::indDestFicaNaRaizDaDps` | `Dps::montar` com `DadosDps{..., indDest = 0, ibscbs = nullopt}` dá certo e `dps.dados().indDest.valor() == tomadorEDestinatario` | montar ignorando o campo e devolvendo sempre 1 |
| `tst_dps.cpp::indDestUmSemIbscbsMontaNoDominio` | o domínio aceita 1 (a recusa é da regra E0910, não do tipo) | tipo que recusa 1 |
| `tst_ibscbs.cpp` (os 7 casos de `:102-195`) | continuam verdes sem o parâmetro `indDest` | não se aplica (só muda a assinatura) |
| `regras/tst_regra_e0910.cpp::indDestUmSemDestRejeitadoPelaE0910` | `verificar` devolve `Erro{"E0910", "Dps.infDPS.dest", ...}` | `verificar` devolvendo `nullopt` |
| `regras/tst_regra_e0910.cpp::indDestZeroPassaNaE0910` | `nullopt` | regra que dispara para qualquer valor |
| `regras/tst_regra_e0390.cpp::subitem1710SemAtvEventoRejeitadoPelaE0390` | cTribNac `171001`: código `E0390` | voltar o teste a `startsWith("12")` |
| `regras/tst_regra_e0390.cpp::subitem1711NaoDisparaE0390` | cTribNac `171101`: `nullopt` (fronteira) | trocar para `startsWith("17")` |
| `regras/tst_regra_e0390.cpp::item12ContinuaRejeitado` | cTribNac `120101`: `E0390` | tirar o ramo do item 12 |
| `regras/tst_regra_e0238.cpp::municipioDoTomadorInexistenteRejeitadoPelaE0238` | catálogo falso sem `9999999`: `Erro{"E0238", "Dps.infDPS.toma.end.endNac.cMun"}` | regra que não consulta o catálogo |
| `regras/tst_regra_e0238.cpp::municipioDoTomadorExistentePassa` | catálogo com o código: `nullopt` | regra que sempre recusa |
| `regras/tst_regra_e0238.cpp::tomadorNoExteriorOuSemEnderecoNaoDisparaE0238` | `endExt` e tomador sem `end`: `nullopt` | regra que recusa quem não tem `endNac` |
| `regras/tst_validador_local.cpp::validadorPadraoAplicaE0238EE0910` | uma DPS que viola as duas sai com os dois códigos, sem parar na primeira | tirar o `push_back` de uma delas em `validador_local.cpp` |

### 4.2 Mid (codec e XSD), `tests/unit/mid/sefin/`

| Teste | Asserção | Sabotagem |
|---|---|---|
| `tst_codec_dps.cpp::indDestNaRaizDepoisDoToma` | leitura por `QXmlStreamReader`: o irmão seguinte de `toma` é `indDest`, e o de `indDest` é `serv` | escrever `indDest` antes de `toma` |
| `tst_codec_dps.cpp::indDestDepoisDoPrestSemTomador` | sem tomador: o irmão seguinte de `prest` é `indDest` | escrever só quando há tomador |
| `tst_codec_dps.cpp::ibscbsSemIndDest` | o grupo IBSCBS não tem filho `indDest` | manter a linha de `grupo_ibscbs.cpp:31` |
| `tst_codec_dps.cpp::dpsMeiSemIbscbsLevaIndDestZero` | MEI mínima: `indDest` = `0` e valida no XSD de 24/11 | escrever `indDest` só com IBSCBS (como hoje) |
| Os 9 `...ValidaNoXsd` de hoje (`tst_codec_dps.cpp:98-121`) | verdes no XSD de 24/11 | **é o vermelho de estreia da troca de XSD**: com o XSD novo e o codec velho, todos reprovam (medido em D1) |
| Fixture negativa nova `tests/fixtures/xsd/invalida_inddest_no_ibscbs.xml` | reprova no validador próprio e no `xmllint` (oráculo, `tests/unit/foundation/xml/oraculo_xmllint.sh`) | apontar o teste para o XSD de 20/10: a fixture passa (prova que o XSD trocou) |
| `fixturesDpsIguaisAoCodec` (`tst_codec_dps.cpp:131`) | igualdade byte a byte depois de regenerar por `tests/tools/gera_fixtures_dps` | editar uma fixture à mão |
| `tests/unit/foundation/xml/hash_zip` (controle e adulterado) | o configure aceita o ZIP de 24/11 e aborta com o ZIP adulterado | já existe; muda só o ZIP certo |
| `tst_validar_xml.cpp` (pin do manifesto) | pin novo aceito e manifesto alterado recusado | já existe |

### 4.3 Mid (catálogo e cliente)

| Teste | Asserção | Sabotagem |
|---|---|---|
| `cliente/tst_catalogo_erros_sefin.cpp::codigosDoEnderecoTemMensagemPropria` (dado por linha: E0244, E0245, E0921, E0922, E0653, E0924, E0238, E0920) | `conhece()` verdadeiro e `mensagem() != kGenerica` | tirar uma entrada |
| `...::e1235DizQueOLeiauteMudou` | texto pede atualizar o programa | entrada sem o texto |
| `cliente/tst_cliente_sefin.cpp::rejeicaoE0244NaoRepeteNemConsulta` (servidor falso, `tools/servidor_falso`) | POST 400 `{"erros":[{"Codigo":"E0244"}]}`: `MotivoFalhaSefin::Rejeitada`, código `E0244`, 1 POST e 0 GET no log | tratar 4xx com código como transitório |
| `...::status403SemCodigoEDefinitivoSemCodigo` (se não existir já) | 403 com corpo HTML: `Definitiva` com `RespostaSemCodigoValido`, sem nova tentativa | tratar 403 como transitório |

### 4.4 Front e e2e

| Teste | Asserção | Sabotagem |
|---|---|---|
| `tests/e2e/dps_validar::indDestNaRaizDoJson` | JSON com `"indDest": "0"` na raiz gera XML com `indDest` na raiz | leitor que ignora a chave da raiz |
| `...::indDestDentroDeIbscbsRecusado` | `ibscbs.indDest` sai com código de saída 4 e campo `ibscbs.indDest` na mensagem | leitor que aceita calado |
| `...::semIndDestUsaZero` (só se a pergunta 2 ficar na opção recomendada) | JSON sem `indDest` gera `0` | leitor que exige a chave |
| As 15 entradas de `tests/e2e/dps_validar/entradas/*.json` com `"indDest"` dentro de `ibscbs` | migradas para a raiz | não se aplica |
| `tools/producao_restrita/emitir_teste.sh --ensaio` | a DPS do ensaio valida no XSD de 24/11 (passo de validação no próprio ensaio, em contêiner) | ensaio com o XSD velho passa, com o novo reprova |

---

## 5. Corte em átomos (NF-32), executores, revisores e ordem

Uma função faz uma coisa; um arquivo junta só funções do mesmo assunto. Executor e revisor distintos (L-12); o main orquestra e não executa (L-10). Teto de 4 agentes, sem disputa de arquivo ou de trava (NF-07). Um commit para a fatia, com o `Status` do `TODO.md` em "Pendente verificação" (NF-08, NF-21), autor e SPDX da NF-10.

| Passo | Átomo | Arquivos (arquivo:linha de hoje) | Executor | Revisor |
|---|---|---|---|---|
| 0 | Leitura, sem código: o executor rebaixa os 4 arquivos do gov.br da seção 0, confere SHA-256 e tamanho, e confere cada linha da seção 1 no XSD e no XLSX. Divergência para a fatia e volta ao CTO | nenhum | backend-engineer | qa-engineer |
| 1 | Pasta do leiaute renomeada (conforme pergunta 1): `git mv` de `src/mid/sefin/leiaute/prodrest_20261020`, `src/mid/include/nl/mid/sefin/leiaute/prodrest_20261020` e `third_party/nfse-xsd/prodrest-20261020`, namespace e includes; sem mudança de comportamento; verde no XSD de 20/10 (cache do CI) | 34 arquivos, 108 linhas com `20261020` fora de `docs/` (medido por `git grep -c`), entre eles `src/mid/sefin/CMakeLists.txt:6-11`, `src/foundation/xml/CMakeLists.txt:21-22`, `tests/unit/mid/sefin/CMakeLists.txt:9`, `tests/tools/gera_fixtures_dps/CMakeLists.txt:7`, `src/mid/composicao/politica_emissao.cpp:36-39`, `src/mid/sefin/cliente/cliente_sefin.cpp`, `src/mid/sefin/assinatura/assinador_dps_sefin.cpp:25-27`, `src/mid/sefin/cliente/comparador_infdps_c14n.cpp:16` | backend-engineer | tech-lead |
| 2 | XSD de 24/11: `SHA256SUMS` novo (URL, ZIP `1ca83c1a...` com 39728 bytes, as duas linhas novas da tabela da seção 0); nome e URL do ZIP no CMake | `third_party/nfse-xsd/prodrest-<nova>/SHA256SUMS:4-5,13,15`; `.../CMakeLists.txt:17-19` | devops-sre | qa-engineer |
| 2a | Vermelho de estreia: com o passo 2 e o codec velho, os testes de 4.2 marcados reprovam (lista no relatório) | só leitura do log | devops-sre | qa-engineer |
| 3 | CI: `NL_XSD_ZIP_SHA256` e `NL_XSD_ZIP_URL` (`ci.yml:23-25`), as 4 chaves de cache (`:157,250,438,948`), os nomes do ZIP (`:167,256,444,954`) e os caminhos do manifesto (`:261,449,958`); o job `xsd-cache` (`:137-170`) passa a baixar no cache miss, como o `nativo` (`:252-280`), no lugar de `fail-on-cache-miss: true` (`:158`), senão o primeiro run com chave nova reprova o `xsd-cache` e o `banco` (`needs`, `:509`) não roda; `tools/ci/zip_xsd.sh:11-12` | `.github/workflows/ci.yml`, `tools/ci/zip_xsd.sh` | devops-sre | qa-engineer |
| 4 | Manifesto Flatpak: `url`, `sha256` e `dest-filename` da fonte do ZIP (`flatpak/io.github.petrinhu.notalivre.yml:93-95`); o passo do CI que põe o ZIP em `.flatpak-builder/downloads/<sha>/` (`ci.yml:948-975`) | manifesto, `ci.yml` | devops-sre | qa-engineer |
| 5 | Domínio: `indDest` em `DadosDps` (`dados_dps.h:33-48`), fora de `Ibscbs` (`ibscbs.h:29,34,41-42,49`; `ibscbs.cpp:25-38`); comentário de `indicador_destinatario.h:3,16` | Back | backend-engineer | qa-engineer |
| 6 | Codec: função `escreveIndicadorDestinatario` (um assunto) chamada em `escreveInfDps` depois do tomador (`codec_dps.cpp:69-72`); tirar `grupo_ibscbs.cpp:31` e ajustar `grupos.h:38` e o comentário de `grupo_ibscbs.cpp:3` | Mid | backend-engineer | qa-engineer |
| 7 | Leitor da CLI: função `lerIndicadorDestinatario` num arquivo próprio `src/mid/composicao/ler_indicador_destinatario.cpp` (raiz do JSON, conforme pergunta 2); `ler_ibscbs.cpp:48-54` recusa a chave antiga; `ler_dados_dps.cpp` preenche `DadosDps.indDest` | Mid (composicao) | backend-engineer | qa-engineer |
| 8 | Regras: `regra_e0910.{h,cpp}` e `regra_e0238.{h,cpp}` novas; `regra_e0390.cpp` com 17.10; registro em `validador_local.cpp:30-42` | Back | backend-engineer | qa-engineer |
| 9 | Catálogo: entradas novas em `catalogo_erros_sefin.cpp:16-24` (o tamanho do `std::array` muda) e o comentário `:3-7` com as linhas da aba RN_RECEP_DPS de 24/11 | Mid | backend-engineer | qa-engineer, mais revisão de texto (NF-18) |
| 10 | Fixtures: regenerar `tests/fixtures/dps/*.xml` e `tests/fixtures/xsd/valida_dps_minima.xml` por `tests/tools/gera_fixtures_dps`; nova `invalida_inddest_no_ibscbs.xml`; `casos_dps.h:201-214,346-356` e `tests/unit/back/regras/fabrica_dps.h:324-336` (o caso "completa" passa a `indDest = 0`, porque 1 sem `dest` é recusado por E3); dourados `tests/fixtures/xmldsig/digest_infdps_sha1.txt` e `..._sha256.txt` recalculados pelo `xmlsec1` (oráculo independente; a referência é `valida_dps_mei_minima.xml`, `tst_assinador_xml.cpp:275-277`); 15 JSON de `tests/e2e/dps_validar/entradas/` | testes | qa-engineer (gera) | backend-engineer (confere) |
| 11 | Script de teste: `"indDest": "0"` na raiz do JSON de `montar_dps` (`emitir_teste.sh:273-299`) se a pergunta 2 não ficar no padrão; ensaio valida no XSD novo | `tools/producao_restrita/emitir_teste.sh` | devops-sre | qa-engineer |
| 12 | Comentários: "linha Excel" e "Anexo I de 20/10/2026" nos 12 cabeçalhos `src/back/include/nl/back/regras/regra_e*.h`, nas mensagens dos `regra_e*.cpp` (por exemplo `regra_e0202.cpp`, "linha Excel 257") e em `ibscbs.cpp:3`, pela tabela D11; `limites_leiaute.h:3,296-300` com o XSD novo | Back, Mid | backend-engineer | qa-engineer |
| 13 | Pisos (seção 6) no mesmo diff | `tools/conteiner/piso_testes.txt`, `flatpak/piso_suite.txt` | qa-engineer | tech-lead |
| 14 | Documentos: spec 4.11 (linha do XSD de 20/10), plano NL-1.6 e NL-1.5 (linhas da tabela), `TESTES.md`, `TODO.md` (linha nova NL-3.L; NL-I.385 aponta para a NL-3.L na parte de codec, regras e mensagens) | docs | main delega a quem escreve docs | qa-engineer |
| 15 (opcional, pergunta 4) | Log do 4xx sem código: campos `tipo_conteudo` e `bytes_corpo` na allowlist (`src/mid/sefin/cliente/allowlist_log_sefin.cpp`), nunca o corpo | Mid | backend-engineer | security-engineer (Narciso), com job `mutacao` (NF-12) |

Ordem: 0, 1, 2, 2a, 3 e 4 (devops em paralelo com 5 a 8 do backend, arquivos disjuntos), 9, 10, 11, 12, 13, 14. O passo 1 fica separado para o diff mecânico não esconder comportamento. Paralelo possível: devops-sre (2 a 4, 11) e backend-engineer (5 a 9, 12), dois agentes; qa-engineer entra no 10 e revisa; um pesado por vez na trava (NF-07).

Ordem de serviço de cada executor, pela regra do CTO: arquivos permitidos e proibidos (os da linha dele; proibido tocar `src/foundation/pkcs12`, `src/mid/cofre`, `src/foundation/persistencia` e qualquer SQL), comportamento esperado (a linha), teste (seção 4), pronto (verde no CI do SHA empurrado, lido pelo MCP do GitHub, NF-09), proibições (sem `float`, sem texto com travessão, sem portão novo, sem apagar arquivo sem caminho conferido, sem `flatpak run` fora do Builder), relatório em até 6 linhas.

---

## 6. Efeito em pisos, portões e na emissão de teste

- **Piso do nativo** (`tools/conteiner/piso_testes.txt`, hoje 462, conferido por `tools/conteiner/piso_testes.sh` com `ctest -N`): sobe pelo número de testes novos registrados (seção 4: 12 no Back, 4 a 5 no codec, 3 a 4 no catálogo e no cliente, 2 a 3 no e2e; o executor conta pelo `ctest -N` e põe o número exato no mesmo diff). É o mesmo valor nas 4 distros da matriz, porque nenhum teste novo depende de distro.
- **Suíte Flatpak** (`flatpak/piso_suite.txt`, hoje 404, contagem exata, `flatpak/suite_flatpak.sh:9-71`): sobe pelo mesmo número dos testes que entram no build Flatpak (os que não dependem de banco nem de servidor falso; o executor mede no espelho ou no job `flatpak`). A sabotagem de estreia do piso (`flatpak/io.github.petrinhu.notalivre.yml:77-81`) continua igual.
- **Portão de funções no CTest** (`tools/portoes/funcoes_no_ctest.py`): cada slot novo tem de estar registrado; se faltar, reprova. Sem exceção nova.
- **Pontos de parse** (`tools/portoes/pontos_de_parse.txt`): sem mudança. O codec escreve com `QXmlStreamWriter`; as regras novas não fazem parse; nenhuma chamada nova da libxml2 em `src/`.
- **Camadas** (`tools/portoes/camadas.py`): `regra_e0238` usa a porta de catálogo do Back (como `regra_e0302`); o leitor novo da CLI fica em `src/mid/composicao`; nada sobe de camada.
- **Pin do XSD na Mid** (`src/mid/composicao/CMakeLists.txt:34`): muda sozinho com o `SHA256SUMS` novo; os testes de pin continuam provando a recusa de outro hash.
- **XSD no manifesto e no cache do CI**: chave nova `xsd-prodrest-20261124-1ca83c1a...`; primeiro run é cache miss e baixa do gov.br (o job `nativo` já tenta 5 vezes, `tools/ci/baixar_zip_xsd.cmake`); o `xsd-cache` precisa do passo 3, senão reprova pelo `fail-on-cache-miss`. O ZIP de 20/10 fica só no cache antigo, que o GitHub apaga sem uso (INFERÊNCIA: 7 dias sem acesso, regra do GitHub Actions, não conferida nesta rodada).
- **Dourado de digest SHA-1 e SHA-256** (NF-31): muda porque a DPS de referência muda; o arnês SHA-1 da ADR-0007 continua; o valor novo vem do `xmlsec1`, não do próprio assinador.
- **Mutação** (NF-12): não roda, salvo o passo 15.
- **NF-17**: nenhuma das sete classes; a Fase 0 não contou.
- **Emissão de teste**: depois da fatia, a DPS do script leva `indDest` 0 e valida no XSD em vigor; o próximo `emitir_teste.sh` só é aceito se a causa do 403 (seção 2) também tiver sido resolvida. Recomendo ao líder, depois do merge, um teste com a mesma série e o próximo número do contador, e um segundo com cTribNac `1710xx` só no ensaio (recusa local por E0390, sem envio).
- **Prazo**: Release 2 e Release 3 em Produção em 07/12/2026 (cronograma "inicial", `contador_6_4.md`). A NL-3.L não depende disso, porque a Produção Restrita já roda o leiaute de 24/11.

---

## 7. Perguntas ao líder

Cada uma com a opção recomendada primeiro. Sem resposta, a fatia para no passo que depende dela.

1. **Pasta do leiaute.** O ZIP de 20/10 saiu do gov.br e a Produção Restrita não o usa mais.
   - (a) **Renomear a pasta e o namespace para `prodrest_20261124` e apagar a de 20/10 no mesmo commit (Recomendado).** Uma versão em uso, nome que diz a verdade; o passo 1 fica mecânico e separado.
   - (b) Pasta nova ao lado, mantendo a de 20/10. Duplica o codec inteiro e o XSD velho só existe no cache do CI.
   - (c) Atualizar no lugar, sem renomear. Menos diff, mas a pasta passa a ter nome de uma versão que não é a dela.
2. **`indDest` no JSON da CLI** (paridade CLI e GUI, contrato do `notalivre emitir`).
   - (a) **Na raiz, opcional, com padrão "0"; `ibscbs.indDest` recusado com mensagem que aponta a raiz (Recomendado).** As entradas sem tomador nem destinatário, como a do script, seguem iguais; ninguém perde o campo calado.
   - (b) Na raiz e obrigatório. Mais explícito; toda entrada existente quebra até migrar.
   - (c) Aceitar nos dois lugares. Compatível, mas dois caminhos para o mesmo dado.
3. **XSD na árvore (parecer K2, NL-I.077 e NL-I.079)**, agora que o risco (a) aconteceu.
   - (a) **Manter download com SHA-256 e cache do CI nesta fatia, e trazer a decisão do K2 ao líder antes da Onda 9, com o fato de 27/11/2026 (Recomendado).** A fatia não depende do jurídico; o Flathub sim.
   - (b) Pôr o XSD na árvore já nesta fatia, com aviso de licença (opção b do K2). Resolve o sumiço; decide hoje a licença CC BY-ND, que o CLO quer ver antes.
   - (c) Espelho próprio do ZIP (opção c do K2, a do CLO). Precisa de onde hospedar e do aval do CLO.
4. **Diagnóstico do 403.**
   - (a) **Registrar no log só o tipo de conteúdo e o tamanho do corpo de um 4xx sem código, nunca o corpo, com revisão do Narciso e mutação (Recomendado).** Diz se o 403 veio de uma porta de entrada (HTML) ou da aplicação (JSON), sem dado pessoal.
   - (b) Não mexer no log nesta fatia; o líder investiga o certificado fora do app.
   - (c) Guardar o corpo inteiro num arquivo 0600 só em Produção Restrita. Mais informação; risco de privacidade se o corpo trouxer dado.
5. **Bloqueio de leiaute vencido (ponto 7 ratificado).**
   - (a) **A NL-3.L tira o app do vencimento de hoje; o detector automático, com aviso e bloqueio em Produção, fica na NL-5.3 com a NL-5.1, que leem a listagem e guardam datas em dado (Recomendado).**
   - (b) Nesta fatia, uma data de "válido até" escrita na pasta do leiaute, que bloqueia a emissão depois dela. Não acompanha o cronograma "inicial" do Portal e vira data em código, contra a NL-I.389.

---

## 8. O que não consegui medir

- O texto do XSD de 20/10 e do Anexo I de 20/10: o gov.br os tirou do ar (302 para login). A diferença do XSD foi medida pelo efeito nas fixtures; os 6 códigos novos que faltam identificar (D12) e se os campos de D9 já existiam em 20/10 ficam NÃO MEDIDOS. Cópia possível: cache do CI (chave `xsd-prodrest-20261020-25fdcecc...`) e `/var/tmp/notalivre-nl111/` na máquina do líder.
- A causa do 403: sem log da resposta, sem o certificado e sem acesso ao Swagger da Sefin (403 para agentes). A leitura da tabela de status vem de fonte secundária pelo resumo do buscador; `www.projetoacbr.com.br` está fora da rede da sessão e não foi aberto.
- Se E0244, E0245, E0910, E0921 e E0922 estão de fato executadas na Produção Restrita: as colunas de execução estão vazias no Anexo; só a emissão de teste mede.
- O XSD de Produção que entra em 07/12/2026: não publicado na data da leitura.
- Quanto tempo o cache antigo do CI ainda dura (regra de despejo do GitHub não conferida nesta rodada).
