# Medição do CI nativo (NL-0.4)

**ID:** NL-0.4 (spike medido do CI nativo). **Data:** 25/10/2026. **Branch:** `onda-0`.
**Escopo medido:** o núcleo NL-0.2 (esqueleto CMake, alvo `tst_sanidade`) compilado e testado em quatro contêineres locais, um por vez, sob a trava `flock` da casa.
**Status do CI remoto:** CI remoto pendente de push. O `ci.yml` está escrito e não foi rodado no GitHub.

## Resultado em uma linha

As quatro distros passaram no `debug` com `ctest` 1/1 e pré-cheque do Qt 7/0/0. A sabotagem do `tst_sanidade` ficou vermelha nas quatro (rc=8). O probe de C++23 mostra que o GCC 13.3 do Mint 22.3 não tem `<print>` nem `<generator>`.

## Tabela distro x versões

Fonte: a tabela gerada por `medir_versoes.sh` no log de cada run (`NL-0.4-<distro>-debug.log`). A coluna "candidato" é a versão que o gerenciador instalaria hoje, lida depois de atualizar o índice.

| Componente | Fedora 44 | Ubuntu 26.04 | Mint 22.3 (imagem) | Manjaro |
| --- | --- | --- | --- | --- |
| Sistema (os-release) | Fedora Linux 44 (Container Image) | Ubuntu 26.04.1 LTS | Ubuntu 24.04.5 LTS (DESVIO, ver abaixo) | Manjaro Linux |
| glibc | 2.43 | 2.43 | 2.39 | 2.44 |
| GCC (g++) | 16.2.1 | 15.2.0 | 13.3.0 | 16.2.1 |
| CMake | 4.3.0 | 4.2.3 | 3.28.3 | 4.4.3 |
| Ninja | 1.13.2 | 1.13.2 | 1.11.1 | 1.13.2 |
| Qt 6.12 (volume) | 6.12.0 | 6.12.0 | 6.12.0 | 6.12.0 |
| OpenSSL instalado (candidato) | 3.5.9 (3.5.9) | 3.5.5 (3.5.5-1ubuntu3.7) | 3.0.13 (3.0.13-0ubuntu3.16) | 3.6.4 (3.6.4-1) |
| Argon2id no `openssl list -kdf-algorithms` | presente | presente | **ausente** | presente |
| libsodium | ausente (candidato 1.0.22) | ausente (candidato 1.0.18-2) | ausente (candidato 1.0.18-1ubuntu0.24.04.1) | ausente (candidato 1.0.22-1) |
| xmlsec1 | ausente (candidato 1.2.41) | ausente (candidato 1.3.9-1) | ausente (candidato 1.2.39-5build2) | ausente (candidato 1.3.12-1) |
| libxml2 | 2.12.10 (candidato 2.12.10) | ausente (candidato 2.15.2+dfsg-0.1ubuntu0.3) | 2.9.14 (candidato 2.9.14+dfsg-1.3ubuntu3.10) | 2.15.4 (candidato 2.15.4-1) |
| zlib | 1.3.1.zlib-ng (candidato zlib-ng-compat-devel 2.3.3) | 1.3.1 (candidato 1:1.3.dfsg+really1.3.1-1ubuntu3.2) | 1.3 (candidato 1:1.3.dfsg-3.1ubuntu2.3) | 1.3.2 (candidato 1:1.3.2-3) |
| Qt 6 no repositório da distro | 6.11.2 | 6.10.2+dfsg-7 | 6.4.2+dfsg-21.1build5 | 6.11.2-3 |

Leitura para a questão Argon2id (plano, NL-0.4): o Argon2id no OpenSSL existe nas imagens com OpenSSL 3.5 ou 3.6 e não existe no OpenSSL 3.0.13 do Mint. Por isso o libsodium continua necessário para o Argon2id nas distros de base 24.04. Nenhuma das quatro traz Qt 6.12 como pacote do repositório, o que confirma o FATO do spec 4.3.

## Qt 6.12 e pré-cheque

- Qt 6.12.0 instalado por aqt no volume `notalivre-qt-6120` (método de D0.4a, sem compilar Qt da fonte). Tamanho do volume: 1.774 GB (`docker system df -v`).
- Pré-cheque (`precheque_qt.sh`) nas quatro distros: 7 alvos analisados, 0 bibliotecas faltando, 0 incompatibilidades de glibc. Nenhuma distro parou em D0.4a.
- Bibliotecas de runtime que faltavam nas imagens base foram instaladas pelos scripts `instalar/<distro>.sh`: GL, EGL, xkbcommon, fontconfig, freetype, X11, glib, dbus e krb5. A imagem base do Fedora 44 e a do Ubuntu 26.04 não as traziam.
- O CMake emite um aviso de `Qt6QmlAssetDownloaderPrivate` (pacote privado opcional, `FOUND FALSE`) nas quatro distros. Não impede o build nem o ctest.

