# Pesquisa de fatos: NFSe, Flatpak e MySQL/MariaDB do hospedeiro

Coleta em 25/10/2026. Só coleta, sem escolha de desenho. Nenhuma instalação ou download (L-51). Nenhum outro arquivo escrito (L-31). Legenda: FATO traz URL e data quando a página mostrou data; INFERÊNCIA é separada; NÃO ENCONTRADO diz o que foi procurado. Fetches usados: 31 de 35.

## Q1. Runtime org.kde.Platform/Sdk e versão de Qt

- FATO: runtime 6.10 anunciado no KDE Discourse, "It tracks Qt 6.10 plus latest KDE Frameworks", base freedesktop SDK 25.08. [https://discuss.kde.org/t/flatpak-org-kde-sdk-6-10-runtime-is-now-available/41158] (30/10/2025)
- FATO: o repositório de empacotamento `flatpak-kde-runtime` tem as branches `qt6.9`, `qt6.10`, `qt6.11` (default) e `qt6.12`; a página listou qt6.9 com última atualização em 03/09/2026. [https://invent.kde.org/packaging/flatpak-kde-runtime/-/branches] (datas exibidas pela página, não de release de runtime)
- FATO: Qt 6.12 LTS publicado: "Qt 6.12 LTS is now available", "Qt 6.12 is a Long-Term Support (Qt LTS) release". [https://www.qt.io/blog/qt-6.12-released] (30/09/2026)
- FATO (só snippet de resultado de busca, post não aberto): Qt 6.11 "Released!" em 23/03/2026, segundo a busca "Qt 6.12 released date qt.io blog"; fonte exata não confirmada.
- NÃO ENCONTRADO: anúncio no Flathub/KDE Discourse de runtime 6.11 ou 6.12 publicado. Procurei: "org.kde.Sdk 6.11 runtime is now available", "discourse.flathub.org KDE runtime 6.11 released". O CDN nightly (cdn.kde.org/flatpak/kde-runtime-nightly) respondeu 404.
- NÃO ENCONTRADO: página do Flathub mostrando versões do `org.kde.Platform` (flathub.org/apps/org.kde.Platform não traz versões; github.com/flathub/org.kde.Platform deu 404).
- NÃO ENCONTRADO: lista de arquivos do branch qt6.12 (a página carregou só "Loading").
- INFERÊNCIA: a existência da branch qt6.12 no GitLab da KDE não prova que o runtime esteja publicado no Flathub. A confirmação exige ler o Flathub ou `flatpak remote-info` (consulta de metadados, sem instalar; decisão do líder).

## Q2. Conexão a MySQL/MariaDB do hospedeiro a partir de Flatpak

- FATO: Beekeeper Studio (`io.beekeeperstudio.Studio`, runtime `org.freedesktop.Platform` 25.08) declara em finish-args: `--share=network`, `--socket=wayland`, `--socket=fallback-x11`, `--socket=ssh-auth`, `--device=dri`, `--share=ipc`, `--talk-name=org.freedesktop.Notifications`. Sem `--filesystem`. [https://raw.githubusercontent.com/flathub/io.beekeeperstudio.Studio/master/io.beekeeperstudio.Studio.yml] (data NÃO ENCONTRADA)
- FATO: o manual Flatpak lista `--share=network`, `--socket=wayland`, `--socket=x11` e `--share=ipc` como permissões de uso comum; para `--talk-name` recomenda "the minimum required"; para `--socket=session-bus` diz "using them is a security risk ... must be avoided, unless the application is a development tool". [https://docs.flatpak.org/en/latest/sandbox-permissions.html] (data NÃO ENCONTRADA)
- FATO: política Flathub: "Static permissions must be kept to an absolute minimum."; "Applications should rely on XDG Portals"; "shipping overly permissive configurations will not be accepted"; exceções "on a case by case basis"; aplicativo pode ser removido se violar política. [https://docs.flathub.org/docs/for-app-authors/requirements] (data NÃO ENCONTRADA)
- FATO: o manual Flatpak diz que `--filesystem=host` aparece na tabela de filesystem mas não é classificado; recomenda "static and permanent filesystem access should be limited as much as possible". Mesma fonte do item anterior.
- FATO: GnuCash trocou o host padrão para 127.0.0.1 no build Flatpak porque "Flatpak build blocks access to the resolver". Um mantenedor disse que MySQL/PostgreSQL com "localhost" usam socket Unix, e que se poderia "allowing the proper file access to these socket files on the host system". [https://lists.gnucash.org/pipermail/gnucash-devel/2025-May/047072.html] (12/05/2025)
- NÃO ENCONTRADO: política específica do Flathub sobre `--filesystem=/run/mysqld` ou `/var/run/mysqld`. Procurei: "Flathub policy --filesystem=host justification", páginas `for-users/permissions` (só trouxe "Modifying default permissions") e requirements.
- NÃO ENCONTRADO: manifesto do DBeaver Community no Flathub (github.com/flathub/org.jkiss.dbeaver.DBeaverCommunity deu 404; flathub.org/apps/... deu 404; busca não achou). MySQL Workbench e outros clientes de banco não verificados.
- NÃO ENCONTRADO: exemplo documentado de app Flatpak que acesse socket Unix de MySQL no hospedeiro.
- INFERÊNCIA (não confirmada por fonte): conexão TCP em 127.0.0.1 precisa de `--share=network`; socket Unix precisa de permissão de sistema de arquivos para o caminho do socket.

## Q3. Driver QMYSQL no runtime KDE

- FATO: Qt documenta QMYSQL/MARIADB como plugin que usa a biblioteca cliente MySQL/MariaDB em runtime. [https://doc.qt.io/qt-6.12/sql-driver.html] (data NÃO ENCONTRADA)
- FATO: Flatpak diz que `org.kde.Platform` traz "most Qt modules and KDE Frameworks". [https://docs.flatpak.org/en/latest/qt.html] (data NÃO ENCONTRADA; página cita runtime-version 5.15-24.08)
- FATO: o manifesto de um runtime fica em `/usr/manifest.json` dentro dele, lido com `flatpak run --command=cat org.kde.Platform//<branch> /usr/manifest.json`. [https://docs.flathub.org/docs/for-users/finding-sources, via busca] (data NÃO ENCONTRADA). Este comando exige runtime instalado, por isso não foi executado (L-51).
- NÃO ENCONTRADO: confirmação de `libqsqlmysql` ou `libmariadb` no runtime 6.10/6.11/6.12. Procurei: "Qt QMYSQL driver flatpak org.kde.Platform qsqlmysql", "flatpak-kde-runtime qtbase sql mysql feature". Não foi possível ler o manifesto KDE (branch sem listagem).
- NÃO ENCONTRADO: exemplo de manifesto que empacote libmariadb-connector-c como módulo para Qt SQL em Flatpak.

## Q4. libmysqld (MySQL) e libmariadbd (MariaDB)

- FATO: MySQL "Starting with MySQL 8.0, we will drop libmysqld (aka 'embedded server')". Alternativa oficial: servidor standalone e `libmysqlclient`. [https://dev.mysql.com/blog-archive/mysql-8-0-retiring-support-for-libmysqld/] (27/10/2016, post de pré-lançamento do 8.0)
- FATO (resultado de busca, página não aberta): o Fedora indica reconstruir apps que pedem `libmysqld` contra `mariadb-embedded-devel`, com symlink `libmysqld.so` para `libmariadbd.so`. Fonte exata não confirmada. (data NÃO ENCONTRADA)
- FATO (resultado de busca, página não aberta): "Starting with MariaDB 10.5 the embedded server library ... no longer part of binary tarball release archives." Fonte secundária. (data NÃO ENCONTRADA)
- NÃO ENCONTRADO: página oficial do MariaDB sobre libmariadbd na linha 11.8/12.x (mariadb.com/kb/en/libmariadbd/ deu 403).
- NÃO ENCONTRADO: nota do MySQL 8.4 ou 9.x de release informando remoção ou não de libmysqld. A busca só achou o aviso de 8.0.
- INFERÊNCIA: pelo aviso de 8.0 e pela ausência de menção de libmysqld no manual, o embedded do MySQL não é alvo viável. A confirmação exige release notes de 8.4/9.x.

## Q5. Criptografia em repouso

- FATO (MySQL): InnoDB usa AES; "Electronic Codebook (ECB) block encryption mode for tablespace key encryption and Cipher Block Chaining (CBC) block encryption mode for data encryption"; cobre file-per-table, general tablespaces, `mysql` system tablespace, redo logs e undo logs; "Encryption is not supported for other tablespace types including the InnoDB [system tablespace]". [https://dev.mysql.com/doc/refman/8.4/en/innodb-data-encryption.html] (data NÃO ENCONTRADA)
- FATO (MySQL): redo e undo log encryption desligados por padrão (`innodb_redo_log_encrypt`, `innodb_undo_log_encrypt`). Mesma fonte.
- FATO (MySQL): "All MySQL editions provide a component_keyring_file component, which stores keyring data in a file local to the server host." Mesma fonte.
- FATO (MySQL): `component_keyring_encrypted_file`, `keyring_okv`, `keyring_aws` e `keyring_hashicorp` são listados como da Enterprise Edition, e a expressão "MySQL Enterprise Transparent Data Encryption (TDE)" é usada para o uso de gestão centralizada de chaves. A página não diz que o recurso de criptografia é Enterprise-only. Mesma fonte.
- FATO (MySQL 9.4): "A keyring component or plugin must be loaded ... prior to InnoDB initialization." [https://dev.mysql.com/doc/refman/9.4/en/keyring-component-installation.html] (data NÃO ENCONTRADA). Mesma página confirma: `component_keyring_file` em Community e Enterprise; `component_keyring_encrypted_file` só Enterprise.
- NÃO ENCONTRADO: se `keyring_file` (plugin antigo) continua em MySQL 9.x. A busca só achou o manual 5.7. Procurei: "keyring_file MySQL 9 removed".
- FATO (MariaDB): plugin `file_key_management` "that ships with MariaDB"; `file_key_management_encryption_algorithm`: AES_CBC (padrão) e AES_CTR; chaves de 128, 192 e 256 bits; cifra tablespaces InnoDB/XtraDB/Aria e temporários; chave em arquivo (`file_key_management_filename`), com opção de arquivo de chave cifrado (`file_key_management_filekey`). [https://mariadb.com/kb/en/file-key-management-encryption-plugin/] (data NÃO ENCONTRADA)
- FATO (MariaDB): rotação de chave via `FLUSH FILE_KEY_MANAGEMENT_KEYS` só no MariaDB Enterprise Server 11.8+; "The File Key Management plugin does not support" rotação no Community. Mesma fonte.
- FATO (MariaDB): "Choose the algorithm before you encrypt any data, and treat the choice as permanent." Mesma fonte.
- NÃO ENCONTRADO: se logs (redo/binlog) do MariaDB são cifrados pelo plugin. A página não afirma.
- NÃO ENCONTRADO: menção de AES-256 como algoritmo nomeado no MySQL 8.4 (a página diz "AES" e não define tamanho de chave).

## Q6. Segredos no Flatpak (portal Secret, KWallet, libsecret)

- FATO: "The Secret portal allows sandboxed applications to retrieve a per-application secret." A página documenta a versão 1 da interface, sem "since version N". "In a typical backend implementation, it is stored in the user's keyring, under the application ID as a key." "the length of the secret might not be sufficient for the use with certain encryption algorithm" (usar KDF). [https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Secret.html] (data NÃO ENCONTRADA)
- FATO: Discourse KDE, 15/05/2024: falhas ao salvar chave via portal de Secret em Frameworks 6.2.0, com "There have been repeated failed attempts to gain access to a wallet". Causa afirmada (22/05/2024): "The Secret Portal implementation generates a new secret every time it's called, which means Flatpak applications will never have the right key to decrypt their secrets." Nenhuma correção citada; referência ao bug 487348. [https://discuss.kde.org/t/kwallet-secrets-portal-cant-get-secret-for-flatpak/15566]
- FATO: Red Hat Bugzilla 2491140 (21/06/2026): "flatpaks couldn't access the secrets manager" após o upgrade para KDE 6.7.0; o workaround documentado é `flatpak override ... --talk-name=org.freedesktop.secrets --talk-name=org.kde.kwalletd6`. [https://bugzilla.redhat.com/show_bug.cgi?id=2491140] (21/06/2026)
- FATO (busca, página não aberta): KDE Phabricator T14316 e T12101 mencionam QtKeychain sem implementação do portal, e que `setService` não teria efeito dentro do sandbox. [https://phabricator.kde.org/T14316] (data NÃO ENCONTRADA)
- FATO (busca, Arch wiki): KWallet suporta a API freedesktop Secret Service desde Frameworks 5.97.0; ativação via `apiEnabled=true` em `[org.freedesktop.secrets]` no `kwalletrc`. [https://wiki.archlinux.org/title/KDE_Wallet] (data NÃO ENCONTRADA; fonte de busca)
- NÃO ENCONTRADO: se o QtKeychain atual usa o portal Secret dentro de Flatpak. Procurei: "QtKeychain flatpak org.freedesktop.portal.Secret support". Não li o repositório do QtKeychain.
- NÃO ENCONTRADO: se o bug de 2024 e o de 2026 têm correção publicada.

## Q7. Impressão, seletor de arquivo e acesso a .pfx/.p12

- FATO: Print portal, versão 4: "The Print portal allows sandboxed applications to print."; métodos `PreparePrint()` ("Presents a print dialog to the user and returns print settings and page setup") e `Print()` ("Asks to print a file."); o arquivo deve ser "a file descriptor open for reading". [https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Print.html] (data NÃO ENCONTRADA)
- FATO: FileChooser, versão 4: "The FileChooser portal allows sandboxed applications to ask the user for access to files outside the sandbox."; URIs retornadas com esquema `file://`; "The selected files will be made accessible to the application (which may involve adding it to the Documents portal)."; "Files added to the Documents portal by this portal stay accessible across sessions." [https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.FileChooser.html] (data NÃO ENCONTRADA)
- FATO: manual Flatpak: a seleção de arquivo pelo usuário "is interpreted as implicitly granting the application access to whatever file is chosen". [https://docs.flatpak.org/en/latest/sandbox-permissions.html] (data NÃO ENCONTRADA)
- FATO (busca, Flatpak docs e man page): `flatpak document-info` mostra o ID do documento, o caminho FUSE e as permissões por app; `flatpak document-export --transient` cria documento só para a sessão atual. [https://man7.org/linux/man-pages/man1/flatpak-document-info.1.html] e [https://mankier.com/1/flatpak-document-export] (data NÃO ENCONTRADA)
- FATO (busca, GNOME Discourse): a permissão persiste entre reboots, mas o caminho `/run/user/<uid>/doc/...` salvo pode ficar obsoleto; a sugestão no fórum é guardar URI e reconstruir o handle. A explicação não foi confirmada. [https://discourse.gnome.org/t/persisting-recent-file-access-across-reboots-in-a-flatpak-sandbox/34284]
- NÃO ENCONTRADO: página oficial do portal Documents (org.freedesktop.portal.Documents) com regras de caminho e permissão. Procurei: docs.flatpak.org/en/latest/portals.html (não cita o Documents) e sandbox-permissions (não cita). Não foi aberta a página de API do Documents.
- NÃO ENCONTRADO: se o Qt 6 usa o portal de impressão no Flatpak (QPrintDialog). Procurei: "Qt 6 flatpak QPrintDialog xdg-desktop-portal print portal". Só achei que existe um plugin de tema `qt6-xdgdesktopportal-platformtheme` para seletor de arquivo, sem confirmação oficial.
- NÃO ENCONTRADO: formato de leitura de .pfx/.p12 com a chave dentro do sandbox (não é tema de portal; só se confirma com teste real).

## Q8. Flatpak no CI (GitHub Actions) e containers

- FATO: flatpak-github-actions na versão `flatpak/flatpak-github-actions/flatpak-builder@v6`; inputs `manifest-path` (obrigatório), `cache-key`, `bundle` (padrão `app.flatpak`), `run-tests` (padrão false; ativa rede e X11 via `xvfb-run`). Imagem: `ghcr.io/flathub-infra/flatpak-github-actions:gnome-48`. [https://github.com/flatpak/flatpak-github-actions] (data NÃO ENCONTRADA; a página não mostrou release)
- FATO (mesma fonte): "`repo-dir` and `state-dir` must be under the same partition."; `stop-at-module` desliga a geração de bundle.
- FATO (busca, Ubuntu guide de 02/03/2026): o bundle é criado com `flatpak build-bundle` e instalado com `flatpak install <arquivo>.flatpak`. [https://oneuptime.com/blog/post/2026-03-02-how-to-build-flatpak-applications-on-ubuntu/markdown] (fonte secundária)
- FATO (busca, Podman Desktop): instalar `.flatpak` por cima de instalação do Flathub pode manter perfil de permissão em cache antigo, causando perda de acesso a arquivo. [https://podman-desktop.io/docs/troubleshooting/troubleshooting-flatpak] (data NÃO ENCONTRADA)
- FATO (busca, Flathub forum e MR do GNOME): bwrap dentro de Docker exige relaxar capabilities/seccomp/AppArmor, ou `--privileged`; um MR do GNOME relata falha mesmo com container privilegiado. [https://discourse.flathub.org/t/error-running-flatpack-in-dockerfile/1636] e [https://gitlab.gnome.org/GNOME/pitivi/-/merge_requests/343] (datas NÃO ENCONTRADAS; fontes de busca)
- FATO (busca, issue do projeto BuildStream): CAP_SYS_ADMIN sozinho não basta; o bwrap falha ao montar `/proc`. [https://gitlab.com/BuildStream/buildstream/issues/156] (data NÃO ENCONTRADA; snippet de busca)
- NÃO ENCONTRADO: exemplo de job que instale o `.flatpak` e rode testes dentro de container Fedora, Ubuntu, Manjaro ou Mint (procurei: "flatpak-github-actions container tests install .flatpak bundle Ubuntu Fedora"). Não há fonte para Manjaro e Mint.
- NÃO ENCONTRADO: lista oficial de limitações de bwrap sem privilégio em containers de GitHub Actions. Só achei relatos.
- INFERÊNCIA: a execução de teste do `.flatpak` em container exige, na prática, que o runner dê ao bwrap permissões que o padrão de container não dá. Não verificado; é o ponto a medir num runner real.

## Lacunas gerais (L-22)

- Nenhuma página do Flathub listou permissões de app de banco de dados (DBeaver, Workbench, DataGrip) porque os manifestos não abriram.
- Nenhuma fonte oficial confirmou algoritmo de criptografia MySQL além de AES com ECB/CBC.
- Fetches usados: 31 de 35. Sem instalações, sem downloads, sem escrita além deste relatório.
