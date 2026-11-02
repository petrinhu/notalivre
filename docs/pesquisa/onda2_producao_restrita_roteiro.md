# Roteiro: emissão de teste em Produção Restrita (NL-2.10)

## Para o líder

Você só precisa de um comando. No terminal, rode:

```
bash <caminho>/tools/producao_restrita/emitir_teste.sh
```

Troque `<caminho>` pela pasta do projeto no seu computador (a pasta que contém `tools`).

O script faz sozinho todos os passos técnicos deste roteiro (o passo a passo manual está no apêndice técnico, mais abaixo).

**Na primeira vez**, o script pergunta, em janelas:

1. o CNPJ da empresa, com 14 dígitos e sem pontos (se a empresa for de CPF, informe o CPF);
2. a Inscrição Municipal (IM) da empresa;
3. o arquivo do certificado `.pfx`. O caminho dele fica guardado para as próximas vezes; o certificado e a senha não ficam;
4. a senha do certificado, numa janela própria, e não no terminal. O script não lê nem guarda a senha.

Depois de receber o CNPJ, o script consulta a BrasilAPI, uma consulta pública, e descobre sozinho se a empresa é do Simples Nacional, é MEI ou não é optante. Para isso, o CNPJ vai à BrasilAPI.

Se a primeira tentativa parar antes de a Sefin responder, nada fica guardado, e a próxima vez pergunta tudo de novo.

**Nas seguintes**, o script usa o CNPJ, a IM e o caminho do certificado que ficaram guardados, e não pergunta mais por eles. Pede só a senha do certificado. Se o arquivo guardado não existir mais, for um link simbólico ou não for seu, a janela pergunta o `.pfx` de novo. O CPF nunca fica guardado: com CPF, o script pergunta o CPF de novo.

- `--esquecer` apaga o que ficou guardado (CNPJ, IM e o caminho do certificado), sem emitir nada.
- `--help` mostra todas as opções.

**Onde achar a Inscrição Municipal.** A IM é o número de inscrição da empresa na Prefeitura. Você encontra no cadastro mobiliário da Prefeitura do Recife, ou no Emissor Nacional, em nfse.gov.br. Se a IM não estiver registrada para o município de Recife no cadastro da Produção Restrita, a Sefin recusa a nota, e a mensagem do script diz o que fazer.

**Aviso sobre CPF e CNPJ na linha de comando.** O script mostra este aviso quando você usa `--cpf` ou `--cnpj`, e também no `--help`:

> Aviso: o número de CPF ou CNPJ digitado na linha de comando fica gravado no histórico do terminal e, enquanto o script roda, aparece para outros usuários desta máquina na lista de processos. Para não deixar esse rastro, rode sem --cpf e sem --cnpj: a janela pede o número. Começar o comando com um espaço só tira a linha do histórico se o seu terminal estiver configurado para isso.

No fim ele mostra uma destas frases: "Nota de teste aceita", com a chave de acesso; ou "A Sefin recusou; avise o Claude". Se a emissão parar antes de chegar à Sefin, ele mostra o código de saída e manda avisar o Claude.

As saídas ficam em `/var/tmp/notalivre-nl210/teste-lider/execucao-<data>/`, e as cópias para devolução em `/var/tmp/notalivre-nl210/devolucao/execucao-<data>/`. Não envie esses arquivos a ninguém. (A pasta do certificado nunca aparece nas saídas.)

Enquanto o script roda, o nome do arquivo do certificado pode aparecer por instantes na lista de processos do computador (a mesma lista que o comando `ps` mostra). O script não grava esse nome em nenhum arquivo de log. Num computador com um único usuário, o risco é baixo. A correção vem numa etapa futura. O teste confere que o nome do certificado e a pasta onde ele está não aparecem fora dos dois usos aceitos; as pastas acima dela não são conferidas, porque são genéricas.