## Probe de C++23 (D0.4b)

Fontes do probe ficam fora do repositório. Compilados com `g++ -std=c++23 -Wall -Wextra -Werror`, um por recurso, e executados. Logs: `NL-0.4-probe-cpp23-<distro>.log`.

| Distro (GCC) | `<expected>` | `<print>` / `std::print` | `<generator>` | Macros `__cpp_lib_*` (expected / print / generator) |
| --- | --- | --- | --- | --- |
| Fedora 44 (16.2.1) | compila e roda | compila e roda | compila e roda | 202211 / 202406 / 202207 |
| Ubuntu 26.04 (15.2.0) | compila e roda | compila e roda | compila e roda | 202211 / 202211 / 202207 |
| Mint 22.3 imagem (13.3.0) | compila e roda | **falha**: `print: No such file or directory` | **falha**: `generator: No such file or directory` | 202211 / ausente / ausente |
| Manjaro (16.2.1) | compila e roda | compila e roda | compila e roda | 202211 / 202406 / 202207 |

Conclusão medida: `<expected>` existe no GCC 13 do Mint, e `std::print` e `<generator>` não. A inferência do CTO em CLAUDE.md ("GCC 13 tem `<expected>`") está confirmada, e a de que `std::print` e `<generator>` só existem no GCC 14 ou mais novo também. Nenhum código do produto foi alterado por causa disso: a regra de D0.4b é não usar o que o GCC 13 não compila.

## Digests das imagens (fixadas no Dockerfile e no `ci.yml`)

| Imagem | Digest (sha256) | Mantenedor |
| --- | --- | --- |
| `fedora:44` | `43b29f65a41eb9c35e1cd5323e3bdf3b655c2357a9f4f1ff2f9c2798e5045d80` | Fedora (imagem oficial) |
| `ubuntu:26.04` | `c098726dd9b9733f3e885f8de0ef375206a311b0a317eadc40d347b1115fb944` | Canonical (imagem oficial) |
| `manjarolinux/base:latest` | `bbf1f1d746f28e138eea610e140d2f28cbb5b7c5da2fbff034b883527aa604e9` | Terceiros (Manjaro, Docker Hub; não é imagem oficial do Arch) |
| `linuxmintd/mint22.3-amd64:latest` | `304bbf7ec2a61252962c5210467d231a141661490e8962f52fd4df7688370369` | Terceiros (linuxmintd, Docker Hub) |

## Tamanhos

| Item | Tamanho (`docker images`) |
| --- | --- |
| `ubuntu:26.04` (base) | 161 MB |
| `fedora:44` (base) | 272 MB |
| `manjarolinux/base` (base) | 1.3 GB |
| `linuxmintd/mint22.3-amd64` (base) | 2.96 GB |
| `notalivre/build-fedora44:nl-0.4` | 1.39 GB |
| `notalivre/build-ubuntu26.04:nl-0.4` | 1.23 GB |
| `notalivre/build-mint22.3:nl-0.4` | 3.21 GB |
| `notalivre/build-manjaro:nl-0.4` | 3.59 GB |
| Volume `notalivre-qt-6120` (Qt 6.12) | 1.774 GB |

Total do Docker antes e depois das imagens: Images 18.55 GB (15 imagens) passou para 27.74 GB (23 imagens); Build Cache passou de 22.4 GB para 31.71 GB. O tamanho baixado comprimido não foi medido separadamente.

Disco do btrfs, medido por `btrfs filesystem usage /`: `Device unallocated` ficou em 2.09 GiB durante todo o spike; `Free (estimated)` min foi de 72.52 GiB para 64.47 GiB. O valor de `Device unallocated` é o teto real para chunks novos e está baixo. Balance é operação do líder.

## Sabotagem (portão visto vermelho, L-36)

Feita numa cópia da árvore fora do repositório, trocando `"base"` por `"quebrado"` em `tests/sanidade/tst_sanidade.cpp` (linha 19 da cópia). O md5 do arquivo do repositório foi conferido antes e depois: não mudou.

