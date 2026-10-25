# NotaLivre: spec visual (v1, fechada para implementação; itens em aberto listados na seção 10)

**Status:** decisões do líder registradas nas rodadas 1 a 5. Tokens, telas e handoff prontos para o frontend-engineer.
**Data:** 25/10/2026.
**Modo:** greenfield. Colaborativo. Pontos de decisão visuais: 5. Decididos: 5 de 5.
**Base funcional:** `docs/superpowers/specs/2026-10-25-notalivre-design.md` (rascunho v1, não aprovado). As telas daqui seguem a seção 4 dele.
**Mockups:** `docs/design/mockups/` (seção 11).

## Legenda de rótulos

- **Decisão do líder:** tomada pelo líder, em AskUserQuestion, com a resposta registrada aqui. Verbatim quando o texto dele foi captado.
- **Proposta do diretor, a confirmar:** proposta deste agente. Não vale como decisão.
- **Fato:** dado com fonte.
- **Conforme spec funcional:** decisão já registrada no spec funcional, citada por rodada. Não foi recapturada nesta rodada visual.
- **EM ABERTO:** o líder ainda não decidiu.

## Avisos

- **AVISO:** não há `TODO.md` na raiz de `Projects/nfse/`. Recomendo gerar via `/tab_pendencias` antes de prosseguir.
- **Limite de ferramenta:** este agente roda como subagente, sem `AskUserQuestion`. As decisões chegaram pelo orquestrador.
- **Fidelidade:** os mockups são HTML/CSS de referência. O Qt Quick não reproduz 1:1 (seção 9).
- **Verificação:** o render dos mockups foi conferido por captura headless própria, com perfil descartável. A auditoria visual formal cabe ao `qa-engineer` (L-13), ainda não acionado.

## 1. Pesquisa (fontes e por que importam)

| Fonte | O que diz, relevante ao visual | Por que importa |
|---|---|---|
| KDE HIG, layout e navegação, https://develop.kde.org/hig/layout_and_nav/ | Sidebar à esquerda no desktop, sempre visível. Até cinco destinos: barra de abas. Menu hambúrguer só com cerca de 15 itens. Agrupamento por espaçamento: menor dentro do grupo, maior entre grupos. | Sustenta a navegação lateral e o ritmo de espaçamento. |
| KDE HIG, estados, https://develop.kde.org/hig/status_changes/ | Sucesso se mostra como mudança visual no elemento. Erro deve ser acionável. "Don't rely on color alone to convey meaning or invite action; also use different icons, shapes, or text." | Cor nunca é o único portador de sentido. |
| KDE HIG, acessibilidade, https://develop.kde.org/hig/accessibility/ | Foco visível e distinto do selecionado. Leitor de tela com rótulo e tipo em cada elemento. Informação importante fora de tooltip. Não traz razão de contraste nem tamanho de alvo. | Contraste medido (seção 5). Alvo mínimo é proposta (seção 10). |
| Qt 6.12, estilos de Qt Quick Controls, https://doc.qt.io/qt-6/qtquickcontrols-styles.html | Estilos Basic, Fusion, Material, Universal, FluentWinUI3, Imagine e nativos de macOS, iOS e Windows. No Linux, o padrão é Fusion. Breeze não é citado. | Não há estilo Breeze pronto. Estilo próprio sobre Basic (seção 9). |
| Plasma 6.7, cantos arredondados em realces (Phoronix, 28/02/2026), https://phoronix.com/news/plasma-6.7-rounded-ui-qtwidgets | KDE arredonda realces de seleção para casar com componentes Qt Quick. | Apoia cantos moderados de 10 a 14 px. |
| Portal NFS-e, https://www.gov.br/nfse/pt-br | Menu "MEI e demais empresas" e atalho "Emitir Nota Fiscal" para o Emissor Nacional. Cores e formulário não foram lidos. | O MEI reconhece o vocabulário oficial. Nomes de campo iguais aos do governo. |
| Dados de tabela e dashboard para finanças, busca de 2026 (sites de opinião) | Densidade como opção. Usuário financeiro quer tabela e dados. Card para cada métrica é criticado. | Base da lista de notas em tabela. Fonte fraca, tratada como opinião. |
| NN/g 2026, data tables | Não encontrado. | Declarado como não achado. |
| UX de apps de NFS-e para MEI, 2026 | Não encontrada. Achei o app estadual do Pará (NFF, IN 013/2026), sem análise de design. | Sem benchmark de concorrente. |

## 2. Decisões do líder

