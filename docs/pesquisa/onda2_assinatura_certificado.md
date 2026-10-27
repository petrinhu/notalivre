# Onda 2: pesquisa de abertura, assinatura, certificado e xmlsec1

**Data:** 27/10/2026. **Branch:** onda-2. **Status:** pesquisa, sem código e sem commit.
**Perguntas cobertas:** (a) XMLDSIG da DPS; (d) OIDs ICP-Brasil de CNPJ e CPF; (e) provider legado do OpenSSL 3 para .pfx antigo; (f) diferenças entre xmlsec1 1.2 e 1.3.
**Fora desta ordem:** (b) corpo do POST e GZip, (c) caminho base da API e (g) formato de resposta da Sefin. Essas perguntas ficam em aberto aqui. O arquivo `docs/pesquisa/onda2_api_sefin.md`, que existe no working tree sem commit, é de outra frente.

**Convenção deste documento.** FATO: afirmação com URL e trecho, ou com arquivo:linha, ou com medição de comando (descrita). INFERÊNCIA: conclusão minha a partir de FATOS. NÃO ACHADO: procurei e não encontrei. NÃO MEDIDO ou NÃO LIDO: não executei ou não consegui abrir a fonte.

**Regra do certificado real.** Não procurei, abri, li, copiei nem listei nenhum .pfx/.p12 ou senha do líder. Para medir (e), gerei um certificado de teste descartável, com chave efêmera, dentro do scratchpad. Apaguei as chaves e os .p12 de teste após a medição. Mantive só as saídas de texto.

**Dados de trabalho, fora do repositório:** textos extraídos dos PDFs em `/var/tmp/builds/claude-1000/-home-petrus-IDrive-Documentos-projetos-claudebrain-Projects-nfse/38f29892-da53-4786-aca6-00ba009df597/scratchpad/pdf/` (`nt009.txt` é o Manual Integrado de 2022; `nt009_ou_outro.txt` é a Nota Técnica 009 v1.01; `contrib_emissor.txt`, `contrib_adn.txt`, `espelho_dc_v101.txt`, `in2009_04.txt`). Logs de medição de (e) em `.../scratchpad/pfx-teste-descartavel/`. XLSX do Anexo I lido com `zipfile` em `.../scratchpad/xlsx-prodrest/anexo_i.xlsx`. Texto do DOC-ICP-04 v8.0 em `.../scratchpad/pdf/doc_icp04_v8.txt`.

---

## (a) Algoritmos, canonicalização e posição da assinatura da DPS

### a.1 Manual oficial de 2022 (versão 1.00 do leiaute)

FATO. Fonte: Manual Integrado do Sistema Nacional NFS-e, versão 1.00.02, setembro de 2022, gov.br. URL: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/leiaute-e-esquemas-antigos/manualintegradosnnfse_v1-00-02-producao.pdf. Seção 6.1.4 (p. 24) e tabela XMLDSIG (p. 25).

- Leiaute "subconjunto do padrão XML definido pelo W3C" (texto da p. 25): `SignatureMethod` = `http://www.w3.org/2000/09/xmldsig#rsa-sha1`.
- `CanonicalizationMethod` = `http://www.w3.org/TR/2001/REC-xml-c14n-20010315`.
- `Transforms` (2 transforms, único cada): `enveloped-signature` e `xml-c14n-20010315`.
- `DigestMethod` = `http://www.w3.org/2000/09/xmldsig#sha1`; `DigestValue` descrito como "Hash SHA-1 em Base64".
- Trecho da p. 26: "A assinatura do contribuinte na DPS será feita sobre a TAG infDPS identificada pelo atributo Id". Ou seja, o `Reference` aponta para o `Id` do `infDPS`.
- O `KeyInfo` leva só `X509Data/X509Certificate`. O manual manda não incluir `X509SubjectName`, `X509IssuerSerial`, `X509IssuerName`, `X509SerialNumber` e `X509SKI`, e evitar `KeyValue`, `RSAKeyValue`, `Modulus` e `Exponent`.
- Exemplo do manual: `<Signature>` é irmão de `<infDPS>`, dentro de `<DPS>`.