| Distro | Resultado | Linha de falha | rc do build |
| --- | --- | --- | --- |
| Fedora 44 | vermelho | `FAIL! TstSanidade::nomeCamadaDaBaseConfere()` | 8 |
| Ubuntu 26.04 | vermelho | idem | 8 |
| Mint 22.3 | vermelho | idem | 8 |
| Manjaro | vermelho | idem | 8 |

Logs: `NL-0.4-sabotagem-<distro>.log`.

**Teste negativo do pré-cheque (portão visto vermelho, L-36).** O `precheque_qt.sh` foi rodado na imagem base crua `ubuntu@sha256:c098...`, sem as bibliotecas de runtime, com o volume montado: saiu com rc=1 e `faltando_lib=57` (conta linhas `not found` do ldd, e uma biblioteca pode aparecer mais de uma vez). Caminho `AUSENTE-NO-VOLUME`: com `QT_PREFIXO=/opt/nada`, saiu com rc=1 e `analisados=0 faltando_lib=7`. Logs: `NL-0.4-precheque-negativo-base-ubuntu.log` e `NL-0.4-precheque-negativo-prefixo-ausente.log`.

## Como reproduzir (a partir da raiz do repositório)

```
tools/conteiner/build_local.sh imagem <distro>     # pesado, sob flock; distro: fedora44 ubuntu26.04 manjaro mint22.3
tools/conteiner/build_local.sh qt                  # Qt 6.12 no volume (só se ainda não estiver lá)
tools/conteiner/build_local.sh <distro> debug      # diagnóstico como root, depois configure, build e ctest como 1000:1000
```

O diagnóstico roda como root porque o índice do gerenciador só atualiza com root. Já o build roda como 1000:1000, para que o host não receba arquivo de root.

## Desvios e pontos abertos

1. **Imagem rotulada Mint 22.3 tem identidade mista (fatos medidos; decisão do líder).** `linuxmintd/mint22.3-amd64:latest`: (a) o os-release diz Ubuntu 24.04.5 LTS, glibc 2.39; (b) `/etc/linuxmint` não existe e o pacote `mint-info` não está instalado; (c) as fontes apt incluem `deb http://packages.linuxmint.com zena main upstream import backport` (zena é o codinome do Mint 22.3), além das fontes noble da Ubuntu; (d) há pacotes da Mint instalados: `linuxmint-keyring 2022.06.21` e `mint-dev-tools 1.5.4`. A base de pacotes é Ubuntu noble, com GCC 13.3. Usei a imagem como foi autorizada e registrei os quatro fatos. Se o líder quiser uma imagem de Mint 22.3 com identidade inequívoca, a escolha é dele.
2. **Atualização não é reprodutível.** O digest fixa só a imagem base. `pacman -Syu` na Manjaro subiu o glibc de 2.43 (base) para 2.44. `apt-get update` e `dnf` pegam o repositório do dia do build. Para reproduzir de verdade, falta fixar snapshot de repositório. Decisão do líder.
3. **Pacotes ausentes nas imagens.** libsodium, xmlsec1 e libxml2 não estão instalados (são "ausente" por desenho do spike, que mede o que a imagem traz). O NL-0.2 não os usa. A criptografia (NL-1.x) vai precisar deles em cada distro.
4. **`asan-ubsan` não foi rodado** nas quatro distros. Está fora do escopo do spike (só `debug`). O `libasan`/`libubsan` só foi instalado no Fedora.
5. **Erro corrigido no próprio script.** A primeira versão da medição rodava o índice do gerenciador como uid 1000 e engolia a falha: as candidatas saíram erradas. Corrigido: o diagnóstico roda como root, e uma falha de índice agora aparece como aviso e marca `sem-indice`, em vez de virar `ausente`. Os logs citados são dos scripts finais.
6. **Aviso de CMake sobre `Qt6QmlAssetDownloaderPrivate`** nas quatro distros. Não quebra o build (ver seção de Qt).
7. **CI remoto: pendente de push.** O `ci.yml` nunca rodou no GitHub. Riscos a conferir na primeira execução: cache do Qt em `${{ runner.temp }}` mapeado para o contêiner; o `actions/checkout` não exige `git` no contêiner: sem git, ele faz fallback pela API REST e baixa a árvore sem histórico. O que `gitleaks git` exige de fato é o histórico completo, com `fetch-depth: 0` no job `higiene` (que roda no runner `ubuntu-latest`, e não em contêiner), e o `git` dentro da imagem de higiene (presente, medido em REV-NL-0.4-higiene-git.log). Nos jobs `nativo`, o `git` é instalado pelo script por ser útil ao próprio build, não pelo checkout; e o Node que o runner injeta nas imagens Manjaro e Mint. A verificação do push é `git ls-remote`, conforme a L-24.

