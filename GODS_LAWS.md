# GODS_LAWS.md: NotaLivre (nfse)

> Ordens do líder para este projeto. Cada lei tem um gatilho, conferido no momento da ação.

## Protocolo de uso

1. Antes de agir, confira o índice de gatilhos do `CLAUDE.md` deste projeto; se um gatilho casa, leia a lei inteira antes do primeiro comando.
2. Siga o protocolo de uso do `~/.claude/GODS_LAWS.md`. No que este arquivo silencia, vale a lei global; `L-NN` sem prefixo é lei global e `NF-NN` é lei daqui. `NL-<onda>.<fatia>` é ID de fatia do `TODO.md`, não lei.
3. As leis com "decisão do líder" ou "ordem do líder" na linha **Data** são decisões do líder. As marcadas "regra de execução (lei global)" decorrem das leis globais, citadas por número, e não são decisões do líder. A tabela completa dos pontos L1 a L10 está na seção 6.2 do plano.
4. Verbatim do líder é só o que está entre aspas, com a grafia dele. O que vem marcado "leitura registrada (L-07)", "redação do Chief of Staff (L-07)", "a ratificar" ou "INFERÊNCIA" conserva essa marca e não é verbatim.

---

## NF-01

**Data:** decisão do líder, 25/10/2026 (ratificada).

Mantenha o README no repositório e não crie o `.env.example`, porque a configuração mora no app.

## NF-02

**Data:** regra permanente.

Organize o código em camadas com dependência só para baixo, conforme a seção 1.1 do plano (L-33). O portão de camadas (NL-0.3) varre os `#include` contra esta tabela.

| Camada | Diretório | O que mora ali | Pode depender de | Proibido |
|---|---|---|---|---|
| Front | `apps/cli`, `apps/gui` | CLI e QML com seus modelos de vista em C++ | fachadas de `src/mid/composicao` | SQL, cripto, HTTP, regra fiscal, include de `src/back` ou `src/foundation` exceto `foundation/base` |
| Mid | `src/mid/<adaptador>` | adaptadores que implementam os ports do Back sobre as primitivas da Foundation; raiz de composição; fachadas | `src/back`, `src/foundation` | QML, widget |
| Back | `src/back/domain`, `src/back/app`, `src/back/ports` | entidades, regras fiscais, casos de uso, interfaces (ports) | só `src/foundation/base` | rede, SQL, cripto, XML, arquivo, Qt GUI |
| Foundation | `src/foundation/<motor>` | motores agnósticos de domínio que embrulham terceiros; schema SQL; config | nada interno acima; `base` não depende de nenhum outro | qualquer tipo do domínio |

Dê a cada adaptador do Mid diretório e razão de mudar próprios (L-34): `sefin`, `adn`, `persistencia`, `cofre`, `assistente_banco`, `correio`, `oauth`, `danfse`, `tabelas`, `cnpj`, `lote`, `composicao`.

## NF-03

**Data:** decisão do líder, 25/10/2026 (ratificada).

Escreva os identificadores em português sem acento, alinhados ao leiaute (Dps, Tomador, Emissor). Classes em PascalCase; arquivos em snake_case. Erro por `std::expected<T, Erro>` (C++23).

**INFERÊNCIA do CTO, a confirmar em NL-0.4:** o GCC 13 do Mint 22.3 tem `<expected>`.

## NF-04

**Data:** decisão do líder, 25/10/2026, ponto L7. **Verbatim (na confirmação da exceção):** "autorizo containers locais" e "pode deixar tudo nas pastas padrao". Execução pelo piso de testes: decisão do líder, 21/11/2026, por AskUserQuestion: "Lei nova: CI decide (Recomendado)". Alinhada à lei global por AskUserQuestion, 23/11/2026: *"Alinhar ao global (Recomendado)"*. Conflito com o texto de 21/11/2026 decidido por AskUserQuestion, 27/11/2026: *"Vale o 23/11 (Recomendado)"*.

Nunca rode na sessão viva do líder teste que abre janela ou mexe no foco, no teclado ou no mouse (L-50). Rode o teste sem janela no host, com os limites da L-11 item 2, e o QML só offscreen (`QT_QPA_PLATFORM=offscreen`), porque assim ele não toca a tela, o foco nem o teclado da sessão viva (L-50). Mande cada fatia commitada por push à branch da onda, onde o CI seletivo é o portão (L-35, L-36). O contêiner local antes do push é opcional em toda fatia, inclusive nas de segurança, privacidade e deploy irreversível (decisão de 27/11/2026). O espelho local e a exceção à L-50 para o `org.flatpak.Builder` estão na NF-20.

**Base do seletivo** (decisão do líder, 23/11/2026, por AskUserQuestion: *"Último ponto verde (Recomendado)"*). Compare a mudança com o último SHA da mesma branch cujo run do CI terminou em `success`; sem esse SHA, com a main. Run cancelado ou vermelho não vira base. Esta leitura troca, no projeto, a base "a última versão publicada, ou o ramo principal" da L-35, porque, contra a main, a primeira fatia que muda o build manda rodar tudo até o fim da onda, e o seletivo não poupa nada.

