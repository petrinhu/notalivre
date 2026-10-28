# NotaLivre: canon do projeto

Este arquivo é o canon do NotaLivre. Ele aponta para as leis globais do usuário por número, em `GODS_LAWS.md` (fora deste repositório), e não as copia. Última revisão: 2026-10-25 (fatia NL-0.1).

## Consumidor e produto

**Consumidor declarado (L-21).** Spec, seção 1, texto completo: "MEI, autônomos e pequenas empresas do Simples Nacional. Escritórios contábeis ficam fora da v1 (paráfrase da resposta do líder na rodada 2). O projeto é para distribuição a terceiros (decisão do líder, rodada 1: emissores = 'Distribuir a terceiros')." Esta frase é o consumidor do produto; nenhuma decisão de escopo usa outro consumidor como critério.

**Frase-guarda (L-21).** Entra verbatim no briefing de qualquer agente que vai produzir ou decidir algo do produto:

> Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição.

**Fontes de decisão de produto.** Spec: `docs/superpowers/specs/2026-10-25-notalivre-design.md` (seções 2 a 4, com as decisões do líder, e 4.14 para as propostas do orquestrador ainda não aprovadas). Plano: `docs/superpowers/plans/2026-10-25-notalivre-plano.md` (ondas e fatias). Stack: C++23 e Qt 6.12, interface QML (spec 4.2). Entrega somente em Flatpak (spec 3.3). Licença GPL-3.0-or-later (spec 4.1). App-id `io.github.petrinhu.notalivre` (decisão do líder, rodada 6, spec 4.1).

**decisão do líder, 25/10/2026 (ratificada):** o README entra; o `.env.example` não entra, porque a configuração mora no app.

## Matriz de camadas (L-33)

Camadas e dependência só para baixo, conforme a seção 1.1 do plano. Verificação: portão de camadas (NL-0.3), que varre os `#include` contra esta tabela.

| Camada | Diretório | O que mora ali | Pode depender de | Proibido |
|---|---|---|---|---|
| Front | `apps/cli`, `apps/gui` | CLI e QML com seus modelos de vista em C++ | fachadas de `src/mid/composicao` | SQL, cripto, HTTP, regra fiscal, include de `src/back` ou `src/foundation` exceto `foundation/base` |
| Mid | `src/mid/<adaptador>` | adaptadores que implementam os ports do Back sobre as primitivas da Foundation; raiz de composição; fachadas | `src/back`, `src/foundation` | QML, widget |
| Back | `src/back/domain`, `src/back/app`, `src/back/ports` | entidades, regras fiscais, casos de uso, interfaces (ports) | só `src/foundation/base` | rede, SQL, cripto, XML, arquivo, Qt GUI |
| Foundation | `src/foundation/<motor>` | motores agnósticos de domínio que embrulham terceiros; schema SQL; config | nada interno acima; `base` não depende de nenhum outro | qualquer tipo do domínio |

Cada adaptador do Mid tem diretório e razão de mudar próprios (L-34): `sefin`, `adn`, `persistencia`, `cofre`, `assistente_banco`, `correio`, `oauth`, `danfse`, `tabelas`, `cnpj`, `lote`, `composicao`.

## Convenção de nomes

**decisão do líder, 25/10/2026 (ratificada).** Identificadores em português sem acento, alinhados ao leiaute (Dps, Tomador, Emissor). Classes em PascalCase; arquivos em snake_case. Erro por `std::expected<T, Erro>` (C++23).

**INFERÊNCIA do CTO, a confirmar em NL-0.4:** o GCC 13 do Mint 22.3 tem `<expected>`.

## Decisões do líder (25/10/2026)

Os itens marcados "decisão do líder" são decisões do líder de 25/10/2026. Os itens marcados "regra de execução (lei global)" são regras que decorrem das leis globais, citadas por número; não são decisões do líder. A tabela completa dos pontos L1 a L10 está na seção 6.2 do plano.