**Rodada 1, personalidade (25/10/2026).** Verbatim: "opcao B. Talao. Mas o campo de descricao do servico deve ser mais de uma linha. Observe o limite de chars qual é, mas é maior que campos de uma linha somente."
- Direção: **B, Talão**. Canhoto de nota em papel, picote, abas de série em pílula.
- Descrição do serviço: área de várias linhas, com contador. Limite: **1000 caracteres**. Fato: Anexo I v1.01, aba "LEIAUTE DPS_NFS-e", linha 197, xDescServ, tipo C, obrigatório (1-1), tamanho 1000. Fonte: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual/anexo_i-sefin_adn-dps_nfse-snnfse-v1-01-20260209.xlsx (lido localmente).

**Rodada 2, paleta (25/10/2026, AskUserQuestion do orquestrador).** Paleta "Musgo (Recomendado)": #2F5D50 no claro, #8FD0B7 no escuro.

**Rodada 3, tipografia (25/10/2026).** "Bricolage + Atkinson (Recomendado)". Título em Bricolage Grotesque, corpo em Atkinson Hyperlegible. Licença: OFL (metadados oficiais do Google Fonts, campo `license`: Bricolage Grotesque, Atkinson Hyperlegible, copyright Braille Institute of America).

**Rodada 4, navegação (25/10/2026).** "Barra lateral (Recomendado)". Sidebar à esquerda, sempre visível, com os oito destinos reais do spec funcional: Emissão, Lote, Tomadores, Histórico, Modelos, Emissores, Tabelas, Configurações. Assistente de primeiro uso é fluxo, não destino.

**Rodada 5, densidade (25/10/2026).** "Confortável (Recomendado)". Linha de tabela de 52 px, campo de 44 px, texto de 16 px.

**Ponto aberto para o Cláudio (não decidido aqui):** OFL 1.1 é software livre, mas a compatibilidade com GPL é contestada. Parecer da Red Hat, no kernel Linux (2023): https://lkml.iu.edu/hypermail/linux/kernel/2311.0/01472.html. A FSF não foi lida (HTTP 429). As fontes entram como arquivos separados no Flatpak. Cláudio confirma antes do empacotamento.

**Resoluções posteriores dos itens em aberto (25/10/2026).** Origem marcada em cada item (tabela da seção 10).
- **Decisão do líder:** senha do certificado, padrão ao reabrir o app, após 15 min parado e ao bloquear a tela; "a cada 20 notas" disponível e desligada por padrão; sempre antes de Produção. Banco: o assistente mostra script SQL para colar com sudo; senha de administrador fica só em memória, e só quando o servidor aceitar; MariaDB aceito (Manjaro usa mariadb). Vários emissores por instalação, sem limite fixo. CPF como emissor entra na v1. Atualizar tabelas "se houver versao nova apenas" (rodada 8, já no spec funcional): não é desvio. Botão de atualizar tabelas no cabeçalho. Outlook por OAuth, com registro no Entra em nome do líder.
- **Decisão do orquestrador, a confirmar:** código de recuperação mostrado uma vez no primeiro uso, com opção de salvar em arquivo ou imprimir; o app não guarda em claro (detalhe no ADR de Narciso). Variáveis do e-mail e anexos. Anexo VII v1.03.00, versão gravada por DPS. Alvo mínimo de 40 px. Contador por caracteres Unicode. Sidebar recolhível para ícones. Estados "Emitindo..." e "Nota emitida" com número. Preferência de movimento reduzido do sistema. Parecer do Cláudio sobre OFL e GPL entra no plano antes do empacotamento.
- **Responsabilidade de texto:** texto da confirmação forte em Produção fica com o `ux-writer`.

## 3. Tokens (paleta Musgo, claro e escuro)

Hex exatos. A contagem de contraste está na seção 5. Formato de entrega: bloco JSON abaixo, que vira o singleton `Theme` (seção 9).

### 3.1 Cores

| Token | Claro | Escuro | Uso |
|---|---|---|---|
| `bg` | #EDF2EE | #111A17 | fundo da janela |
| `surface` | #FFFFFF | #18241F | campo, card, tabela |
| `text-1` | #16241E | #E6F0EB | texto principal |
| `text-2` | #4B5E56 | #A7BAB1 | rótulo, ajuda, contador |
| `border` | #6F8178 | #6E8379 | borda de campo e de botão secundário |
| `border-soft` | #D3DDD6 | #25352E | divisória, borda de card |
| `brand` | #2F5D50 | #8FD0B7 | botão primário, marca, aba ativa |
| `brand-hover` | #26493F | #A5DEC9 | botão primário em hover |
| `brand-press` | #1E3B33 | #7BC2A8 | botão primário pressionado |
| `on-brand` | #FFFFFF | #0E1714 | texto sobre `brand` |
| `hover` | #E1EAE4 | #1F2F28 | linha, item de menu em hover |
| `sel` | #D3E4DB | #21372E | linha selecionada |
| `focus` | #2F5D50 | #8FD0B7 | anel de foco |
| `ok` | #2F6B3A | #8FD0B7 | situação emitida |
| `warn` | #7A4A00 | #E8B65B | aviso |
| `danger` | #A3262B | #FF9A9E | erro, cancelada, falha |
| `faixa-bg` | #F6E7C8 | #2E2614 | faixa de Produção Restrita |
| `faixa-fg` | #5C3600 | #F3D08C | texto da faixa |
| `stamp` | #8A4B00 | #E8B65B | ícone de aviso |
| `on-stamp` | #FFFFFF | #111A17 | glifo sobre `stamp` |
| `dis-bg` | #DDE4E0 | #223029 | fundo desabilitado |
| `dis-fg` | #5E6E67 | #6F817A | texto desabilitado |