**Histórico.** Pela decisão de 27/11/2026, vale o texto de 23/11/2026, acima: o contêiner local é opcional em toda fatia, inclusive nas de segurança, privacidade e deploy irreversível. Os textos abaixo ficam só como histórico, substituídos.

- Texto de 25/10/2026 (ponto L7; título antigo: Execução: contêiner local primeiro, CI depois): cada fatia roda primeiro em contêiner local, nunca na sessão do líder (L-50), e só então vai por push à branch da onda, onde o CI repete (L-36). QML roda só offscreen, dentro do contêiner.
- Texto de 21/11/2026 (título antigo: Execução pelo piso de testes), que substituiu o de 25/10/2026 e foi substituído pelo de 23/11/2026 (decisão de 27/11/2026): o CI é o portão de cada fatia (L-35 e L-36 globais, piso de testes de 20/11/2026). O contêiner local fica para segurança, privacidade e deploy irreversível, e para depurar uma falha do CI. Fora desses casos, não há pré-CI obrigatório antes do push.

## NF-05

**Data:** decisões do líder de 25/10/2026 (pontos L7 e L9), 28/10/2026, 29/10/2026, 30/10/2026, 04/11/2026, 05/11/2026, 09/11/2026, 14/11/2026 e 19/11/2026. **Verbatim:** no ponto L9, "Autorizo a lista toda"; por AskUserQuestion, "Autorizo nas 3 imagens (Recomendado)" (28/10/2026, NL-2.2), "Autorizo agora (Recomendado)" (29/10/2026, libsodium), "Autorizo o download do ITI (Recomendado)" (29/10/2026), "Autorizo a zlib (Recomendado)" (29/10/2026) e "ITI + Serpro (Recomendado)" (30/10/2026, NL-2.13). Os itens de 04/11/2026 em diante trazem a data e o verbatim no próprio item.

Use o que está nas duas listas autorizadas e nos itens autorizados depois delas (L-51):

- **Lista fechada de imagens e pacotes (ponto L7).** Imagens `ubuntu:26.04`, `manjarolinux/base:latest`, `linuxmintd/mint22.3-amd64:latest`, `mysql:8.4` e `axllent/mailpit:latest` (`mariadb:11.8` já estava no Docker local; imagem Fedora, ver decisão de 05/11/2026, no item "Imagem Fedora da matriz", abaixo). Qt 6.12 por aqt, base `linux_gcc_64` mais `qtnetworkauth`, `qthttpserver`, `qtimageformats`, `qtshadertools` e `qtwebsockets`. Flatpak `org.kde.Sdk//6.11` e `org.kde.Platform//6.11`. As ferramentas de build dentro dessas imagens.
- **Lista complementar (ponto L9).** XSD e anexos gov.br, fontes Bricolage Grotesque e Atkinson Hyperlegible (OFL), gitleaks, `flatpak-builder`, cups-pdf e AT-SPI e Orca. Baixe cada item só quando a fatia que o usa começar, com o tamanho medido no relatório da fatia.
- **Cabeçalhos de desenvolvimento do OpenSSL (NL-2.2).** `openssl-devel` no fedora44 e `libssl-dev` no ubuntu26.04 e no mint22.3, só dentro das imagens de contêiner e do CI, pelos scripts de `tools/conteiner/instalar/`. O manjaro já traz os cabeçalhos.
- **libsodium.** Pacote de desenvolvimento da libsodium nas imagens de contêiner e no CI, para o tipo SenhaSegura e o Argon2id da NL-3.2. Entra quando a fatia que a usa começar, com o tamanho medido no relatório da fatia.
- **Raízes da ICP-Brasil.** Certificados raiz da ICP-Brasil baixados do ITI, para o app validar a cadeia do A1 antes do envio. Baixados só quando a fatia que os usa começar, com o tamanho e o hash medidos no relatório da fatia.
- **Intermediários da Let's Encrypt e conferência no Serpro (NL-2.13).** O servidor do ITI não manda a cadeia TLS completa. Ficam autorizados os 2 certificados intermediários da Let's Encrypt (YE1 e Root YE assinado pela ISRG Root X2), só para completar a conexão segura com o ITI. Cada raiz baixada do ITI só entra se o SHA-256 for igual ao da cópia do Serpro, quando o Serpro tiver aquela versão. Os arquivos conferidos entram versionados no repositório, com `SHA256SUMS`.
- **zlib.** zlib como dependência declarada do app, para o GZip do envelope da DPS (NL-2.4), nas imagens de contêiner, no CI e no Flatpak. Medido em 29/10: já presente nas 4 imagens como dependência da libxml2 (no fedora44, a variante zlib-ng-compat).
- **Bibliotecas (decisão do líder, 04/11/2026, via AskUserQuestion: "Biblioteca direto (Recomendado)").** Autoriza cliente MariaDB e libsodium nos testes, no CI e no pacote; desvio de CONTRACT:800 registrado na NF-34 (L-73); sem parsec. Cada item é baixado só quando a fatia que o usa começar, com o tamanho medido no relatório da fatia.
- **Servidores (decisões do líder, via AskUserQuestion).** Em 04/11/2026, "Testar todas (Recomendado)": autoriza as imagens `mysql:8.0`, `mariadb:10.11` e `mariadb:13.0`, só para os testes. Em 14/11/2026, "Autorizo as duas (Recomendado)": autoriza baixar as imagens do MariaDB 12.3 e do MySQL 9.7 do Fedora 45, só dentro de contêiner e só para os testes. Verbatim do líder são só as respostas entre aspas; o resto é redação do Chief of Staff (L-07) a partir da pergunta que ele respondeu, que nomeava o MariaDB 12.3 e o MySQL 9.7 do Fedora 45 e pedia a autorização só dentro de contêiner e só para os testes. Fonte: registro de decisões da Onda 3, 14/11/2026, fora do repositório e sujeito à limpeza de 30 dias; por isso fica aqui.
- **Imagem Fedora da matriz (decisão do líder, 05/11/2026, via AskUserQuestion: "Trocar por fedora:45 (Recomendado)").** Autoriza baixar a imagem `fedora:45` e os pacotes de build dela, a mesma lista de antes, dentro do contêiner e no CI. Se travar, verbatim: "tente no fedora 45, é a máquina que eu uso. Busque a raiz do problema, conserte e siga no fedora 45 no container". O aqt não entra nessa imagem: o Qt vem do volume e do cache do CI (CTO, a ratificar).
- **Connector/C 3.4.11 também no Manjaro (decisão do líder, 19/11/2026, via AskUserQuestion: "3.4.11 também no Manjaro (Recomendado)").** A imagem do manjaro monta o módulo 3.4.11 como as outras 3; o prefixo do manjaro passa a /usr/local (`tools/conteiner/prefixo_mariadb.txt`).
- **Trechos PHP do Connector/C (decisão do líder, 19/11/2026, via AskUserQuestion: "Usar a v4 e dispensar o advogado (Recomendado)").** Os trechos derivados do mysqlnd do PHP no Connector/C 3.4.11 são usados pela PHP License versão 4 (BSD-3-Clause), com o aviso de copyright da PHP Group em `share/licenses` e na tela Sobre (NL-I.350). O advogado humano antes da v1.0.0.0, pedido no parecer K2 do CLO, está dispensado.

