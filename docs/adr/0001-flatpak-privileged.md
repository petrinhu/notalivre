# ADR 0001: job flatpak com contêiner privilegiado (desvio de risco com controles compensatórios)

**Status:** aceito como desvio de risco, com controles compensatórios verificados. Pendência aberta para o líder: `pull_request` vindo de fork (ver C6).
**Data:** 25/10/2026.
**Origem:** achado A0-07 da auditoria da Onda 0 (`docs/auditoria/onda-0.md`). Regra: L-77 (desvio de risco não é conformidade).

## Contexto

O job `flatpak` do `.github/workflows/ci.yml` roda o `flatpak-builder` dentro do contêiner `flatpak-github-actions` com `options: --privileged`. O motivo é técnico e foi medido na NL-0.5: o `flatpak-builder` usa `bwrap`, que precisa de namespaces que o contêiner comum não concede. A ação do GitHub também monta o socket do Docker do runner. Antes desta ADR, a imagem estava fixada por tag móvel (`kde-6.11`), e o risco não estava registrado.

## Decisão

1. Manter `--privileged` **somente** no job `flatpak`. Nenhum outro job recebe privilégio. O portão `tools/portoes/fixacao_ci.py` reprova `--privileged` em qualquer outro job.
2. Fixar a imagem por digest do índice multi-arch (`sha256:b0a8a2acb9e420dd0e3e083ff552bea5a817e17e41532f1d2287ff49ef9aae0e`, obtido pela API do registro em 25/10/2026).
3. Localmente, o build Flatpak roda pelo `org.flatpak.Builder` (sandbox do flatpak, autorizado em NL-0.5), **sem** `--privileged` (tools/espelho/job_flatpak.sh). `--privileged` local continua proibido.

## Alternativas consideradas

- **Build sem privilégio:** não é viável hoje com o contêiner comum (medido: a opção de `cap-add` saiu da lista na NL-0.5). Reavaliar quando houver outro caminho.
- **Build no host do runner:** já é o plano B do smoke, mas não substitui o build dentro de contêiner. Não aceito como troca silenciosa.

## Riscos residuais

- Contêiner privilegiado num runner compartilhado pode, em tese, escapar para o host do runner. Mitigação: runner hospedado pelo GitHub (`ubuntu-latest`), uma VM efêmera por job.
- Código de PR pode rodar no job privilegiado. Mitigação: o job não recebe segredo, e o `GITHUB_TOKEN` é só leitura (`contents: read`).

## Controles compensatórios e como cada um é verificado

Verificação de todos os controles: `python3 -I tools/portoes/fixacao_ci.py <raiz>` (job `higiene` do CI, a cada push), com autoteste de sabotagem `--autoteste` no mesmo job.

| # | Controle | Como se verifica | Estado |
|---|---|---|---|
| C1 | Imagem do job flatpak fixada por digest (`@sha256:` de 64 hex) | `fixacao_ci.py`: linha `image:` sem digest reprova; autoteste sabota com tag `:kde-6.11` | Ativo (CI a cada push) |
| C2 | Actions fixadas por SHA de commit de 40 hex | `fixacao_ci.py`: `uses:` com tag reprova; autoteste sabota com `@v7` | Ativo |
| C3 | `--privileged` só no job `flatpak` | `fixacao_ci.py`: privilégio em outro job reprova; autoteste sabota | Ativo |
| C4 | Sem segredo e sem permissão de escrita no `ci.yml` | `fixacao_ci.py`: `secrets.` e `*: write` reprovam; autoteste sabota com `secrets.TOKEN` | Ativo |
| C5 | Runner hospedado e efêmero (`ubuntu-latest`), sem `self-hosted` | `fixacao_ci.py`: `runs-on` do job privilegiado diferente de `ubuntu-latest` e qualquer `self-hosted` reprovam; autoteste sabota | Ativo |
| C6 | Decisão sobre `pull_request` de fork no job privilegiado | Não há verificação automática. Opções para o líder: (a) manter; (b) não rodar o job em PR de fork (`if:` com `github.event.pull_request.head.repo.fork`); (c) exigir aprovação manual do workflow de fork nas configurações do repositório | **Aberto (decisão do líder)** |
| C7 | Revisão desta ADR | Revisada em toda mudança do `ci.yml` que toque o job `flatpak`, e na Onda 9 (decisão da matriz Flatpak, ponto L6 do plano). A revisão é humana: o portão não a mede | Por revisão |
| C8 | Espelho local sem `--privileged` | `tools/espelho/job_flatpak.sh` roda o `org.flatpak.Builder` sem `--privileged`; o modo `docker` do harness de mutação só existe no CI | Ativo (espelho a cada push local, L-36) |

## Consequências

- O job `flatpak` continua com privilégio. O desvio é aceito, não eliminado.
- Reverter: quando existir build sem privilégio, trocar a imagem e remover `options: --privileged`, e o `fixacao_ci.py` passa a exigir ausência de privilégio em todos os jobs.