### 3.2 Tipografia

| Papel | Família | Tamanho / peso (confortável) | Uso |
|---|---|---|---|
| `display` | Bricolage Grotesque | 26 px / 700 | título de janela (h1) |
| `h2` | Bricolage Grotesque | 21 px / 700 | título de tela |
| `h3` | Bricolage Grotesque | 17 px / 700 | seção de formulário |
| `brand` | Bricolage Grotesque | 22 px / 700 | marca no cabeçalho |
| `body` | Atkinson Hyperlegible | 16 px / 400 | texto corrido, campo, tabela |
| `label` | Atkinson Hyperlegible | 15 px / 700 | rótulo de campo, cabeçalho de tabela |
| `help` | Atkinson Hyperlegible | 14 px / 400 | ajuda, contador, rodapé de campo |
| `button` | Atkinson Hyperlegible | 15 px / 700 | botão |

Regra: rótulo em caixa normal, nunca em caixa alta espaçada. Números em algarismos tabulares, alinhados à direita nas tabelas e nos valores. Fonte mono não entra.

### 3.3 Espaçamento, raio, sombra, foco e movimento

- **Espaçamento:** base de 4 px. Escala 4, 8, 12, 16, 24, 32. Dentro de grupo, 8 a 12 px. Entre grupos, 16 a 24 px.
- **Altura de controle:** campo 44 px, botão 40 px, linha de tabela 52 px, item de navegação 44 px. (Compacta, se for escolhida no futuro, seria 32, 32, 34 e 34 px. Fica fora da v1.)
- **Raio:** campo 10 px, card 12 px, janela e diálogo 14 px, faixa 10 px, botão em pílula (999 px), checkbox 5 px, radio redondo.
- **Sombra:** no claro, janela com `0 1px 0 #D3DDD6, 0 10px 24px rgb(0 0 0 / .10)` e diálogo com `0 10px 24px rgb(0 0 0 / .14)`. No escuro, sem sombra: separação por `border-soft`.
- **Foco:** anel de 2 px com deslocamento de 2 px, cor `focus`. Visível em todo controle, nunca só por cor de fundo.
- **Estado de erro:** borda `danger` em 1,5 px, texto `danger` em negrito, mensagem com o que houve e como corrigir. Nunca só cor.
- **Estado desabilitado:** `dis-bg` e `dis-fg`. Isento de contraste pelo WCAG, medido só como informação (seção 5). O motivo do bloqueio fica escrito perto do controle.
- **Movimento:** entrada da tela em fade de 300 ms e deslocamento de 8 px, curva `OutCubic`. Sem animação em loop. Respeita a preferência de movimento reduzido do sistema (orquestrador, a confirmar).

### 3.4 Bloco de tokens para o frontend

```json
{
  "tema": "musgo-talao",
  "claro": {"bg":"#EDF2EE","surface":"#FFFFFF","text1":"#16241E","text2":"#4B5E56","border":"#6F8178","borderSoft":"#D3DDD6","brand":"#2F5D50","brandHover":"#26493F","brandPress":"#1E3B33","onBrand":"#FFFFFF","hover":"#E1EAE4","sel":"#D3E4DB","focus":"#2F5D50","ok":"#2F6B3A","warn":"#7A4A00","danger":"#A3262B","faixaBg":"#F6E7C8","faixaFg":"#5C3600","stamp":"#8A4B00","onStamp":"#FFFFFF","disBg":"#DDE4E0","disFg":"#5E6E67"},
  "escuro": {"bg":"#111A17","surface":"#18241F","text1":"#E6F0EB","text2":"#A7BAB1","border":"#6E8379","borderSoft":"#25352E","brand":"#8FD0B7","brandHover":"#A5DEC9","brandPress":"#7BC2A8","onBrand":"#0E1714","hover":"#1F2F28","sel":"#21372E","focus":"#8FD0B7","ok":"#8FD0B7","warn":"#E8B65B","danger":"#FF9A9E","faixaBg":"#2E2614","faixaFg":"#F3D08C","stamp":"#E8B65B","onStamp":"#111A17","disBg":"#223029","disFg":"#6F817A"},
  "fontes": {"titulo":"Bricolage Grotesque","corpo":"Atkinson Hyperlegible"},
  "escala": {"display":26,"h2":21,"h3":17,"marca":22,"body":16,"label":15,"help":14,"button":15},
  "espaco": [4,8,12,16,24,32],
  "raio": {"campo":10,"card":12,"janela":14,"faixa":10,"pilula":999,"checkbox":5}
}
```