**Instalação de item fora das duas listas (decisão do líder, 09/11/2026).** Ordem, verbatim: "instale o que precisar. Autorizo push/merge/tag". Na pergunta seguinte, por AskUserQuestion, o líder escolheu, verbatim: "Ampla, só em contêiner (Recomendado)". Dentro das imagens de contêiner e no CI, o item entra sem pergunta, registrado e avisado. Na máquina do líder e no Flatpak entregue, o item pede autorização nova do líder antes da fatia que o usa; sem ela, a fatia para, e não se roda só no CI para contornar (L-36, L-51). Esta linha substitui a leitura anterior do main, registrada às 22:51 de 09/11/2026, que admitia instalar no host com sudo. Regra de execução (leitura do main, não decisão do líder): cada instalação fica registrada com o quê, onde, tamanho e motivo, e é avisada; contêiner sempre que der; merge em main e tag só no fechamento da onda pela definição dela (L-24).

## NF-06

**Data:** decisão do líder, 29/10/2026. **Verbatim (AskUserQuestion):** "Fora da v1 (Recomendado)".

Não consulte CRL nem OCSP na v1. Um certificado revogado é detectado pela recusa da Sefin, que o app mostra com mensagem clara (NL-2.8).

## NF-07

**Data:** decisão do líder, 25/10/2026, ponto L7 (pastas padrão) e achado A0-08 (`NL_TMP_BASE`); decisão do líder, 30/10/2026 (teto de agentes), confirmada por AskUserQuestion no mesmo dia, que substitui o teto de 2 de 27/10/2026. **Verbatim:** "pode deixar tudo nas pastas padrao"; e "pode continuar com teto de 4 agentes em paralelo, sem concorrencia". O resto é regra de execução (lei global L-11).