FATO, divergência interna do manual de 2022. A extração do PDF traz o texto "precedido do literal 'ID,'" e o exemplo traz `URI="#ID ID3106..."`, com espaço e o literal ID repetido. INFERÊNCIA: erro tipográfico de um leiaute 1.00 que já não vale. Para a v1.01 vale o `Id` do XSD, ver a.4.

### a.2 Leiaute v1.01 (atual): o que os documentos oficiais dizem

FATO, XSD. Arquivo local: `/var/tmp/notalivre-pesquisa-onda1/xsd-dl/x/tiposComplexos_v1.01.xsd`, do ZIP oficial de Produção Restrita (SHA-256 `25fdcecc651a67a216496ab10d7566631e6ed10f721e37d62c7845dadb0ea267`, conferido nesta pesquisa). Linhas 742 a 745: `TCDPS` tem `infDPS` e, como irmão, `ds:Signature` com `minOccurs="0"`. Ou seja, a assinatura fica FORA de `infDPS`. Linha 9: `ds:Signature` sem `minOccurs` no tipo da NFS-e (`infNFSe` vem antes na mesma sequência).

FATO, Anexo I v1.01 (Produção Restrita, 20/10/2026). XLSX baixado de https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/anexo_i-sefin_adn-dps_nfse-snnfse_prodrest-v1-01-20261020.xlsx (SHA-256 `72b05981c8882cdd807f3acf901b669465360f9cf1b9d9e235ac4363dacfb2d8`), lido com `zipfile` e `python3 -I`. Busca por `Signature` nas 7 abas:
- Aba 'LEIAUTE DPS_NFS-e 1.01', linha 403: coluna B = `NFSe/infNFSe/DPS/infDPS/`, campo `Signature`, grupo G, ocorrência 0-1. Descrição da mesma linha: "Assinatura XML da NFS-e Segundo o Padrão XML Digital Signature. Obrigatório quando for enviado para API."
- Aba 'LEIAUTE DPS_NFS-e 1.01', linha 404: coluna B = `NFSe/`, `Signature`, 1-1, "Assinatura XML da NFS-e".
- Aba 'RN DPS_NFS-e 1.01', linha 653: coluna B = `NFSe/infNFSe/DPS/`, `Signature`, regra "A assinatura da DPS deve ser válida.", obrigatória, rejeição **E0714** ("Arquivo enviado com erro na assinatura").
- Aba 'RN DPS_NFS-e 1.01', linha 658: `NFSe/`, regra "A assinatura da NFS-e deve ser válida.", rejeição E1630.
Nenhuma linha de `SignatureMethod` nas 7 abas (a busca foi por `Signature`, que cobre esse nome). `CanonicalizationMethod` e `DigestMethod` NÃO foram buscados.

O primeiro achado desta pesquisa veio do dump de onda1 (`prod.json`, numeração própria, linhas 939 e 940), que corresponde às linhas 403 e 404 do XLSX. O caminho é texto literal da célula, não artefato do parser.

**INCONSISTÊNCIA dentro do Anexo I.** A linha 403 (leiaute) põe `Signature` dentro de `infDPS`, mas a descrição fala em NFS-e. A linha 653 (regra de negócio) põe a assinatura da DPS como irmã de `infDPS`, em `NFSe/infNFSe/DPS/`, igual ao XSD. INFERÊNCIA: a linha 403 é provável erro de caminho do leiaute, e XSD e regra de negócio concordam entre si. O caminho do leiaute, porém, é de documento oficial e precisa de confirmação da Sefin.

FATO, espelho municipal de 17/03/2026 (NÃO é fonte oficial da Sefin). URL: https://portalcontribuinte.duquedecaxias.rj.gov.br/pdfs/Manual_integracao_v101.pdf (Prefeitura de Duque de Caxias, "Manual de Integração Webservice NFS-e Padrão Nacional, Versão 1.01, revisada em 17/03/2026"). Seção 7.3.3, tabela XS01 a XS21 (pp. 23 e 24 do PDF): o leiaute de assinatura usa `rsa-sha1` (XS07), `enveloped-signature` e `xml-c14n-20010315` (XS13), `sha1` (XS16), e "Digest Value (Hash SHA-1 em Base64)" (XS17; no original, a expressão tem meia-risca entre "SHA-1" e "Base64", trocada por "em" nesta citação, adaptação). Esse manual trata dos serviços web das administrações municipais, não do ADN/Sefin Nacional.