## 4. Componentes (uma vez, em todas as telas)

- **Botão primário:** `brand` com `on-brand`. Hover em `brand-hover`, pressionado em `brand-press`, carregando com indicador e texto que não muda, desabilitado em `dis-bg`.
- **Botão secundário:** borda `border`, texto `brand`, hover em `hover`.
- **Campo:** rótulo acima, borda `border` (3:1 contra `surface`), ajuda abaixo. Erro: borda `danger` e mensagem.
- **Área de texto:** campo com contador "Restam N de 1000 caracteres" alinhado à direita.
- **Lista suspensa:** campo com lista de opções. Padrão sempre no topo, marcado.
- **Caixa de seleção e radio:** rótulo ao lado, área de toque de 32 px ou mais.
- **Tabela:** cabeçalho em `text-2` negrito, linha de 52 px, linha selecionada em `sel`, hover em `hover`. Situação sempre com ícone e texto (bolinha colorida mais palavra).
- **Badge:** pílula com texto, sem cor sozinha: "Último usado", "padrão", "CPF".
- **Diálogo:** `surface`, borda de 1,5 px, raio 14 px, ações no canto inferior direito. Ação principal à direita.
- **Faixa de Produção Restrita:** fixa no topo da janela, abaixo do cabeçalho, em todas as telas enquanto o ambiente for Restrita. Texto: "Produção Restrita. As notas emitidas aqui não têm validade fiscal." Com ícone. Não pode ser fechada.

## 5. Contraste (WCAG 2.2 AA, medido contra o fundo real)

Medido com a fórmula de luminância relativa do WCAG. Critérios: texto 4,5:1; componente, foco e borda 3:1. Script de medição na pasta de rascunho da sessão, não versionado.

**Paleta Musgo, conjunto completo de tokens: 50 pares medidos, 0 reprovas.** Principais:

| Tema | Par | Razão | Critério |
|---|---|---|---|
| claro | `text-1` / `surface` | 16,09:1 | 4,5:1 |
| claro | `text-2` / `surface` | 6,92:1 | 4,5:1 |
| claro | `on-brand` / `brand` | 7,49:1 | 4,5:1 |
| claro | `on-brand` / `brand-hover` | 9,97:1 | 4,5:1 |
| claro | `on-brand` / `brand-press` | 12,15:1 | 4,5:1 |
| claro | `border` / `surface` | 4,13:1 | 3:1 |
| claro | `focus` / `bg` | 6,61:1 | 3:1 |
| claro | `danger` / `surface` | 7,32:1 | 4,5:1 |
| claro | `faixa-fg` / `faixa-bg` | 8,67:1 | 4,5:1 |
| claro | `text-1` / `sel` | 12,17:1 | 4,5:1 |
| claro | `text-2` / `hover` | 5,63:1 | 4,5:1 |
| escuro | `text-1` / `surface` | 13,75:1 | 4,5:1 |
| escuro | `text-2` / `surface` | 7,86:1 | 4,5:1 |
| escuro | `on-brand` / `brand` | 10,31:1 | 4,5:1 |
| escuro | `on-brand` / `brand-hover` | 12,07:1 | 4,5:1 |
| escuro | `on-brand` / `brand-press` | 8,79:1 | 4,5:1 |
| escuro | `border` / `surface` | 3,96:1 | 3:1 |
| escuro | `focus` / `bg` | 10,03:1 | 3:1 |
| escuro | `danger` / `surface` | 7,91:1 | 4,5:1 |
| escuro | `faixa-fg` / `faixa-bg` | 10,13:1 | 4,5:1 |
| escuro | `text-1` / `sel` | 10,92:1 | 4,5:1 |
| escuro | `text-2` / `hover` | 6,89:1 | 4,5:1 |

**Desabilitado (isento pelo WCAG, medido como informação):** claro 4,16:1; escuro 3,35:1. Por isso o texto explicativo fica sempre perto do controle desabilitado.

**Pares de rodadas anteriores:** 92 pares das três direções (rodada 1) e 54 pares das três candidatas Talão (rodada 2), todos sem reprovas.