- Use o Docker em `/var/lib/docker`.
- Ponha build e temporários em `/var/tmp`, com `TMPDIR` exportado, porque `/tmp` é tmpfs. A base é configurável pela variável `NL_TMP_BASE` (padrão `/var/tmp`).
- Rode cada trabalho pesado (build, contêiner, suíte completa) um por vez, na trava única e com os limites da L-11 item 2: serializado com `flock` e com `nice -n 5`.
- Use no máximo 4 agentes em paralelo neste projeto, sem concorrência entre eles: nunca dois disputando o contêiner, o lock pesado ou os mesmos arquivos. Esse teto fica abaixo do teto de 10 da L-11 global.
- Confira o disco por `btrfs filesystem usage /`, lendo `Device unallocated` e `Free (estimated) min`, nunca por `df`.
- Deixe o resultado de build fora da árvore.
- Não escreva laço que crie um processo por item varrido (L-11).
- **O que dura e o que não dura (regra de execução, L-11; `q /var/tmp 1777 root root 30d`).** `/var/tmp` guarda só build e temporário regenerável, porque o sistema apaga em 30 dias o que não é usado; log citado em ordem ou dossiê, registro de decisões e contador de DPS duram em `~/.local/share/notalivre/`, copiados pelo main ao fechar cada fatia.

## NF-08

**Data:** decisão do líder, 25/10/2026. **Verbatim:** "Branch de onda, push por fatia". Commit por fatia: regra de execução (leis globais L-24 e L-63).

Dê a cada onda uma branch própria (`onda-0`, `onda-1` e assim por diante). Mande cada fatia commitada por push à branch da onda, só para o CI rodar (L-24, tempo 2). Confira o push por `git ls-remote`, nunca pela mensagem do push. O `main` recebe a onda só depois de fechada pela definição de fechamento dela, com autorização explícita de merge do líder (L-24, tempo 4). Faça um commit por fatia em Conventional Commits, citando o ID `NL-<onda>.<fatia>` e tocando o `Status` do `TODO.md` no mesmo commit (o valor do `Status`: NF-21). Merge na branch da onda: NF-20.

## NF-09

**Data:** regra de execução (leis globais L-49 e L-12); ajustada à NF-04 por AskUserQuestion, 23/11/2026: *"Alinhar ao global (Recomendado)"*.

Quando houver rodada em contêiner local, faça o executor gravar o log dela em `/var/tmp` e citá-lo na entrega. O main lê esse log, se houver, e o do job de CI do SHA empurrado, este pelo MCP do GitHub, e faz spot-check arquivo:linha no blob commitado.

## NF-10

**Data:** decisão do líder, confirmada em 25/10/2026; achado A0-13.

Use como titular do copyright Petrus Alves da Silva Costa, como nos cabeçalhos SPDX, e como e-mail de autoria dos commits petrus@drpetrus.top.

## NF-11

**Data:** decisão do líder, 25/10/2026.

Versione no formato vA.B.C.D, estilo GlintFx. Crie tag de marco a cada onda fechada, só com aval explícito do líder.

## NF-12

**Data:** decisão do líder, 25/10/2026; alinhada à lei global por AskUserQuestion, 23/11/2026: *"Alinhar ao global (Recomendado)"*.

Rode o job `mutacao` só quando a mudança tocar segurança, privacidade ou deploy irreversível (L-35, L-27), num CI seletivo pelo que a mudança toca (L-35), porque fora desses casos a mutação custa mais do que confere.

**Histórico.** Texto de 25/10/2026 (título antigo: Mutação a cada push): o job `mutacao` roda em todo push, sem filtro de caminho. Substituído pelo alinhamento de 23/11/2026.

## NF-13

**Data:** decisão do líder, 25/10/2026.

Mantenha o brief fundador em `docs/origem/`.

## NF-14

**Data:** decisão do líder, 25/10/2026; achado A0-07.

Faça o workflow disparado por PR de colaborador externo rodar só com aprovação manual, configurado no repositório.

## NF-15

**Data:** decisão do líder, 01/11/2026. **Verbatim (19:53, trecho):** "E não é pra rodar a cli em segundo plano. Deve rodar normalmente." **Esclarecimento do líder (19:55):** "Falo da GUI. Ela não deve ser apenas uma chamadora da CLI em segundo plano".

Faça a GUI usar o núcleo, nunca a CLI. Regra derivada (CTO, DA-2.83 e DA-2.85, a ratificar): a GUI chama a biblioteca núcleo pelas fachadas de `src/mid/composicao`, as mesmas da CLI (spec, linhas 149 e 362). A GUI não executa a CLI nem chama programa por processo ou por shell; abrir URL pelo portal do sistema é permitido. Verificação: portão de camadas (NL-2.38).

## NF-16

**Data:** decisão do líder, 01/11/2026. **Verbatim (trecho):** "A GUI basta ser wayland. Não precisa compatibilidade x11." Depois do argumento do main de que o Mint abre o Cinnamon em X11 por padrão, resposta por AskUserQuestion: "Manter X11 de reserva (Recomendado)".

Mantenha a GUI em Wayland com X11 de reserva: o manifesto fica com `--socket=wayland` e `--socket=fallback-x11`.

## NF-17

**Data:** decisão do líder, 25/10/2026, pelo rito da LEI DAS LEIS. **Verbatim:** "Cancelo: L-70 inteira". Alcance (b) por AskUserQuestion, 23/11/2026: *"As sete da L-70 (Recomendado)"*. Fases 0, 1, 3, 4, 5 e 6: decisões do líder, 04/11/2026; acréscimos às Fases 3 e 5: decisões do líder, 25/11/2026. Cada fase traz o verbatim dela abaixo.

