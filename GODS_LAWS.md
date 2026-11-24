# GODS_LAWS.md: NotaLivre (nfse)

> Ordens do líder para este projeto. Cada lei tem um gatilho, conferido no momento da ação.

## Protocolo de uso

1. Antes de agir, confira o índice de gatilhos do `CLAUDE.md` deste projeto; se um gatilho casa, leia a lei inteira antes do primeiro comando.
2. Siga o protocolo de uso do `~/.claude/GODS_LAWS.md`. No que este arquivo silencia, vale a lei global; `L-NN` sem prefixo é lei global e `NF-NN` é lei daqui. `NL-<onda>.<fatia>` é ID de fatia do `TODO.md`, não lei.
3. As leis com "decisão do líder" na linha **Data** são decisões do líder. As marcadas "regra de execução (lei global)" decorrem das leis globais, citadas por número, e não são decisões do líder. A tabela completa dos pontos L1 a L10 está na seção 6.2 do plano.

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

**Data:** decisão do líder, 25/10/2026, ponto L7. **Verbatim (na confirmação da exceção):** "autorizo containers locais" e "pode deixar tudo nas pastas padrao". Alinhada à lei global por AskUserQuestion, 23/11/2026: *"Alinhar ao global (Recomendado)"*.

Nunca rode na sessão viva do líder teste que abre janela ou mexe no foco, no teclado ou no mouse (L-50). Rode o teste sem janela no host, com os limites da L-11 item 2, e o QML só offscreen (`QT_QPA_PLATFORM=offscreen`), porque assim ele não toca a tela, o foco nem o teclado da sessão viva (L-50). Mande cada fatia commitada por push à branch da onda, onde o CI seletivo é o portão (L-35, L-36). O contêiner local antes do push é opcional.

## NF-05

**Data:** decisão do líder, 25/10/2026 (pontos L7 e L9), 28/10/2026, 29/10/2026 e 30/10/2026. **Verbatim:** no ponto L9, "Autorizo a lista toda"; por AskUserQuestion, "Autorizo nas 3 imagens (Recomendado)" (28/10/2026, NL-2.2), "Autorizo agora (Recomendado)" (29/10/2026, libsodium), "Autorizo o download do ITI (Recomendado)" (29/10/2026), "Autorizo a zlib (Recomendado)" (29/10/2026) e "ITI + Serpro (Recomendado)" (30/10/2026, NL-2.13).

Use só o que está nas duas listas autorizadas (L-51):

- **Lista fechada de imagens e pacotes (ponto L7).** Imagens `ubuntu:26.04`, `manjarolinux/base:latest`, `linuxmintd/mint22.3-amd64:latest`, `mysql:8.4` e `axllent/mailpit:latest` (`fedora:44` e `mariadb:11.8` já estavam no Docker local). Qt 6.12 por aqt, base `linux_gcc_64` mais `qtnetworkauth`, `qthttpserver`, `qtimageformats`, `qtshadertools` e `qtwebsockets`. Flatpak `org.kde.Sdk//6.11` e `org.kde.Platform//6.11`. As ferramentas de build dentro dessas imagens.
- **Lista complementar (ponto L9).** XSD e anexos gov.br, fontes Bricolage Grotesque e Atkinson Hyperlegible (OFL), gitleaks, `flatpak-builder`, cups-pdf e AT-SPI e Orca. Baixe cada item só quando a fatia que o usa começar, com o tamanho medido no relatório da fatia.
- **Cabeçalhos de desenvolvimento do OpenSSL (NL-2.2).** `openssl-devel` no fedora44 e `libssl-dev` no ubuntu26.04 e no mint22.3, só dentro das imagens de contêiner e do CI, pelos scripts de `tools/conteiner/instalar/`. O manjaro já traz os cabeçalhos.
- **libsodium.** Pacote de desenvolvimento da libsodium nas imagens de contêiner e no CI, para o tipo SenhaSegura e o Argon2id da NL-3.2. Entra quando a fatia que a usa começar, com o tamanho medido no relatório da fatia.
- **Raízes da ICP-Brasil.** Certificados raiz da ICP-Brasil baixados do ITI, para o app validar a cadeia do A1 antes do envio. Baixados só quando a fatia que os usa começar, com o tamanho e o hash medidos no relatório da fatia.
- **Intermediários da Let's Encrypt e conferência no Serpro (NL-2.13).** O servidor do ITI não manda a cadeia TLS completa. Ficam autorizados os 2 certificados intermediários da Let's Encrypt (YE1 e Root YE assinado pela ISRG Root X2), só para completar a conexão segura com o ITI. Cada raiz baixada do ITI só entra se o SHA-256 for igual ao da cópia do Serpro, quando o Serpro tiver aquela versão. Os arquivos conferidos entram versionados no repositório, com `SHA256SUMS`.
- **zlib.** zlib como dependência declarada do app, para o GZip do envelope da DPS (NL-2.4), nas imagens de contêiner, no CI e no Flatpak. Medido em 29/10: já presente nas 4 imagens como dependência da libxml2 (no fedora44, a variante zlib-ng-compat).