## 6. Navegação e shell

- **Cabeçalho:** marca "NotaLivre" à esquerda, emissor atual com "Trocar emissor" à direita. Altura de 44 px.
- **Faixa fixa de Produção Restrita:** logo abaixo do cabeçalho, em todas as telas (seção 4).
- **Sidebar:** 184 px de largura, dois grupos ("Notas": Emissão, Lote, Tomadores, Histórico; "Cadastros": Modelos, Emissores, Tabelas, Configurações). Item ativo com fundo `hover`, barra de 3 px em `brand` e texto em `brand` negrito. Não depende só de cor.
- **Botão "Atualizar tabelas"** no cabeçalho da janela, visível em toda tela. Decisão do líder: "visivel em alguma barra de botoes/menus da interface principal" (rodada 8, verbatim do spec funcional).
- **Colapso da sidebar:** recolhível para ícones, com rótulo acessível (orquestrador, a confirmar).

## 7. Telas

Mockup de referência: `docs/design/mockups/04-telas-principais.html` (claro e escuro, cada tela). Textos de botão seguem a regra de resultado: o botão nomeia a ação, e o estado de sucesso mostra o resultado.

### 7.1 Emissão

- **Objetivo:** emitir uma NFS-e pela API oficial sem se perder no formulário.
- **Layout:** cabeçalho, faixa de Produção Restrita, série e próxima DPS no topo. Blocos: Tomador, Serviço, Confirmação para Produção (aparece só em Produção). Rodapé com "Salvar rascunho" e "Emitir nota".
- **Campos (nomes do leiaute):** CNPJ ou CPF do tomador; Nome ou razão social; Município do tomador; caixa "Atualizar o cadastro deste tomador ao emitir (mostra o que muda)"; Descrição do serviço (área de texto, até 1000 caracteres, xDescServ); Valor do serviço; Alíquota do ISSQN; Classificação para IBS e CBS (sugerida pelo serviço).
- **Texto de ajuda da classificação:** "Confirme antes de emitir. O cálculo do imposto não é feito pelo app." (a DPS v1.01 não traz alíquota: conforme spec funcional, seção 4.11).
- **Confirmação forte em Produção:** digitar o CNPJ do emitente para liberar "Emitir nota". Conforme spec funcional, rodada 5 (paráfrase). O botão fica desabilitado até conferir, e a mensagem diz o que falta. Texto exato: ux-writer.
- **Estados:**
  - vazio: campos vazios com ajuda por campo;
  - validando: erro no campo com mensagem, por exemplo "Descrição do serviço passou de 1000 caracteres. Encurte o texto para emitir.";
  - carregando: botão mostra "Emitindo..." com indicador, campos bloqueados (orquestrador, a confirmar);
  - erro da API: "Não foi possível emitir. [motivo do código E]. [o que fazer]." Exemplo: "O município não aceitou o código do serviço. Escolha outro item da lista e emita de novo.";
  - sucesso: o resultado aparece como "Nota emitida" com o número da NFS-e (orquestrador, a confirmar).

### 7.2 Lote

- **Objetivo:** emitir várias notas a partir de uma planilha, sem perder a numeração e sem reemitir às cegas.
- **Layout:** cabeçalho com nome da planilha e total de linhas. Barra de progresso com texto ao lado ("34 de 40 processadas"). Tabela com Tomador, Valor, Situação e Motivo. Relatório de falhas no rodapé.
- **Comportamento:** o lote continua até o fim e relata no fim. Só as falhas são reenviadas (conforme spec funcional, rodada 3). Antes de reenviar, o app consulta a DPS (GET /dps/{id}), para não emitir de novo nota que já saiu (spec funcional, seção 4.14).
- **Relatório de falhas:** motivo com código e ação, por exemplo "E0234: endereço obrigatório para este caso" e "E0190: CNPJ ou CPF não encontrado na data da competência". Ação: "Exportar relatório" e "Reenviar N falhas".
- **Estados:** vazio ("Escolha uma planilha para começar"), em andamento (barra e contagem), concluído com falhas (relatório), concluído sem falhas ("Lote concluído. 40 notas emitidas."), erro de leitura da planilha com a coluna que faltou.
- **Colunas e tamanho máximo do lote:** EM ABERTO.

### 7.3 Tomadores