Aplique a L-70 inteira, com uma única troca, na Fase 2:

- As 7 fases da L-70 valem para duas situações: (a) a liberação de emissão real a terceiros, que é a virada fiscal de Produção (NL-12); (b) toda versão do app que toque uma das sete classes da L-70: login, permissão, token, segredo, apagar dado, schema de banco e rotação de chave. A situação (b) conta a partir do marco da Fase 0, abaixo.
- Na Fase 2, faça "dupla validação manual em staging (ambiente de Produção Restrita)" no lugar do Shadow Deployment. Justificativa de inviabilidade técnica: o NotaLivre é um app desktop instalado na máquina de cada usuário, sem balanceador e sem tráfego que se possa espelhar.
- As outras 6 fases são obrigatórias, sem exceção.
- **Fase 0, a partir de quando conta (decisão do líder, 04/11/2026, pelo rito da LEI DAS LEIS).** Resposta verbatim: "Primeiro dado real (Recomendado)". A L-70 passa a contar da primeira instalação com dado real, registrada em `docs/auditoria/deploys/`; antes dela, a estrutura se ajusta direto. Depois, cada versão é classificada antes de publicar; a autorização do modo sem tela conta como mudança sensível.
- **Fase 1 (decisão do líder, 04/11/2026).** Resposta verbatim: "Confirmar, script para (Recomendado)".
- **Fase 3 (decisão do líder, 04/11/2026).** Resposta verbatim: "Copiar, conferir, trocar (Recomendado)". Acréscimo (decisão do líder, 25/11/2026, via AskUserQuestion: "04/11 + ensaio no CI (Recomendado)"): toda versão que mude o schema tem a migração ensaiada no CI a partir de cada schema já publicado, com dados de fixture, no MySQL 8.4 e no MariaDB 11.8.
- **Fase 4 (decisão do líder, 04/11/2026).** Resposta livre, verbatim: "com segundo passo opcional, opt-in, nas configuracoes". Pergunta seguinte: "Guardar no Bitwarden (Recomendado)". Texto: `onda3-seguranca-final.md`, do CISO.
- **Fase 5 (decisão do líder, 04/11/2026).** Resposta verbatim: "Cópia e série nova (Recomendado)". Emenda a DA-2.93, ratificada. Acréscimo (decisão do líder, 25/11/2026, via AskUserQuestion: "Somar as duas (Recomendado)", sobre a resposta "Beta 48 h + cópia 30 dias (Recomendado)" do mesmo dia): a versão fica 48 h no canal beta antes do estável, o líder assina o checklist pelo nome, e a cópia de antes da migração fica 30 dias na máquina do usuário, sem que o app a apague antes disso.
- **Fase 6 (decisão do líder, 04/11/2026).** Resposta verbatim: "Sem envio automático (Recomendado)".

Corpo das 7 fases: L-70 do `GODS_LAWS.md` global (anexo `docs/deploy-irreversivel.md`).

**Histórico.** Texto de (b) antes do alcance de 23/11/2026: toda versão do app que mude schema do banco, autenticação ou chave criptográfica (categorias da própria L-70).

## NF-18

**Data:** decisão do líder, 25/10/2026 (exceção do portão); achado A0-16. O resto é regra de execução (leis globais L-31, L-32 e L-73).

Escreva o texto de interface e de documento em pt-br, sem travessão nem meia-risca (L-32) e sem emoji (L-31). Mantenha o `README.md` como o único hub, em hub-and-spoke (L-73). As pastas internas `docs/origem/`, `docs/auditoria/` e `docs/adr/` ficam fora do portão de travessão; o texto voltado ao usuário fora dessas pastas continua sob a L-32.

## NF-19

**Data:** decisão do líder, 21/11/2026; verificação recortada em 23/11/2026. **Verbatim:** "esse é um app financeiro que envolve calculos com dinheiro, não use float!" e "falei isso apenas para dinheiro, pode usar no restante do app"; por AskUserQuestion, "Aprovo (Recomendado)" (regra derivada do CTO, aprovada pelo líder em 21/11/2026), "Contam (Recomendado)" (alíquota e percentual contam como dinheiro) e, em 23/11/2026, "Só o bloqueio no tipo (Recomendado)".

Guarde valor monetário só em `nl::back::Valor`, em centavos `int64`, e faça-o entrar ou sair só por inteiro ou por texto. Não use `float`, `double`, `qreal` nem `long double`, nem conversão para eles, no motor `src/back/domain/dinheiro` nem em arquivo de `src/` e `apps/` que chama a API do `Valor` (`de`, `deTextoPtBr`, `somaDeLote`, `centavos`, `paraLeiaute`). Calcule com dinheiro, alíquota e percentual no motor `dinheiro`, em aritmética inteira e com o arredondamento declarado. Na interface, mostre dinheiro como texto vindo do núcleo, nunca como número do QML. Fora do dinheiro, ponto flutuante é permitido. Verificação: a sobrecarga apagada `Valor::de(std::floating_point auto) = delete` e o par de não compilação `dinheiro.naoCompila.valorDeDouble`, sem portão de script (L-35, item 3).