Para item fora das duas listas, peça autorização nova do líder antes da fatia que o usa. Sem ela, a fatia para. Não rode só no CI para contornar (L-36, L-51).

## NF-06

**Data:** decisão do líder, 29/10/2026. **Verbatim (AskUserQuestion):** "Fora da v1 (Recomendado)".

Não consulte CRL nem OCSP na v1. Um certificado revogado é detectado pela recusa da Sefin, que o app mostra com mensagem clara (NL-2.8).

## NF-07

**Data:** decisão do líder, 25/10/2026, ponto L7 (pastas padrão) e achado A0-08 (`NL_TMP_BASE`); decisão do líder, 30/10/2026 (teto de agentes), confirmada por AskUserQuestion no mesmo dia. **Verbatim:** "pode deixar tudo nas pastas padrao"; e "pode continuar com teto de 4 agentes em paralelo, sem concorrencia". O resto é regra de execução (lei global L-11).

- Use o Docker em `/var/lib/docker`.
- Ponha build e temporários em `/var/tmp`, com `TMPDIR` exportado, porque `/tmp` é tmpfs. A base é configurável pela variável `NL_TMP_BASE` (padrão `/var/tmp`).
- Rode cada trabalho pesado (build, contêiner, suíte completa) um por vez, na trava única e com os limites da L-11 item 2.
- Use no máximo 4 agentes em paralelo neste projeto, sem concorrência entre eles: nunca dois disputando o contêiner, o lock pesado ou os mesmos arquivos. Esse teto fica abaixo do teto de 10 da L-11 global.
- Deixe o resultado de build fora da árvore.

## NF-08

**Data:** decisão do líder, 25/10/2026. **Verbatim:** "Branch de onda, push por fatia".

Dê a cada onda uma branch própria (`onda-0`, `onda-1` e assim por diante). Mande cada fatia commitada por push à branch da onda, só para o CI rodar (L-24, tempo 2). O `main` recebe a onda só depois de fechada pela definição de fechamento dela, com autorização explícita de merge do líder (L-24, tempo 4). Cite em cada commit de fatia o ID `NL-<onda>.<fatia>`.

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

**Data:** decisão do líder, 25/10/2026, pelo rito da LEI DAS LEIS. **Verbatim:** "Cancelo: L-70 inteira". Alcance (b) por AskUserQuestion, 23/11/2026: *"As sete da L-70 (Recomendado)"*.

Aplique a L-70 inteira, com uma única troca, na Fase 2:

- As 7 fases da L-70 valem para duas situações: (a) a liberação de emissão real a terceiros, que é a virada fiscal de Produção (NL-12); (b) toda versão do app que toque uma das sete classes da L-70: login, permissão, token, segredo, apagar dado, schema de banco e rotação de chave.
- Na Fase 2, faça "dupla validação manual em staging (ambiente de Produção Restrita)" no lugar do Shadow Deployment. Justificativa de inviabilidade técnica: o NotaLivre é um app desktop instalado na máquina de cada usuário, sem balanceador e sem tráfego que se possa espelhar.
- As outras 6 fases são obrigatórias, sem exceção.
- As fases da L-70 que não couberem num app desktop voltam ao líder, uma a uma, antes da Onda 3 (pendente, não decidido).

Corpo das 7 fases: L-70 do `GODS_LAWS.md` global (anexo `docs/deploy-irreversivel.md`).

## NF-18

**Data:** decisão do líder, 25/10/2026 (exceção do portão); achado A0-16.

Escreva o texto de interface e de documento em pt-br. As pastas internas `docs/origem/`, `docs/auditoria/` e `docs/adr/` ficam fora do portão de travessão; o texto voltado ao usuário fora dessas pastas continua sob a L-32.