- **Objetivo:** guardar quem já recebeu nota, para não digitar de novo.
- **Layout:** lista com busca por nome e documento. Por tomador: editar, exportar, excluir (LGPD, conforme spec funcional, rodada 3).
- **Atualização perguntada ao emitir:** quando o usuário muda o endereço ou outro dado de um tomador já cadastrado, o app pergunta antes de emitir. O diálogo mostra uma tabela Campo, Cadastrado, Novo. Ações: "Emitir sem atualizar" (secundária) e "Atualizar e emitir" (primária). Caixa "Sempre atualizar o cadastro ao emitir". Conforme spec funcional, rodada 6 (paráfrase).
- **Aviso fixo:** "A nota emitida guarda os dados da época, mesmo que o cadastro mude depois." Conforme spec funcional, rodada 6.
- **Estados:** vazio ("Nenhum tomador ainda. Eles aparecem aqui depois da primeira nota emitida."), busca sem resultado, erro de consulta de CNPJ com ação ("Digite os dados à mão e siga.").

### 7.4 Histórico

- **Objetivo:** consultar, imprimir e reenviar notas emitidas.
- **Layout:** busca no topo. Tabela: Número da NFS-e (13 dígitos, tabular), Emissão, Tomador, Valor, Situação, Ações. Ações por linha como links: Imprimir, Enviar por e-mail, Ver motivo, Ver nota nova.
- **Situação:** Emitida (`ok`), Cancelada (`danger`), Substituída (`text-2`). Sempre com texto.
- **Número:** o número é atribuído pela Sefin Nacional (spec funcional, R4). A tela mostra essa origem na ajuda do cabeçalho.
- **Estados:** vazio ("Nenhuma nota emitida ainda."), busca sem resultado, erro de consulta com ação, carregando com linhas em esqueleto.

### 7.5 Modelos (nota e e-mail)

- **Objetivo:** reutilizar texto e dados fixos.
- **Modelo de e-mail (lista suspensa):** o primeiro item é sempre o padrão pré-formatado, marcado "padrão", não editável. Depois, os modelos do usuário, marcados "meu modelo". Último item: "Criar modelo próprio". Conforme spec funcional, rodada 7, verbatim: "Nas opcoes do app deve ter já um modelo pre-formatado de email para encaminhar a nota com opcao de criar modelo proprio, ou mais de um modelo, selecionaveis em menu dropdown. A primeira opcao sempre o default/pre-formatado".
- **Variáveis (orquestrador, a confirmar):** {tomador_nome}, {numero_nfse}, {data_emissao}, {valor}, {emissor_nome}, {chave_acesso}. PDF da nota (DANFSe) e XML anexos por padrão.
- **Prévia:** assunto e corpo com variáveis já preenchidas no exemplo.
- **Modelo de nota:** pré-preenche descrição do serviço e alíquota. Tomador e valor são escolhidos ao emitir.
- **Estados:** modelo padrão sem botão "Salvar" (não editável), modelo próprio com "Salvar modelo", erro de variável inexistente com a lista das variáveis válidas.

### 7.6 Emissores

- **Objetivo:** trocar de emissor sem perder o contexto.
- **Layout:** lista de cards. Por emissor: nome, CPF ou CNPJ, ambiente, certificado A1 com validade. O emissor usado por último tem a badge "Último usado". Ações: "Editar", "Trocar certificado", e, no rodapé, "Importar certificado A1" e "Trocar para [nome]".
- **Regras:** o app abre com o último emissor usado (conforme spec funcional, rodada 1, verbatim: "App deve abrir com o último usuário emissor utilizado."). Emissor CPF e CNPJ, com CNPJ como padrão. Cada emissor tem certificado, senha e numeração próprios (spec funcional, seção 4.5). CPF como emissor entra na v1 (decisão do líder).
- **Estados:** vazio ("Cadastre o primeiro emissor. Você precisa do certificado A1 em .pfx ou .p12."), certificado perto do vencimento com aviso e ação, certificado vencido com "Importar certificado A1" em destaque.
- **Limite de emissores:** vários emissores por instalação, sem limite fixo (decisão do líder).

### 7.7 Tabelas fiscais

- **Objetivo:** ver de que versão cada tabela está e atualizar.
- **Layout:** botão primário "Atualizar tabelas" no topo da área de conteúdo. Tabela: Tabela, Versão em uso, Origem, Situação. Caixa "Atualizar ao abrir o app, só quando houver versão nova".
- **Linhas:** Municípios e códigos municipais (API de localidades do IBGE); Classificações tributárias CST e cClassTrib (Informe Técnico 2025.002 v1.60, formato de download a confirmar); cIndOp (Anexo VII, v1.03.00, orquestrador, a confirmar); cNBS (embutida no pacote).
- **Estados:** atualizada (`ok`), versão nova não encontrada, verificar (`danger` com ação), sem rede (usa a versão embarcada, com aviso), erro de atualização com a tabela que falhou.
- **Regra:** atualiza ao abrir o app só quando há versão nova. Decisão do líder: "se houver versao nova apenas" (rodada 8, verbatim do spec funcional). Não é desvio.
- **Versão gravada:** cada DPS grava a versão da tabela usada (orquestrador, a confirmar). Anexo VII em v1.03.00, a mais recente da biblioteca (orquestrador, a confirmar).