Não apague o arquivo `/var/tmp/notalivre-nl210/ultimo-ndps.txt`. Ele guarda o último número de DPS usado. Se ele sumir, o próximo envio volta a usar o número 1, e a Sefin pode recusar por número repetido.

**Lista de opções, para quem quiser.** Cada opção vale como `--opcao valor` ou `--opcao=valor`. Se você não passar uma opção, o script usa o que ficou guardado, a consulta pública ou o padrão, e pergunta só o que faltar.

| Opção | O que faz |
|---|---|
| `--cnpj N` | CNPJ do emissor, com 14 dígitos e DV. Sem ele, e sem CNPJ guardado, a janela pede CNPJ ou CPF. |
| `--cpf N` | CPF do emissor, com 11 dígitos e DV. O CPF nunca fica guardado. |
| `--im T` | Inscrição municipal: de 1 a 15 letras, números, ponto, barra ou traço. |
| `--sem-im` | A empresa não tem inscrição municipal no CNC: a DPS sai sem IM. |
| `--ie T` | Inscrição estadual, de 1 a 20 caracteres. Só confere: não vai na NFS-e e não é guardada. |
| `--municipio N` | Código IBGE do município emissor, com 7 dígitos, começando de 1 a 5. Padrão: o da consulta, ou 2611606. |
| `--ctribnac N` | Código de tributação nacional, com 6 dígitos (não 9903xx nem 990401). Padrão 040101. |
| `--opsimpnac N` | Regime do Simples: 1 não optante, 2 MEI, 3 ME/EPP. Padrão: o da consulta, ou 3. |
| `--regaptribsn N` | Regime de apuração (1, 2 ou 3), só com a opção 3 do Simples. Padrão 1. |
| `--regesptrib N` | Regime especial (0, 1, 2, 3, 4, 5, 6 ou 9). Padrão 0. |
| `--local-prestacao N` | Código IBGE do local da prestação (7 dígitos), ou 0000000. Padrão: o do município emissor. |
| `--trib-issqn N` | Tipo de tributação do ISSQN (1, 2, 3 ou 4). Padrão 1. |
| `--ret-issqn N` | Retenção do ISSQN: só 1 neste script, porque não há tomador. Padrão 1. |
| `--valor V` | Valor do serviço, como 10,00 (ponto ou vírgula). Padrão 10,00. |
| `--descricao T` | Descrição do serviço, de 1 a 2000 caracteres: letras, números, espaço e . , ; : - / ( ) %. |
| `--esquecer` | Apaga tudo o que o script guardou (CNPJ, IM e o caminho do certificado) e sai, sem emitir. |
| `--help`, `-h` | Mostra a ajuda e sai, sem perguntar nada. |

**Para o agente:** `bash <caminho>/tools/producao_restrita/emitir_teste.sh --ensaio` roda todos os casos com stubs, dentro de um contêiner, sem flatpak e sem Sefin.

---

**Situação:** pronto para o líder. Uma verificação fica aberta e declarada (seção 6, item 4): o seu teste é a prova.
**Data:** 01/11/2026.
**Código:** o pacote do run `41362487894` foi feito sobre o commit `22cdb30b` (merge da NL-2.10e na onda-2). O app não mudou desde esse commit. O script `tools/producao_restrita/emitir_teste.sh` está na fatia NL-2.10g (branch `fatia/NL-2.10g`, ainda fora da onda-2), e nada no empacotamento do Flatpak o referencia.
**Pacote a instalar:** veja a seção "Pacote a instalar", logo abaixo.

Nos comandos abaixo, o que está entre `< >` é um texto que você troca pelo seu valor. O caminho do seu certificado nunca aparece neste roteiro: use sempre `<caminho do seu .pfx>`.

---

## Apêndice técnico: passo a passo manual (o script faz o mesmo)

## Pacote a instalar

Um registro só. Confira antes de instalar.