## NF-20

**Data:** exceção à L-50: decisão do líder, 04/11/2026, pelo rito da LEI DAS LEIS. **Verbatim:** "Autorizar só o Builder (Recomendado)". Espelho local opcional: alinhado à lei global por AskUserQuestion, 23/11/2026: "Alinhar ao global (Recomendado)", que substitui a decisão de 01/11/2026 (N-01, "Adotar na Onda 3 (Recomendado)"). **Verbatim da L-50:** "nenhum teste dinamico toca minha sessao, sempre fazer em docker".

- Rode o espelho local (`tools/espelho/rodar_espelho.sh`) só quando o executor quiser, na trava única `~/.claude/bin/pesado` (L-11, item 2). O CI seletivo é o portão de cada fatia (NF-04).
- Exceção à L-50, sobre `flatpak run`: dos `flatpak run` no host, só o `org.flatpak.Builder`, pelo espelho (`tools/espelho`), sem janela, certificado ou privilégio. O build Flatpak do espelho roda `org.flatpak.Builder` sob esta exceção. Qualquer outro `flatpak run` no host segue proibido. O alcance da L-50 no projeto (teste sem janela no host, QML só offscreen) está na NF-04.
- A L-50 protege a sessão de trabalho do líder. O que se perde, como foi dito a ele na pergunta: esse build e esse teste invisível rodam sempre sob a sessão dele, sem contêiner.
- Faça merge na branch da onda só com APROVA no registro, com hora, citando o SHA igual ao `<merge>^2` (N-11).

## NF-21

**Data:** regra de execução (leis globais L-24 e L-63); quando vira "Concluído", decisão do líder, 13/11/2026. **Verbatim:** via AskUserQuestion, "Vale (Recomendado)"; no rito da LEI DAS LEIS sobre a L-63, "Confirmar". Verbatim do líder são só as duas respostas entre aspas; o resto é leitura registrada pelo main (L-07).

Registre implementação entregue como "Pendente verificação", nunca como "Concluído" direto. A rodada de teste e auditoria correspondente, que a L-63 exige antes do "Concluído", é a auditoria de fechamento da onda que contém o item, como fizeram as Ondas 1 e 2. "Concluído" deixa de esperar as ondas finais de teste e auditoria do plano. O cabeçalho do `TODO.md` diz o mesmo.

## NF-22

**Data:** decisão do líder, 04/11/2026. **Verbatim (AskUserQuestion):** "Adiantar o contrato (Recomendado)".

Rode a NL-3.8 (contrato) depois da NL-3.1d, em série.

## NF-23

**Data:** ordem do líder, 04/11/2026. **Verbatim:** "quando for criar a UI eu quero ver mocks de opcoes de  design no firefox para escolher. use a skill de frontend".

Não implemente tela antes da escolha. A spec visual fechada (`docs/design/spec-visual.md`) é o ponto de partida; opção que a contrarie é dita ao líder.

## NF-24

**Data:** ordem do líder, 05/11/2026. **Verbatim:** "não autorizo downgrade".

Corrija para a frente: nenhum plano volta kernel, Python, pacote ou imagem de versão.

## NF-25

**Data:** ordem do líder, 05/11/2026; refinada em 09/11/2026, com correção do líder no mesmo minuto. **Verbatim (05/11/2026):** "duvidas com clevel e se nao der conta, com advisor. Sempre busque na web por questoes da comunidade, documentacoes, outros projetos de nota fiscal no github, gitlab e demais repos git. USe como aprendizado, não cópia." **Verbatim (09/11/2026):** "duvidas tire com o cto, buscando na web sempre o mais completo a ser feito, não o mais rápido. Se não resolver, fale com o advisor". **Correção (mesmo minuto), verbatim:** "cto ou outro c-level mais adequado".

Aplique por L-22 e L-37: leia para aprender a técnica, reescreva do zero e cite a fonte. Ordem com decisão técnica ou fiscal traz o passo de pesquisa antes do plano.

## NF-26

**Data:** decisão do líder, 06/11/2026. **Verbatim (AskUserQuestion):** "Ratificar tudo (Recomendado)".

Portões de sanitizer: guarda cruzada e risco aceito. A resposta ratificou de uma vez seis decisões do CTO da NL-3.0h: o código de saída 86 do sanitizer, o regex sem `ERROR:`, os pinos do preset, o 14º portão, a guarda cruzada entre `tools/portoes/sanitizer_ativo.py` e `tools/portoes/sanitizer_ligado.py` (cada um confere a ligação do outro) e o risco aceito: retirar os dois portões no mesmo commit, ajustando piso e autotestes, só a revisão do diff pega. Fonte: registro de decisões da Onda 3, 06/11/2026, 17:29 e 18:44, fora do repositório, em `/var/tmp`, que apaga em 30 dias; por isso fica aqui. Verificação: `TESTES.md`, T4.