### 7.8 Configurações

- **Envio por e-mail:**
  - provedor em lista suspensa, com os 11 provedores e "Personalizado" (seção 4.9 da spec funcional). Cada item diz o método: "OAuth, recomendado", "senha de app", ou "não confirmado pelo provedor";
  - Gmail: "Conectar com a conta Google" abre o navegador do sistema (OAuth, loopback, PKCE). Alternativa: "Usar senha de app". Gmail usa a API REST gmail.send (rodada 7 da spec funcional; checagem na implementação);
  - Outlook e Hotmail: "Conectar com a conta Microsoft" (OAuth2 Entra ID). Registro no Entra em nome do líder (decisão do líder).
- **Senha do certificado (decisão do líder):** padrão pedir ao reabrir o app, após 15 minutos parado e ao bloquear a tela. Opção "A cada 20 notas emitidas" disponível, desligada por padrão. Sempre pedir antes de emitir em Produção. Uma senha vale para o lote inteiro (spec funcional, rodada 3). Caixas: "Guardar a senha no chaveiro do sistema" e "Travar ao bloquear a tela".
- **Senha-mestra do banco:** separada da senha do certificado (rodada builder check, item b). Botões "Alterar senha-mestra" e "Gerar código de recuperação". Texto: "Sem ela e sem o código de recuperação, os dados cifrados não se abrem." Código de recuperação mostrado uma vez, no primeiro uso, com opção de salvar em arquivo ou imprimir. O app não guarda em claro (orquestrador, a confirmar; detalhe no ADR de Narciso).
- **Estados:** teste de envio com resultado e ação ("Enviar e-mail de teste"), senha de app recusada com instrução, OAuth cancelado com "Tentar de novo".

### 7.9 Assistente de primeiro uso (MySQL)

- **Objetivo:** deixar o banco pronto sem o usuário precisar saber SQL.
- **Passos:** (1) procurar o MySQL no sistema; (2) criar banco e usuário da NotaLivre; (3) ligar a cifra dos dados. Cada passo com número e estado (feito, pendente, falhou).
- **Banco (decisão do líder):** o assistente mostra um script SQL para colar com sudo, que cria banco e usuário. A senha de administrador fica só em memória, e só quando o servidor aceitar. MariaDB é aceito (Manjaro usa mariadb).
- **Se o MySQL ou MariaDB não estiver instalado:** mostra o comando da distribuição, copiável, e o botão "Testar de novo". O app não instala pacotes (conforme spec funcional, seção 4.4).
  - Fedora: `sudo dnf install mysql-server`
  - Ubuntu e Mint: `sudo apt install mysql-server`
  - Manjaro: `sudo pacman -S mariadb`
- **Credencial administrativa para o passo 2:** só em memória, e só quando o servidor aceitar (decisão do líder).
- **Estados:** encontrado, ausente com comando, falha de conexão com a causa e a ação, sucesso com "Banco pronto". "Continuar" fica desabilitado até o passo 2 passar, com o motivo escrito.

### 7.10 Confirmação forte para Produção

- Vive na tela de Emissão (seção 7.1). Mesma regra em Lote: uma confirmação para o lote inteiro.
- Visual conforme a seção 7.1. Texto exato: ux-writer.

## 8. Acessibilidade

- Contraste medido (seção 5). Cor nunca carrega sentido sozinha (ícone e texto em situação, erro, aviso e sucesso).
- Foco visível em todo controle. Ordem de tabulação da esquerda para a direita e de cima para baixo.
- Cada controle tem nome acessível. Teste com leitor de tela (Orca) na implementação.
- Fonte Atkinson Hyperlegible no corpo, desenhada para baixa visão.
- Alvo de clique mínimo de 40 px (orquestrador, a confirmar), na densidade confortável.

## 9. Handoff ao frontend-engineer

**Stack:** C++23, Qt 6.12, QML e Qt Quick Controls 6.12. Sem estilo Breeze nativo: estilo próprio sobre o estilo Basic, com todos os tokens da seção 3.4. O Fusion, padrão no Linux, não é usado como base de visual.

**Fontes:** Bricolage Grotesque e Atkinson Hyperlegible, OFL, carregadas por `QFontDatabase::addApplicationFont` a partir de recursos empacotados. Empacotamento no Flatpak depende do parecer da seção 2.

