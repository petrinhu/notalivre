# Roteiro: emissão de teste em Produção Restrita (NL-2.10)

**Situação:** pronto para o líder, com uma verificação aberta antes do teste: a seção 6, item 4 (o Qt usa o arquivo de CAs do runtime dentro do programa isolado).
**Data:** 31/10/2026.
**Código:** branch `onda-2`, commit `cca6d76` (merge da NL-2.10b, com bundle, âncoras por ambiente e `--numero` conferido contra a DPS). Esta fatia está sobre ele.
**Bundle (programa instalável):** `/var/tmp/notalivre-nl210/bundle/run-41251698052/extraido/notalivre.flatpak`, 619896 bytes, gerado pelo CI do run `41251698052`, verde nos 10 jobs, commit `cca6d76`.
**SHA-256 do bundle:** `261eb964902215ca0e8b5a01ea65ad146c8835d706ffcf96f7418794db3350c2` (calculado pelo agente sobre o arquivo extraído do artefato `notalivre-bundle`). O digest do ZIP do artefato, dado pela API do GitHub, é `01dab6a02066c0f71c46171f4a995e087240a267329180f917f4d9f41b97e953`. O job não imprime o SHA-256 do `.flatpak`; por isso a conferência é a do ZIP contra a API e a do `.flatpak` contra este valor.