## Arquivos desta fatia

- `tools/conteiner/build_local.sh` (generalizado: `<distro> <preset>`; forma antiga `<preset>` mantida como atalho para o Fedora)
- `tools/conteiner/medir_versoes.sh`, `tools/conteiner/precheque_qt.sh`, `tools/conteiner/executar_no_conteiner.sh`
- `tools/conteiner/instalar/{fedora44,ubuntu26.04,manjaro,mint22.3}.sh`
- `tools/conteiner/{fedora44,ubuntu26.04,manjaro,mint22.3}-build.Dockerfile`
- `.github/workflows/ci.yml`
- `docs/pesquisa/ci_medicao.md` (este arquivo)

## Flatpak (NL-0.5)

**ID:** NL-0.5 (spike medido do Flatpak). **Data:** 25/10/2026. **Branch:** `onda-0`.
**Resultado:** o build Flatpak do manifesto contra o SDK KDE 6.11 fecha (rc=0), o bundle é gerado, e dentro do sandbox passam o smoke QML (Qt 6.11.2), o cliente TCP a 127.0.0.1 e a leitura de `/run/host/os-release`. O job `flatpak` e o job `mutacao` estão no `ci.yml`, mas o CI remoto ainda não rodou (sem push desta branch).

### Ferramentas e runtime

| Item | Medido | Log (rc no próprio log, salvo indicação) |
| --- | --- | --- |
| Download do SDK 6.11 | 1,2 GB | NL-0.5-remote-info-sdk.log (rc=0) |
| Download do Platform 6.11 | 409,9 MB; instalado 1,1 GB | NL-0.5-remote-info-platform.log (rc=0) |
| Instalação do runtime (usuário, host) | rc=0; SDK 3,2 GB instalado | NL-0.5-flatpak-install-runtime.log (rc=0) |
| `org.flatpak.Builder` (Flathub, autorizado pelo líder) | download 136,8 MB; instalado 360,1 MB | NL-0.5-remote-info-builder.log (rc=0); instalação concluída em NL-0.5-flatpak-install-builder.log (sem linha rc); verificação em NL-0.5-builder-verificacao.log (rc=0) |
| `flatpak-builder` dentro do Builder | 1.4.9 | NL-0.5-builder-version.log (rc=0) |
| Instalação isolada (`FLATPAK_USER_DIR` em pasta fora do repositório): runtime 6.11 e bundle | 2,7 GB em disco | NL-0.5-isolada-list.log (rc=0); NL-0.5-isolada-install-bundle.log (rc=0) |

**Por que o Platform aparece com 87 MB em uma medição antiga:** essa leitura foi `du` com o SDK e o Platform na mesma chamada. O `du` conta cada inode uma vez, e o SDK já contém os arquivos do Platform, então o número era só o que o Platform acrescentava. Lido sozinho, `du` dá 1,1 GB, igual ao `remote-info`.

### Build e bundle

- Build pelo `org.flatpak.Builder`, com `--disable-rofiles-fuse --force-clean`, manifesto `flatpak/io.github.petrinhu.notalivre.yml`, cópia limpa da árvore: **rc=0** (NL-0.5-flatpak-builder-build.log). Compila os 28 alvos com `-Werror` contra o Qt 6.11, sem patch.
- Bundle `notalivre.flatpak` gerado com `flatpak build-bundle`: **rc=0** (NL-0.5-bundle.log), **18,5 KB**. O bundle traz só o app; o runtime não entra nele.
- O flatpak-builder adiciona uma extensão de debug (`io.github.petrinhu.notalivre.Debug`) que não está no manifesto.

### Sandbox (medido dentro do `flatpak run`, instalação isolada)

| Teste | rc | Saída | Log |
| --- | --- | --- | --- |
| Smoke QML, `--env=QT_QPA_PLATFORM=offscreen --command=notalivre-qml-smoke` | 0 | `qt_em_execucao=6.11.2`, `raizes_qml=1` | NL-0.5-sandbox-smoke.log |
| `notalivre-gui` (stub) | 0 | sem saída | NL-0.5-sandbox-gui.log |
| `notalivre-cli` (stub) | 0 | sem saída | NL-0.5-sandbox-cli.log |
| TCP `127.0.0.1:47891` (servidor `http.server` no host, cliente `bash /dev/tcp` no sandbox) | 0 | `HTTP/1.0 200 OK` | NL-0.5-sandbox-tcp-cliente.log |
| `cat /run/host/os-release` no sandbox | 0 | `Fedora Linux 44 (Forty Four)` | NL-0.5-sandbox-os-release.log |