NÃO ACHADO, nas fontes oficiais lidas. Procurei em: Manual dos Contribuintes, Guia do Emissor Público Nacional (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-emissor-publico-api-sistema-nacional-nfs-e-v1-2-out2025.pdf; texto de 215 linhas, sem seção de assinatura; a única menção a assinatura é de eventos); Manual de Contribuintes das APIs do ADN (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf; sem menção a assinatura, certificado ou algoritmo); Nota Técnica SE/CGNFS-e nº 009 v1.01 de 11/09/2026 (https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/rtc/nota-tecnica-009-se-cgnfs-e-v-1-01.pdf; a busca por assinatura, XMLDSIG e certificado não achou nada, porque trata de IBS/CBS). Não há "manual de assinatura" na página de Produção Restrita nem na documentação atual (listagens de WebFetch de 27/10/2026).

INFERÊNCIA. O leiaute v1.01 da Sefin provavelmente mantém `rsa-sha1`, `sha1` e C14N 20010315, porque o manual oficial de 2022 e o espelho de 2026 concordam. Não há texto oficial v1.01 que confirme isso. Não tenho como afirmar a versão vigente do algoritmo sem a fonte oficial.

### a.3 O XMLDSIG local não restringe os algoritmos

FATO. Arquivo local `xmldsig-core-schema.xsd` (SHA-256 `8c9792a056aedcadfc95bf16f2d7448a35da40df71533e20aea37d0329980c48`, mesmo nas duas cópias locais). Tem 309 linhas. `grep` por `fixed=` não achou nada. `Algorithm` é `type="anyURI" use="required"` em `SignatureMethodType`, `CanonicalizationMethodType` e `DigestMethodType` (linhas 72, 82, 113, 123). `targetNamespace` é `http://www.w3.org/2000/09/xmldsig#`.

FATO, cópia W3C: https://www.w3.org/TR/xmldsig-core/xmldsig-core-schema.xsd (lida por WebFetch, resumo, não cópia byte a byte). Sem `fixed=`, mesmos tipos e `Algorithm` obrigatório. O resumo estimou cerca de 330 linhas.

INFERÊNCIA. O XSD não impõe o algoritmo. A escolha de algoritmo é de documento e de validação na Sefin, não do schema. Isso desfaz a hipótese do advisor de que o XSD restrito fixaria o algoritmo.

NÃO ACHADO. Comparação byte a byte entre o XSD local e o W3C.

### a.4 Id, URI e onde a referência aponta

FATO (leiaute v1.01). Pelo `TSIdDPS` do XSD (`tiposSimples_v1.01.xsd`, linhas 44 a 54, descrito em `docs/pesquisa/onda1_leiaute.md`), o `Id` da DPS começa com `DPS` seguido de 42 caracteres, total 45. O manual de 2022 usa o prefixo `ID`.

