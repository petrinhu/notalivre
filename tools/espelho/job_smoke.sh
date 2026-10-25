#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Espelho do job `smoke-host` do ci.yml (R2), com LACUNA DECLARADA.
# O CI instala o bundle no host do runner (plano B). Aqui isso exigiria instalar no host (proibido sem
# autorização, L-51) ou --privileged (proibido local). Equivalente parcial: roda o build do job flatpak
# dentro do sandbox do flatpak-builder (`--run`), sem instalar nada. Não cobre a instalação do bundle.
. "$(dirname "${BASH_SOURCE[0]}")/comum.sh"
travar
D="$ESPELHO_TRAB/flatpak"
cd "$D" 2>/dev/null || exit 1
MANIFESTO="$D/fonte/flatpak/io.github.petrinhu.notalivre.yml"
if [ ! -d "$D/build" ] || [ ! -f "$MANIFESTO" ]; then
    echo "REPROVA: falta o build do job flatpak em $D (rode job_flatpak.sh antes)"
    exit 1
fi
FB=(flatpak run --user --filesystem="$D" --filesystem=xdg-data/flatpak:ro
    --env=FLATPAK_USER_DIR="$HOME/.local/share/flatpak" --command=flatpak-builder org.flatpak.Builder --run "$D/build" "$MANIFESTO")
# O veredito é o do APP: rc dele impresso no sandbox (inner_rc=0) e um QML carregado (raizes_qml=1).
# O rc do embrulho `flatpak-builder --run` sai 1 sem mensagem (medido em 25/10/2026): é achado aberto, impresso abaixo.
SAIDA="$D/smoke.log"
"${FB[@]}" sh -c 'QT_QPA_PLATFORM=offscreen notalivre-qml-smoke; echo inner_rc=$?' > "$SAIDA" 2>&1
wrapper=$?
echo "embrulho_flatpak_builder_rc=$wrapper (achado aberto: rc 1 sem mensagem; o veredito abaixo é o do app)"
if grep -q '^inner_rc=0$' "$SAIDA" && grep -q '^raizes_qml=1$' "$SAIDA"; then
    echo "passo=smoke-qml-offscreen rc=0 (app: inner_rc=0, raizes_qml=1)"
else
    echo "passo=smoke-qml-offscreen rc=1 (app não confirmou o QML; ver $SAIDA)"
    PASSOS_FALHOS=$((PASSOS_FALHOS + 1))
fi
# LACUNA DECLARADA: o CI lê /run/host/os-release e o TCP dentro do bundle instalado. O sandbox do builder não
# tem /run/host, então esses passos não existem aqui. Não são contados como verdes.
echo "lacuna=os-release-e-tcp (so no CI, bundle instalado no host do runner)"
[ "$PASSOS_FALHOS" -eq 0 ] && { echo "JOB smoke (equivalente parcial, sem instalar bundle): VERDE"; exit 0; }
echo "JOB smoke: VERMELHO ($PASSOS_FALHOS passo(s) com rc diferente de 0)"; exit 1