- **Run do CI:** `41362487894` (verde, incluindo `flatpak` e o smoke no host do runner)
- **Commit:** `22cdb30b9f22fd56bbef2b9deb6047a87a294aad` (merge da NL-2.10e na onda-2)
- **Artefato:** `notalivre-bundle`
- **Digest do ZIP do artefato, pela API do GitHub:** `sha256:bf64796c188e477a490080ce4e5c0fcf30005dd053fb9a2bd83ec84fc246d2a5`
- **SHA-256 completo do arquivo `notalivre.flatpak`:** `2c2372581c392b0088faa6271bb40a9333f658b047ba6398e5acef23a90164c0`
- **Caminho do arquivo:** `/var/tmp/notalivre-nl210/bundle/run-41362487894/notalivre.flatpak`
- **Tamanho:** 648336 bytes
- **App desde este commit:** nenhum arquivo de `src/`, `apps/`, `flatpak/` ou `CMakeLists.txt` mudou entre `22cdb30b` e o commit `8e856d2` do script (conferido em 01/11/2026). O run `41362487894` continua sendo o pacote certo.

Antes de instalar, rode:

```
sha256sum /var/tmp/notalivre-nl210/bundle/run-41362487894/notalivre.flatpak
```

O resultado tem de ser exatamente `2c2372581c392b0088faa6271bb40a9333f658b047ba6398e5acef23a90164c0`. Se for outro, não instale e avise o agente. O job do CI não imprime esse SHA-256 no log; o valor foi calculado pelo agente sobre o arquivo baixado do artefato, e o digest do ZIP confere com a API.

**Este pacote substitui o anterior** (run `41329186551`, commit `6c6118a`, SHA-256 `157af29c…`). Use só este.

---

## 1. O que é este teste

Você vai mandar uma nota de teste à Sefin Nacional, o sistema da Receita que recebe as notas de serviço. O ambiente é o de Produção Restrita: a nota não tem validade fiscal e serve para confirmar que o programa conversa com a Sefin do jeito certo.

Quem roda os comandos é você, no seu computador, com o seu certificado. O agente só prepara os arquivos e confere o resultado.

---

## 2. O que você precisa ter em mãos

1. **Certificado A1** (arquivo `.pfx` ou `.p12`) de e-CNPJ ou e-CPF, com a finalidade de autenticação de cliente (clientAuth). Não copie esse arquivo para pasta nenhuma além do lugar indicado no passo 3.3.
2. **A senha do certificado.** Você a digita no terminal quando o programa pedir. Ela não aparece na tela e não vai para a linha de comando.
3. **Confirmação de que a prefeitura do seu município aderiu ao Sistema Nacional da NFS-e.** Sem adesão, a Sefin recusa a nota.
4. **O código cTribNac do serviço que você presta.** É um código de 6 dígitos da lista nacional de serviços. O modelo deste roteiro usa `170201` (consultoria em tecnologia) só como exemplo.
5. **Um valor pequeno** para o serviço de teste, por exemplo `10,00`.
6. **O seu regime tributário**, que vai no campo `opSimpNac`: `1` para não optante pelo Simples, `2` para MEI, `3` para ME ou EPP optante pelo Simples.
7. **Nenhum tomador.** A nota de teste não tem cliente. Assim não aparece dado de terceiro.

---

## 3. Passo a passo

### 3.1 Conferir o arquivo do programa

```
sha256sum /var/tmp/notalivre-nl210/bundle/run-41362487894/notalivre.flatpak
```

O resultado tem de ser exatamente `2c2372581c392b0088faa6271bb40a9333f658b047ba6398e5acef23a90164c0`. Se for outro, pare e avise o agente.

### 3.2 Instalar o programa

```
flatpak install --user --bundle /var/tmp/notalivre-nl210/bundle/run-41362487894/notalivre.flatpak
```

Se o sistema pedir o ambiente de execução do KDE 6.11 e você ainda não o tiver, instale com `flatpak install --user flathub org.kde.Platform//6.11` e repita este passo.

### 3.3 Liberar o certificado para o programa

O programa roda isolado do resto do computador, por isso ele não enxerga o seu certificado sem uma liberação. Libere só este arquivo, uma vez:

```
flatpak document-export --app=io.github.petrinhu.notalivre --forbid-write "<caminho do seu .pfx>"
```

O comando imprime um caminho que começa com `/run/user/`. Copie esse caminho inteiro. Vamos chamá-lo de `<caminho do certificado no app>`. O `--forbid-write` impede o programa de alterar o seu arquivo.

### 3.4 Liberar a DPS de teste

Depois de montar a DPS (passo 3.8), libere também o arquivo dela:

```
flatpak document-export --app=io.github.petrinhu.notalivre --forbid-write "/var/tmp/notalivre-nl210/teste-lider/dps-teste.json"
```

Guarde o caminho impresso como `<caminho da DPS no app>`.

### 3.5 Conferir a cadeia de segurança do servidor

Antes de mandar qualquer coisa, confirme que o arquivo de CAs que o programa usa para falar com o servidor está dentro do programa. Você não configura esse arquivo: a CLI usa o repositório de CAs do próprio sistema do programa. Este comando só confere que o arquivo existe:

```
flatpak run --command=ls io.github.petrinhu.notalivre -l /etc/pki/tls/certs/ca-bundle.crt
```

Se o comando disser que o arquivo não existe, pare. Não siga para a configuração e avise o agente. Mesmo que o arquivo exista, a verificação da seção 6, item 4, continua aberta.

Confira também a pasta de configuração que o programa vai usar:

```
flatpak run --command=sh io.github.petrinhu.notalivre -c 'echo $XDG_CONFIG_HOME'
```

O normal é aparecer algo como `/home/<seu usuário>/.var/app/io.github.petrinhu.notalivre/config`. A partir daí, o arquivo de configuração fica em `$XDG_CONFIG_HOME/notalivre/config.ini`.

### 3.6 Preencher a configuração

Crie o arquivo `config.ini` na pasta `notalivre` dentro da pasta mostrada no passo anterior. Escreva exatamente estas chaves, sem nenhuma outra:

```
[sefin]
url = https://sefin.producaorestrita.nfse.gov.br
prefixo = /SefinNacional

[assinatura]
algoritmo = rsa-sha1

[emissao]
producao_liberada = false
```

Regras que valem para este arquivo:

- **Não escreva** `ancoras_tls` nem `certificado.ancoras`. Em Produção Restrita, qualquer uma dessas chaves faz o programa parar com código de saída 10. Os certificados de confiança já vêm dentro do programa.
- **Não escreva** a seção `[xsd]`. Fora de Homologação, o programa também sai com código 10 se ela existir. Os esquemas XSD já vêm instalados no programa, com conferência de SHA-256.
- **Não deixe nenhuma chave com valor vazio**, por exemplo `algoritmo =` sem nada depois. Chave vazia sai com código 10. A mesma coisa vale para uma chave escrita duas vezes no mesmo arquivo (`configuracao.chave_duplicada`).
- A chave `certificado.caminho` não entra aqui. O passo 3.7 cria a seção `[certificado]` sozinho. Se faltar essa chave, o programa sai com código 10 (`configuracao.chave_ausente`).
- A chave `sefin.url` tem de ser `https` e ter o host `sefin.producaorestrita.nfse.gov.br`. Só o host é conferido. Se o host for outro, o programa para com saída 2 e mostra a mensagem fixa `o endpoint não é de Produção, de Produção Restrita nem do servidor local de teste`. Ele não repete o endereço que você digitou.
- Nunca deixe `producao_liberada` como `true` neste teste. Ela só muda com a decisão do líder para a Produção.

### 3.7 Gravar o caminho do certificado

```
flatpak run --command=notalivre-cli io.github.petrinhu.notalivre config certificado "<caminho do certificado no app>"
```

O comando não mostra o caminho de volta, por segurança, porque ele pode conter o seu nome. Se aparecer `certificado.caminho_ilegivel`, o programa não consegue ler o arquivo: refaça o passo 3.3.