**Componentes sugeridos (QML):**
- `Theme.qml` (singleton): todos os tokens da seção 3.4. Único lugar com hex.
- Janela com cabeçalho, faixa de Produção Restrita e sidebar (`ListView` com `ItemDelegate`).
- `TextArea` com contador (descrição do serviço).
- `ComboBox` (provedor, modelo de e-mail, emissor) com o padrão no topo.
- `TableView` (Histórico, Lote, Tomadores, Tabelas), linha de 52 px, situação com ícone e texto.
- `Dialog` (atualização do tomador, confirmação de Produção, falhas).
- `RadioButton` e `CheckBox` com área de toque de 32 px ou mais.

**Plano faseado, com critério de pronto:**
1. **Fundação de tokens.** `Theme.qml` gerado do bloco JSON. Pronto quando: troca de tema claro e escuro sem reinício, e nenhum hex fora de `Theme.qml` (grep no código). Teste: os 50 pares da seção 5 são medidos em teste automatizado antes do visual (L-35).
2. **Shell e navegação.** Janela, sidebar, cabeçalho, emissor atual, faixa fixa, botão "Atualizar tabelas". Pronto quando: navegação por teclado completa, foco visível, item ativo sem depender só de cor.
3. **Componentes base.** Botão (todos os estados), campo com erro e ajuda, área de texto com contador, lista suspensa, tabela, diálogo, badge. Pronto quando: cada componente tem teste de estado antes do visual.
4. **Telas, nesta ordem:** Emissão, Histórico, Tomadores, Lote, Emissores, Tabelas, Modelos, Configurações, Assistente. Pronto quando: cada tela tem vazio, carregando, erro e sucesso implementados, e o `qa-engineer` revisou a captura.

## 10. Pendências após resolução (25/10/2026)

Nenhum item pendente depende do líder. Cada item está marcado pela origem.

| # | Item | Origem |
|---|---|---|
| 1 | Senha do certificado: padrão ao reabrir, 15 min parado, bloqueio de tela; 20 notas desligada; sempre antes de Produção | Decisão do líder |
| 2 | Código de recuperação: mostrado uma vez, salvar ou imprimir, sem guardar em claro | Orquestrador, a confirmar (ADR de Narciso) |
| 3 | Credencial do MySQL: script SQL com sudo, senha só em memória | Decisão do líder |
| 4 | MariaDB aceito (Manjaro) | Decisão do líder |
| 5 | Variáveis do e-mail e anexos PDF e XML | Orquestrador, a confirmar |
| 6 | Vários emissores por instalação, sem limite fixo | Decisão do líder |
| 7 | CPF como emissor na v1 | Decisão do líder |
| 8 | Atualizar tabelas só com versão nova: não é desvio | Decisão do líder (rodada 8) |
| 9 | Anexo VII v1.03.00, versão gravada por DPS | Orquestrador, a confirmar |
| 10 | Alvo mínimo de 40 px | Orquestrador, a confirmar |
| 11 | Contador por caracteres Unicode | Orquestrador, a confirmar |
| 12 | Texto da confirmação forte em Produção | ux-writer |
| 13 | Botão de atualizar tabelas no cabeçalho | Decisão do líder |
| 14 | Sidebar recolhível para ícones | Orquestrador, a confirmar |
| 15 | "Emitindo..." e depois "Nota emitida" com número | Orquestrador, a confirmar |
| 16 | Preferência de movimento reduzido do sistema | Orquestrador, a confirmar |
| 17 | Outlook por OAuth, registro no Entra em nome do líder | Decisão do líder |
| 18 | Parecer do Cláudio sobre OFL e GPL antes do empacotamento | Orquestrador, plano antes do empacotamento |

Itens marcados "orquestrador, a confirmar" são decisões do orquestrador. O líder não precisa responder agora, a não ser que queira revisar.

## 11. Índice dos mockups

| Arquivo | Conteúdo | Status |
|---|---|---|
| `docs/design/mockups/01-direcoes-personalidade.html` | Três direções, claro e escuro | Rodada 1, decidida: B |
| `docs/design/mockups/02-paleta-talao.html` | Três candidatas de cor, Talão | Rodada 2, decidida: Musgo |
| `docs/design/mockups/03-tipo-navegacao-densidade.html` | Pares tipográficos, navegação, densidade | Rodadas 3 a 5, decididas |
| `docs/design/mockups/04-telas-principais.html` | Nove telas principais em Musgo, claro e escuro | Fechada para handoff |

## 12. Escopo v1 e o que fica depois

- **Entra na v1:** tudo da seção 7, nos dois temas.
- **Fica para depois:** visual de Windows e macOS, temas além de claro e escuro, idiomas além de pt-br, telas de escritório contábil multi-cliente, densidade compacta (desenhada na rodada 5, não entra na v1).
- **Placeholder honesto:** tela de escritório contábil, se aparecer no menu: "Disponível em uma versão futura."