Nos comandos abaixo, o que está entre `< >` é um texto que você troca pelo seu valor. O caminho do seu certificado nunca aparece neste roteiro: use sempre `<caminho do seu .pfx>`.

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
sha256sum /var/tmp/notalivre-nl210/bundle/run-41251698052/extraido/notalivre.flatpak
```

O resultado tem de ser exatamente `261eb964902215ca0e8b5a01ea65ad146c8835d706ffcf96f7418794db3350c2`. Se for outro, pare e avise o agente.

### 3.2 Instalar o programa

```
flatpak install --user --bundle /var/tmp/notalivre-nl210/bundle/run-41251698052/extraido/notalivre.flatpak
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
- A chave `sefin.url` tem de ser exatamente o endereço acima. Se for outro endereço, o programa para com saída 2 e diz qual endereço recebeu.
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
flatpak run --command=notalivre-cli io.github.petrinhu.notalivre emitir "<caminho da DPS no app>" --serie 1 --numero 1 --ambiente producao-restrita 2> /var/tmp/notalivre-nl210/teste-lider/log-tentativa-1.txt
```

Como funciona:

- Quando aparecer `senha do certificado:`, digite a senha e tecle Enter. Nada aparece na tela enquanto você digita, e isso é o esperado.
- O argumento `--numero` tem de ser igual ao `nDps` do arquivo, e o `--serie` igual ao `serie`. O programa confere os dois contra a DPS: se algum for diferente, ele recusa a emissão com saída 2 e diz qual campo não bate. Ele não escolhe um dos dois valores.
- Códigos de saída que você pode ver nesta etapa:
  - **2:** uso errado da linha de comando, `--serie` ou `--numero` diferente da DPS, ou endereço (`sefin.url`) que não é de Produção Restrita. A mensagem diz o que corrigir.
  - **10:** problema de configuração: chave vazia, chave repetida, chave que não pode existir em Produção Restrita, chave obrigatória ausente, ou certificado ilegível.
  - Qualquer outro código de erro da Sefin não entra nesta tabela: devolva o log ao agente.
- Se der certo, a última linha da tela diz `emitida: chave` seguida de um número de 50 caracteres. Esse é o número da nota de teste.
- O log, com os eventos do programa, fica no arquivo `log-tentativa-1.txt`. A senha não entra nesse arquivo.

Se a tela mostrar uma mensagem de erro, copie o código que aparece entre parênteses e o texto dele. Veja a seção 4 para devolver o log.

### 3.10 Tentativa 2: segunda base

Faça esta tentativa só se a Sefin responder que a rota não existe (um erro 404, ou uma mensagem dizendo que o endereço não foi encontrado).

1. Abra o `config.ini` e troque a linha `prefixo` para `prefixo = /API/SefinNacional`.
2. No arquivo `dps-teste.json`, troque `nDps` para `2`.
3. Refaça o passo 3.8 se precisar, com o mesmo CNPJ e o mesmo cTribNac.
4. Rode de novo, com o número novo:

```
flatpak run --command=notalivre-cli io.github.petrinhu.notalivre emitir "<caminho da DPS no app>" --serie 1 --numero 2 --ambiente producao-restrita 2> /var/tmp/notalivre-nl210/teste-lider/log-tentativa-2.txt
```

Se a tentativa 1 já tiver ido para o endereço certo e a Sefin tiver recusado por outro motivo, não faça a tentativa 2. Essa decisão é do agente, com o log na mão.

### 3.11 Tentativa 3: algoritmo rsa-sha256

Faça esta tentativa só se a Sefin responder com o código `E0714` (assinatura com algoritmo recusado).

1. No `config.ini`, na seção `[assinatura]`, troque a linha para `algoritmo = rsa-sha256`.
2. Use a base que não deu 404 nas tentativas anteriores. Não mude o `prefixo` sem motivo.
3. No `dps-teste.json`, troque `nDps` para o próximo número livre, por exemplo `3`.
4. Rode com o número novo:

```
flatpak run --command=notalivre-cli io.github.petrinhu.notalivre emitir "<caminho da DPS no app>" --serie 1 --numero 3 --ambiente producao-restrita 2> /var/tmp/notalivre-nl210/teste-lider/log-tentativa-3.txt
```

Se vier qualquer outro código de erro, que não seja 404 ou E0714, pare. Não tente de novo às cegas: devolva o log ao agente.

---

## 4. Onde ficam o log e as respostas, como devolver e como apagar

**O log do programa** é o texto que a CLI imprime enquanto roda. Com o `2>` dos comandos acima, ele vai para `/var/tmp/notalivre-nl210/teste-lider/log-tentativa-N.txt`, fora de qualquer pasta sincronizada.

**As respostas cruas da Sefin** são gravadas pelo próprio programa, em um arquivo por DPS, dentro de uma pasta chamada `respostas`. Essa pasta fica na pasta de dados do app, que está dentro de `~/.var/app/io.github.petrinhu.notalivre`. Para achar a pasta:

```
find ~/.var/app/io.github.petrinhu.notalivre -type d -name respostas
```

Cada arquivo sai com permissão só para você (0600). O conteúdo é mascarado pelo programa, mas continua sendo a resposta da Sefin para a sua nota, então trate como dado pessoal.

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

Depois que o agente confirmar a análise, apague também as cópias em `/var/tmp/notalivre-nl210/devolucao` e a pasta `/var/tmp/notalivre-nl210/teste-lider`, sempre um caminho exato de cada vez.

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

Os três pontos que bloqueavam o teste antes da NL-2.10b estão resolvidos no código do `cca6d76`. Fica um ponto aberto.

1. **Esquemas XSD (resolvido).** O bundle instala os XSD conferidos em `/app/share/notalivre/xsd`, e o programa usa essa pasta sozinho. Por isso o `config.ini` não tem `[xsd]`.
2. **Raízes da ICP-Brasil (resolvido).** O bundle instala as raízes em `/app/share/notalivre/icp-brasil`, e o programa as usa sozinho para validar o seu certificado. Por isso o `config.ini` não tem `certificado.ancoras`.
3. **`--serie` e `--numero` (resolvido).** Os dois entram na DPS e são conferidos contra o JSON. Conflito sai com saída 2, sem escolher um valor (seção 3.9).
4. **Aberto (VERIF-1): confirmar que o programa isolado usa o arquivo de CAs do runtime.** A CLI não recebe o caminho desse arquivo. Ela usa o repositório de CAs que o sistema oferece dentro do programa. Ainda não se mediu, de dentro do programa isolado, de qual arquivo esse repositório vem. Se não houver nenhum repositório de CAs, a CLI recusa antes de conectar e não envia nada. Se houver um repositório diferente do runtime KDE 6.11, a verificação não fecha e o agente registra qual é. Esta verificação é feita pelo agente, antes do seu teste, sem comando novo seu além dos já descritos neste roteiro.

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