### 3.8 Montar a DPS de teste

Crie a pasta e o arquivo fora de qualquer pasta sincronizada:

```
mkdir -p /var/tmp/notalivre-nl210/teste-lider
```

Monte o arquivo `/var/tmp/notalivre-nl210/teste-lider/dps-teste.json` com este modelo, trocando os campos marcados:

```json
{
  "tpAmb": "2",
  "dhEmi": "<data e hora atuais, no formato 2026-10-31T10:00:00-03:00>",
  "verAplic": "NotaLivre 0.1",
  "serie": 1,
  "nDps": 1,
  "dCompet": "<primeiro dia do mês atual, por exemplo 2026-10-01>",
  "cLocEmi": "<código IBGE de 7 dígitos do seu município>",
  "prestador": {
    "cnpj": "<seu CNPJ de 14 dígitos, sem pontos ou barras>",
    "regime": { "opSimpNac": "<1, 2 ou 3, ver seção 2>", "regEspTrib": "0" }
  },
  "servico": {
    "cLocPrestacao": "<código IBGE de 7 dígitos do município da prestação>",
    "cTribNac": "<seu código cTribNac de 6 dígitos>",
    "xDescServ": "Teste de emissão em Produção Restrita"
  },
  "valores": {
    "vServ": "10,00",
    "tribMun": { "tribISSQN": "1", "tpRetISSQN": "1" },
    "totTrib": { "indTotTrib": "0" }
  }
}
```

Regras que valem para o arquivo:

- Se o certificado for e-CPF, troque `cnpj` por `cpf`. Nunca deixe os dois campos juntos.
- `tpAmb` fica `"2"`, que é o código de Produção Restrita. Não mude.
- Não acrescente nenhum campo de tomador.
- O campo `nDps` é o número da DPS. Cada tentativa precisa de um número novo.
- O campo `serie` tem de ser igual ao `--serie` do comando do passo 3.9, e o `nDps` tem de ser igual ao `--numero`. Se não forem, o programa recusa com saída 2 e não escolhe um dos dois valores.
- Deixe o arquivo com permissão restrita: `chmod 600 /var/tmp/notalivre-nl210/teste-lider/dps-teste.json`.

### 3.9 Emitir (tentativa 1)

Antes, faça o passo 3.4 com o arquivo pronto. Depois rode:

```
flatpak run --command=notalivre-cli io.github.petrinhu.notalivre emitir "<caminho da DPS no app>" --serie 1 --numero 1 --ambiente producao-restrita > /var/tmp/notalivre-nl210/teste-lider/saida-tentativa-1.txt 2>&1; echo "saida=$?"; cat /var/tmp/notalivre-nl210/teste-lider/saida-tentativa-1.txt
```

Como funciona:

- O comando grava tudo o que o programa escreve (a linha `emitida`, os erros e as mensagens) no arquivo `saida-tentativa-1.txt`. Os erros não aparecem na tela sozinhos, por isso o comando mostra o código de saída (`saida=...`) e depois o conteúdo do arquivo. Olhe sempre as duas coisas antes de fazer outra tentativa.
- Quando aparecer `senha do certificado:`, digite a senha e tecle Enter. Esse prompt aparece direto no terminal, não no arquivo. Nada aparece na tela enquanto você digita, e isso é o esperado.
- O argumento `--numero` tem de ser igual ao `nDps` do arquivo, e o `--serie` igual ao `serie`. O programa confere os dois contra a DPS: se algum for diferente, ele recusa a emissão com saída 2 e diz qual campo não bate. Ele não escolhe um dos dois valores.
- **Nunca repita um `nDps` depois de uma saída diferente de 0 sem ler o arquivo.** Uma recusa pode já ter registrado o número na Sefin.
- Códigos de saída:
  - **0:** emissão aceita. O arquivo tem a linha `emitida: chave` seguida de um número de 50 caracteres. Esse é o número da nota de teste.
  - **1:** recusa de credencial, ou recusa da Sefin. A credencial cobre senha errada e certificado que não carrega (`credencial.nao_carregou`). A recusa da Sefin aparece como `emissão recusada (sefin.<código>)`, e é aí que você distingue a rota não encontrada (404) do E0714: leia o texto no arquivo, porque ambos saem com 1. Para o 404, procure no arquivo a linha `sefin.requisicao` com `status` igual a 404, porque a mensagem pode vir vazia.
  - **2:** uso errado da linha de comando, `--serie` ou `--numero` diferente da DPS, host de `sefin.url` que não é o de Produção Restrita, esquema XSD instalado com problema, ou senha não digitada (sem terminal).
  - **4:** documento reprovado: a DPS não passou na validação do esquema XSD, ou um campo (CPF, CNPJ, série ou entrada) está errado. Corrija o arquivo, use um número novo e tente de novo.
  - **5:** a DPS já existe na Sefin com outro conteúdo (`sefin.dps_duplicada`). Não reenvie o mesmo `nDps`. Use o próximo número livre e avise o agente.
  - **7:** a Sefin não conseguiu comparar a DPS com a nota já existente (`sefin.comparacao_indisponivel`). Devolva o arquivo ao agente.
  - **10:** problema de configuração: chave vazia, chave repetida, chave que não pode existir em Produção Restrita, chave obrigatória ausente, certificado ilegível, ou manifesto dos XSD não fixado (`xsd.manifesto_nao_fixado`).
  - **70:** erro interno do programa. Devolva o arquivo ao agente.
  - Qualquer outro código de erro da Sefin não está nesta tabela: devolva o arquivo ao agente.

Se a saída não for `saida=0`, não faça outra tentativa antes de devolver o arquivo ao agente.

### 3.10 Tentativa 2: segunda base

Faça esta tentativa só se a linha `sefin.requisicao` do arquivo da tentativa 1 tiver `status` igual a 404. A mensagem de erro pode vir vazia, por isso a linha de status é a referência.

1. Abra o `config.ini` e troque a linha `prefixo` para `prefixo = /API/SefinNacional`.
2. No arquivo `dps-teste.json`, troque `nDps` para `2`.
3. Refaça o passo 3.8 se precisar, com o mesmo CNPJ e o mesmo cTribNac.
4. Rode de novo, com o número novo:

```
flatpak run --command=notalivre-cli io.github.petrinhu.notalivre emitir "<caminho da DPS no app>" --serie 1 --numero 2 --ambiente producao-restrita > /var/tmp/notalivre-nl210/teste-lider/saida-tentativa-2.txt 2>&1; echo "saida=$?"; cat /var/tmp/notalivre-nl210/teste-lider/saida-tentativa-2.txt
```

Se a tentativa 1 já tiver ido para o endereço certo e a Sefin tiver recusado por outro motivo, não faça a tentativa 2. Essa decisão é do agente, com o log na mão.

### 3.11 Tentativa 3: algoritmo rsa-sha256

Faça esta tentativa só se a Sefin responder com o código `E0714` (assinatura com algoritmo recusado).

1. No `config.ini`, na seção `[assinatura]`, troque a linha para `algoritmo = rsa-sha256`.
2. Use a base que não deu 404 nas tentativas anteriores. Não mude o `prefixo` sem motivo.
3. No `dps-teste.json`, troque `nDps` para o próximo número livre, por exemplo `3`.
4. Rode com o número novo:

```
flatpak run --command=notalivre-cli io.github.petrinhu.notalivre emitir "<caminho da DPS no app>" --serie 1 --numero 3 --ambiente producao-restrita > /var/tmp/notalivre-nl210/teste-lider/saida-tentativa-3.txt 2>&1; echo "saida=$?"; cat /var/tmp/notalivre-nl210/teste-lider/saida-tentativa-3.txt
```

Se vier qualquer outro código de erro, que não seja 404 ou E0714, pare. Não tente de novo às cegas: devolva o log ao agente.

---

