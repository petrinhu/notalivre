# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Sefin (NL-1.6b): codec da DPS no leiaute de Produção Restrita de 20/10/2026, uma pasta por versão (PREMISSA 4.14).
# Acrescenta os arquivos ao alvo da camada com target_sources(nl_mid PRIVATE ...); não cria alvo próprio.
target_sources(nl_mid PRIVATE
    leiaute/prodrest_20261020/codec_dps.cpp
    leiaute/prodrest_20261020/envelope_dps.cpp
    leiaute/prodrest_20261020/codigos_leiaute.cpp
    leiaute/prodrest_20261020/grupo_ibscbs.cpp
    leiaute/prodrest_20261020/grupos_pessoa.cpp
    leiaute/prodrest_20261020/grupos_tributos.cpp
    cliente/politica_retry.cpp
    cliente/catalogo_erros_sefin.cpp
    cliente/leitor_erro_sefin.cpp
    cliente/cliente_sefin.cpp
    cliente/comparador_infdps_c14n.cpp
    cliente/allowlist_log_sefin.cpp
    assinatura/assinador_dps_sefin.cpp)