FATO, W3C: um `URI` que começa com `#` é referência same-document ao atributo `Id` (https://www.w3.org/TR/xmldsig-core/, seção 4.4.3.2 na numeração da página consultada por WebFetch: "a 'same-document' reference is defined as a URI-Reference that consists of a hash sign ('#')"). Combinado com o `TSIdDPS` do XSD (FATO, acima), a v1.01 leva `URI="#DPS..."` apontando para o `Id` de `infDPS`, e não `#ID...` como no manual de 2022. Esta combinação é lógica sobre duas fontes FATO; nenhum documento v1.01 cita o `URI` diretamente.

### a.5 Decisão que vai ao líder (não decidir sozinho)

Algoritmo de assinatura: os documentos lidos só trazem SHA-1 (`rsa-sha1`, `sha1`). Isso conflita com a política de hash moderno do projeto.

**Recomendação técnica desta pesquisa: seguir o leiaute (`rsa-sha1`, `sha1`).** O verificador é a Sefin, e nenhuma fonte lida aceita outro algoritmo. SHA-256 é hipótese a testar em Produção Restrita, não opção equivalente. A escolha afeta interoperabilidade e segurança, então ela é do líder (L-01) e do main. Esta pesquisa não decide. Leva-se por `AskUserQuestion`, com a recomendação em primeiro lugar.

---

## (d) OIDs ICP-Brasil de CNPJ e CPF no certificado

### d.1 Fonte primária: ITI, IN 04/2009 (DOC-ICP-04.01 versão 2.0)

FATO. URL: https://www.gov.br/iti/pt-br/assuntos/legislacao/instrucoes-normativas/in_n_2009-04.pdf (Instrução Normativa ITI nº 4, de 28/01/2009, aprova a versão 2.0 do documento "Atribuição de OID na ICP-Brasil (DOC-ICP-04.01)"). Texto extraído com `pdftotext -layout`.

- Item 2.3: "Os OID 2.16.76.1.3.n são utilizados para identificação de atributos obrigatórios de certificados... Todos os certificados da ICP-Brasil devem conter este OID."
- `2.16.76.1.3.1`: "campo otherName em certificado de pessoa física, contento os dados do titular (data de nascimento, CPF, PIS/PASEP/CI, RG)".
- `2.16.76.1.3.2`: "campo otherName em certificado de pessoa jurídica, contendo o nome do responsável pelo certificado".
- `2.16.76.1.3.3`: "campo otherName em certificado de pessoa jurídica, contendo o Cadastro Nacional de Pessoa Jurídica (CNPJ) da pessoa jurídica titular do certificado".
- `2.16.76.1.3.4`: "campo otherName em certificado de pessoa jurídica, contendo os dados do responsável pelo certificado de pessoa jurídica titular do certificado (data de nascimento, CPF, PIS/PASEP/CI, RG)".
- `2.16.76.1.3.5` (título de eleitor), `2.16.76.1.3.6` (CEI da PF, 12 posições), `2.16.76.1.3.7` (CEI da PJ).

### d.1b Layout dos campos: DOC-ICP-04 versão 8.0 (FATO)

FATO. DOC-ICP-04 versão 8.0, aprovada pela Resolução CG ICP-Brasil nº 179, de 20/10/2020. Fonte no gov.br: https://www.gov.br/iti/pt-br/assuntos/legislacao/resolucoes/resolucoes-old/Resoluo179Dec10139Etapa2DOC04_assinada.pdf (texto extraído com `pdftotext -layout`). Seção 7.1.2.3, pp. 22 e 23 de 33.
- Pessoa física, campo `2.16.76.1.3.1`: "nas primeiras 8 (oito) posições, a data de nascimento do titular, no formato ddmmaaaa; nas 11 (onze) posições subsequentes, o Cadastro de Pessoa Física (CPF) do titular; nas 11 (onze) posições subsequentes, o ... NIS (PIS, PASEP ou CI); nas 15 (quinze) posições subsequentes, o ... RG; nas 10 (dez) posições subsequentes, ... órgão expedidor do RG e respectiva UF." (reticências substituem texto omitido; o layout é de 8, 11, 11, 15 e 10 posições.)
- Pessoa jurídica, `2.16.76.1.3.3`: "nas 14 (quatorze) posições o número do Cadastro Nacional de Pessoa Jurídica (CNPJ) da pessoa jurídica titular do certificado".
- Pessoa jurídica, `2.16.76.1.3.4` (dados do responsável): mesmo layout de 8, 11, 11, 15 e 10 posições, com CPF do responsável nas posições 9 a 19. `2.16.76.1.3.2`: nome do responsável.
- Equipamento ou aplicação, c.1: `2.16.76.1.3.8` (nome empresarial do CNPJ), depois `2.16.76.1.3.3` (CNPJ de 14 posições).

INFERÊNCIA, de posição: em `2.16.76.1.3.1`, o CPF ocupa as posições 9 a 19 do conteúdo. É aritmética sobre o layout FATO, não medida em certificado real.

NÃO VERIFICADO: se a versão 8.1 (Resolução 196/2021, citada em resultado de busca, não lida) mudou esses campos. A versão vigente não foi lida.

### d.2 Confirmação pelo manual oficial da Sefin

FATO. Manual Integrado 1.00.02 (2022), seção 6.1.3, p. 24: o certificado "deverá conter o CNPJ da pessoa jurídica titular do certificado digital no campo otherName OID=2.16.76.1.3.3 ou o CPF da pessoa física titular do certificado digital no campo otherName OID=2.16.76.1.3.1".

FATO, ponto de contradição interna do manual. Seção 6.1.3, alínea b, p. 24: o certificado "deverá conter a raiz do CNPJ ou CPF do emitente". Seção 6.1.4, p. 24: a DPS deve ser assinada com certificado "que contenha o CNPJ ou CPF do emitente da DPS". A primeira fala em raiz e a segunda em CNPJ inteiro.

### d.3 Versões mais novas do DOC-ICP-04.01

NÃO LIDO. O DOC-ICP-04.01 vigente (v4.0, URL de mailman.iti.gov.br) e as versões do DOC-ICP-04 em mailman.iti.gov.br (v6.4 e v3.2) responderam HTTP 404 ao WebFetch. A versão 8.0 do d.1b é a mais nova lida em texto.

Fonte secundária (resultado de busca, não lido diretamente): a Resolução CG ICP-Brasil nº 196/2021 (versão 8.1 do DOC-ICP-04) e a nº 211/2024 mantêm `2.16.76.1.3.2` e `2.16.76.1.3.3`. NÃO VERIFICADO.

NÃO ACHADO. A convenção de CN contendo o CNPJ. Não pesquisei. Não use o CN como fonte de CNPJ sem conferir a fonte.

### d.4 "CNPJ raiz"

FATO. Manual dos Contribuintes das APIs do ADN, seção 1.1 (p. 1 do PDF): "certificado cujo CNPJ tenha o mesmo CNPJ Raiz do contribuinte". Mesmo manual, parágrafo sobre `NSU`: validação do CNPJ Raiz "entre o parâmetro informado e o CNPJ do certificado utilizado". URL: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/manual-contribuintes-apis-adn-sistema-nacional-nfse.pdf.

NÃO ACHADO. Definição oficial de "raiz". Fonte secundária (blog, resultado de busca): raiz são os oito primeiros algarismos do CNPJ, iguais para matriz e filiais. NÃO VERIFICADO na Receita.

FATO (notícia secundária): o CNPJ alfanumérico mantém 14 posições, e a raiz pode ter letras e números; os dois dígitos finais continuam numéricos. Ver `docs/pesquisa/onda1_cnpj_alfanumerico.md`.

INFERÊNCIA. Comparar a "raiz" tem de ser comparação de string de 8 caracteres, nunca numérica. Se o CNPJ tiver letras, uma conversão para inteiro quebra a comparação.

### d.5 Certificado para transmissão (mTLS)

FATO. Manual 2022, 6.1.3 alínea a: a transmissão exige "Extended Key Usage com permissão de Autenticação Cliente". Espelho v1.01, seção 7.3.2 (p. 23 do PDF): mesma exigência de EKU clientAuth. Isso vale para os certificados de teste do NL-2.1.

---

## (e) Provider legado do OpenSSL 3 para .pfx antigo

### e.1 Premissa corrigida

FATO. Documentação do OpenSSL 3.0, provider default: https://docs.openssl.org/3.0/man7/OSSL_PROVIDER-default/ lista `3DES` entre as cifras simétricas do provider default. Logo, 3DES NÃO é legado no OpenSSL 3.

FATO. Documentação do OpenSSL 3.0, provider legacy: https://docs.openssl.org/3.0/man7/OSSL_PROVIDER-legacy/ lista RC2, RC4, DES, Blowfish, CAST, IDEA, SEED, RC5 (desligado por padrão) e MD2, MD4, MDC2, WHIRLPOOL, RIPEMD160. MD5 não aparece nessa lista. A página não cita PKCS#12 nem 3DES.

FATO. Guia de migração do OpenSSL 3.0: https://docs.openssl.org/3.0/man7/migration_guide/. Trecho: "If you want to use them then you must load the legacy provider". Seção PKCS#12: "The pkcs12 application now supports -legacy option that restores the previous default algorithms".

**Conclusão:** o problema real de .pfx antigo é RC2-40-CBC (`pbeWithSHA1And40BitRC2-CBC`) em cert bag. A 3DES em key bag não precisa de legacy.

### e.2 Medição (não é o certificado do líder)

Certificado de teste, chave EC P-256 efêmera, gerado no scratchpad. Exportei duas versões: uma legada (cert bag com `PBE-SHA1-RC2-40`, key bag com `PBE-SHA1-3DES`) e uma moderna (PBES2 com AES-256). Ambiente com `OPENSSL_CONF` vazio, para não ativar provider por configuração.

| Caso | Host Fedora (OpenSSL 3.5.9) | Runtime org.kde.Platform//6.11 (OpenSSL 3.5.7) |
|---|---|---|
| Legado, sem `-legacy` | rc=1, "Error outputting keys and certificates" | rc=1, mesmo erro |
| Legado, com `-legacy` | rc=0 | rc=0 |
| Chave 3DES + cert AES-256, sem `-legacy` | rc=0 | não medido |
| Moderno (AES), sem `-legacy` | rc=0 | não medido |

FATO, erro medido: `error:0308010C:digital envelope routines:...:unsupported ... Algorithm (RC2-40-CBC : 0)`. Código `0308010C` e motivo "unsupported" é o sinal de detecção.

FATO, armadilha medida: `openssl list -cipher-algorithms` lista `rc2` (10 linhas) mesmo com o provider default sozinho. A listagem de nome não prova que a cifra funciona. Não use `list` para detectar. Use a falha de parse.

FATO, medido: `openssl pkcs12 -info` mostra o nome do PBE (`pbeWithSHA1And40BitRC2-CBC`), o que permite identificar o caso sem extrair a chave.

### e.3 Presença do provider legado

FATO, medido por `ls` no diretório de módulos:
- Host Fedora 44: `legacy.so` em `/usr/lib64/ossl-modules` (OpenSSL 3.5.9).
- Imagem `notalivre/build-fedora44:nl-0.4`: `legacy.so` em `/usr/lib64/ossl-modules` (3.5.9).
- Imagem `notalivre/build-ubuntu26.04:nl-0.4`: `legacy.so` em `/usr/lib/x86_64-linux-gnu/ossl-modules` (3.5.5).
- Imagem `notalivre/build-mint22.3:nl-0.4`: `legacy.so` (3.0.13).
- Imagem `notalivre/build-manjaro:nl-0.4`: `legacy.so` em `/usr/lib/ossl-modules` (3.6.4).
- Runtime `org.kde.Platform//6.11` (instalado) e `org.kde.Sdk//6.11` (instalado): `legacy.so` em `/usr/lib/x86_64-linux-gnu/ossl-modules`, OpenSSL 3.5.7.

FATO. No runtime, `/etc/ssl/openssl.cnf` tem a linha de ativação do legacy comentada (`# activate = 1`). A falha sem `-legacy` confirma que o provider não é ativado por padrão ali.

NÃO MEDIDO. O comportamento de parse PKCS#12 nas imagens Mint (OpenSSL 3.0.13), Ubuntu, Manjaro e Fedora. Só medi o host e o runtime.

### e.4 Carregamento no código e no Qt (não verificado)

FATO. Qt: `static bool QSslCertificate::importPkcs12(QIODevice *device, QSslKey *key, QSslCertificate *certificate, QList<QSslCertificate> *caCertificates = nullptr, const QByteArray &passPhrase = QByteArray())`. Retorna `bool`. Página: https://doc.qt.io/qt-6/qsslcertificate.html. A documentação não cita OpenSSL, legacy nem algoritmos antigos.

NÃO VERIFICADO. Se o `importPkcs12` do Qt 6.12 carrega o provider legado e como reporta o erro RC2. O código-fonte no `code.qt.io` e no `raw.githubusercontent.com` respondeu HTTP 404 ao WebFetch.

FATO, OpenSSL (crypto(7), seção "Default provider", https://docs.openssl.org/3.0/man7/crypto/): "If you explicitly load any provider then the "default" provider would also need to be explicitly loaded". Logo, carregar só o legacy tira o AES do default. Esse é o erro clássico de NL-2.2.

INFERÊNCIA de desenho. Não confiar no `importPkcs12` do Qt para .pfx legado sem teste. Detectar pelo erro `unsupported` e carregar o legacy de forma explícita, junto com o default, no contexto da biblioteca e não no processo inteiro. Isso precisa de teste em NL-2.2.

---

## (f) xmlsec1 1.2 versus 1.3

### f.1 Versões nas distros do CI

FATO. `docs/pesquisa/ci_medicao.md`, linha 26 (tabela com colunas Fedora 44, Ubuntu 26.04, Mint 22.3 e Manjaro). Linha "xmlsec1": Fedora 44, ausente (candidato 1.2.41); Ubuntu 26.04, ausente (candidato 1.3.9-1); Mint 22.3, ausente (candidato 1.2.39-5build2); Manjaro, ausente (candidato 1.3.12-1). "Candidato" é a versão do repositório na medição, não instalada.

FATO, medido nesta pesquisa: host Fedora tem `xmlsec1 1.2.41 (openssl)` em `/usr/bin/xmlsec1`. As quatro imagens `notalivre/build-*:nl-0.4` não têm `xmlsec1` (comando não encontrado). Os runtimes `org.kde.Platform//6.11` e `org.kde.Sdk//6.11` não têm `xmlsec1` no `PATH` nem `pkg-config` com `xmlsec1`.

### f.2 Diferenças de comportamento que afetam o oráculo

FATO, notas de release do xmlsec, lidas por WebFetch (resumo, não texto bruto). URL: https://github.com/lsh123/xmlsec/releases/tag/xmlsec_1_3_0.
- 1.3.0: "(API breaking change) Changed the key search to strict mode: only keys referenced by KeyInfo are used." Para voltar ao modo lax: opção `--lax-key-search` na CLI ou flag `XMLSEC_KEYINFO_FLAGS_LAX_KEY_SEARCH` no `xmlSecKeyInfoCtx`. A CLI usa o modo strict por padrão.
- 1.3.0: "Disabled MD5 digest method by default." (MD5 volta com `--enable-md5`).
- 1.3.0: "Removed deprecated functions" (ABI breaking). Também "Disabled FTP support by default".
- 1.3.0: KeyName agora é aparado (trim) antes da busca de chave.

FATO, notas de release, lidas por WebFetch. URL: https://www.aleksey.com/xmlsec/news.html. Versões seguintes:
- 1.2.35: migração para a API do OpenSSL 3.0; engines desativados por padrão com OpenSSL 3.
- 1.3.3: remoção de funções obsoletas; versão mínima do OpenSSL 1.1.1.
- 1.3.7: "Disabled old crypto algorithms (MD5, RIPEMD160) and the old crypto engines... by default". Esta linha conflita com a de 1.3.0 para MD5. Ambas vêm de resumos e não consegui o texto bruto (`NEWS` em `raw.githubusercontent.com` deu 404). Não resolvi a divergência.
- 1.3.8: opção `--add-id-attr`.
- 1.3.10: versão mínima do OpenSSL 3.0.13. A imagem Mint 22.3 tem exatamente 3.0.13.
- 1.3.13: data de 27/10/2026 no resumo. NÃO VERIFICADO na distro.

FATO, medido: `xmlsec1 --help-verify` na 1.2.41 traz `--id-attr[:<attr-name>] [<node-namespace-uri>:]<node-name>`, e `--trusted-pem`, `--untrusted-pem`, `--keys-file`. Não há `--lax-key-search` na 1.2.41 (não aparece na saída de `--help-verify`). Esta diferença só vale se a versão for 1.3.

NÃO ACHADO. Qualquer nota de release (lida em resumo) que diga que SHA-1 (`rsa-sha1`) foi desativado no xmlsec 1.3.x. Só achei MD5, RIPEMD160, FTP e HTTP.

INFERÊNCIA. O oráculo `xmlsec1 --verify` de 1.3 deve aceitar `rsa-sha1` se a versão for 1.3.x, mas isso precisa de teste em NL-2.3 e NL-2.6.

INFERÊNCIA, sobre o uso prático. Em strict, `xmlsec1` só usa chave que esteja em `KeyInfo`. O certificado de cliente deve estar no `KeyInfo` e a AC confiável vai em `--trusted-pem`. A forma exata para o `--id-attr` do `infDPS` (`--id-attr:Id http://www.sped.fazenda.gov.br/nfse:infDPS`) não foi testada na 1.3.

NÃO ACHADO. Remoção de `xmlSecCryptoAppKeyLoad` na API C da 1.3. Uma fonte secundária (commit do Lasso, resultado de busca) diz que `xmlSecCryptoAppKeyLoadEx` apareceu em 1.3.0, sem texto primário.

### f.3 Consequência de empacotamento

FATO. `xmlsec1` não existe no runtime KDE 6.11, nem na imagem Flatpak de build (SDK), nem nas quatro imagens de build.

INFERÊNCIA. O binário não pode ser dependência do app. Ele serve só como oráculo nos contêineres de teste. Se o app usar `libxmlsec1` em runtime, ela vira módulo do manifesto Flatpak. Isso é um pacote fora das duas listas do plano (lista fechada e lista complementar, ponto L9). Pela regra L-51 e pelo plano, precisa de autorização nova do líder antes da fatia.

---

## O que não achei

- Manual de assinatura específico da Sefin para a v1.01: NÃO ACHADO. A página de Produção Restrita não lista esse documento.
- Algoritmo de assinatura da v1.01 em fonte oficial: NÃO ACHADO.
- Versão vigente do DOC-ICP-04.01 e do DOC-ICP-04 (8.1 ou posterior): NÃO LIDA (HTTP 404). Layout dos OIDs lido na versão 8.0 (d.1b).
- `CanonicalizationMethod` e `DigestMethod` no Anexo I: NÃO BUSCADOS nas abas.
- Convenção de CN: não pesquisada.
- Qt `importPkcs12` com legado: NÃO VERIFICADO (código-fonte inacessível).
- Medição de parse PKCS#12 nas imagens Mint, Ubuntu e Manjaro: NÃO MEDIDO.
- Notas de release do xmlsec em texto bruto: NÃO LIDAS (só resumos de WebFetch). Divergência de MD5 entre 1.3.0 e 1.3.7 não resolvida.
- Itens (b), (c) e (g) da pesquisa de abertura: fora desta ordem.

---

## Implicações para o plano

1. **Assinatura (NL-2.3):** `Signature` é irmã de `infDPS` (XSD, tiposComplexos_v1.01.xsd, linhas 742 a 745). `URI="#"+Id` aponta para o `Id` de `infDPS` (`DPS...`), com `enveloped-signature` e C14N 20010315. Algoritmo: as fontes lidas só trazem SHA-1. Recomendação: seguir o leiaute. A escolha é do líder (L-01), e precisa de confirmação na Sefin antes de fechar.
2. **Posição divergente (NL-2.3, NL-2.6):** a linha 403 do leiaute do Anexo I põe `Signature` dentro de `infDPS`. O XSD e a regra de negócio (linha 653, E0714) põem como irmã. Seguir o XSD, que o `xmllint` do NL-2.6 usa, e levar a divergência à Sefin.
3. **CNPJ e CPF (NL-2.2):** OID 2.16.76.1.3.3 para CNPJ (14 posições) em PJ e em equipamento; OID 2.16.76.1.3.1 para PF (CPF nas posições 9 a 19 do conteúdo, depois da data). Comparar a raiz como string de 8 caracteres, por causa do CNPJ alfanumérico. Confirmar o layout na versão vigente do DOC-ICP-04.
4. **Legado (NL-2.2):** detectar pelo erro `unsupported` (`0308010C`) com `RC2-40-CBC`. O provider legado existe no runtime KDE 6.11 e nas quatro imagens. Não confiar no `importPkcs12` do Qt sem teste. Premissa corrigida: 3DES está no provider default.
5. **Oráculo (NL-2.3, NL-2.6):** fixar a versão do xmlsec1 por imagem, porque Fedora tem 1.2.41, Ubuntu 1.3.9, Mint 1.2.39 e Manjaro 1.3.12. Testar `--id-attr` explícito e strict key search na 1.3. Mint (OpenSSL 3.0.13) está na versão mínima exigida pela 1.3.10.
6. **Empacotamento:** `xmlsec1` não está no runtime KDE 6.11. Se o app o usar, pede autorização nova do líder (lista fechada do plano).
7. **Fixtures (NL-2.1):** certificados de teste precisam de EKU `clientAuth` para o mTLS, de OID 2.16.76.1.3.1 ou 3.3 e de cadeia de AC de teste.
8. **Lacuna para o líder:** pedir à Sefin o manual de assinatura da v1.01 com algoritmos (Onda 2, antes do NL-2.3).
9. **Dúvida de implementação:** o `Id` da v1.01 começa com `DPS` (XSD), e o manual de 2022 usa `ID`. A referência `URI` precisa de confirmação oficial.
10. **Versão nova do xmlsec:** o resumo indica 1.3.13 em 27/10/2026. Não usar sem medir a versão da distro.