Nenhuma janela foi aberta (L-50): o smoke roda só com `offscreen`, e os stubs não criam janela.

### Compilação e testes contra o Qt 6.11 (medição histórica, fora do sandbox)

Medida em contêiner `notalivre/build-fedora44:nl-0.4` (GCC 16.2.1, CMake 4.3.0), com runtimes copiados para fora do repositório. Os rc estão em NL-0.5-compilacao-resumo.log, que é o registro do script de medição (script removido depois que o patch saiu do repositório).

| Etapa | rc |
| --- | --- |
| (a) `find_package(Qt6 6.12)` contra o Qt 6.11.2 | 1 (configure: versão 6.12 não compatível) |
| (b) configure, build com `-Werror` e `tst_sanidade` (com patch 6.11, equivalente ao estado atual) | 0, 0, 0 (3 de 3 testes passam) |
| (c) harness QML, build e execução | 0, 0 |
| (d) TCP e os-release fora do sandbox | 0, 0 (referência, não vale como medição do Flatpak) |

A mudança para 6.11 está no `CMakeLists.txt` raiz (uma linha, e o comentário ao lado). O CI nativo continua com 6.12.

### Job `flatpak` e job `smoke-host` no `ci.yml`

- `flatpak`: build contra o SDK 6.11 pela action `flatpak/flatpak-github-actions/flatpak-builder` (commit `79327416609af08178ad73b352877e51450790b3`, tag `v6`), no contêiner `ghcr.io/flathub-infra/flatpak-github-actions:kde-6.11` com `options: --privileged` (exemplo oficial da action). Artefato `notalivre-bundle` com o arquivo `notalivre.flatpak`. A action não tem `restore-keys`; a `cache-key` é estável por manifesto e pelo código do smoke, então só casa com a chave exata.
- `smoke-host` (plano B padrão, D0.5): instala o bundle no host do runner, sem contêiner e sem `--privileged`, e roda smoke, TCP e os-release dentro do sandbox. Primeira execução no GitHub pendente. Pode ser que o Ubuntu do runner restrinja o bwrap por AppArmor; isso só o CI mostra.
- A matriz das quatro distros saiu do ci.yml: um job com `echo` não mede nada. A medição do bwrap nas distros fica como pendência, sem job.

### Job `mutacao` (definição de fechamento da Onda 0, passo 2)

Script `tools/mutacao/sabotar_portoes.sh`, que sabota só cópias em pasta de rascunho. Cada portão tem de reprovar pelo motivo esperado. Execução local (`flatpak` no modo do Builder), rc do script = 0:

| Portão | rc da sabotagem | Motivo |
| --- | --- | --- |
| A2 (`camadas.py`) | 1 | `VIOLACAO src/back/camada.cpp:8: back inclui nl/mid/camada.h` |
| gitleaks (`higiene_segredos.sh`) | 1 | `REPROVA: segredo detectado`, chave gerada na hora |
| Travessão (`higiene_travessao.py`) | 1 | `VIOLACAO README.md:36: travessao U+2014` |
| Flatpak 6.11 (build) | 1 | `qSharedPointerStaticCast` não declarado no Qt 6.11 |

Log: NL-0.5-mutacao-local.log. No CI, o mesmo script roda com o contêiner `flathub kde-6.11` (modo `docker`). Esse modo ainda não rodou no GitHub.

### Permissões: de onde vêm

O manifesto pede só `--share=network`, `--socket=wayland`, `--socket=fallback-x11`, `--share=ipc` e `--device=dri`. Medido com `flatpak info --show-permissions` na instalação isolada (NL-0.5-permissoes-app.log, rc=0), o app também mostra `filesystems=xdg-config/kdeglobals:ro` e as três linhas de `[Session Bus Policy]` (`com.canonical.AppMenu.Registrar`, `org.kde.kconfig.notify`, `org.kde.KGlobalSettings`), que o manifesto não tem. Essas mesmas linhas estão no metadata do runtime `org.kde.Platform//6.11` (NL-0.5-permissoes-platform.log, rc=0). Então vêm do runtime KDE, não do manifesto. A origem exata no código do flatpak-builder não foi lida; a afirmação se apoia na comparação dos metadados.