## NF-27

**Data:** decisões do líder, 13/11/2026, Bloco B do cofre, via AskUserQuestion. Fonte: registro de decisões da Onda 3, 13/11/2026, fora do repositório e sujeito à limpeza de 30 dias; por isso o verbatim fica aqui.

**Verbatim (cofre: cadastro, admin e quiz).** Cadastrar, resposta livre, verbatim: "Opcional. Ponha caixa parte digitar e abaixo,  2 radiobox: com certificado e sem certificado. Default: sem". Aviso, resposta livre, verbatim: "Mostra aos usarios admin (apenas o cpf/cnpj) para entrar em contato.". Admin: "Admin marcado (Recomendado)". Ocupação, resposta livre, verbatim: "Sem certificado, o app deve verificar todas as respostas certas em 3 quizes de 4 opções com apenas uma certa sobre a empresa: nome fantasia, razão social, cnae com descrição, nome de um dos sócios, regime tributário adotado, cidade sede, [sugira]. Deve ter apenas uma opção. Fecha o app ao primeiro erro e deixa msg ao admin. Admin pode liberar criação de conta ou finalização de conta sem quiz ou certificado". Reforço: "Quiz com travas (Recomendado)". Finalizar: "É a remoção (Recomendado)". Sem admin: "1º cadastro entra (Recomendado)". Ordem: "Cofre agora, quiz na W6 (Recomendado)". Admin órfão (13/11/2026, depois da ordem do CISO): "Próximo com certificado (Recomendado)". K4 (mesma rodada): "Parte antes (Recomendado)". Verbatim do líder são só as respostas entre aspas, com a grafia dele.

Leitura registrada (L-07), com o desenho na ADR-0003, regras 10, 13, 19 e 20: certificado opcional, sem certificado de fábrica; o primeiro cofre de um computador sem cofre nasce admin, com ou sem certificado, e sem quiz; o admin vê só o número de quem está em remoção, depois de entrar com a própria senha, libera cadastro e pede a remoção de qualquer cofre; num computador com cofre e sem admin, o próximo cofre criado com certificado nasce admin; as linhas da pauta do K4 sobre o que nasce na NL-3.3 viram o K4a, antes dela (plano, seção 3); o quiz vale só para CNPJ e entra na NL-6.7, na Onda 6, com os erros contados nas reaberturas e, depois do terceiro, só certificado ou admin.

## NF-28

**Data:** decisões do líder, 13/11/2026, Bloco B do cofre, via AskUserQuestion. Mesma fonte da NF-27.

**Verbatim (cofre: remoção e prazo).** Remover: "Com prova, 30 dias (Recomendado)". Prazo: "Aviso sem número (Recomendado)". Arquivo (13/11/2026, depois da revisão de segurança): "Aviso vence, cofre espera (Recomendado)".

Leitura registrada (L-07; ADR-0003, regra 13): 30 dias contados do pedido; o aviso ao admin vence sempre em 30 dias; sem o arquivo S, o cofre fica guardado até o arquivo voltar ou a prova ser repetida; aviso selado aos admins e, sem admin que o receba, aviso sem número a quem abrir o app; num cofre que nunca recebeu certificado, só certificado cancela a remoção pedida com certificado (resposta Reforço, na NF-27).

## NF-29

**Data:** decisões do líder, 13/11/2026, Bloco B do cofre, via AskUserQuestion. Mesma fonte da NF-27.

**Verbatim (cofre: segredos e login).** Certificado: "Sem caminho, com aviso (Recomendado)". Código: "Também em arquivo", contra a recomendação do CISO, com o risco dito na pergunta. Localizar: "Cálculo lento (Recomendado)". Chaveiro: "Opcional, com aviso (Recomendado)". Senha: "Confirmar os três (Recomendado)".

Leitura registrada (L-07): o A1 sem `keyEncipherment` emite e fica sem a cópia da DEK, com aviso na importação (ADR-0002, decisão 9); o código também vai a arquivo, como desvio de risco com os controles C16 a C20 (ADR-0003, regra 21); o localizador é por Argon2id (ADR-0003, regra 12); o chaveiro é opcional e desligado de fábrica (ADR-0003, regra 5); o login usa o custo da ADR-0006, o medidor próprio do app e símbolo como o que não é letra nem dígito (spec 4.4).

## NF-30

**Data:** decisões do líder, 13/11/2026, N7, via AskUserQuestion. Mesma fonte da NF-27.

**Verbatim (banco: senha e servidor).** Banco: "Arquivo só do dono (Recomendado)". Porta: "Conferir o servidor (Recomendado)". Senha admin (13/11/2026, depois da ordem do CISO): "Só o script (Recomendado)".