## 4. Onde ficam o log e as respostas, como devolver e como apagar

**A saída do programa** é o que a CLI escreve durante a emissão: a linha `emitida`, os erros e as mensagens. Os comandos das seções 3.9 a 3.11 a gravam em `/var/tmp/notalivre-nl210/teste-lider/saida-tentativa-N.txt`, fora de qualquer pasta sincronizada. Esse arquivo é o log que você devolve ao agente.

**As respostas cruas da Sefin** são gravadas pelo próprio programa, em um arquivo por DPS, dentro de uma pasta chamada `respostas`, com o nome `<hash>.jsonl`, onde o hash é o do identificador da DPS (o identificador em texto não aparece no nome do arquivo). Cada linha do arquivo é uma resposta HTTP da Sefin. O programa grava a resposta de toda emissão, aceita ou recusada. Essa pasta fica na pasta de dados do app, que está dentro de `~/.var/app/io.github.petrinhu.notalivre`. Para achar a pasta:

```
find ~/.var/app/io.github.petrinhu.notalivre -type d -name respostas
```

O programa mascara o que sai no arquivo: a chave de acesso sai com `#` no lugar dos caracteres, o XML comprimido vira um marcador, e cada outro valor sai como o tipo e o tamanho dele (só os nomes dos campos que o programa lê continuam: `chaveAcesso`, `nfseXmlGZipB64`, `erro`, `erros`, `Codigo` e `codigo`; os outros nomes viram `<chave:N:classe:k>`). Cada arquivo sai com permissão só para você (0600). Mesmo mascarado, é a resposta da Sefin para a sua nota, então trate como dado pessoal.

Se o programa não conseguir gravar essa resposta, a emissão continua (a Sefin já recebeu a nota) e o log mostra o evento `resposta_crua.nao_gravada`, em nível de erro. Se a resposta passar do teto de tamanho, o log mostra `resposta_crua.cortada`, em nível de aviso. Nesses dois casos, copie o log junto com a saída: ele é a prova de que a resposta falhou.

**Como devolver ao agente.** Copie os arquivos para a pasta de devolução, sem alterar o nome, e avise no chat:

```
mkdir -p /var/tmp/notalivre-nl210/devolucao
cp -p -- "<caminho exato do log>" /var/tmp/notalivre-nl210/devolucao/
cp -p -- "<caminho exato do arquivo de resposta>" /var/tmp/notalivre-nl210/devolucao/
```

Não envie esses arquivos por e-mail, por mensagem de aplicativo, para o GitHub, nem para nenhuma pasta do IDrive.

**Como apagar depois.** Os arquivos de resposta crua são apagados depois que a análise da NL-2.10 estiver concluída. Só apague quando o agente disser que a análise terminou. Apague um caminho de cada vez, com o caminho exato, sem usar `*` e sem `-r` em pasta de caminho que você não conferiu. Não há lixeira nesta máquina: o que sai com `rm` não volta.

```
rm -- "<caminho exato do arquivo de resposta>"
```

Depois que o agente confirmar a análise, apague também as cópias em `/var/tmp/notalivre-nl210/devolucao` e a pasta `/var/tmp/notalivre-nl210/teste-lider`, sempre um caminho exato de cada vez. O arquivo `/var/tmp/notalivre-nl210/ultimo-ndps.txt` (o contador dos números de DPS) não entra nessa limpeza: nunca se apaga.

---

## 5. O que NÃO fazer

- **Não rode com `--ambiente producao`.** A Produção está bloqueada. Só o líder decide quando ela abre, em decisão própria.
- **Não envie o arquivo `.pfx` a ninguém.** Nem por e-mail, nem por GitHub, nem por pendrive de outra pessoa.
- **Não coloque a senha do certificado em nenhum lugar:** nem na linha de comando, nem em arquivo, nem em anotação.
- **Não guarde o certificado, a DPS ou as respostas dentro de pasta do projeto, de IDrive ou de qualquer pasta sincronizada com nuvem.**
- **Não repita um número de DPS já usado.** Cada tentativa usa `nDps` novo.
- **Não mande log ou resposta crua sem olhar antes.** Eles têm o seu CNPJ ou CPF, mesmo mascarados em parte.
- **Não rode duas emissões ao mesmo tempo.**