Para o dossiê da Onda 9: o acesso a `kdeglobals` e os talk-names do KDE são herança do runtime, e mudar o runtime muda essas permissões.

### Ponto de decisão D0.5 (resolvido no que cabia)

- O sandbox fechou com o `org.flatpak.Builder` autorizado pelo líder. A opção de contêiner com cap-add e a de `flatpak-builder` no host saíram da lista.
- O CI nativo (Qt 6.12) segue como estava. O build Flatpak (6.11) é portão novo, ativo.
- Pendente: a primeira execução remota dos jobs `flatpak`, `smoke-host` e `mutacao`.

### Desvios de execução

1. O Builder não enxergava o SDK da instalação do usuário: corrigido com `--filesystem=xdg-data/flatpak:ro` e `FLATPAK_USER_DIR` no `flatpak run` do Builder, só para aquela execução.
2. Acesso à pasta de rascunho dado com `--filesystem` no mesmo `flatpak run`, também só para aquela execução.
3. O `cmake-ninja` falhou em `ninja install` (rc=1), porque o projeto não tem regra de install. O módulo `notalivre` passou a `buildsystem: simple`, com `install` explícito de `notalivre-gui` e `notalivre-cli`.
4. A instalação isolada baixou o runtime de novo (2,7 GB em disco no total).
5. Na compilação de medição, o runtime precisou de cópia fora do repositório por causa do SELinux, de `CMAKE_SKIP_RPATH` e de isolamento do glibc (`stack smashing detected` em rodadas anteriores).

### Disco e limpeza

- btrfs (`btrfs filesystem usage /`, sem sudo): `Used` 123,15 GiB, `Free (estimated) min` 59,52 GiB, `Device unallocated` 2,09 GiB (sem mudança). `df` mostra 60 GB livres, só como indicativo.
- Para remover depois do QA da onda (L-53): a pasta de rascunho `notalivre-nl05` (4,3 GB) e a instalação isolada `notalivre-flatpak-user` (2,7 GB), ambas fora do repositório. Os logs `NL-0.5-*` ficam na pasta `notalivre-logs`.
- Os runtimes e o Builder do usuário ficam instalados.

### Comandos exatos

```
flock <trava> nice -n 5 flatpak install --user -y flathub org.flatpak.Builder
flock <trava> nice -n 5 flatpak run --user --filesystem=<rascunho> --filesystem=xdg-data/flatpak:ro --env=FLATPAK_USER_DIR=<home>/.local/share/flatpak --command=flatpak-builder org.flatpak.Builder --disable-rofiles-fuse --force-clean --repo=<rascunho>/repo --state-dir=<rascunho>/state <rascunho>/build <rascunho>/fonte/flatpak/io.github.petrinhu.notalivre.yml
flatpak build-bundle <rascunho>/repo <rascunho>/notalivre.flatpak io.github.petrinhu.notalivre master
FLATPAK_USER_DIR=<isolada> flatpak run --user --env=QT_QPA_PLATFORM=offscreen --command=notalivre-qml-smoke io.github.petrinhu.notalivre
tools/mutacao/sabotar_portoes.sh <raiz> <trabalho> flatpak
```

`<trava>` é a trava compartilhada do NL-0.4 (`flock`). `<rascunho>`, `<isolada>` e `<trabalho>` são pastas fora do repositório. `<raiz>` é a árvore do repositório.

## Remediação da auditoria da Onda 0 (25/10/2026)

- **Flatpak local (ratificado pelo orquestrador):** o build local usa `org.flatpak.Builder` (sandbox autorizado pelo líder na NL-0.5), sem `--privileged`. Espelho: `tools/espelho/job_flatpak.sh`. O CI segue no contêiner com `--privileged` (ADR 0001).
- **Sabotagem do tst_sanidade (ratificado):** fica como step do job `nativo` (não do job `mutacao`, que não tem Qt). O step `tools/conteiner/sabotar_sanidade.sh` exige o vermelho da cópia sabotada pelo motivo (`FAIL!` em `nomeCamadaDaBaseConfere`) e sai 1 caso contrário. O resumo do job mostra o outcome do step; o rc de build e de ctest fica no log do step.
- **Espelho local:** `tools/espelho_local.txt` e `tools/espelho/rodar_espelho.sh`; a comparação com o workflow e o último run empurrado é `tools/espelho/comparar_jobs.sh` (run 40236959902, SHA 60748ca, 9 de 9 batem).
- **Status desta seção:** implementação entregue, pendente verificação. Os rc dos jobs do espelho completo ainda não foram medidos neste registro.
