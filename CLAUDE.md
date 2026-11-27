# NotaLivre: canon do projeto

O canon do NotaLivre é este `CLAUDE.md` e o `GODS_LAWS.md` do projeto. As leis globais do usuário, em `~/.claude/GODS_LAWS.md` (fora deste repositório), são citadas por número e não copiadas.

## Consumidor e produto

**Consumidor declarado (L-21).** Spec, seção 1, texto completo: "MEI, autônomos e pequenas empresas do Simples Nacional. Escritórios contábeis ficam fora da v1 (paráfrase da resposta do líder na rodada 2). O projeto é para distribuição a terceiros (decisão do líder, rodada 1: emissores = 'Distribuir a terceiros')." Esta frase é o consumidor do produto; nenhuma decisão de escopo usa outro consumidor como critério.

**Frase-guarda (L-21).** Entra verbatim no briefing de qualquer agente que vai produzir ou decidir algo do produto:

> Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição.

**Fontes de decisão de produto.** Spec: `docs/superpowers/specs/2026-10-25-notalivre-design.md` (seções 2 a 4, com as decisões do líder, e 4.14 para as propostas do orquestrador ainda não aprovadas). Plano: `docs/superpowers/plans/2026-10-25-notalivre-plano.md` (ondas e fatias). Stack: C++23 e Qt 6.12, interface QML (spec 4.2). Entrega somente em Flatpak (spec 3.3). Licença GPL-3.0-or-later (spec 4.1). App-id `io.github.petrinhu.notalivre` (decisão do líder, rodada 6, spec 4.1).

## Adaptação da L-70 (deploy irreversível)

A L-70 vale inteira, com uma única troca, na Fase 2: no lugar do Shadow Deployment, dupla validação manual em staging (ambiente de Produção Restrita). Justificativa de inviabilidade técnica: o NotaLivre é um app desktop instalado na máquina de cada usuário, sem balanceador e sem tráfego que se possa espelhar. As outras 6 fases são obrigatórias, sem exceção. Decisão, verbatim, alcance e as decisões das Fases 0, 1, 3, 4, 5 e 6: NF-17.

## Documentos do projeto

- Spec: `docs/superpowers/specs/2026-10-25-notalivre-design.md`
- Spec visual: `docs/design/spec-visual.md` (fechada pelo líder; só o ponteiro fica aqui)
- Plano: `docs/superpowers/plans/2026-10-25-notalivre-plano.md`
- Pendências: `TODO.md` (raiz). Pontos em aberto para o líder: seção 6.1 do plano.
- Testes: `TESTES.md`
- Auditorias: `AUDITORIAS.md`
- Pesquisas de base: `docs/pesquisa/`
- Hub para o usuário final: `README.md`

## Leis do projeto

Leia [`GODS_LAWS.md`](GODS_LAWS.md) antes de agir. `L-NN` sem prefixo é lei global.

| Lei | Gatilho: dispara quando você vai... |
|---|---|
| [NF-01](GODS_LAWS.md#nf-01) | criar ou apagar README ou arquivo de exemplo de configuração |
| [NF-02](GODS_LAWS.md#nf-02) | criar diretório ou adaptador, ou escrever `#include` entre camadas |
| [NF-03](GODS_LAWS.md#nf-03) | nomear classe, arquivo ou identificador, ou devolver erro |
| [NF-04](GODS_LAWS.md#nf-04) | rodar uma fatia, teste ou QML, ou fazer push |
| [NF-05](GODS_LAWS.md#nf-05) | baixar ou instalar imagem, pacote, fonte, certificado ou biblioteca |
| [NF-06](GODS_LAWS.md#nf-06) | mexer em validação de certificado ou revogação |
| [NF-07](GODS_LAWS.md#nf-07) | rodar trabalho pesado, escolher pasta de build ou temporário, ou despachar agentes, ou guardar log citado, registro de decisões ou contador de DPS, ou fechar fatia |
| [NF-08](GODS_LAWS.md#nf-08) | criar branch, commitar, fazer push ou pedir merge |
| [NF-09](GODS_LAWS.md#nf-09) | entregar uma fatia ou conferir a entrega de um executor |
| [NF-10](GODS_LAWS.md#nf-10) | escrever cabeçalho SPDX ou configurar a autoria de commit |
| [NF-11](GODS_LAWS.md#nf-11) | mexer em versão ou criar tag |
| [NF-12](GODS_LAWS.md#nf-12) | mexer no CI de mutação |
| [NF-13](GODS_LAWS.md#nf-13) | procurar ou mover o brief fundador |
| [NF-14](GODS_LAWS.md#nf-14) | mexer em workflow disparado por PR |
| [NF-15](GODS_LAWS.md#nf-15) | ligar a GUI ao núcleo ou à CLI |
| [NF-16](GODS_LAWS.md#nf-16) | mexer no manifesto Flatpak ou no suporte a Wayland e X11 |
| [NF-17](GODS_LAWS.md#nf-17) | planejar emissão real a terceiros, ou versão que toque login, permissão, token, segredo, apagar dado, schema ou chave, ou publicar qualquer versão |
| [NF-18](GODS_LAWS.md#nf-18) | escrever texto de interface ou de documento |
| [NF-19](GODS_LAWS.md#nf-19) | escrever código que guarda, calcula ou mostra dinheiro |
| [NF-20](GODS_LAWS.md#nf-20) | rodar o espelho local, um build Flatpak ou `flatpak run` no host, ou fazer merge na branch da onda |
| [NF-21](GODS_LAWS.md#nf-21) | marcar o `Status` de um item do `TODO.md` como "Pendente verificação" ou "Concluído" |
| [NF-22](GODS_LAWS.md#nf-22) | ordenar ou começar a NL-3.8 (contrato) ou a NL-3.1d |
| [NF-23](GODS_LAWS.md#nf-23) | criar ou mudar uma tela da interface |
| [NF-24](GODS_LAWS.md#nf-24) | propor uma correção que volte a versão de kernel, Python, pacote ou imagem |
| [NF-25](GODS_LAWS.md#nf-25) | tirar uma dúvida, pesquisar, ou planejar uma ordem com decisão técnica ou fiscal |
| [NF-26](GODS_LAWS.md#nf-26) | mexer nos portões de sanitizer, no preset, no piso de portões ou nos autotestes deles |
| [NF-27](GODS_LAWS.md#nf-27) | mexer no cadastro de cofre, no admin, no aviso ao admin ou no quiz |
| [NF-28](GODS_LAWS.md#nf-28) | mexer na remoção de cofre, no prazo dela ou no aviso de remoção |
| [NF-29](GODS_LAWS.md#nf-29) | mexer no certificado, no código de recuperação, no localizador, no chaveiro ou na senha de login do cofre |
| [NF-30](GODS_LAWS.md#nf-30) | mexer na senha do banco, no assistente de banco ou na conferência do servidor do banco |
| [NF-31](GODS_LAWS.md#nf-31) | usar SHA-1 no arnês de teste, mexer no cTribMun ou citar a ADR-0007 ou a DA-2.98 |
| [NF-32](GODS_LAWS.md#nf-32) | escrever, cortar ou revisar função ou arquivo de código, ou planejar a Onda 9A |
| [NF-33](GODS_LAWS.md#nf-33) | despachar ou revisar um agente, abrir ou fechar fatia ou onda, contar um portão, escrever teste ou apagar arquivo |

## Pendências
A tabela de pendências e planejamento do projeto está em `TODO.md` na raiz (ordenada por execução, coluna Onda marca passos paralelizáveis).
