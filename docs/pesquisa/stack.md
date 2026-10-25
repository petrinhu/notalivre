# Pesquisa de fatos: stack Qt + CI (agente devops-sre/haiku, 25/10/2026)

Desvio declarado pelo agente: ~67 fetches (teto 35). Não pesquisado: install-qt-action, linuxdeploy-plugin-qt, QPdfWriter/QtPdf, vmime, signxml, requests.

## Qt
- FATO: Qt 6.12.0 em 30/09/2026 (https://download.qt.io/official_releases/qt/6.12/); LTS (Phoronix; regra oficial "a cada quarta minor desde 6.8"); suporte até 2031-09-30 (doc.qt.io/qt-6/qt-releases.html). Qt 6.8 LTS até 2029-10-08.
- PySide6 6.12.0 (08/10/2026), LGPL-3.0/GPL (https://pypi.org/project/PySide6/).

## Tabela distro (estado 25/10/2026)
| Distro | qt6-base | GCC | CMake | xmlsec1 | Imagem container |
|---|---|---|---|---|---|
| Fedora 44 | 6.11.2 | 16.2.1 | 4.3.0 | 1.2.41 | fedora:44 (Docker Official) |
| Ubuntu 26.04 LTS | 6.10.2 | 15 (gcc-16 disponível) | 4.2.3 | 1.3.9 | ubuntu:26.04 (Docker Official) |
| Ubuntu 26.10 | 6.11.2 | 15 | 4.3.4 | 1.3.12 | não verificado |
| Mint 22.3 (base Ubuntu 24.04) | 6.4.2 (INFERÊNCIA) | 13 | 3.28.3 | 1.2.39 | linuxmintd/mint22.3-amd64 (NÃO oficial) |
| Manjaro stable | 6.11.2 (unstable 6.12.0) | n/e (Arch 16.2.1) | n/e (Arch 4.4.4) | n/e (Arch 1.3.12, MIT) | manjarolinux/base (sem selo oficial, tag latest, 7 meses) |
Fontes: packages.fedoraproject.org, launchpad.net/ubuntu, manjaristas.org/branch_compare, archlinux.org/packages, hub.docker.com, en.wikipedia.org/wiki/Linux_Mint. Mint 23 (base 26.04) previsto dez/2026-jan/2027 (divergência de numeração).

## GitHub Actions
- FATO: só runners Ubuntu (ubuntu-22.04/24.04/26.04, x64 e arm); "We do not plan to offer other Linux distributions" (https://github.com/actions/runner-images). Demais distros via `container:`; shell padrão `sh`. Root por padrão: secundário + L-49.

## Libs
- aqtinstall 3.3.0 (jun/2025); suporte a Qt 6.11+ em Linux não confirmado.
- QtKeychain 0.15.0 (libsecret/KWallet 6); empacotamento por distro não verificado.
- SMTP: Qt sem módulo SMTP (fonte de fórum, não doc oficial). Alternativas: SimpleMail (github.com/amantia/simple-mail), libcurl SMTP.
- QMYSQL: Fedora 44 qt6-qtbase-mysql 6.11.2; demais não verificado.
- QSslCertificate::importPkcs12 existe (doc PySide6); mTLS via QSslConfiguration (exemplos de fórum).