---

## 6. Pendências que bloqueiam o teste

Os três pontos que bloqueavam o teste antes da NL-2.10b estão resolvidos no código do `1384887` (merge da NL-2.10c). Fica um ponto aberto.

1. **Esquemas XSD (resolvido).** O bundle instala os XSD conferidos em `/app/share/notalivre/xsd`, e o programa usa essa pasta sozinho. Por isso o `config.ini` não tem `[xsd]`.
2. **Raízes da ICP-Brasil (resolvido).** O bundle instala as raízes em `/app/share/notalivre/icp-brasil`, e o programa as usa sozinho para validar o seu certificado. Por isso o `config.ini` não tem `certificado.ancoras`.
3. **`--serie` e `--numero` (resolvido).** Os dois entram na DPS e são conferidos contra o JSON. Conflito sai com saída 2, sem escolher um valor (seção 3.9).
4. **Aberto (VERIF-1): confirmar que o programa isolado usa o arquivo de CAs do runtime.** A CLI não recebe o caminho desse arquivo. Ela usa o repositório de CAs que o sistema oferece dentro do programa. Ainda não se mediu, de dentro do programa isolado, de qual arquivo esse repositório vem. Se não houver nenhum repositório de CAs, a CLI recusa antes de conectar e não envia nada. Se houver um repositório diferente do runtime KDE 6.11, a verificação não fecha e o agente registra qual é. Decisão do CISO, 31/10/2026 (20:31:27): esta verificação fica aberta e declarada. O seu teste é a prova. A sonda que confere isso entra na NL-9.2.

---

## 7. O que já foi medido sobre o servidor

Medido em 31/10/2026, só com o handshake TLS, sem nenhuma requisição HTTP, num contêiner, com o comando `openssl s_client` contra `sefin.producaorestrita.nfse.gov.br:443`:

- Protocolo TLSv1.2, cifra `ECDHE-RSA-AES256-GCM-SHA384`.
- Certificado do servidor: `CN=*.producaorestrita.nfse.gov.br`, emitido pela `AC SERPRO AR46 OV TLS CA 2025`. SHA-256: `A8:11:1F:75:84:42:9E:45:38:32:A0:F8:48:21:96:51:FD:15:F2:74:83:05:07:21:ED:05:0C:16:51:74:4D:98`. Validade até 02/03/2027.
- Certificado intermediário: `AC SERPRO AR46 OV TLS CA 2025`, emitido pela `GlobalSign Root R46`. SHA-256: `F2:70:95:D0:B4:0D:F7:65:59:94:04:01:1C:F8:B1:67:DA:06:26:0E:81:DD:A7:27:F1:63:EF:08:22:10:71:E4`. Validade até 15/10/2030.
- Raiz: `GlobalSign Root R46`. SHA-256: `4F:A3:12:6D:8D:3A:11:D1:C4:85:5A:4F:80:7C:BA:D6:CF:91:9D:3A:5A:88:B0:3B:EA:2C:63:72:D9:3C:40:C9`.
- Tipo da raiz: CA pública internacional. **Não é ICP-Brasil.**
- A raiz `GlobalSign Root R46` está no bundle de CAs do runtime KDE 6.11, arquivo `ca-bundle.crt` (SHA-256 do arquivo `9a9c1b798f6ae057ecb84545704e38c89e10f3db5519b24cd4647a1bb2dbf299`). A cadeia do servidor valida contra esse arquivo.

Ainda não medido: de qual arquivo o repositório de CAs do programa isolado é lido (seção 6, item 4). O passo 3.5 só confere que o arquivo `/etc/pki/tls/certs/ca-bundle.crt` existe dentro do programa.