Leitura registrada (L-07; plano, NL-3.6): a senha do banco, longa e sorteada pelo app, fica num arquivo do app só do dono, exceção escrita à regra de senhas só no cofre (spec 4.16); o app confere o servidor antes de mandar a senha. Pela resposta "Só o script (Recomendado)": o assistente nunca pede a senha de administrador do banco, e a spec visual, que dizia o contrário em nome do líder, foi corrigida por essa resposta.

## NF-31

**Data:** decisão do líder, 13/11/2026. **Verbatim (AskUserQuestion):** "Ratificar as duas (Recomendado)". Mesma fonte da NF-27.

Ratificadas a ADR-0007 (SHA-1 no arnês de teste, aceita pelo CISO em 30/10/2026) e a DA-2.98 (cTribMun, `docs/pesquisa/parametros_municipais.md`). A emenda da ADR-0007 para o Fedora 45 continua a ratificar pelo CISO.

## NF-32

**Data:** ordem do líder, 14/11/2026. **Verbatim:** "lei do projeto: atomizar por funcao. O que já está pronto, crie onda no final para atomizar". Nas perguntas seguintes, por AskUserQuestion, o líder respondeu, verbatim: sobre o corte, "Uma função, um assunto (Recomendado)"; sobre os três monólitos anteriores (`cliente_sefin.cpp`, `emissao_cli.cpp` e `comando_emitir.cpp`), "Vai para a onda final (Recomendado)"; sobre o lugar da onda, "Antes dos testes finais (Recomendado)". Verbatim do líder são só os trechos entre aspas. Fonte: registro de decisões da Onda 3, 14/11/2026, fora do repositório e sujeito à limpeza de 30 dias; por isso o verbatim fica aqui.

Átomos sem monolito: L-34 e a lei do projeto. Leitura registrada pelo main junto da resposta sobre o corte (L-07): cada função faz uma coisa só; cada arquivo reúne só funções do mesmo assunto, com nome próprio; checado em toda revisão. Redação do Chief of Staff (L-07): código escrito a partir de 14/11/2026 já nasce assim, e o código pronto antes dessa data é cortado na Onda 9A do plano, entre a Onda 9 e a Onda 10, onde ficam os testes e as auditorias finais.

## NF-33

**Data:** regra de execução (leis globais L-10, L-12, L-16, L-33, L-35, L-36 e L-53), citada por número; não é decisão do líder.

- Faça o agente especialista executar, nunca o orquestrador (L-10).
- Mantenha implementador, revisor e orquestrador como agentes distintos (L-12).
- Siga, em cada fatia e em cada onda, os passos da L-16.
- Conte um portão só depois de visto vermelho (L-36).
- Use TDD estrito e sem meta numérica de cobertura (L-35).
- Mantenha a dependência unidirecional (L-33; tabela na NF-02).
- Destrua arquivo só com caminho conferido (L-53).

## NF-34

**Data:** desvios (a) e (b): regra de execução (leis globais L-73, L-33 e L-35), registrada no plano, seção 0, NL-P.3. Desvio (c): decisão do líder, 04/11/2026, via AskUserQuestion: "Biblioteca direto (Recomendado)"; emenda do CTO (NL-P.4) ratificada pelo líder, 27/11/2026, via AskUserQuestion: "Ratifico a emenda (Recomendado)".

Desvios de manual (L-73). Onde um manual do plugin bigtech diverge de uma lei, siga o que vale no projeto, abaixo, e não o manual.

- **(a) CONTRACT 5.1 contra a L-33.** O manual diz "Backend ← Infrastructure" e Middleware sem HTTP nem SQL. Vale a L-33, pela matriz da NF-02 (plano, seção 1.1, e seção 0, NL-P.3): cada camada só depende das de baixo, com a Foundation embaixo e o Mid como orquestração e protocolo. Por quê: a lei do usuário prevalece sobre o manual.
- **(b) TESTES T1 contra a L-35.** O manual dizia "Cobertura mínima de 70%" (versão do manual da época do registro; na versão lida em 27/11/2026, o T1 do manual, plugin `TESTES.md:67-73`, diz só "0 falhas", sem percentual). Vale a L-35, que proíbe meta numérica: o critério de verde de T1 é zero falha, sem percentual de cobertura (plano, seção 5). Por quê: a lei do usuário prevalece sobre o manual.
- **(c) CONTRACT:800 contra a biblioteca direta.** O manual diz, verbatim: "MUST use `QSqlQuery` with `bindValue()`  -  NEVER string concatenation for SQL." (linha 800 na versão do manual da época do registro; na versão lida em 27/11/2026, a mesma frase está na linha 814). Vale a emenda do CTO, ratificada: sem `QSqlQuery`, o cliente MariaDB direto, pela decisão "Biblioteca direto (Recomendado)" de 04/11/2026; a concatenação de SQL continua proibida, e todo valor entra por parâmetro vinculado da biblioteca direta. Por quê: a decisão do líder de 04/11/2026 tirou o `QSqlQuery` do projeto, e a ratificação de 27/11/2026 confirmou a emenda.