- **Execução: contêiner local primeiro, CI depois (decisão do líder, 25/10/2026, ponto L7).** Cada fatia roda primeiro em contêiner local, nunca na sessão do líder (L-50), e só então vai por push à branch da onda, onde o CI repete (L-36). Na confirmação da exceção, o líder respondeu "autorizo containers locais" e "pode deixar tudo nas pastas padrao". QML roda só offscreen, dentro do contêiner.
- **Lista fechada de imagens e pacotes (decisão do líder, 25/10/2026, ponto L7; L-51).** Imagens `ubuntu:26.04`, `manjarolinux/base:latest`, `linuxmintd/mint22.3-amd64:latest`, `mysql:8.4` e `axllent/mailpit:latest` (`fedora:44` e `mariadb:11.8` já estavam no Docker local). Qt 6.12 por aqt, base `linux_gcc_64` mais `qtnetworkauth`, `qthttpserver`, `qtimageformats`, `qtshadertools` e `qtwebsockets`. Flatpak `org.kde.Sdk//6.11` e `org.kde.Platform//6.11`. As ferramentas de build dentro dessas imagens.
- **Lista complementar (decisão do líder, 25/10/2026, ponto L9: "Autorizo a lista toda").** XSD e anexos gov.br, fontes Bricolage Grotesque e Atkinson Hyperlegible (OFL), gitleaks, `flatpak-builder`, cups-pdf e AT-SPI e Orca. Cada item é baixado só quando a fatia que o usa começa, com o tamanho medido no relatório da fatia.
- **Item fora das duas listas** pede autorização nova do líder antes da fatia que o usa. Sem ela, a fatia para. Não se roda só no CI para contornar (L-36, L-51).
- **Pastas e limites (regra de execução, lei global L-11; a frase "pode deixar tudo nas pastas padrao" é do líder, ponto L7).** Docker em `/var/lib/docker`. Build e temporários em `/var/tmp`, com `TMPDIR` exportado, porque `/tmp` é tmpfs. A base é configurável pela variável `NL_TMP_BASE` (padrão `/var/tmp`; decisão do líder de 25/10/2026, achado A0-08). Um trabalho pesado por vez (build, contêiner, suíte completa), serializado com `flock` e com `nice -n 5`. Neste projeto, no máximo 2 agentes em paralelo, e sem concorrência entre eles (nunca dois disputando o contêiner, o lock pesado ou os mesmos arquivos); decisão do líder, 27/10/2026, verbatim: "maximo 2 agentes em paralelo sem concorrência entre eles", escopo escolhido "Só no NotaLivre (Recomendado)". É especialização da L-11 global (teto de 4), que continua valendo nos outros projetos. Disco conferido por `btrfs filesystem usage /`, lendo `Device unallocated` e `Free (estimated) min`, nunca por `df`. Resultado de build fica fora da árvore. Nenhum laço que crie um processo por item varrido (L-11).
- **Fluxo de branch (decisão do líder, 25/10/2026: "Branch de onda, push por fatia").** Cada onda tem branch própria (`onda-0`, `onda-1` e assim por diante). Cada fatia commitada vai por push à branch da onda, só para o CI rodar (L-24, tempo 2). `main` recebe a onda só depois de fechada pela definição de fechamento dela, com autorização explícita de merge do líder (L-24, tempo 4). O push é conferido por `git ls-remote`, nunca pela mensagem do push.
- **Commit e pendências (regra de execução, lei global L-24 e L-63).** Commit por fatia em Conventional Commits, citando o ID `NL-<onda>.<fatia>` e tocando o `Status` do `TODO.md` no mesmo commit. Implementação entregue entra como "Pendente verificação", nunca como "Concluído" direto.
- **Verificação (regra de execução, lei global L-49 e L-12).** O executor grava o log do contêiner local em `/var/tmp` e o cita na entrega. O main lê esse log e o do job de CI do SHA empurrado, este pelo MCP do GitHub, e faz spot-check arquivo:linha no blob commitado.
- **Titular e autoria (decisão do líder, confirmada em 25/10/2026; achado A0-13).** Titular do copyright: Petrus Alves da Silva Costa, como nos cabeçalhos SPDX. E-mail de autoria dos commits: petrus@drpetrus.top.
- **Versionamento (decisão do líder, 25/10/2026).** Formato vA.B.C.D, estilo GlintFx. Tag de marco a cada onda fechada, só com aval explícito do líder.
- **Mutação a cada push (decisão do líder, 25/10/2026).** O job `mutacao` roda em todo push, sem filtro de caminho.
- **Brief fundador (decisão do líder, 25/10/2026).** Fica em `docs/origem/`.
- **CI em PR de terceiros (decisão do líder, 25/10/2026; achado A0-07).** Workflow disparado por PR de colaborador externo só roda com aprovação manual, configurado no repositório.

## Adaptação da L-70 (deploy irreversível)

Decisão do líder, 25/10/2026, pelo rito da LEI DAS LEIS: "Cancelo: L-70 inteira". A L-70 vale inteira, com uma única substituição, na Fase 2.

- As 7 fases da L-70 valem para duas situações: (a) a liberação de emissão real a terceiros, que é a virada fiscal de Produção (NL-12); (b) toda versão do app que mude schema do banco, autenticação ou chave criptográfica (categorias da própria L-70).
- A Fase 2, Shadow Deployment, é substituída por "dupla validação manual em staging (ambiente de Produção Restrita)". Justificativa de inviabilidade técnica: o NotaLivre é um app desktop instalado na máquina de cada usuário, sem balanceador e sem tráfego que se possa espelhar.
- As outras 6 fases são obrigatórias, sem exceção.
- As fases da L-70 que não couberem num app desktop voltam ao líder, uma a uma, antes da Onda 3 (pendente, não decidido).

Corpo das 7 fases: L-70 do `GODS_LAWS.md` global (anexo `docs/deploy-irreversivel.md`).

## Documentos do projeto

- Spec: `docs/superpowers/specs/2026-10-25-notalivre-design.md`
- Spec visual: `docs/design/spec-visual.md` (fechada pelo líder; só o ponteiro fica aqui)
- Plano: `docs/superpowers/plans/2026-10-25-notalivre-plano.md`
- Pendências: `TODO.md` (raiz). Pontos em aberto para o líder: seção 6.1 do plano.
- Testes: `TESTES.md`
- Auditorias: `AUDITORIAS.md`
- Pesquisas de base: `docs/pesquisa/`
- Hub para o usuário final: `README.md`

## Estilo e leis aplicáveis

- Hub-and-spoke: `README.md` é o único hub (L-73).
- Texto de interface e de documento em pt-br, sem travessão nem meia-risca (L-32), sem emoji (L-31); exceção do portão nas pastas internas (decisão do líder, 25/10/2026; achado A0-16): `docs/origem/`, `docs/auditoria/` e `docs/adr/` ficam fora do portão de travessão, e o texto voltado ao usuário fora dessas pastas continua sob a L-32.
- Pelas leis globais: agente especialista executa, nunca o orquestrador (L-10); implementador, revisor e orquestrador são agentes distintos (L-12); cada fatia e cada onda seguem os passos da L-16; portão só conta depois de visto vermelho (L-36); TDD estrito e sem meta numérica de cobertura (L-35); átomos sem monolito (L-34); dependência unidirecional (L-33); destruição de arquivo só com caminho conferido (L-53).

## Pendências
A tabela de pendências e planejamento do projeto está em `TODO.md` na raiz (ordenada por execução, coluna Onda marca passos paralelizáveis).
